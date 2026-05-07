/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32h7rs_xspi.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <syslog.h>

#include <debug.h>
#include <nuttx/arch.h>

#include "arm_internal.h"
#include "stm32h7rs.h"

#include "hardware/stm32h7rs_gpio.h"
#include "hardware/stm32h7rs_pwr.h"
#include "hardware/stm32h7rs_rcc.h"
#include "hardware/stm32h7rs_sbs.h"
#include "hardware/stm32h7rs_xspi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32H7RS_XSPI_TIMEOUT           1000000u
#define STM32H7RS_XSPI_STARTUP_PRESCALER 3u
#define STM32H7RS_XSPI_OPTIONAL_PRESCALER 0u
#define STM32H7RS_XSPI_NOR_RESET_DELAY_MS 100u

#define MX66UW1G45G_RESET_ENABLE_CMD     0x66u
#define MX66UW1G45G_RESET_MEMORY_CMD     0x99u
#define MX66UW1G45G_OCTA_RESET_ENABLE_CMD 0x6699u
#define MX66UW1G45G_OCTA_RESET_MEMORY_CMD 0x9966u
#define MX66UW1G45G_READ_ID_CMD          0x9fu
#define MX66UW1G45G_READ_STATUS_CMD      0x05u
#define MX66UW1G45G_WRITE_ENABLE_CMD     0x06u
#define MX66UW1G45G_CFG2_WRITE_CMD       0x72u
#define MX66UW1G45G_CFG2_READ_CMD        0x71u
#define MX66UW1G45G_OCTAL_CFG2_READAW_CMD 0x718eu
#define MX66UW1G45G_OCTAL_READ_STATUS_CMD 0x05fau
#define MX66UW1G45G_CFG2_DOPI_ADDR       0x00000300u
#define MX66UW1G45G_CFG2_DOPI_VALUE      0x00u
#define MX66UW1G45G_CFG2_DOPI_MASK       0x07u
#define MX66UW1G45G_CFG2_OPI_ADDR        0x00000000u
#define MX66UW1G45G_CFG2_OPI_VALUE       0x02u
#define MX66UW1G45G_CFG2_OPI_MASK        0x02u
#define MX66UW1G45G_OCTAL_READ_CMD       0xee11u
#define MX66UW1G45G_OCTAL_WRITE_CMD      0x12edu

#define MX66UW1G45G_SR_WIP               0x01u
#define MX66UW1G45G_SR_WEL               0x02u
#define MX66UW1G45G_DEVSIZE              26u
#define MX66UW1G45G_OCTAL_READ_DUMMY     20u
#define MX66UW1G45G_OCTAL_REG_DUMMY      4u

#define APS256_RESET_CMD                 0xffu
#define APS256_READ_CMD                  0x00u
#define APS256_WRITE_CMD                 0x80u
#define APS256_HEXA_WRITE_CMD            0xa0u
#define APS256_REG_READ_CMD              0x40u
#define APS256_REG_WRITE_CMD             0xc0u

#define APS256_MR0_ADDR                  0x00000000u
#define APS256_MR4_ADDR                  0x00000004u
#define APS256_MR8_ADDR                  0x00000008u
#define APS256_MR0_VALUE                 0x11u
#define APS256_MR4_VALUE                 0x20u
#define APS256_MR8_VALUE                 0x40u
#define APS256_REG_DUMMY                 4u
#define APS256_READ_DUMMY                4u
#define APS256_WRITE_DUMMY               4u
#define APS256_HEXA_DUMMY                6u
#define APS256_DEVSIZE                   24u
#define APS256_RESET_DELAY_MS            1u

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32h7rs_xspi_wait_reg(uintptr_t addr, uint32_t mask,
                                   uint32_t value)
{
  uint32_t timeout = STM32H7RS_XSPI_TIMEOUT;

  while ((getreg32(addr) & mask) != value)
    {
      if (timeout-- == 0)
        {
          return -ETIMEDOUT;
        }
    }

  return OK;
}

static int stm32h7rs_xspi_wait_reg_trace(uintptr_t addr, uint32_t mask,
                                         uint32_t value,
                                         FAR const char *name)
{
  int ret;

  ret = stm32h7rs_xspi_wait_reg(addr, mask, value);
  if (ret < 0)
    {
      syslog(LOG_ERR, "XSPI: %s timeout reg=%08" PRIx32
             " mask=%08" PRIx32 " want=%08" PRIx32 "\n",
             name, getreg32(addr), mask, value);
    }

  return ret;
}

