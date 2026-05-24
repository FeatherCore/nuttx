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

#include "hardware/stm32_dlyb.h"
#include "hardware/stm32_memorymap.h"
#include "hardware/stm32u5xx_rcc.h"

#if defined(CONFIG_STM32U5_OCTOSPI1) || defined(CONFIG_STM32U5_OCTOSPI2) || \
    defined(CONFIG_STM32U5_HSPI1)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_XSPI_TIMEOUT                 1000000
#define STM32_XSPI_DLYB_TIMEOUT            100000
#define STM32_XSPI_DLYB_MAX_UNIT           0x80u
#define STM32_XSPI_DLYB_MAX_SELECT         0x0cu
#define STM32_XSPI_DLYB_LNG_10_0_MASK      0x07ff0000u
#define STM32_XSPI_DLYB_LNG_11_10_MASK     0x0c000000u

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

static uintptr_t stm32_xspi_dlyb_base(enum stm32_xspi_port_e port)
{
  switch (port)
    {
#ifdef CONFIG_STM32U5_OCTOSPI1
      case STM32_XSPI_PORT_OCTOSPI1:
        return STM32_DLYBOS1_BASE;
#endif

#ifdef CONFIG_STM32U5_OCTOSPI2
      case STM32_XSPI_PORT_OCTOSPI2:
        return STM32_DLYBOS2_BASE;
#endif

      default:
        return 0;
    }
}

