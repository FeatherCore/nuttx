/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_rcc.c
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
#include <stdint.h>
#include <syslog.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "stm32n6.h"

#include "hardware/stm32n6_gpio.h"
#include "hardware/stm32n6_pwr.h"
#include "hardware/stm32n6_rcc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_RCC_TIMEOUT       1000000u

#define STM32N6_PLL1_DIVM         8u
#define STM32N6_PLL1_DIVN         100u

#define STM32N6_IC1_DIV           1u
#define STM32N6_IC2_DIV           2u
#define STM32N6_IC3_DIV           4u
#define STM32N6_IC6_DIV           4u
#define STM32N6_IC11_DIV          2u

#define STM32N6_SMPS_PIN          (1u << 4)
#define STM32N6_SMPS_PIN_INDEX    4u

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool stm32n6_clock_ready(void)
{
  const uint32_t cfgr1_mask = RCC_CFGR1_CPUSWS_MASK |
                              RCC_CFGR1_SYSSWS_MASK;
  const uint32_t cfgr1_target = RCC_CFGR1_CPUSWS_IC1 |
                                RCC_CFGR1_SYSSWS_IC2_IC6_IC11;
  const uint32_t diven_mask = RCC_DIVENR_IC1EN | RCC_DIVENR_IC2EN |
                              RCC_DIVENR_IC3EN | RCC_DIVENR_IC6EN |
                              RCC_DIVENR_IC11EN;
  uint32_t sr;

  sr = getreg32(STM32N6_RCC_SR);

  return (sr & (RCC_SR_HSIRDY | RCC_SR_PLL1RDY)) ==
           (RCC_SR_HSIRDY | RCC_SR_PLL1RDY) &&
         (getreg32(STM32N6_RCC_CFGR1) & cfgr1_mask) == cfgr1_target &&
         (getreg32(STM32N6_RCC_DIVENR) & diven_mask) == diven_mask;
}

static int stm32n6_wait_reg(uintptr_t addr, uint32_t mask, uint32_t value,
                            FAR const char *name)
{
  uint32_t timeout = STM32N6_RCC_TIMEOUT;

  while ((getreg32(addr) & mask) != value)
    {
      if (timeout-- == 0)
        {
          syslog(LOG_ERR, "stm32n6: %s timeout reg=%08" PRIx32
                 " mask=%08" PRIx32 " want=%08" PRIx32 "\n",
                 name, getreg32(addr), mask, value);
          return -ETIMEDOUT;
        }
    }

  return OK;
}

static void stm32n6_ic_config(uintptr_t reg, uint32_t div)
{
  putreg32(RCC_ICCFGR(RCC_ICCFGR_SEL_PLL1, div), reg);
}