static bool stm32h7rs_xspi_is_mapped(uintptr_t base)
{
  uint32_t regval = getreg32(STM32H7RS_XSPI_CR(base));

  return (regval & XSPI_CR_EN) != 0 &&
         (regval & XSPI_CR_FMODE_MASK) == XSPI_CR_FMODE_MEMORY_MAPPED;
}

static int stm32h7rs_xspi_wait_idle(uintptr_t base)
{
  return stm32h7rs_xspi_wait_reg_trace(STM32H7RS_XSPI_SR(base),
                                       XSPI_SR_BUSY, 0, "idle");
}

static int stm32h7rs_xspi_wait_flag(uintptr_t base, uint32_t mask)
{
  uint32_t timeout = STM32H7RS_XSPI_TIMEOUT;

  while ((getreg32(STM32H7RS_XSPI_SR(base)) & mask) == 0)
    {
      if ((getreg32(STM32H7RS_XSPI_SR(base)) & XSPI_SR_TEF) != 0)
        {
          putreg32(XSPI_FCR_CTEF, STM32H7RS_XSPI_FCR(base));
          return -EIO;
        }

      if (timeout-- == 0)
        {
          syslog(LOG_ERR, "XSPI: flag timeout base=%08" PRIxPTR
                 " sr=%08" PRIx32 " mask=%08" PRIx32 "\n",
                 base, getreg32(STM32H7RS_XSPI_SR(base)), mask);
          return -ETIMEDOUT;
        }
    }

  return OK;
}

static void stm32h7rs_xspi_config_gpio(uintptr_t portbase, uint32_t pins,
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

static int stm32h7rs_xspi_common_setup(void)
{
  int ret;

  modifyreg32(STM32H7RS_PWR_CSR2, 0,
              PWR_CSR2_EN_XSPIM1 | PWR_CSR2_EN_XSPIM2);

  modifyreg32(STM32H7RS_RCC_APB4ENR, 0, RCC_APB4ENR_SBSEN);
  (void)getreg32(STM32H7RS_RCC_APB4ENR);

  modifyreg32(STM32H7RS_RCC_CR, 0, RCC_CR_CSION);
  ret = stm32h7rs_xspi_wait_reg_trace(STM32H7RS_RCC_CR, RCC_CR_CSIRDY,
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
  ret = stm32h7rs_xspi_wait_reg_trace(STM32H7RS_SBS_CCCSR,
                                      SBS_CCCSR_XSPI1_COMP_RDY,
                                      SBS_CCCSR_XSPI1_COMP_RDY,
                                      "XSPI1 compensation ready");
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_xspi_wait_reg_trace(STM32H7RS_SBS_CCCSR,
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
           STM32H7RS_XSPIM_CR);
  return OK;
}

static int stm32h7rs_xspi_command(uintptr_t base, uint32_t instruction,
                                  uint32_t ccr)
{
  int ret;

  ret = stm32h7rs_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32H7RS_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(0, STM32H7RS_XSPI_TCR(base));
  putreg32(ccr, STM32H7RS_XSPI_CCR(base));
  putreg32(instruction, STM32H7RS_XSPI_IR(base));

  ret = stm32h7rs_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF, STM32H7RS_XSPI_FCR(base));
  return ret;
}

static int stm32h7rs_xspi_command_addr(uintptr_t base, uint32_t instruction,
                                       uint32_t address, uint32_t ccr,
                                       uint32_t tcr)
{
  int ret;

  ret = stm32h7rs_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32H7RS_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(tcr, STM32H7RS_XSPI_TCR(base));
  putreg32(ccr, STM32H7RS_XSPI_CCR(base));
  putreg32(instruction, STM32H7RS_XSPI_IR(base));
  putreg32(address, STM32H7RS_XSPI_AR(base));

  ret = stm32h7rs_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF, STM32H7RS_XSPI_FCR(base));
  return ret;
}

