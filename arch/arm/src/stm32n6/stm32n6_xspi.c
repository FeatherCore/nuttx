/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_xspi.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <syslog.h>

#include <arch/board/board.h>
#include <nuttx/compiler.h>

#include "arm_internal.h"

#include "hardware/stm32n6_bsec.h"
#include "hardware/stm32n6_gpio.h"
#include "hardware/stm32n6_memorymap.h"
#include "hardware/stm32n6_pwr.h"
#include "hardware/stm32n6_rcc.h"
#include "hardware/stm32n6_xspi.h"

#include "stm32n6.h"
#include "stm32n6_xspi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_XSPI_TIMEOUT             1000000u
#define STM32N6_XSPI_200MHZ              200000000u
#define STM32N6_XSPI_50MHZ               50000000u

#ifdef CONFIG_ARCH_RAMFUNCS
#  define STM32N6_RAMFUNC locate_code(".ramfunc") noinline_function
#else
#  define STM32N6_RAMFUNC
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_xspi_common_done;
static bool g_vddio2_hslv;
static bool g_vddio3_hslv;
static uint32_t g_xspi1_source_hz = STM32N6_HCLK_FREQUENCY;
static uint32_t g_xspi2_source_hz = STM32N6_XSPI_50MHZ;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static STM32N6_RAMFUNC int stm32n6_xspi_wait_reg(uintptr_t addr,
                                                 uint32_t mask,
                                                 uint32_t value)
{
  uint32_t timeout = STM32N6_XSPI_TIMEOUT;

  while ((getreg32(addr) & mask) != value)
    {
      if (timeout-- == 0)
        {
          return -ETIMEDOUT;
        }
    }

  return OK;
}

static STM32N6_RAMFUNC int stm32n6_xspi_wait_reg_trace(
  uintptr_t addr, uint32_t mask, uint32_t value, FAR const char *name)
{
  int ret;

  ret = stm32n6_xspi_wait_reg(addr, mask, value);
#ifdef CONFIG_NXBOOT_BOOTLOADER
  if (ret < 0)
    {
      syslog(LOG_ERR, "XSPI: %s timeout reg=%08" PRIx32
             " mask=%08" PRIx32 " want=%08" PRIx32 "\n",
             name, getreg32(addr), mask, value);
    }
#else
  UNUSED(name);
#endif

  return ret;
}

static STM32N6_RAMFUNC int stm32n6_xspi_wait_flag(uintptr_t base,
                                                  uint32_t mask)
{
  uint32_t timeout = STM32N6_XSPI_TIMEOUT;

  while ((getreg32(STM32N6_XSPI_SR(base)) & mask) == 0)
    {
      if ((getreg32(STM32N6_XSPI_SR(base)) & XSPI_SR_TEF) != 0)
        {
          putreg32(XSPI_FCR_CTEF, STM32N6_XSPI_FCR(base));
          return -EIO;
        }

      if (timeout-- == 0)
        {
#ifdef CONFIG_NXBOOT_BOOTLOADER
          syslog(LOG_ERR, "XSPI: flag timeout base=%08" PRIxPTR
                 " sr=%08" PRIx32 " mask=%08" PRIx32 "\n",
                 base, getreg32(STM32N6_XSPI_SR(base)), mask);
#endif
          return -ETIMEDOUT;
        }
    }

  return OK;
}

static void stm32n6_xspi_read_hslv_otp(void)
{
  uint32_t fuse;

  modifyreg32(STM32N6_RCC_APB4ENR2, 0,
              RCC_APB4ENR2_SYSCFGEN | RCC_APB4ENR2_BSECEN);
  (void)getreg32(STM32N6_RCC_APB4ENR2);

  fuse = getreg32(STM32N6_BSEC_FVR(STM32N6_BSEC_HSLV_FUSE));
  g_vddio3_hslv = (fuse & STM32N6_BSEC_HSLV_VDDIO3) != 0;
  g_vddio2_hslv = (fuse & STM32N6_BSEC_HSLV_VDDIO2) != 0;

  syslog(LOG_INFO, "stm32n6: HSLV OTP124=0x%08" PRIx32
         " VDDIO2=%s VDDIO3=%s\n",
         fuse, g_vddio2_hslv ? "enabled" : "disabled",
         g_vddio3_hslv ? "enabled" : "disabled");

  g_xspi1_source_hz = STM32N6_HCLK_FREQUENCY;
  g_xspi2_source_hz = g_vddio3_hslv ? STM32N6_XSPI_200MHZ :
                                      STM32N6_XSPI_50MHZ;

  if (!g_vddio2_hslv)
    {
      syslog(LOG_WARNING,
             "stm32n6: VDDIO2 HSLV fuse disabled, XSPI1 limited to 50MHz\n");
    }

  if (!g_vddio3_hslv)
    {
      syslog(LOG_WARNING,
             "stm32n6: VDDIO3 HSLV fuse disabled, XSPI2 limited to 50MHz\n");
    }
}

