/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32h7rs_rcc.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "stm32h7rs.h"

#include "hardware/stm32h7rs_flash.h"
#include "hardware/stm32h7rs_pwr.h"
#include "hardware/stm32h7rs_rcc.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32h7rs_wait_for(uintptr_t addr, uint32_t mask, uint32_t value)
{
  while ((getreg32(addr) & mask) != value)
    {
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void stm32h7rs_clockconfig(void)
{
  uint32_t regval;

  if ((getreg32(STM32H7RS_RCC_CFGR) & RCC_CFGR_SWS_MASK) ==
      RCC_CFGR_SWS_PLL1)
    {
      return;
    }

  modifyreg32(STM32H7RS_FLASH_ACR, FLASH_ACR_LATENCY_MASK,
              FLASH_ACR_LATENCY_7);
  stm32h7rs_wait_for(STM32H7RS_FLASH_ACR, FLASH_ACR_LATENCY_MASK,
                     FLASH_ACR_LATENCY_7);

  modifyreg32(STM32H7RS_PWR_CSR4, PWR_CSR4_VOS, PWR_CSR4_VOS);
  stm32h7rs_wait_for(STM32H7RS_PWR_CSR4, PWR_CSR4_VOSRDY,
                     PWR_CSR4_VOSRDY);

  modifyreg32(STM32H7RS_RCC_CR, 0, RCC_CR_HSION);
  stm32h7rs_wait_for(STM32H7RS_RCC_CR, RCC_CR_HSIRDY, RCC_CR_HSIRDY);

  modifyreg32(STM32H7RS_RCC_HSICFGR, RCC_HSICFGR_HSITRIM_MASK,
              RCC_HSICFGR_HSITRIM(64));
  modifyreg32(STM32H7RS_RCC_CR, RCC_CR_HSIDIV_MASK, 0);

  modifyreg32(STM32H7RS_RCC_PLLCKSELR,
              RCC_PLLCKSELR_PLLSRC_MASK | RCC_PLLCKSELR_DIVM1_MASK,
              RCC_PLLCKSELR_PLLSRC_HSI | RCC_PLLCKSELR_DIVM1(32));

  modifyreg32(STM32H7RS_RCC_PLLCKSELR, RCC_PLLCKSELR_DIVM2_MASK,
              RCC_PLLCKSELR_DIVM2(4));

  modifyreg32(STM32H7RS_RCC_PLLCFGR,
              RCC_PLLCFGR_PLL1VCOSEL | RCC_PLLCFGR_PLL1RGE_MASK,
              RCC_PLLCFGR_PLL1RGE_2_4);

  modifyreg32(STM32H7RS_RCC_PLLCFGR,
              RCC_PLLCFGR_PLL2VCOSEL | RCC_PLLCFGR_PLL2RGE_MASK,
              RCC_PLLCFGR_PLL2RGE_8_16);

  regval = RCC_PLL1DIVR1_DIVN(300) | RCC_PLL1DIVR1_DIVP(1) |
           RCC_PLL1DIVR1_DIVQ(2) | RCC_PLL1DIVR1_DIVR(2);
  putreg32(regval, STM32H7RS_RCC_PLL1DIVR1);
  putreg32(0, STM32H7RS_RCC_PLL1FRACR);
  modifyreg32(STM32H7RS_RCC_PLL1DIVR2, RCC_PLL1DIVR2_DIVS_MASK,
              RCC_PLL1DIVR2_DIVS(2));

  regval = RCC_PLL2DIVR1_DIVN(25) | RCC_PLL2DIVR1_DIVP(2) |
           RCC_PLL2DIVR1_DIVQ(2) | RCC_PLL2DIVR1_DIVR(2);
  putreg32(regval, STM32H7RS_RCC_PLL2DIVR1);
  putreg32(0, STM32H7RS_RCC_PLL2FRACR);
  modifyreg32(STM32H7RS_RCC_PLL2DIVR2,
              RCC_PLL2DIVR2_DIVS_MASK | RCC_PLL2DIVR2_DIVT_MASK,
              RCC_PLL2DIVR2_DIVS(2) | RCC_PLL2DIVR2_DIVT(2));

  modifyreg32(STM32H7RS_RCC_CR, 0, RCC_CR_PLL1ON | RCC_CR_PLL2ON);
  stm32h7rs_wait_for(STM32H7RS_RCC_CR, RCC_CR_PLL1RDY, RCC_CR_PLL1RDY);
  stm32h7rs_wait_for(STM32H7RS_RCC_CR, RCC_CR_PLL2RDY, RCC_CR_PLL2RDY);

  modifyreg32(STM32H7RS_RCC_PLLCFGR, 0,
              RCC_PLLCFGR_PLL1PEN | RCC_PLLCFGR_PLL2SEN);

  modifyreg32(STM32H7RS_RCC_CDCFGR, RCC_CDCFGR_CPRE_MASK,
              RCC_CDCFGR_CPRE_DIV1);
  modifyreg32(STM32H7RS_RCC_BMCFGR, RCC_BMCFGR_BMPRE_MASK,
              RCC_BMCFGR_BMPRE_DIV2);
  modifyreg32(STM32H7RS_RCC_APBCFGR,
              RCC_APBCFGR_PPRE1_MASK | RCC_APBCFGR_PPRE2_MASK |
              RCC_APBCFGR_PPRE4_MASK | RCC_APBCFGR_PPRE5_MASK,
              RCC_APBCFGR_PPRE1_DIV2 | RCC_APBCFGR_PPRE2_DIV2 |
              RCC_APBCFGR_PPRE4_DIV2 | RCC_APBCFGR_PPRE5_DIV2);

  modifyreg32(STM32H7RS_RCC_CFGR, RCC_CFGR_SW_MASK, RCC_CFGR_SW_PLL1);
  stm32h7rs_wait_for(STM32H7RS_RCC_CFGR, RCC_CFGR_SWS_MASK,
                     RCC_CFGR_SWS_PLL1);
}