static int stm32h7rs_xspi_readid(uint8_t id[3])
{
  uintptr_t base = STM32H7RS_XSPI2_BASE;
  volatile uint8_t *dr = (volatile uint8_t *)STM32H7RS_XSPI_DR(base);
  int ret;
  int i;

  ret = stm32h7rs_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32H7RS_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(2, STM32H7RS_XSPI_DLR(base));
  putreg32(0, STM32H7RS_XSPI_TCR(base));
  putreg32(XSPI_CCR_IMODE_1_LINE | XSPI_CCR_DMODE_1_LINE,
           STM32H7RS_XSPI_CCR(base));
  putreg32(MX66UW1G45G_READ_ID_CMD, STM32H7RS_XSPI_IR(base));

  modifyreg32(STM32H7RS_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_INDIRECT_READ);
  putreg32(MX66UW1G45G_READ_ID_CMD, STM32H7RS_XSPI_IR(base));

  for (i = 0; i < 3; i++)
    {
      ret = stm32h7rs_xspi_wait_flag(base, XSPI_SR_FTF | XSPI_SR_TCF);
      if (ret < 0)
        {
          return ret;
        }

      id[i] = *dr;
    }

  ret = stm32h7rs_xspi_wait_flag(base, XSPI_SR_TCF);
  putreg32(XSPI_FCR_CTCF, STM32H7RS_XSPI_FCR(base));
  return ret;
}

static bool stm32h7rs_xspi_id_blank(FAR const uint8_t id[3])
{
  return (id[0] == 0x00 && id[1] == 0x00 && id[2] == 0x00) ||
         (id[0] == 0xff && id[1] == 0xff && id[2] == 0xff);
}

static int stm32h7rs_xspi_read_data(uintptr_t base, uint32_t instruction,
                                    uint32_t address, uint32_t ccr,
                                    uint32_t tcr, size_t nbytes,
                                    uint8_t *value)
{
  volatile uint8_t *dr = (volatile uint8_t *)STM32H7RS_XSPI_DR(base);
  size_t i;
  int ret;

  if (nbytes == 0)
    {
      return -EINVAL;
    }

  ret = stm32h7rs_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32H7RS_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(nbytes - 1, STM32H7RS_XSPI_DLR(base));
  putreg32(tcr, STM32H7RS_XSPI_TCR(base));
  putreg32(ccr, STM32H7RS_XSPI_CCR(base));
  putreg32(instruction, STM32H7RS_XSPI_IR(base));
  putreg32(address, STM32H7RS_XSPI_AR(base));

  modifyreg32(STM32H7RS_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_INDIRECT_READ);
  putreg32(instruction, STM32H7RS_XSPI_IR(base));
  putreg32(address, STM32H7RS_XSPI_AR(base));

  for (i = 0; i < nbytes; i++)
    {
      uint8_t data;

      ret = stm32h7rs_xspi_wait_flag(base, XSPI_SR_FTF | XSPI_SR_TCF);
      if (ret < 0)
        {
          return ret;
        }

      data = *dr;
      if (i == 0)
        {
          *value = data;
        }
    }

  ret = stm32h7rs_xspi_wait_flag(base, XSPI_SR_TCF);
  putreg32(XSPI_FCR_CTCF, STM32H7RS_XSPI_FCR(base));
  return ret;
}

static int stm32h7rs_xspi_write_data(uintptr_t base, uint32_t instruction,
                                     uint32_t address, uint32_t ccr,
                                     uint32_t tcr, uint8_t value,
                                     size_t nbytes, bool repeat)
{
  volatile uint8_t *dr = (volatile uint8_t *)STM32H7RS_XSPI_DR(base);
  size_t i;
  int ret;

  if (nbytes == 0)
    {
      return -EINVAL;
    }

  ret = stm32h7rs_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32H7RS_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(nbytes - 1, STM32H7RS_XSPI_DLR(base));
  putreg32(tcr, STM32H7RS_XSPI_TCR(base));
  putreg32(ccr, STM32H7RS_XSPI_CCR(base));
  putreg32(instruction, STM32H7RS_XSPI_IR(base));
  putreg32(address, STM32H7RS_XSPI_AR(base));

  modifyreg32(STM32H7RS_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_INDIRECT_WRITE);
  for (i = 0; i < nbytes; i++)
    {
      *dr = (repeat || i == 0) ? value : 0;
    }

  ret = stm32h7rs_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF, STM32H7RS_XSPI_FCR(base));
  return ret;
}

static void stm32h7rs_xspi2_gpio_config(void)
{
  uint32_t pins = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) |
                  (1u << 4) | (1u << 5) | (1u << 6) | (1u << 8) |
                  (1u << 9) | (1u << 10) | (1u << 11);

  modifyreg32(STM32H7RS_RCC_AHB4ENR, 0, RCC_AHB4ENR_GPIONEN);
  (void)getreg32(STM32H7RS_RCC_AHB4ENR);
  stm32h7rs_xspi_config_gpio(STM32H7RS_GPION_BASE, pins, GPIO_AF_XSPIM_P2);
}