static void stm32n6_xspi_config_ic3(void)
{
  uint32_t div = g_vddio3_hslv ? 6u : 24u;

  putreg32(RCC_ICCFGR(RCC_ICCFGR_SEL_PLL1, div), STM32N6_RCC_IC3CFGR);
  modifyreg32(STM32N6_RCC_DIVENR, 0, RCC_DIVENR_IC3EN);

  syslog(LOG_INFO, "stm32n6: XSPI2 IC3 divider=%" PRIu32
         " source=%" PRIu32 "Hz\n", div, g_xspi2_source_hz);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

uint32_t stm32n6_xspi_prescaler(uint32_t source_hz, uint32_t target_hz)
{
  uint32_t div;

  if (source_hz <= target_hz)
    {
      return 0;
    }

  div = source_hz / target_hz;
  if (div == 0)
    {
      return 0;
    }

  return div - 1;
}

uint32_t stm32n6_xspi_effective_hz(uint32_t source_hz,
                                   uint32_t prescaler)
{
  return source_hz / (prescaler + 1u);
}

bool stm32n6_xspi_common_ready(void)
{
  return g_xspi_common_done;
}

uint32_t stm32n6_xspi_get_source_hz(uintptr_t base)
{
  return base == STM32N6_XSPI2_BASE ? g_xspi2_source_hz :
                                      g_xspi1_source_hz;
}

bool stm32n6_xspi_hslv_enabled(uintptr_t base)
{
  return base == STM32N6_XSPI2_BASE ? g_vddio3_hslv : g_vddio2_hslv;
}

bool stm32n6_xspi_is_mapped(uintptr_t base)
{
  uint32_t regval = getreg32(STM32N6_XSPI_CR(base));

  return (regval & XSPI_CR_EN) != 0 &&
         (regval & XSPI_CR_FMODE_MASK) == XSPI_CR_FMODE_MEMORY_MAPPED;
}

STM32N6_RAMFUNC int stm32n6_xspi_wait_idle(uintptr_t base)
{
  return stm32n6_xspi_wait_reg_trace(STM32N6_XSPI_SR(base),
                                     XSPI_SR_BUSY, 0, "idle");
}

void stm32n6_xspi_config_gpio(uintptr_t portbase, uint32_t pins,
                              uint32_t af)
{
  uint32_t regval;
  int pin;

  regval = getreg32(STM32N6_GPIO_MODER(portbase));
  for (pin = 0; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_MODE_MASK(pin);
          regval |= GPIO_MODE_ALT << GPIO_MODE_SHIFT(pin);
        }
    }

  putreg32(regval, STM32N6_GPIO_MODER(portbase));

  regval = getreg32(STM32N6_GPIO_OTYPER(portbase));
  regval &= ~pins;
  putreg32(regval, STM32N6_GPIO_OTYPER(portbase));

  regval = getreg32(STM32N6_GPIO_OSPEEDR(portbase));
  for (pin = 0; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_SPEED_MASK(pin);
          regval |= GPIO_SPEED_VERYHIGH << GPIO_SPEED_SHIFT(pin);
        }
    }

  putreg32(regval, STM32N6_GPIO_OSPEEDR(portbase));

  regval = getreg32(STM32N6_GPIO_PUPDR(portbase));
  for (pin = 0; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_PUPD_MASK(pin);
        }
    }

  putreg32(regval, STM32N6_GPIO_PUPDR(portbase));

  regval = getreg32(STM32N6_GPIO_AFRL(portbase));
  for (pin = 0; pin <= 7; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_AFR_MASK(pin);
          regval |= af << GPIO_AFR_SHIFT(pin);
        }
    }

  putreg32(regval, STM32N6_GPIO_AFRL(portbase));

  regval = getreg32(STM32N6_GPIO_AFRH(portbase));
  for (pin = 8; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_AFR_MASK(pin);
          regval |= af << GPIO_AFR_SHIFT(pin);
        }
    }

  putreg32(regval, STM32N6_GPIO_AFRH(portbase));
}