static void stm32_xspi_frck_enable(uintptr_t base, bool enable)
{
  modifyreg32(STM32_XSPI_DCR1(base),
              enable ? 0 : XSPI_DCR1_FRCK,
              enable ? XSPI_DCR1_FRCK : 0);
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

static int stm32_xspi_abort_base(uintptr_t base)
{
  uint32_t timeout = STM32_XSPI_TIMEOUT;
  int ret;

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

static int stm32_xspi_dlyb_get_period(enum stm32_xspi_port_e port,
                                      FAR uint32_t *phase,
                                      FAR uint32_t *units)
{
  uintptr_t base = stm32_xspi_base(port);
  uintptr_t dlybbase = stm32_xspi_dlyb_base(port);
  uint32_t timeout;
  uint32_t regval;
  uint32_t lng;
  uint32_t nb;
  uint32_t i;

  if (base == 0 || dlybbase == 0)
    {
      return -ENODEV;
    }

  stm32_xspi_frck_enable(base, true);
  modifyreg32(STM32_DLYB_CR(dlybbase), 0, DLYB_CR_DEN | DLYB_CR_SEN);

  for (i = 0; i < STM32_XSPI_DLYB_MAX_UNIT; i++)
    {
      putreg32(STM32_XSPI_DLYB_MAX_SELECT | DLYB_CFGR_UNIT(i),
               STM32_DLYB_CFGR(dlybbase));

      timeout = STM32_XSPI_DLYB_TIMEOUT;
      while ((getreg32(STM32_DLYB_CFGR(dlybbase)) & DLYB_CFGR_LNGF) == 0)
        {
          if (timeout-- == 0)
            {
              modifyreg32(STM32_DLYB_CR(dlybbase),
                          DLYB_CR_DEN | DLYB_CR_SEN, 0);
              stm32_xspi_frck_enable(base, false);
              return -ETIMEDOUT;
            }
        }

      regval = getreg32(STM32_DLYB_CFGR(dlybbase));
      if ((regval & STM32_XSPI_DLYB_LNG_10_0_MASK) != 0 &&
          (regval & STM32_XSPI_DLYB_LNG_11_10_MASK) !=
          STM32_XSPI_DLYB_LNG_11_10_MASK)
        {
          break;
        }
    }

  if (i >= STM32_XSPI_DLYB_MAX_UNIT)
    {
      modifyreg32(STM32_DLYB_CR(dlybbase), DLYB_CR_DEN | DLYB_CR_SEN, 0);
      stm32_xspi_frck_enable(base, false);
      return -EIO;
    }

  lng = (getreg32(STM32_DLYB_CFGR(dlybbase)) &
         DLYB_CFGR_LNG_MASK) >> DLYB_CFGR_LNG_SHIFT;
  nb = 10;
  while (nb > 0 && (lng >> nb) == 0)
    {
      nb--;
    }

  modifyreg32(STM32_DLYB_CR(dlybbase), DLYB_CR_DEN | DLYB_CR_SEN, 0);
  stm32_xspi_frck_enable(base, false);

  if (nb == 0)
    {
      return -EIO;
    }

  *phase = nb;
  *units = i;
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

#ifdef CONFIG_STM32U5_OCTOSPIM
int stm32_xspim_configure(FAR const struct stm32_xspim_config_s *config)
{
  uint32_t instance;
  uint32_t port;

  if (config == NULL || config->clk_port == 0 || config->ncs_port == 0 ||
      config->req2ack_time == 0 || config->req2ack_time > 256 ||
      config->clk_port > XSPIM_PORT_MAX ||
      config->dqs_port > XSPIM_PORT_MAX ||
      config->ncs_port > XSPIM_PORT_MAX ||
      config->iolow_port > XSPIM_PORT_MAX ||
      config->iohigh_port > XSPIM_PORT_MAX)
    {
      return -EINVAL;
    }

  switch (config->port)
    {
#ifdef CONFIG_STM32U5_OCTOSPI1
      case STM32_XSPI_PORT_OCTOSPI1:
        instance = 0;
        break;
#endif

#ifdef CONFIG_STM32U5_OCTOSPI2
      case STM32_XSPI_PORT_OCTOSPI2:
        instance = 1;
        break;
#endif

      default:
        return -ENODEV;
    }

  modifyreg32(STM32_XSPIM_CR, XSPIM_CR_REQ2ACK_TIME_MASK,
              XSPIM_CR_REQ2ACK_TIME(config->req2ack_time));

  port = config->clk_port - 1;
  modifyreg32(STM32_XSPIM_PCR(port),
              XSPIM_PCR_CLKEN | XSPIM_PCR_CLKSRC_MASK,
              XSPIM_PCR_CLKEN |
              (instance << XSPIM_PCR_CLKSRC_SHIFT));

  if (config->dqs_port != 0)
    {
      port = config->dqs_port - 1;
      modifyreg32(STM32_XSPIM_PCR(port),
                  XSPIM_PCR_DQSEN | XSPIM_PCR_DQSSRC_MASK,
                  XSPIM_PCR_DQSEN |
                  (instance << XSPIM_PCR_DQSSRC_SHIFT));
    }

  port = config->ncs_port - 1;
  modifyreg32(STM32_XSPIM_PCR(port),
              XSPIM_PCR_NCSEN | XSPIM_PCR_NCSSRC_MASK,
              XSPIM_PCR_NCSEN |
              (instance << XSPIM_PCR_NCSSRC_SHIFT));

  if (config->iolow_port != 0)
    {
      port = config->iolow_port - 1;
      modifyreg32(STM32_XSPIM_PCR(port),
                  XSPIM_PCR_IOLEN | XSPIM_PCR_IOLSRC_MASK,
                  XSPIM_PCR_IOLEN |
                  (instance << (XSPIM_PCR_IOLSRC_SHIFT + 1)));
    }

  if (config->iohigh_port != 0)
    {
      port = config->iohigh_port - 1;
      modifyreg32(STM32_XSPIM_PCR(port),
                  XSPIM_PCR_IOHEN | XSPIM_PCR_IOHSRC_MASK,
                  XSPIM_PCR_IOHEN |
                  XSPIM_PCR_IOHSRC_0 |
                  (instance << (XSPIM_PCR_IOHSRC_SHIFT + 1)));
    }

  return OK;
}
#endif

int stm32_xspi_configure(FAR const struct stm32_xspi_config_s *config)
{
  uintptr_t base;
  uint32_t reset;
  int ret;

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
  putreg32(XSPI_DCR3_MAXTRAN(config->maxtran) |
           XSPI_DCR3_CSBOUND(config->csbound), STM32_XSPI_DCR3(base));
  putreg32(XSPI_DCR4_REFRESH(config->refresh), STM32_XSPI_DCR4(base));
  putreg32((config->fifo_threshold - 1) << XSPI_CR_FTHRES_SHIFT,
           STM32_XSPI_CR(base));

  ret = stm32_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

  putreg32(XSPI_DCR2_PRESCALER(config->prescaler),
           STM32_XSPI_DCR2(base));

  ret = stm32_xspi_wait_idle(base);
  if (ret < 0)
    {
      return ret;
    }

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

int stm32_xspi_set_memory_type(enum stm32_xspi_port_e port,
                               uint32_t memory_type)
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

  modifyreg32(STM32_XSPI_DCR1(base), XSPI_DCR1_MTYP_MASK, memory_type);
  return OK;
}

int stm32_xspi_dlyb_set(enum stm32_xspi_port_e port, uint32_t phase,
                        uint32_t units)
{
  uintptr_t base = stm32_xspi_base(port);
  uintptr_t dlybbase = stm32_xspi_dlyb_base(port);
  uint32_t regval;
  int ret;

  if (base == 0 || dlybbase == 0)
    {
      return -ENODEV;
    }

  if (phase > STM32_XSPI_DLYB_MAX_SELECT ||
      units >= STM32_XSPI_DLYB_MAX_UNIT)
    {
      return -EINVAL;
    }

  stm32_xspi_frck_enable(base, true);
  modifyreg32(STM32_DLYB_CR(dlybbase), 0, DLYB_CR_DEN | DLYB_CR_SEN);
  putreg32((phase & DLYB_CFGR_SEL_MASK) | DLYB_CFGR_UNIT(units),
           STM32_DLYB_CFGR(dlybbase));
  modifyreg32(STM32_DLYB_CR(dlybbase), DLYB_CR_SEN, 0);

  ret = stm32_xspi_abort_base(base);
  stm32_xspi_frck_enable(base, false);
  if (ret < 0)
    {
      return ret;
    }

  regval = getreg32(STM32_DLYB_CFGR(dlybbase));
  if ((regval & DLYB_CFGR_SEL_MASK) != phase ||
      ((regval & DLYB_CFGR_UNIT_MASK) >> DLYB_CFGR_UNIT_SHIFT) != units)
    {
      return -EIO;
    }

  return OK;
}

int stm32_xspi_dlyb_configure(enum stm32_xspi_port_e port,
                              FAR uint32_t *phase, FAR uint32_t *units)
{
  uint32_t cfgphase;
  uint32_t cfgunits;
  int ret;

  ret = stm32_xspi_dlyb_get_period(port, &cfgphase, &cfgunits);
  if (ret < 0)
    {
      return ret;
    }

  cfgphase /= 4u;
  ret = stm32_xspi_dlyb_set(port, cfgphase, cfgunits);
  if (ret < 0)
    {
      return ret;
    }

  if (phase != NULL)
    {
      *phase = cfgphase;
    }

  if (units != NULL)
    {
      *units = cfgunits;
    }

  return OK;
}

int stm32_xspi_abort(enum stm32_xspi_port_e port)
{
  uintptr_t base = stm32_xspi_base(port);

  if (base == 0)
    {
      return -ENODEV;
    }

  return stm32_xspi_abort_base(base);
}

int stm32_xspi_command_tcr(enum stm32_xspi_port_e port,
                           uint32_t instruction, uint32_t ccr,
                           uint32_t tcr)
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

  ret = stm32_xspi_wait_idle(base);
  putreg32(XSPI_FCR_CTCF, STM32_XSPI_FCR(base));
  return ret;
}

int stm32_xspi_command(enum stm32_xspi_port_e port, uint32_t instruction,
                       uint32_t ccr)
{
  return stm32_xspi_command_tcr(port, instruction, ccr, 0);
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

  if ((ccr & XSPI_CCR_ADMODE_MASK) != 0)
    {
      putreg32(address, STM32_XSPI_AR(base));
    }
  else
    {
      putreg32(instruction, STM32_XSPI_IR(base));
    }

  for (i = 0; i < nbytes; i++)
    {
      ret = stm32_xspi_wait_flag(base, XSPI_SR_FTF | XSPI_SR_TCF);
      if (ret < 0)
        {
          (void)stm32_xspi_abort_base(base);
          return ret;
        }

      buffer[i] = *dr;
    }

  ret = stm32_xspi_wait_flag(base, XSPI_SR_TCF);
  if (ret < 0)
    {
      (void)stm32_xspi_abort_base(base);
      return ret;
    }

  modifyreg32(STM32_XSPI_CR(base), XSPI_CR_FMODE_MASK, 0);
  putreg32(XSPI_FCR_CTEF | XSPI_FCR_CTCF, STM32_XSPI_FCR(base));
  return OK;
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