static void stm32h7rs_xspi1_gpio_config(void)
{
  uint32_t gpioo_pins = (1u << 0) | (1u << 2) | (1u << 3) | (1u << 4);
  uint32_t gpiop_pins = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) |
                        (1u << 4) | (1u << 5) | (1u << 6) | (1u << 7) |
                        (1u << 8) | (1u << 9) | (1u << 10) |
                        (1u << 11) | (1u << 12) | (1u << 13) |
                        (1u << 14) | (1u << 15);

  modifyreg32(STM32H7RS_RCC_AHB4ENR, 0,
              RCC_AHB4ENR_GPIOOEN | RCC_AHB4ENR_GPIOPEN);
  (void)getreg32(STM32H7RS_RCC_AHB4ENR);
  stm32h7rs_xspi_config_gpio(STM32H7RS_GPIOO_BASE, gpioo_pins,
                             GPIO_AF_XSPIM_P1);
  stm32h7rs_xspi_config_gpio(STM32H7RS_GPIOP_BASE, gpiop_pins,
                             GPIO_AF_XSPIM_P1);
}

static void stm32h7rs_xspi_controller_config(uintptr_t base, uint32_t reset,
                                             uint32_t memory_type,
                                             uint32_t device_size,
                                             uint32_t csht,
                                             uint32_t prescaler,
                                             uint32_t fifo_threshold,
                                             uint32_t csbound)
{
  modifyreg32(STM32H7RS_RCC_AHB5RSTR, 0, reset);
  modifyreg32(STM32H7RS_RCC_AHB5RSTR, reset, 0);

  putreg32(0, STM32H7RS_XSPI_CR(base));
  putreg32(memory_type | XSPI_DCR1_DEVSIZE(device_size) | XSPI_DCR1_CSHT(csht),
           STM32H7RS_XSPI_DCR1(base));
  putreg32(XSPI_DCR2_PRESCALER(prescaler), STM32H7RS_XSPI_DCR2(base));
  putreg32(XSPI_DCR3_CSBOUND(csbound), STM32H7RS_XSPI_DCR3(base));
  putreg32(0, STM32H7RS_XSPI_DCR4(base));
  putreg32((fifo_threshold - 1u) << XSPI_CR_FTHRES_SHIFT,
           STM32H7RS_XSPI_CR(base));
  modifyreg32(STM32H7RS_XSPI_CR(base), 0, XSPI_CR_EN);
}

static int stm32h7rs_xspi_set_prescaler(uintptr_t base, uint32_t prescaler)
{
  int ret;

  ret = stm32h7rs_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32H7RS_XSPI_DCR2(base), XSPI_DCR2_PRESCALER_MASK,
              XSPI_DCR2_PRESCALER(prescaler));
  return OK;
}

static int stm32h7rs_nor_read_cfg2_spi(uint32_t address, uint8_t *value)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_1_LINE | XSPI_CCR_ADMODE_1_LINE |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_1_LINE;

  return stm32h7rs_xspi_read_data(STM32H7RS_XSPI2_BASE,
                                  MX66UW1G45G_CFG2_READ_CMD, address,
                                  ccr, 0, 1, value);
}

static int stm32h7rs_nor_read_status_spi(uint8_t *status)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_1_LINE | XSPI_CCR_DMODE_1_LINE;
  return stm32h7rs_xspi_read_data(STM32H7RS_XSPI2_BASE,
                                  MX66UW1G45G_READ_STATUS_CMD, 0,
                                  ccr, 0, 1, status);
}

static int stm32h7rs_nor_read_status_octal(uint8_t *status)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16 |
        XSPI_CCR_ADMODE_8_LINES | XSPI_CCR_ADDTR |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  return stm32h7rs_xspi_read_data(STM32H7RS_XSPI2_BASE,
                                  MX66UW1G45G_OCTAL_READ_STATUS_CMD,
                                  0, ccr,
                                  XSPI_TCR_DCYC(MX66UW1G45G_OCTAL_REG_DUMMY),
                                  2, status);
}

static int stm32h7rs_nor_wait_status(
  int (*read_status)(uint8_t *status), uint8_t mask, uint8_t value)
{
  uint32_t timeout = STM32H7RS_XSPI_TIMEOUT;
  uint8_t status;
  int ret;

  do
    {
      ret = read_status(&status);
      if (ret < 0)
        {
          return ret;
        }

      if ((status & mask) == value)
        {
          return OK;
        }
    }
  while (timeout-- > 0);

  return -ETIMEDOUT;
}

