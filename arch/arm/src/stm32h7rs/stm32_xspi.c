/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32_xspi.c
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

#include "arm_internal.h"

#include "hardware/stm32h7rs_gpio.h"
#include "hardware/stm32h7rs_pwr.h"
#include "hardware/stm32h7rs_rcc.h"
#include "hardware/stm32h7rs_sbs.h"
#include "hardware/stm32_xspi.h"

#include "stm32_xspi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_XSPI_TIMEOUT 1000000u

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_xspi_wait_reg(uintptr_t addr, uint32_t mask,
                               uint32_t value)
{
  uint32_t timeout = STM32_XSPI_TIMEOUT;

  while ((getreg32(addr) & mask) != value)
    {
      if (timeout-- == 0)
        {
          return -ETIMEDOUT;
        }
    }

  return OK;
}

static int stm32_xspi_wait_reg_trace(uintptr_t addr, uint32_t mask,
                                     uint32_t value,
                                     FAR const char *name)
{
  int ret;

  ret = stm32_xspi_wait_reg(addr, mask, value);
  if (ret < 0)
    {
      syslog(LOG_ERR, "XSPI: %s timeout reg=%08" PRIx32
             " mask=%08" PRIx32 " want=%08" PRIx32 "\n",
             name, getreg32(addr), mask, value);
    }

  return ret;
}

static int stm32_xspi_wait_flag(uintptr_t base, uint32_t mask)
{
  uint32_t timeout = STM32_XSPI_TIMEOUT;

  while ((getreg32(STM32_XSPI_SR(base)) & mask) == 0)
    {
      if ((getreg32(STM32_XSPI_SR(base)) & XSPI_SR_TEF) != 0)
        {
          putreg32(XSPI_FCR_CTEF, STM32_XSPI_FCR(base));
          return -EIO;
        }

      if (timeout-- == 0)
        {
          syslog(LOG_ERR, "XSPI: flag timeout base=%08" PRIxPTR
                 " sr=%08" PRIx32 " mask=%08" PRIx32 "\n",
                 base, getreg32(STM32_XSPI_SR(base)), mask);
          return -ETIMEDOUT;
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool stm32_xspi_is_mapped(uintptr_t base)
{
  uint32_t regval = getreg32(STM32_XSPI_CR(base));

  return (regval & XSPI_CR_EN) != 0 &&
         (regval & XSPI_CR_FMODE_MASK) == XSPI_CR_FMODE_MEMORY_MAPPED;
}

int stm32_xspi_wait_idle(uintptr_t base)
{
  return stm32_xspi_wait_reg_trace(STM32_XSPI_SR(base),
                                   XSPI_SR_BUSY, 0, "idle");
}

void stm32_xspi_config_gpio(uintptr_t portbase, uint32_t pins,
                            uint32_t af)
{
  uint32_t regval;
  int pin;

  regval = getreg32(portbase + STM32H7RS_GPIO_MODER_OFFSET);
  for (pin = 0; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_MODE_MASK(pin);
          regval |= GPIO_MODE_ALT << GPIO_MODE_SHIFT(pin);
        }
    }

  putreg32(regval, portbase + STM32H7RS_GPIO_MODER_OFFSET);

  regval = getreg32(portbase + STM32H7RS_GPIO_OTYPER_OFFSET);
  regval &= ~pins;
  putreg32(regval, portbase + STM32H7RS_GPIO_OTYPER_OFFSET);

  regval = getreg32(portbase + STM32H7RS_GPIO_OSPEEDR_OFFSET);
  for (pin = 0; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_SPEED_MASK(pin);
          regval |= GPIO_SPEED_VERYHIGH << GPIO_SPEED_SHIFT(pin);
        }
    }

  putreg32(regval, portbase + STM32H7RS_GPIO_OSPEEDR_OFFSET);

  regval = getreg32(portbase + STM32H7RS_GPIO_PUPDR_OFFSET);
  for (pin = 0; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_PUPD_MASK(pin);
        }
    }

  putreg32(regval, portbase + STM32H7RS_GPIO_PUPDR_OFFSET);

  regval = getreg32(portbase + STM32H7RS_GPIO_AFRL_OFFSET);
  for (pin = 0; pin <= 7; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_AFR_MASK(pin);
          regval |= af << GPIO_AFR_SHIFT(pin);
        }
    }

  putreg32(regval, portbase + STM32H7RS_GPIO_AFRL_OFFSET);

  regval = getreg32(portbase + STM32H7RS_GPIO_AFRH_OFFSET);
  for (pin = 8; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_AFR_MASK(pin);
          regval |= af << GPIO_AFR_SHIFT(pin);
        }
    }

  putreg32(regval, portbase + STM32H7RS_GPIO_AFRH_OFFSET);
}