static void stm32n6_smps_overdrive(void)
{
  uint32_t regval;

  /* STM32N6570-DK uses PF4 to select the external SMPS output voltage.
   * Cube's 800 MHz FSBL drives it high before requesting VOS scale 0.
   */

  putreg32(RCC_AHB4ENSR_GPIOFENS, STM32N6_RCC_AHB4ENSR);
  (void)getreg32(STM32N6_RCC_AHB4ENR);

  regval = getreg32(STM32N6_GPIO_MODER(STM32N6_GPIOF_BASE));
  regval &= ~GPIO_MODE_MASK(STM32N6_SMPS_PIN_INDEX);
  regval |= GPIO_MODE_OUTPUT << GPIO_MODE_SHIFT(STM32N6_SMPS_PIN_INDEX);
  putreg32(regval, STM32N6_GPIO_MODER(STM32N6_GPIOF_BASE));

  regval = getreg32(STM32N6_GPIO_OTYPER(STM32N6_GPIOF_BASE));
  regval &= ~STM32N6_SMPS_PIN;
  putreg32(regval, STM32N6_GPIO_OTYPER(STM32N6_GPIOF_BASE));

  regval = getreg32(STM32N6_GPIO_OSPEEDR(STM32N6_GPIOF_BASE));
  regval &= ~GPIO_SPEED_MASK(STM32N6_SMPS_PIN_INDEX);
  regval |= GPIO_SPEED_VERYHIGH << GPIO_SPEED_SHIFT(STM32N6_SMPS_PIN_INDEX);
  putreg32(regval, STM32N6_GPIO_OSPEEDR(STM32N6_GPIOF_BASE));

  regval = getreg32(STM32N6_GPIO_PUPDR(STM32N6_GPIOF_BASE));
  regval &= ~GPIO_PUPD_MASK(STM32N6_SMPS_PIN_INDEX);
  regval |= GPIO_FLOAT << GPIO_PUPD_SHIFT(STM32N6_SMPS_PIN_INDEX);
  putreg32(regval, STM32N6_GPIO_PUPDR(STM32N6_GPIOF_BASE));

  putreg32(STM32N6_SMPS_PIN, STM32N6_GPIO_BSRR(STM32N6_GPIOF_BASE));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void stm32n6_power_config(void)
{
  int ret;

  putreg32(RCC_AHB4ENSR_PWRENS, STM32N6_RCC_AHB4ENSR);
  (void)getreg32(STM32N6_RCC_AHB4ENR);

  /* Match CubeN6's 800 MHz FSBL before raising the clock tree:
   * external SMPS overdrive, external VCORE supply, then VOS scale 0.
   */

  stm32n6_smps_overdrive();

  modifyreg32(STM32N6_PWR_CR1, PWR_SUPPLY_CONFIG_MASK,
              PWR_EXTERNAL_SOURCE_SUPPLY);

  ret = stm32n6_wait_reg(STM32N6_PWR_VOSCR, PWR_VOSCR_ACTVOSRDY,
                         PWR_VOSCR_ACTVOSRDY, "ACTVOS ready");
  if (ret < 0)
    {
      goto out;
    }

  modifyreg32(STM32N6_PWR_VOSCR, PWR_VOSCR_VOS,
              PWR_REGULATOR_VOLTAGE_SCALE0);

  ret = stm32n6_wait_reg(STM32N6_PWR_VOSCR, PWR_VOSCR_VOSRDY,
                         PWR_VOSCR_VOSRDY, "VOS ready");
  if (ret < 0)
    {
      goto out;
    }

  /* Cube's FSBL configures VDDIO2 (GPIOO/GPIOP) and VDDIO3 (GPION) as
   * valid 1.8 V supplies before touching XSPI pins.
   */

  modifyreg32(STM32N6_PWR_SVMCR3, 0,
              PWR_SVMCR3_VDDIO2SV | PWR_SVMCR3_VDDIO3SV |
              PWR_SVMCR3_VDDIO2VRSEL | PWR_SVMCR3_VDDIO3VRSEL);

  /* Cube only sets VDDIOxSV and VDDIOxVRSEL here.  The VDDIOxRDY bits
   * belong to the independent voltage monitor path and remain clear when
   * VDDIOxVMEN is not enabled.
   */

  ret = OK;

out:
  (void)ret;
}

void stm32n6_clockconfig(void)
{
  int ret;

  if (stm32n6_clock_ready())
    {
      stm32n6_power_config();
      putreg32(RCC_BUSENSR_ALL_BUS, STM32N6_RCC_BUSENSR);
      putreg32(RCC_MEMENSR_AXISRAM | RCC_MEMENSR_AHBSRAM,
               STM32N6_RCC_MEMENSR);
      putreg32(RCC_APB4ENSR2_SYSCFGENS | RCC_APB4ENSR2_BSECENS,
               STM32N6_RCC_APB4ENSR2);
      modifyreg32(STM32N6_RCC_CCIPR13, RCC_CCIPR13_USART1SEL_MASK,
                  RCC_CCIPR13_USART1SEL_PCLK2);
      return;
    }

  stm32n6_power_config();

  putreg32(RCC_BUSENSR_ALL_BUS, STM32N6_RCC_BUSENSR);
  putreg32(RCC_MEMENSR_AXISRAM | RCC_MEMENSR_AHBSRAM,
           STM32N6_RCC_MEMENSR);
  putreg32(RCC_APB4ENSR2_SYSCFGENS | RCC_APB4ENSR2_BSECENS,
           STM32N6_RCC_APB4ENSR2);
  modifyreg32(STM32N6_RCC_CCIPR13, RCC_CCIPR13_USART1SEL_MASK,
              RCC_CCIPR13_USART1SEL_PCLK2);

  putreg32(RCC_CSR_HSIONS, STM32N6_RCC_CSR);
  ret = stm32n6_wait_reg(STM32N6_RCC_SR, RCC_SR_HSIRDY, RCC_SR_HSIRDY,
                         "HSI ready");
  if (ret < 0)
    {
      return;
    }

  modifyreg32(STM32N6_RCC_HSICFGR, RCC_HSICFGR_HSIDIV_MASK,
              RCC_HSICFGR_HSIDIV_DIV1);

  modifyreg32(STM32N6_RCC_CFGR1,
              RCC_CFGR1_CPUSW_MASK | RCC_CFGR1_SYSSW_MASK,
              RCC_CFGR1_CPUSW_HSI | RCC_CFGR1_SYSSW_HSI);
  ret = stm32n6_wait_reg(STM32N6_RCC_CFGR1,
                         RCC_CFGR1_CPUSWS_MASK | RCC_CFGR1_SYSSWS_MASK,
                         RCC_CFGR1_CPUSWS_HSI | RCC_CFGR1_SYSSWS_HSI,
                         "HSI switch");
  if (ret < 0)
    {
      return;
    }

  putreg32(RCC_CCR_PLL1ONC, STM32N6_RCC_CCR);
  ret = stm32n6_wait_reg(STM32N6_RCC_SR, RCC_SR_PLL1RDY, 0,
                         "PLL1 off");
  if (ret < 0)
    {
      return;
    }

  putreg32(RCC_PLL1CFGR1_PLL1SEL_HSI |
           RCC_PLL1CFGR1_DIVM(STM32N6_PLL1_DIVM) |
           RCC_PLL1CFGR1_DIVN(STM32N6_PLL1_DIVN),
           STM32N6_RCC_PLL1CFGR1);
  putreg32(0, STM32N6_RCC_PLL1CFGR2);
  putreg32(RCC_PLL1CFGR3_MODSSRST | RCC_PLL1CFGR3_MODSSDIS |
           RCC_PLL1CFGR3_PDIV1(1) | RCC_PLL1CFGR3_PDIV2(1) |
           RCC_PLL1CFGR3_PDIVEN, STM32N6_RCC_PLL1CFGR3);

  modifyreg32(STM32N6_RCC_CFGR2,
              RCC_CFGR2_PPRE1_MASK | RCC_CFGR2_PPRE2_MASK |
              RCC_CFGR2_PPRE4_MASK | RCC_CFGR2_PPRE5_MASK |
              RCC_CFGR2_HPRE_MASK,
              RCC_CFGR2_HPRE_DIV2);

  putreg32(RCC_CSR_PLL1ONS, STM32N6_RCC_CSR);
  ret = stm32n6_wait_reg(STM32N6_RCC_SR, RCC_SR_PLL1RDY, RCC_SR_PLL1RDY,
                         "PLL1 ready");
  if (ret < 0)
    {
      return;
    }

  stm32n6_ic_config(STM32N6_RCC_IC1CFGR, STM32N6_IC1_DIV);
  stm32n6_ic_config(STM32N6_RCC_IC2CFGR, STM32N6_IC2_DIV);
  stm32n6_ic_config(STM32N6_RCC_IC3CFGR, STM32N6_IC3_DIV);
  stm32n6_ic_config(STM32N6_RCC_IC6CFGR, STM32N6_IC6_DIV);
  stm32n6_ic_config(STM32N6_RCC_IC11CFGR, STM32N6_IC11_DIV);

  putreg32(RCC_DIVENSR_IC1ENS | RCC_DIVENSR_IC2ENS | RCC_DIVENSR_IC3ENS |
           RCC_DIVENSR_IC6ENS | RCC_DIVENSR_IC11ENS,
           STM32N6_RCC_DIVENSR);
  ret = stm32n6_wait_reg(STM32N6_RCC_DIVENR,
                         RCC_DIVENR_IC1EN | RCC_DIVENR_IC2EN |
                         RCC_DIVENR_IC3EN | RCC_DIVENR_IC6EN |
                         RCC_DIVENR_IC11EN,
                         RCC_DIVENR_IC1EN | RCC_DIVENR_IC2EN |
                         RCC_DIVENR_IC3EN | RCC_DIVENR_IC6EN |
                         RCC_DIVENR_IC11EN,
                         "IC dividers enable");
  if (ret < 0)
    {
      return;
    }

  modifyreg32(STM32N6_RCC_CFGR1,
              RCC_CFGR1_CPUSW_MASK | RCC_CFGR1_SYSSW_MASK,
              RCC_CFGR1_CPUSW_IC1 | RCC_CFGR1_SYSSW_IC2_IC6_IC11);
  ret = stm32n6_wait_reg(STM32N6_RCC_CFGR1,
                         RCC_CFGR1_CPUSWS_MASK | RCC_CFGR1_SYSSWS_MASK,
                         RCC_CFGR1_CPUSWS_IC1 |
                         RCC_CFGR1_SYSSWS_IC2_IC6_IC11,
                         "PLL1 switch");
  if (ret < 0)
    {
      return;
    }
}