static int stm32h7rs_nor_wait_wip_spi(void)
{
  return stm32h7rs_nor_wait_status(stm32h7rs_nor_read_status_spi,
                                   MX66UW1G45G_SR_WIP, 0);
}

static int stm32h7rs_nor_wait_wip_octal(void)
{
  return stm32h7rs_nor_wait_status(stm32h7rs_nor_read_status_octal,
                                   MX66UW1G45G_SR_WIP, 0);
}

static int stm32h7rs_nor_write_enable_spi(void)
{
  int ret;

  ret = stm32h7rs_xspi_command(STM32H7RS_XSPI2_BASE,
                               MX66UW1G45G_WRITE_ENABLE_CMD,
                               XSPI_CCR_IMODE_1_LINE);
  if (ret < 0)
    {
      return ret;
    }

  return stm32h7rs_nor_wait_status(stm32h7rs_nor_read_status_spi,
                                   MX66UW1G45G_SR_WEL,
                                   MX66UW1G45G_SR_WEL);
}

static int stm32h7rs_nor_write_cfg2_spi(uint32_t address, uint8_t value,
                                        bool wait_after)
{
  uint32_t ccr;
  int ret;

  ret = stm32h7rs_nor_wait_wip_spi();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_nor_write_enable_spi();
  if (ret < 0)
    {
      return ret;
    }

  ccr = XSPI_CCR_IMODE_1_LINE | XSPI_CCR_ADMODE_1_LINE |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_1_LINE;

  ret = stm32h7rs_xspi_write_data(STM32H7RS_XSPI2_BASE,
                                  MX66UW1G45G_CFG2_WRITE_CMD, address,
                                  ccr, 0, value, 1, false);
  if (ret < 0 || !wait_after)
    {
      return ret;
    }

  return stm32h7rs_nor_wait_wip_spi();
}

static int stm32h7rs_nor_read_cfg2_octal(uint32_t address, uint8_t *value)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16 |
        XSPI_CCR_ADMODE_8_LINES | XSPI_CCR_ADDTR |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  return stm32h7rs_xspi_read_data(STM32H7RS_XSPI2_BASE,
                                  MX66UW1G45G_OCTAL_CFG2_READAW_CMD,
                                  address, ccr,
                                  XSPI_TCR_DCYC(MX66UW1G45G_OCTAL_REG_DUMMY),
                                  2, value);
}

static int stm32h7rs_nor_config_cfg2_spi(uint32_t address, uint8_t value,
                                         uint8_t mask)
{
  uint8_t readback;
  uint8_t writeval;
  int ret;

  ret = stm32h7rs_nor_read_cfg2_spi(address, &readback);
  if (ret < 0)
    {
      return ret;
    }

  writeval = (readback & ~mask) | (value & mask);
  ret = stm32h7rs_nor_write_cfg2_spi(address, writeval, true);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_nor_read_cfg2_spi(address, &readback);
  if (ret < 0)
    {
      return ret;
    }

  if ((readback & mask) != (value & mask))
    {
      syslog(LOG_ERR, "XSPI2 NOR CFG2[0x%08" PRIx32
             "] readback %02x expected %02x\n",
             address, readback, value);
      return -EIO;
    }

  return OK;
}

static int stm32h7rs_xspi2_enter_memory_mapped(void)
{
  uintptr_t base = STM32H7RS_XSPI2_BASE;
  uint32_t ccr;
  int ret;

  ret = stm32h7rs_xspi_set_prescaler(base,
                                     STM32H7RS_XSPI_OPTIONAL_PRESCALER);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16 |
        XSPI_CCR_ADMODE_8_LINES | XSPI_CCR_ADDTR |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  modifyreg32(STM32H7RS_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_INDIRECT_WRITE);

  putreg32(ccr, STM32H7RS_XSPI_CCR(base));
  putreg32(XSPI_TCR_DCYC(MX66UW1G45G_OCTAL_READ_DUMMY),
           STM32H7RS_XSPI_TCR(base));
  putreg32(MX66UW1G45G_OCTAL_READ_CMD, STM32H7RS_XSPI_IR(base));

  putreg32(ccr, STM32H7RS_XSPI_WCCR(base));
  putreg32(0, STM32H7RS_XSPI_WTCR(base));
  putreg32(MX66UW1G45G_OCTAL_WRITE_CMD, STM32H7RS_XSPI_WIR(base));

  modifyreg32(STM32H7RS_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_MEMORY_MAPPED);
  return OK;
}

