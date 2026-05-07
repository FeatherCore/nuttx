/****************************************************************************
 * arch/arm/src/stm32u5/stm32_dma2d.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdint.h>

#include "arm_internal.h"
#include "stm32_dma2d.h"

#include "hardware/stm32_dma2d.h"
#include "hardware/stm32u5xx_rcc.h"

#ifdef CONFIG_STM32U5_DMA2D

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_dma2d_waitdone(void)
{
  uint32_t timeout;

  for (timeout = 1000000; timeout > 0; timeout--)
    {
      if ((getreg32(STM32_DMA2D_ISR) & DMA2D_ISR_TCIF) != 0)
        {
          putreg32(DMA2D_IFCR_CTCIF, STM32_DMA2D_IFCR);
          return OK;
        }
    }

  return -ETIMEDOUT;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_dma2dinitialize(void)
{
  modifyreg32(STM32_RCC_AHB1ENR, 0, RCC_AHB1ENR_DMA2DEN);
  putreg32(DMA2D_IFCR_ALL, STM32_DMA2D_IFCR);
  return OK;
}

int stm32_dma2dfill(uintptr_t dest, uint32_t color, uint16_t width,
                    uint16_t height, uint16_t stride)
{
  int ret;

  ret = stm32_dma2dinitialize();
  if (ret < 0)
    {
      return ret;
    }

  putreg32(DMA2D_IFCR_ALL, STM32_DMA2D_IFCR);
  putreg32(DMA2D_OPFCCR_ARGB8888, STM32_DMA2D_OPFCCR);
  putreg32(color, STM32_DMA2D_OCOLR);
  putreg32(dest, STM32_DMA2D_OMAR);
  putreg32(stride - width, STM32_DMA2D_OOR);
  putreg32(DMA2D_NLR_PL(width) | DMA2D_NLR_NL(height), STM32_DMA2D_NLR);
  putreg32(DMA2D_CR_MODE_R2M | DMA2D_CR_START, STM32_DMA2D_CR);

  return stm32_dma2d_waitdone();
}

#endif /* CONFIG_STM32U5_DMA2D */
