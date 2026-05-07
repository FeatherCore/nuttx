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
#include <stdint.h>
#include <syslog.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "stm32n6.h"

#include "hardware/stm32n6_pwr.h"
#include "hardware/stm32n6_rcc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_RCC_TIMEOUT       1000000u

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_pwr_status = -ENODEV;
static int g_clock_status = -ENODEV;
static uint32_t g_pwr_svmcr3;
static uint32_t g_rcc_sr;
static uint32_t g_rcc_cfgr1;
static uint32_t g_rcc_cfgr2;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

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

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void stm32n6_power_config(void)
{
  int ret;

  modifyreg32(STM32N6_RCC_AHB4ENR, 0, RCC_AHB4ENR_PWREN);
  (void)getreg32(STM32N6_RCC_AHB4ENR);

  /* Cube's FSBL configures VDDIO2 (GPIOO/GPIOP) and VDDIO3 (GPION) as
   * valid 1.8 V supplies before touching XSPI pins.
   */

  modifyreg32(STM32N6_PWR_SVMCR3, 0,
              PWR_SVMCR3_VDDIO2SV | PWR_SVMCR3_VDDIO3SV |
              PWR_SVMCR3_VDDIO2VRSEL | PWR_SVMCR3_VDDIO3VRSEL);

  ret = stm32n6_wait_reg(STM32N6_PWR_SVMCR3,
                         PWR_SVMCR3_VDDIO2RDY | PWR_SVMCR3_VDDIO3RDY,
                         PWR_SVMCR3_VDDIO2RDY | PWR_SVMCR3_VDDIO3RDY,
                         "VDDIO2/3 ready");

  g_pwr_status = ret;
  g_pwr_svmcr3 = getreg32(STM32N6_PWR_SVMCR3);
}

void stm32n6_clockconfig(void)
{
  int ret;

  g_clock_status = -EINPROGRESS;
  stm32n6_power_config();

  modifyreg32(STM32N6_RCC_BUSENR, 0, RCC_BUSENR_ALL_BUS);
  modifyreg32(STM32N6_RCC_MEMENR, 0,
              RCC_MEMENR_AXISRAM | RCC_MEMENR_AHBSRAM);
  modifyreg32(STM32N6_RCC_APB4ENR2, 0,
              RCC_APB4ENR2_SYSCFGEN | RCC_APB4ENR2_BSECEN);
  modifyreg32(STM32N6_RCC_CCIPR13, RCC_CCIPR13_USART1SEL_MASK,
              RCC_CCIPR13_USART1SEL_PCLK2);

  modifyreg32(STM32N6_RCC_CR, 0, RCC_CR_HSION);
  ret = stm32n6_wait_reg(STM32N6_RCC_SR, RCC_SR_HSIRDY, RCC_SR_HSIRDY,
                         "HSI ready");
  if (ret < 0)
    {
      g_clock_status = ret;
      return;
    }

  modifyreg32(STM32N6_RCC_CFGR1,
              RCC_CFGR1_CPUSW_MASK | RCC_CFGR1_SYSSW_MASK,
              RCC_CFGR1_CPUSW_HSI | RCC_CFGR1_SYSSW_HSI);
  ret = stm32n6_wait_reg(STM32N6_RCC_CFGR1,
                         RCC_CFGR1_CPUSWS_MASK | RCC_CFGR1_SYSSWS_MASK,
                         RCC_CFGR1_CPUSWS_HSI | RCC_CFGR1_SYSSWS_HSI,
                         "HSI switch");
  if (ret < 0)
    {
      g_clock_status = ret;
      return;
    }

  modifyreg32(STM32N6_RCC_CR, RCC_CR_PLL1ON, 0);
  ret = stm32n6_wait_reg(STM32N6_RCC_SR, RCC_SR_PLL1RDY, 0,
                         "PLL1 off");
  if (ret < 0)
    {
      g_clock_status = ret;
      return;
    }

  putreg32(RCC_PLL1CFGR1_PLL1SEL_HSI | RCC_PLL1CFGR1_DIVM(4) |
           RCC_PLL1CFGR1_DIVN(75), STM32N6_RCC_PLL1CFGR1);
  putreg32(0, STM32N6_RCC_PLL1CFGR2);
  putreg32(RCC_PLL1CFGR3_PDIV1(1) | RCC_PLL1CFGR3_PDIV2(1) |
           RCC_PLL1CFGR3_PDIVEN, STM32N6_RCC_PLL1CFGR3);

  stm32n6_ic_config(STM32N6_RCC_IC1CFGR, 2);
  stm32n6_ic_config(STM32N6_RCC_IC2CFGR, 3);
  stm32n6_ic_config(STM32N6_RCC_IC3CFGR, 6);
  stm32n6_ic_config(STM32N6_RCC_IC6CFGR, 4);
  stm32n6_ic_config(STM32N6_RCC_IC11CFGR, 3);

  modifyreg32(STM32N6_RCC_DIVENR, 0,
              RCC_DIVENR_IC1EN | RCC_DIVENR_IC2EN | RCC_DIVENR_IC3EN |
              RCC_DIVENR_IC6EN | RCC_DIVENR_IC11EN);

  modifyreg32(STM32N6_RCC_CFGR2,
              RCC_CFGR2_PPRE1_MASK | RCC_CFGR2_PPRE2_MASK |
              RCC_CFGR2_PPRE4_MASK | RCC_CFGR2_PPRE5_MASK |
              RCC_CFGR2_HPRE_MASK,
              RCC_CFGR2_HPRE_DIV2);

  modifyreg32(STM32N6_RCC_CR, 0, RCC_CR_PLL1ON);
  ret = stm32n6_wait_reg(STM32N6_RCC_SR, RCC_SR_PLL1RDY, RCC_SR_PLL1RDY,
                         "PLL1 ready");
  if (ret < 0)
    {
      g_clock_status = ret;
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
  g_clock_status = ret;
  g_rcc_sr       = getreg32(STM32N6_RCC_SR);
  g_rcc_cfgr1    = getreg32(STM32N6_RCC_CFGR1);
  g_rcc_cfgr2    = getreg32(STM32N6_RCC_CFGR2);

  if (ret == OK)
    {
      syslog(LOG_INFO,
             "stm32n6: clock CPU=600MHz SYS=400MHz HCLK/PCLK=200MHz\n");
    }
}

void stm32n6_clock_bootlog(void)
{
  syslog(LOG_INFO,
         "stm32n6: PWR VDDIO2/3 status=%d SVMCR3=%08" PRIx32 "\n",
         g_pwr_status, g_pwr_svmcr3);
  syslog(LOG_INFO,
         "stm32n6: clock status=%d SR=%08" PRIx32
         " CFGR1=%08" PRIx32 " CFGR2=%08" PRIx32 "\n",
         g_clock_status, g_rcc_sr, g_rcc_cfgr1, g_rcc_cfgr2);
  syslog(LOG_INFO,
         "stm32n6: clock CPU=%lu SYS=%lu HCLK=%lu PCLK1=%lu PCLK2=%lu\n",
         STM32N6_CPUCLK_FREQUENCY, STM32N6_SYSCLK_FREQUENCY,
         STM32N6_HCLK_FREQUENCY, STM32N6_PCLK1_FREQUENCY,
         STM32N6_PCLK2_FREQUENCY);
}