static int stm32h7rs_xspi1_enter_memory_mapped(void)
{
  uintptr_t base = STM32H7RS_XSPI1_BASE;
  uint32_t ccr;
  int ret;

  ret = stm32h7rs_xspi_set_prescaler(base,
                                     STM32H7RS_XSPI_OPTIONAL_PRESCALER);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_16_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  modifyreg32(STM32H7RS_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_INDIRECT_WRITE);
  putreg32(ccr, STM32H7RS_XSPI_CCR(base));
  putreg32(XSPI_TCR_DCYC(APS256_HEXA_DUMMY), STM32H7RS_XSPI_TCR(base));
  putreg32(APS256_READ_CMD, STM32H7RS_XSPI_IR(base));

  putreg32(ccr, STM32H7RS_XSPI_WCCR(base));
  putreg32(XSPI_TCR_DCYC(APS256_HEXA_DUMMY), STM32H7RS_XSPI_WTCR(base));
  putreg32(APS256_HEXA_WRITE_CMD, STM32H7RS_XSPI_WIR(base));

  modifyreg32(STM32H7RS_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_MEMORY_MAPPED);
  return OK;
}

static int stm32h7rs_psram_write_reg(uint32_t address, uint8_t value)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  return stm32h7rs_xspi_write_data(STM32H7RS_XSPI1_BASE,
                                   APS256_REG_WRITE_CMD, address, ccr,
                                   0, value, 2, true);
}

static int stm32h7rs_psram_read_reg(uint32_t address, uint8_t *value)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  return stm32h7rs_xspi_read_data(STM32H7RS_XSPI1_BASE,
                                  APS256_REG_READ_CMD, address, ccr,
                                  XSPI_TCR_DCYC(APS256_REG_DUMMY),
                                  2, value);
}

static int stm32h7rs_psram_config_reg(uint32_t address, uint8_t value,
                                      uint8_t mask)
{
  uint8_t initial;
  uint8_t readback;
  uint8_t writeval;
  int ret;

  ret = stm32h7rs_psram_read_reg(address, &initial);
  if (ret < 0)
    {
      return ret;
    }

  writeval = (initial & ~mask) | (value & mask);
  ret = stm32h7rs_psram_write_reg(address, writeval);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_psram_read_reg(address, &readback);
  if (ret < 0)
    {
      return ret;
    }

  if ((readback & mask) != (value & mask))
    {
      ferr("ERROR: PSRAM reg 0x%08" PRIx32 " readback %02x expected %02x\n",
           address, readback, value);
      syslog(LOG_ERR, "XSPI1 PSRAM MR%08" PRIx32
             " initial %02x write %02x readback %02x expected %02x\n",
             address, initial, writeval, readback, value);
      return -EIO;
    }

  syslog(LOG_INFO, "XSPI1 PSRAM MR%08" PRIx32
         " initial %02x write %02x readback %02x\n",
         address, initial, writeval, readback);
  return OK;
}