int stm32_xspi_common_setup(void)
{
  int ret;

  modifyreg32(STM32H7RS_PWR_CSR2, 0,
              PWR_CSR2_EN_XSPIM1 | PWR_CSR2_EN_XSPIM2);

  modifyreg32(STM32H7RS_RCC_APB4ENR, 0, RCC_APB4ENR_SBSEN);
  (void)getreg32(STM32H7RS_RCC_APB4ENR);

  modifyreg32(STM32H7RS_RCC_CR, 0, RCC_CR_CSION);
  ret = stm32_xspi_wait_reg_trace(STM32H7RS_RCC_CR, RCC_CR_CSIRDY,
                                  RCC_CR_CSIRDY, "CSI ready");
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32H7RS_SBS_CCCSR,
              SBS_CCCSR_XSPI1_COMP_CODESEL | SBS_CCCSR_XSPI2_COMP_CODESEL,
              0);
  modifyreg32(STM32H7RS_SBS_CCCSR, 0,
              SBS_CCCSR_XSPI1_COMP_EN | SBS_CCCSR_XSPI2_COMP_EN);
  ret = stm32_xspi_wait_reg_trace(STM32H7RS_SBS_CCCSR,
                                  SBS_CCCSR_XSPI1_COMP_RDY,
                                  SBS_CCCSR_XSPI1_COMP_RDY,
                                  "XSPI1 compensation ready");
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_xspi_wait_reg_trace(STM32H7RS_SBS_CCCSR,
                                  SBS_CCCSR_XSPI2_COMP_RDY,
                                  SBS_CCCSR_XSPI2_COMP_RDY,
                                  "XSPI2 compensation ready");
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32H7RS_SBS_CCCSR, 0,
              SBS_CCCSR_XSPI1_IOHSLV | SBS_CCCSR_XSPI2_IOHSLV);

  modifyreg32(STM32H7RS_RCC_CCIPR1,
              RCC_CCIPR1_XSPI1SEL_MASK | RCC_CCIPR1_XSPI2SEL_MASK,
              RCC_CCIPR1_XSPI1SEL_PLL2S | RCC_CCIPR1_XSPI2SEL_PLL2S);
  modifyreg32(STM32H7RS_RCC_AHB5ENR, 0,
              RCC_AHB5ENR_XSPI1EN | RCC_AHB5ENR_XSPI2EN |
              RCC_AHB5ENR_XSPIMEN);
  (void)getreg32(STM32H7RS_RCC_AHB5ENR);

  /* MODE=0 maps XSPI1 to XSPIM_P1 and XSPI2 to XSPIM_P2.  Cube also enables
   * the chip-select override and routes both XSPI instances to NCS1.
   */

  putreg32(XSPIM_CR_CSSEL_OVR_EN | XSPIM_CR_REQ2ACK_TIME(1),
           STM32_XSPIM_CR);
  return OK;
}

int stm32_xspi_command(uintptr_t base, uint32_t instruction,
                       uint32_t ccr)
{
  int ret;

  ret = stm32_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(0, STM32_XSPI_TCR(base));
  putreg32(ccr, STM32_XSPI_CCR(base));
  putreg32(instruction, STM32_XSPI_IR(base));

  ret = stm32_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF, STM32_XSPI_FCR(base));
  return ret;
}

int stm32_xspi_command_addr(uintptr_t base, uint32_t instruction,
                            uint32_t address, uint32_t ccr,
                            uint32_t tcr)
{
  int ret;

  ret = stm32_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(tcr, STM32_XSPI_TCR(base));
  putreg32(ccr, STM32_XSPI_CCR(base));
  putreg32(instruction, STM32_XSPI_IR(base));
  putreg32(address, STM32_XSPI_AR(base));

  ret = stm32_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF, STM32_XSPI_FCR(base));
  return ret;
}

