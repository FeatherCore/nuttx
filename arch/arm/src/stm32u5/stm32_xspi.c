/****************************************************************************
 * arch/arm/src/stm32u5/stm32_xspi.c
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
#include <stdint.h>
#include <syslog.h>

#include "arm_internal.h"
#include "stm32_xspi.h"

#include "hardware/stm32_memorymap.h"
#include "hardware/stm32u5xx_rcc.h"

#if defined(CONFIG_STM32U5_OCTOSPI1) || defined(CONFIG_STM32U5_OCTOSPI2) || \
    defined(CONFIG_STM32U5_HSPI1)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_XSPI_TIMEOUT                 1000000

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uintptr_t stm32_xspi_base(enum stm32_xspi_port_e port)
{
  switch (port)
    {
#ifdef CONFIG_STM32U5_OCTOSPI1
      case STM32_XSPI_PORT_OCTOSPI1:
        return STM32_OCTOSPI1_BASE;
#endif

#ifdef CONFIG_STM32U5_OCTOSPI2
      case STM32_XSPI_PORT_OCTOSPI2:
        return STM32_OCTOSPI2_BASE;
#endif

#ifdef CONFIG_STM32U5_HSPI1
      case STM32_XSPI_PORT_HSPI1:
        return STM32_HSPI1_REG_BASE;
#endif

      default:
        return 0;
    }
}

static uint32_t stm32_xspi_reset_bit(enum stm32_xspi_port_e port)
{
  switch (port)
    {
#ifdef CONFIG_STM32U5_OCTOSPI1
      case STM32_XSPI_PORT_OCTOSPI1:
        return RCC_AHB2RSTR2_OCTOSPI1RST;
#endif

#ifdef CONFIG_STM32U5_OCTOSPI2
      case STM32_XSPI_PORT_OCTOSPI2:
        return RCC_AHB2RSTR2_OCTOSPI2RST;
#endif

#ifdef CONFIG_STM32U5_HSPI1
      case STM32_XSPI_PORT_HSPI1:
        return RCC_AHB2RSTR2_HSPI1RST;
#endif

      default:
        return 0;
    }
}

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

static int stm32_xspi_wait_idle(uintptr_t base)
{
  return stm32_xspi_wait_reg(STM32_XSPI_SR(base), XSPI_SR_BUSY, 0);
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
          syslog(LOG_ERR,
                 "stm32u5: XSPI flag timeout base=%08" PRIxPTR
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

int stm32_xspi_common_setup(void)
{
  uint32_t enr2 = 0;

  modifyreg32(STM32_RCC_CCIPR2,
              RCC_CCIPR2_OSPISEL_MASK | RCC_CCIPR2_HSPISEL_MASK,
              RCC_CCIPR2_OSPISEL_SYSCLK | RCC_CCIPR2_HSPISEL_SYSCLK);

#ifdef CONFIG_STM32U5_OCTOSPIM
  modifyreg32(STM32_RCC_AHB2ENR1, 0, RCC_AHB2ENR1_OCTOSPIMEN);
#endif

#ifdef CONFIG_STM32U5_OCTOSPI1
  enr2 |= RCC_AHB2ENR2_OCTOSPI1EN;
#endif

#ifdef CONFIG_STM32U5_OCTOSPI2
  enr2 |= RCC_AHB2ENR2_OCTOSPI2EN;
#endif

#ifdef CONFIG_STM32U5_HSPI1
  enr2 |= RCC_AHB2ENR2_HSPI1EN;
#endif

  modifyreg32(STM32_RCC_AHB2ENR2, 0, enr2);
  (void)getreg32(STM32_RCC_AHB2ENR2);

#ifdef CONFIG_STM32U5_OCTOSPIM
  putreg32(XSPIM_CR_REQ2ACK_TIME(1), STM32_XSPIM_CR);
#endif

  return OK;
}

int stm32_xspi_configure(FAR const struct stm32_xspi_config_s *config)
{
  uintptr_t base;
  uint32_t reset;

  if (config == NULL || config->fifo_threshold == 0)
    {
      return -EINVAL;
    }

  base = stm32_xspi_base(config->port);
  reset = stm32_xspi_reset_bit(config->port);
  if (base == 0 || reset == 0)
    {
      return -ENODEV;
    }

  modifyreg32(STM32_RCC_AHB2RSTR2, 0, reset);
  modifyreg32(STM32_RCC_AHB2RSTR2, reset, 0);

  putreg32(0, STM32_XSPI_CR(base));
  putreg32(config->memory_type | XSPI_DCR1_DEVSIZE(config->device_size) |
           XSPI_DCR1_CSHT(config->csht), STM32_XSPI_DCR1(base));
  putreg32(XSPI_DCR2_PRESCALER(config->prescaler),
           STM32_XSPI_DCR2(base));
  putreg32(XSPI_DCR3_MAXTRAN(config->maxtran) |
           XSPI_DCR3_CSBOUND(config->csbound), STM32_XSPI_DCR3(base));
  putreg32(XSPI_DCR4_REFRESH(config->refresh), STM32_XSPI_DCR4(base));
  putreg32((config->fifo_threshold - 1) << XSPI_CR_FTHRES_SHIFT,
           STM32_XSPI_CR(base));
  modifyreg32(STM32_XSPI_CR(base), 0, XSPI_CR_EN);
  return OK;
}

bool stm32_xspi_is_mapped(enum stm32_xspi_port_e port)
{
  uintptr_t base = stm32_xspi_base(port);
  uint32_t regval;

  if (base == 0)
    {
      return false;
    }

  regval = getreg32(STM32_XSPI_CR(base));
  return (regval & XSPI_CR_EN) != 0 &&
         (regval & XSPI_CR_FMODE_MASK) == XSPI_CR_FMODE_MEMORY_MAPPED;
}

int stm32_xspi_set_prescaler(enum stm32_xspi_port_e port,
                             uint32_t prescaler)
{
  uintptr_t base = stm32_xspi_base(port);
  int ret;

  if (base == 0)
    {
      return -ENODEV;
    }

  ret = stm32_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32_XSPI_DCR2(base), XSPI_DCR2_PRESCALER_MASK,
              XSPI_DCR2_PRESCALER(prescaler));
  return OK;
}

int stm32_xspi_abort(enum stm32_xspi_port_e port)
{
  uintptr_t base = stm32_xspi_base(port);
  uint32_t timeout = STM32_XSPI_TIMEOUT;
  int ret;

  if (base == 0)
    {
      return -ENODEV;
    }

  modifyreg32(STM32_XSPI_CR(base), 0, XSPI_CR_ABORT);
  while ((getreg32(STM32_XSPI_CR(base)) & XSPI_CR_ABORT) != 0)
    {
      if (timeout-- == 0)
        {
          return -ETIMEDOUT;
        }
    }

  ret = stm32_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(XSPI_FCR_CTEF | XSPI_FCR_CTCF, STM32_XSPI_FCR(base));
  return OK;
}

int stm32_xspi_command(enum stm32_xspi_port_e port, uint32_t instruction,
                       uint32_t ccr)
{
  uintptr_t base = stm32_xspi_base(port);
  int ret;

  if (base == 0)
    {
      return -ENODEV;
    }

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

int stm32_xspi_command_addr(enum stm32_xspi_port_e port,
                            uint32_t instruction, uint32_t address,
                            uint32_t ccr, uint32_t tcr)
{
  uintptr_t base = stm32_xspi_base(port);
  int ret;

  if (base == 0)
    {
      return -ENODEV;
    }

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

int stm32_xspi_read_data(enum stm32_xspi_port_e port, uint32_t instruction,
                         uint32_t address, uint32_t ccr, uint32_t tcr,
                         size_t nbytes, FAR uint8_t *buffer)
{
  uintptr_t base = stm32_xspi_base(port);
  volatile uint8_t *dr;
  size_t i;
  int ret;

  if (base == 0)
    {
      return -ENODEV;
    }

  if (nbytes == 0 || buffer == NULL)
    {
      return -EINVAL;
    }

  dr = (volatile uint8_t *)STM32_XSPI_DR(base);

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

int stm32_xspi_write_data(enum stm32_xspi_port_e port, uint32_t instruction,
                          uint32_t address, uint32_t ccr, uint32_t tcr,
                          uint8_t value, size_t nbytes, bool repeat)
{
  uintptr_t base = stm32_xspi_base(port);
  volatile uint8_t *dr;
  size_t i;
  int ret;

  if (base == 0)
    {
      return -ENODEV;
    }

  if (nbytes == 0)
    {
      return -EINVAL;
    }

  dr = (volatile uint8_t *)STM32_XSPI_DR(base);

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

  for (i = 0; i < nbytes; i++)
    {
      *dr = (repeat || i == 0) ? value : 0;
    }

  ret = stm32_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF, STM32_XSPI_FCR(base));
  return ret;
}

int stm32_xspi_write_buffer(enum stm32_xspi_port_e port,
                            uint32_t instruction, uint32_t address,
                            uint32_t ccr, uint32_t tcr, size_t nbytes,
                            FAR const uint8_t *buffer)
{
  uintptr_t base = stm32_xspi_base(port);
  volatile uint8_t *dr;
  size_t i;
  int ret;

  if (base == 0)
    {
      return -ENODEV;
    }

  if (nbytes == 0 || buffer == NULL)
    {
      return -EINVAL;
    }

  dr = (volatile uint8_t *)STM32_XSPI_DR(base);

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

  for (i = 0; i < nbytes; i++)
    {
      *dr = buffer[i];
    }

  ret = stm32_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF, STM32_XSPI_FCR(base));
  return ret;
}

int stm32_xspi_enter_memory_mapped(enum stm32_xspi_port_e port,
                                   uint32_t readccr, uint32_t readtcr,
                                   uint32_t readinst)
{
  return stm32_xspi_enter_memory_mapped_rw(port, readccr, readtcr,
                                           readinst, 0, 0, 0);
}

int stm32_xspi_enter_memory_mapped_rw(enum stm32_xspi_port_e port,
                                      uint32_t readccr, uint32_t readtcr,
                                      uint32_t readinst,
                                      uint32_t writeccr,
                                      uint32_t writetcr,
                                      uint32_t writeinst)
{
  uintptr_t base = stm32_xspi_base(port);
  int ret;

  if (base == 0)
    {
      return -ENODEV;
    }

  ret = stm32_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  putreg32(readccr, STM32_XSPI_CCR(base));
  putreg32(readtcr, STM32_XSPI_TCR(base));
  putreg32(readinst, STM32_XSPI_IR(base));

  if (writeccr != 0)
    {
      putreg32(writeccr, STM32_XSPI_WCCR(base));
      putreg32(writetcr, STM32_XSPI_WTCR(base));
      putreg32(writeinst, STM32_XSPI_WIR(base));
    }

  modifyreg32(STM32_XSPI_CR(base), XSPI_CR_FMODE_MASK,
              XSPI_CR_FMODE_MEMORY_MAPPED);
  return OK;
}

#endif /* OCTOSPI1 || OCTOSPI2 || HSPI1 */