#ifdef CONFIG_NXBOOT_BOOTLOADER
static int stm32h7rs_psram_selftest(void)
{
  volatile uint32_t *mem = (volatile uint32_t *)STM32H7RS_XSPI1_MEM_BASE;
  uint32_t saved[8];
  uint32_t pattern[8] =
    {
      0x55aa55aau, 0xaa55aa55u, 0x01234567u, 0x89abcdefu,
      0xf0f00f0fu, 0x0f0ff0f0u, 0x13579bdfu, 0x2468ace0u
    };
  int i;

  for (i = 0; i < 8; i++)
    {
      saved[i] = mem[i];
    }

  for (i = 0; i < 8; i++)
    {
      mem[i] = pattern[i];
    }

  for (i = 0; i < 8; i++)
    {
      if (mem[i] != pattern[i])
        {
          ferr("ERROR: PSRAM self-test[%d] got %08" PRIx32
               " expected %08" PRIx32 "\n", i, mem[i], pattern[i]);
          while (i-- > 0)
            {
              mem[i] = saved[i];
            }

          return -EIO;
        }
    }

  for (i = 0; i < 8; i++)
    {
      mem[i] = saved[i];
    }

  syslog(LOG_INFO, "XSPI1 PSRAM self-test passed\n");
  return OK;
}
#else
static int stm32h7rs_psram_selftest(void)
{
  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32h7rs_xspi2_nor_initialize(void)
{
  uint8_t cfg2;
  uint32_t header;
  uint8_t id[3] = {0};
  int ret;

  if (stm32h7rs_xspi_is_mapped(STM32H7RS_XSPI2_BASE))
    {
      syslog(LOG_INFO, "XSPI2 NOR already memory-mapped\n");
      return OK;
    }

  ret = stm32h7rs_xspi_common_setup();
  if (ret < 0)
    {
      ferr("ERROR: XSPI common setup failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR common setup failed: %d\n", ret);
      return ret;
    }

  stm32h7rs_xspi2_gpio_config();
  stm32h7rs_xspi_controller_config(STM32H7RS_XSPI2_BASE,
                                   RCC_AHB5RSTR_XSPI2RST,
                                   XSPI_DCR1_MTYP_MACRONIX,
                                   MX66UW1G45G_DEVSIZE, 2,
                                   STM32H7RS_XSPI_STARTUP_PRESCALER, 1,
                                   0);

  /* Cube's MX66UW1G45G reset method is 6699/9966 followed by 66/99 when
   * optional octal configuration is present.  This lets the loader recover
   * a device left in Octal/DTR mode by a previous external loader or app.
   */

  ret = stm32h7rs_xspi_command(STM32H7RS_XSPI2_BASE,
                               MX66UW1G45G_OCTA_RESET_ENABLE_CMD,
                               XSPI_CCR_IMODE_8_LINES |
                               XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16);
  if (ret < 0)
    {
      syslog(LOG_WARNING, "XSPI2 NOR octal reset-enable failed: %d\n",
             ret);
    }

  ret = stm32h7rs_xspi_command(STM32H7RS_XSPI2_BASE,
                               MX66UW1G45G_OCTA_RESET_MEMORY_CMD,
                               XSPI_CCR_IMODE_8_LINES |
                               XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16);
  if (ret < 0)
    {
      syslog(LOG_WARNING, "XSPI2 NOR octal reset failed: %d\n", ret);
    }

  up_mdelay(STM32H7RS_XSPI_NOR_RESET_DELAY_MS);

  ret = stm32h7rs_xspi_command(STM32H7RS_XSPI2_BASE,
                               MX66UW1G45G_RESET_ENABLE_CMD,
                               XSPI_CCR_IMODE_1_LINE);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR reset-enable failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR reset-enable failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7rs_xspi_command(STM32H7RS_XSPI2_BASE,
                               MX66UW1G45G_RESET_MEMORY_CMD,
                               XSPI_CCR_IMODE_1_LINE);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR reset failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR reset failed: %d\n", ret);
      return ret;
    }

  up_mdelay(STM32H7RS_XSPI_NOR_RESET_DELAY_MS);

  ret = stm32h7rs_xspi_readid(id);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR read-id failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR read-id failed: %d sr=%08" PRIx32 "\n",
             ret, getreg32(STM32H7RS_XSPI_SR(STM32H7RS_XSPI2_BASE)));
      return ret;
    }

  syslog(LOG_INFO, "XSPI2 NOR JEDEC ID %02x %02x %02x\n",
         id[0], id[1], id[2]);
  if (stm32h7rs_xspi_id_blank(id))
    {
      ferr("ERROR: XSPI2 NOR JEDEC ID is blank\n");
      syslog(LOG_ERR, "XSPI2 NOR JEDEC ID is blank\n");
      syslog(LOG_ERR, "XSPI2 regs CR=%08" PRIx32 " SR=%08" PRIx32
             " DCR1=%08" PRIx32 " DCR2=%08" PRIx32
             " CCR=%08" PRIx32 " TCR=%08" PRIx32
             " DLR=%08" PRIx32 "\n",
             getreg32(STM32H7RS_XSPI_CR(STM32H7RS_XSPI2_BASE)),
             getreg32(STM32H7RS_XSPI_SR(STM32H7RS_XSPI2_BASE)),
             getreg32(STM32H7RS_XSPI_DCR1(STM32H7RS_XSPI2_BASE)),
             getreg32(STM32H7RS_XSPI_DCR2(STM32H7RS_XSPI2_BASE)),
             getreg32(STM32H7RS_XSPI_CCR(STM32H7RS_XSPI2_BASE)),
             getreg32(STM32H7RS_XSPI_TCR(STM32H7RS_XSPI2_BASE)),
             getreg32(STM32H7RS_XSPI_DLR(STM32H7RS_XSPI2_BASE)));
      return -EIO;
    }

  ret = stm32h7rs_nor_config_cfg2_spi(MX66UW1G45G_CFG2_DOPI_ADDR,
                                      MX66UW1G45G_CFG2_DOPI_VALUE,
                                      MX66UW1G45G_CFG2_DOPI_MASK);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR DOPI config failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR DOPI config failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7rs_nor_read_cfg2_spi(MX66UW1G45G_CFG2_OPI_ADDR, &cfg2);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR OPI config read failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR OPI config read failed: %d\n", ret);
      return ret;
    }

  cfg2 = (cfg2 & ~MX66UW1G45G_CFG2_OPI_MASK) |
         (MX66UW1G45G_CFG2_OPI_VALUE & MX66UW1G45G_CFG2_OPI_MASK);
  ret = stm32h7rs_nor_write_cfg2_spi(MX66UW1G45G_CFG2_OPI_ADDR, cfg2,
                                     false);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR OPI config write failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR OPI config write failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7rs_xspi_set_prescaler(STM32H7RS_XSPI2_BASE,
                                     STM32H7RS_XSPI_OPTIONAL_PRESCALER);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_nor_wait_wip_octal();
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR optional WIP wait failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR optional WIP wait failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7rs_nor_read_cfg2_octal(MX66UW1G45G_CFG2_OPI_ADDR, &cfg2);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR optional read-after-write failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR optional read-after-write failed: %d\n",
             ret);
      return ret;
    }

  if ((cfg2 & MX66UW1G45G_CFG2_OPI_MASK) != MX66UW1G45G_CFG2_OPI_VALUE)
    {
      ferr("ERROR: XSPI2 NOR OPI readback %02x expected %02x\n",
           cfg2, MX66UW1G45G_CFG2_OPI_VALUE);
      syslog(LOG_ERR, "XSPI2 NOR OPI readback %02x expected %02x\n",
             cfg2, MX66UW1G45G_CFG2_OPI_VALUE);
      return -EIO;
    }

  syslog(LOG_INFO, "XSPI2 NOR OPI/DTR config readback %02x\n", cfg2);

  ret = stm32h7rs_xspi2_enter_memory_mapped();
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR memory-map failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR memory-map failed: %d\n", ret);
      return ret;
    }

  header = *(volatile uint32_t *)STM32H7RS_XSPI2_MEM_BASE;
  syslog(LOG_INFO, "XSPI2 NOR mapped 0x%08x header[0]=0x%08" PRIx32 "\n",
         STM32H7RS_XSPI2_MEM_BASE, header);
  return OK;
}