int stm32n6_xspi_common_setup(void)
{
  if (g_xspi_common_done)
    {
      return OK;
    }

  stm32n6_power_config();
  stm32n6_xspi_read_hslv_otp();
  stm32n6_xspi_config_ic3();

  modifyreg32(STM32N6_RCC_MISCENR, 0, RCC_MISCENR_XSPIPHYCOMPEN);
  modifyreg32(STM32N6_RCC_CCIPR6,
              RCC_CCIPR6_XSPI1SEL_MASK | RCC_CCIPR6_XSPI2SEL_MASK,
              RCC_CCIPR6_XSPI1SEL_HCLK | RCC_CCIPR6_XSPI2SEL_IC3);
  modifyreg32(STM32N6_RCC_AHB5ENR, 0,
              RCC_AHB5ENR_XSPI1EN | RCC_AHB5ENR_XSPI2EN |
              RCC_AHB5ENR_XSPIMEN);
  (void)getreg32(STM32N6_RCC_AHB5ENR);

  putreg32(XSPIM_CR_CSSEL_OVR_EN | XSPIM_CR_REQ2ACK_TIME(1),
           STM32N6_XSPIM_CR);

  syslog(LOG_INFO, "stm32n6: XSPI1 source=%" PRIu32
         "Hz XSPI2 source=%" PRIu32 "Hz XSPIM_CR=%08" PRIx32 "\n",
         g_xspi1_source_hz, g_xspi2_source_hz,
         getreg32(STM32N6_XSPIM_CR));

  g_xspi_common_done = true;
  return OK;
}

STM32N6_RAMFUNC int stm32n6_xspi_command(uintptr_t base,
                                         uint32_t instruction,
                                         uint32_t ccr)
{
  int ret;

  ret = stm32n6_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32N6_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(0, STM32N6_XSPI_TCR(base));
  putreg32(ccr, STM32N6_XSPI_CCR(base));
  putreg32(instruction, STM32N6_XSPI_IR(base));

  ret = stm32n6_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF, STM32N6_XSPI_FCR(base));
  return ret;
}

STM32N6_RAMFUNC int stm32n6_xspi_command_addr(
  uintptr_t base, uint32_t instruction, uint32_t address, uint32_t ccr,
  uint32_t tcr)
{
  int ret;

  ret = stm32n6_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32N6_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(tcr, STM32N6_XSPI_TCR(base));
  putreg32(ccr, STM32N6_XSPI_CCR(base));
  putreg32(instruction, STM32N6_XSPI_IR(base));
  putreg32(address, STM32N6_XSPI_AR(base));

  ret = stm32n6_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF, STM32N6_XSPI_FCR(base));
  return ret;
}

STM32N6_RAMFUNC int stm32n6_xspi_read_data(
  uintptr_t base, uint32_t instruction, uint32_t address, uint32_t ccr,
  uint32_t tcr, size_t nbytes, FAR uint8_t *buffer)
{
  volatile uint8_t *dr = (volatile uint8_t *)STM32N6_XSPI_DR(base);
  size_t i;
  int ret;

  if (nbytes == 0 || buffer == NULL)
    {
      return -EINVAL;
    }

  ret = stm32n6_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32N6_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(nbytes - 1, STM32N6_XSPI_DLR(base));
  putreg32(tcr, STM32N6_XSPI_TCR(base));
  putreg32(ccr, STM32N6_XSPI_CCR(base));
  putreg32(instruction, STM32N6_XSPI_IR(base));
  putreg32(address, STM32N6_XSPI_AR(base));

  modifyreg32(STM32N6_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_INDIRECT_READ);
  putreg32(instruction, STM32N6_XSPI_IR(base));
  putreg32(address, STM32N6_XSPI_AR(base));

  for (i = 0; i < nbytes; i++)
    {
      ret = stm32n6_xspi_wait_flag(base, XSPI_SR_FTF | XSPI_SR_TCF);
      if (ret < 0)
        {
          return ret;
        }

      buffer[i] = *dr;
    }

  ret = stm32n6_xspi_wait_flag(base, XSPI_SR_TCF);
  putreg32(XSPI_FCR_CTCF, STM32N6_XSPI_FCR(base));
  return ret;
}

STM32N6_RAMFUNC int stm32n6_xspi_write_data(
  uintptr_t base, uint32_t instruction, uint32_t address, uint32_t ccr,
  uint32_t tcr, uint8_t value, size_t nbytes, bool repeat)
{
  volatile uint8_t *dr = (volatile uint8_t *)STM32N6_XSPI_DR(base);
  size_t i;
  int ret;

  if (nbytes == 0)
    {
      return -EINVAL;
    }

  ret = stm32n6_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32N6_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(nbytes - 1, STM32N6_XSPI_DLR(base));
  putreg32(tcr, STM32N6_XSPI_TCR(base));
  putreg32(ccr, STM32N6_XSPI_CCR(base));
  putreg32(instruction, STM32N6_XSPI_IR(base));
  putreg32(address, STM32N6_XSPI_AR(base));

  modifyreg32(STM32N6_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_INDIRECT_WRITE);
  for (i = 0; i < nbytes; i++)
    {
      *dr = (repeat || i == 0) ? value : 0;
    }

  ret = stm32n6_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF, STM32N6_XSPI_FCR(base));
  return ret;
}