int stm32_xspi_read_data(uintptr_t base, uint32_t instruction,
                         uint32_t address, uint32_t ccr,
                         uint32_t tcr, size_t nbytes,
                         FAR uint8_t *buffer)
{
  volatile uint8_t *dr = (volatile uint8_t *)STM32_XSPI_DR(base);
  size_t i;
  int ret;

  if (nbytes == 0 || buffer == NULL)
    {
      return -EINVAL;
    }

  ret = stm32_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(nbytes - 1, STM32_XSPI_DLR(base));
  putreg32(tcr, STM32_XSPI_TCR(base));
  putreg32(ccr, STM32_XSPI_CCR(base));
  putreg32(instruction, STM32_XSPI_IR(base));
  putreg32(address, STM32_XSPI_AR(base));

  modifyreg32(STM32_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_INDIRECT_READ);
  putreg32(instruction, STM32_XSPI_IR(base));
  putreg32(address, STM32_XSPI_AR(base));

  for (i = 0; i < nbytes; i++)
    {
      ret = stm32_xspi_wait_flag(base, XSPI_SR_FTF | XSPI_SR_TCF);
      if (ret < 0)
        {
          return ret;
        }

      buffer[i] = *dr;
    }

  ret = stm32_xspi_wait_flag(base, XSPI_SR_TCF);
  putreg32(XSPI_FCR_CTCF, STM32_XSPI_FCR(base));
  return ret;
}

int stm32_xspi_write_data(uintptr_t base, uint32_t instruction,
                          uint32_t address, uint32_t ccr,
                          uint32_t tcr, uint8_t value,
                          size_t nbytes, bool repeat)
{
  volatile uint8_t *dr = (volatile uint8_t *)STM32_XSPI_DR(base);
  size_t i;
  int ret;

  if (nbytes == 0)
    {
      return -EINVAL;
    }

  ret = stm32_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(nbytes - 1, STM32_XSPI_DLR(base));
  putreg32(tcr, STM32_XSPI_TCR(base));
  putreg32(ccr, STM32_XSPI_CCR(base));
  putreg32(instruction, STM32_XSPI_IR(base));
  putreg32(address, STM32_XSPI_AR(base));

  modifyreg32(STM32_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_INDIRECT_WRITE);
  for (i = 0; i < nbytes; i++)
    {
      *dr = (repeat || i == 0) ? value : 0;
    }

  ret = stm32_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF, STM32_XSPI_FCR(base));
  return ret;
}

void stm32_xspi_controller_config(
  FAR const struct stm32_xspi_config_s *config)
{
  uint32_t fthreshold;

  fthreshold = config->fifo_threshold > 0 ? config->fifo_threshold - 1u : 0;

  modifyreg32(STM32H7RS_RCC_AHB5RSTR, 0, config->reset);
  modifyreg32(STM32H7RS_RCC_AHB5RSTR, config->reset, 0);

  putreg32(0, STM32_XSPI_CR(config->base));
  putreg32(config->memory_type | XSPI_DCR1_DEVSIZE(config->device_size) |
           XSPI_DCR1_CSHT(config->csht),
           STM32_XSPI_DCR1(config->base));
  putreg32(XSPI_DCR2_PRESCALER(config->prescaler),
           STM32_XSPI_DCR2(config->base));
  putreg32(XSPI_DCR3_CSBOUND(config->csbound) |
           XSPI_DCR3_MAXTRAN(config->maxtran),
           STM32_XSPI_DCR3(config->base));
  putreg32(XSPI_DCR4_REFRESH(config->refresh),
           STM32_XSPI_DCR4(config->base));
  putreg32(fthreshold << XSPI_CR_FTHRES_SHIFT,
           STM32_XSPI_CR(config->base));
  modifyreg32(STM32_XSPI_CR(config->base), 0, XSPI_CR_EN);
}

int stm32_xspi_set_prescaler(uintptr_t base, uint32_t prescaler)
{
  int ret;

  ret = stm32_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32_XSPI_DCR2(base), XSPI_DCR2_PRESCALER_MASK,
              XSPI_DCR2_PRESCALER(prescaler));
  return OK;
}

int stm32_xspi_enter_memory_mapped(uintptr_t base, uint32_t read_ccr,
                                   uint32_t read_tcr,
                                   uint32_t read_instruction,
                                   uint32_t write_ccr,
                                   uint32_t write_tcr,
                                   uint32_t write_instruction)
{
  int ret;

  ret = stm32_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_INDIRECT_WRITE);

  putreg32(read_ccr, STM32_XSPI_CCR(base));
  putreg32(read_tcr, STM32_XSPI_TCR(base));
  putreg32(read_instruction, STM32_XSPI_IR(base));

  putreg32(write_ccr, STM32_XSPI_WCCR(base));
  putreg32(write_tcr, STM32_XSPI_WTCR(base));
  putreg32(write_instruction, STM32_XSPI_WIR(base));

  modifyreg32(STM32_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_MEMORY_MAPPED);
  return OK;
}