int stm32h7rs_xspi1_psram_initialize(void)
{
  int ret;

  if (stm32h7rs_xspi_is_mapped(STM32H7RS_XSPI1_BASE))
    {
      syslog(LOG_INFO, "XSPI1 PSRAM already memory-mapped\n");
      return OK;
    }

  ret = stm32h7rs_xspi_common_setup();
  if (ret < 0)
    {
      ferr("ERROR: XSPI common setup failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM common setup failed: %d\n", ret);
      return ret;
    }

  stm32h7rs_xspi1_gpio_config();
  stm32h7rs_xspi_controller_config(STM32H7RS_XSPI1_BASE,
                                   RCC_AHB5RSTR_XSPI1RST,
                                   XSPI_DCR1_MTYP_APMEM_16BIT,
                                   APS256_DEVSIZE, 5,
                                   STM32H7RS_XSPI_STARTUP_PRESCALER, 2,
                                   10);

  ret = stm32h7rs_xspi_command_addr(STM32H7RS_XSPI1_BASE,
                                    APS256_RESET_CMD, 0,
                                    XSPI_CCR_IMODE_8_LINES |
                                    XSPI_CCR_ADMODE_8_LINES |
                                    XSPI_CCR_ADSIZE_32,
                                    0);
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM reset failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM reset failed: %d\n", ret);
      return ret;
    }

  up_mdelay(APS256_RESET_DELAY_MS);

  ret = stm32h7rs_psram_config_reg(APS256_MR0_ADDR, APS256_MR0_VALUE, 0x3f);
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM MR0 config failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM MR0 config failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7rs_psram_config_reg(APS256_MR4_ADDR, APS256_MR4_VALUE, 0xe0);
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM MR4 config failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM MR4 config failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7rs_psram_config_reg(APS256_MR8_ADDR, APS256_MR8_VALUE, 0x40);
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM MR8 config failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM MR8 config failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7rs_xspi1_enter_memory_mapped();
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM memory-map failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM memory-map failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7rs_psram_selftest();
  if (ret < 0)
    {
      return ret;
    }

  syslog(LOG_INFO, "XSPI1 PSRAM mapped at 0x%08x\n",
         STM32H7RS_XSPI1_MEM_BASE);
  return OK;
}