STM32N6_RAMFUNC int stm32n6_xspi_write_buffer(
  uintptr_t base, uint32_t instruction, uint32_t address, uint32_t ccr,
  uint32_t tcr, FAR const uint8_t *buffer, size_t nbytes)
{
  volatile uint8_t *dr = (volatile uint8_t *)STM32N6_XSPI_DR(base);
  size_t i;
  int ret;

  if (nbytes == 0 || buffer == NULL)
    {
      return -EINVAL;
    }

  ret = stm32n6_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32N6_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(nbytes - 1, STM32N6_XSPI_DLR(base));
  putreg32(tcr, STM32N6_XSPI_TCR(base));
  putreg32(ccr, STM32N6_XSPI_CCR(base));
  putreg32(instruction, STM32N6_XSPI_IR(base));
  putreg32(address, STM32N6_XSPI_AR(base));

  modifyreg32(STM32N6_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_INDIRECT_WRITE);
  for (i = 0; i < nbytes; i++)
    {
      *dr = buffer[i];
    }

  ret = stm32n6_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF | XSPI_FCR_CTEF, STM32N6_XSPI_FCR(base));
  return ret;
}

void stm32n6_xspi_controller_config(
  FAR const struct stm32n6_xspi_config_s *config)
{
  uint32_t fthreshold;

  fthreshold = config->fifo_threshold > 0 ? config->fifo_threshold - 1u : 0;

  modifyreg32(STM32N6_RCC_AHB5RSTR, 0, config->reset);
  modifyreg32(STM32N6_RCC_AHB5RSTR, config->reset, 0);

  putreg32(0, STM32N6_XSPI_CR(config->base));
  putreg32(config->memory_type | XSPI_DCR1_DEVSIZE(config->device_size) |
           XSPI_DCR1_CSHT(config->csht),
           STM32N6_XSPI_DCR1(config->base));
  putreg32(XSPI_DCR2_PRESCALER(config->prescaler),
           STM32N6_XSPI_DCR2(config->base));
  putreg32(XSPI_DCR3_CSBOUND(config->csbound),
           STM32N6_XSPI_DCR3(config->base));
  putreg32(config->refresh, STM32N6_XSPI_DCR4(config->base));
  putreg32(fthreshold << XSPI_CR_FTHRES_SHIFT,
           STM32N6_XSPI_CR(config->base));
  modifyreg32(STM32N6_XSPI_CR(config->base), 0, XSPI_CR_EN);
}

STM32N6_RAMFUNC int stm32n6_xspi_set_prescaler(uintptr_t base,
                                               uint32_t prescaler)
{
  int ret;

  ret = stm32n6_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32N6_XSPI_DCR2(base), XSPI_DCR2_PRESCALER_MASK,
              XSPI_DCR2_PRESCALER(prescaler));
  return OK;
}

STM32N6_RAMFUNC int stm32n6_xspi_abort_memory_mapped(uintptr_t base)
{
  int ret;

  if (!stm32n6_xspi_is_mapped(base))
    {
      return stm32n6_xspi_wait_idle(base);
    }

  modifyreg32(STM32N6_XSPI_CR(base), 0, XSPI_CR_ABORT);
  ret = stm32n6_xspi_wait_reg_trace(STM32N6_XSPI_CR(base),
                                    XSPI_CR_ABORT, 0, "abort");
  putreg32(XSPI_FCR_CTCF | XSPI_FCR_CTEF, STM32N6_XSPI_FCR(base));
  modifyreg32(STM32N6_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);

  if (ret < 0)
    {
      return ret;
    }

  return stm32n6_xspi_wait_idle(base);
}

STM32N6_RAMFUNC int stm32n6_xspi_enter_memory_mapped(
  uintptr_t base, uint32_t read_ccr, uint32_t read_tcr,
  uint32_t read_instruction, uint32_t write_ccr, uint32_t write_tcr,
  uint32_t write_instruction)
{
  int ret;

  ret = stm32n6_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32N6_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_INDIRECT_WRITE);

  putreg32(read_ccr, STM32N6_XSPI_CCR(base));
  putreg32(read_tcr, STM32N6_XSPI_TCR(base));
  putreg32(read_instruction, STM32N6_XSPI_IR(base));

  putreg32(write_ccr, STM32N6_XSPI_WCCR(base));
  putreg32(write_tcr, STM32N6_XSPI_WTCR(base));
  putreg32(write_instruction, STM32N6_XSPI_WIR(base));

  modifyreg32(STM32N6_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_MEMORY_MAPPED);
  return OK;
}
