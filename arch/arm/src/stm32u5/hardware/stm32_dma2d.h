/****************************************************************************
 * arch/arm/src/stm32u5/hardware/stm32_dma2d.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DMA2D_H
#define __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DMA2D_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_DMA2D_CR         (STM32_DMA2D_BASE + 0x0000)
#define STM32_DMA2D_ISR        (STM32_DMA2D_BASE + 0x0004)
#define STM32_DMA2D_IFCR       (STM32_DMA2D_BASE + 0x0008)
#define STM32_DMA2D_OPFCCR     (STM32_DMA2D_BASE + 0x0034)
#define STM32_DMA2D_OCOLR      (STM32_DMA2D_BASE + 0x0038)
#define STM32_DMA2D_OMAR       (STM32_DMA2D_BASE + 0x003c)
#define STM32_DMA2D_OOR        (STM32_DMA2D_BASE + 0x0040)
#define STM32_DMA2D_NLR        (STM32_DMA2D_BASE + 0x0044)

#define DMA2D_CR_START         (1 << 0)
#define DMA2D_CR_MODE_R2M      (3 << 16)

#define DMA2D_ISR_TCIF         (1 << 1)
#define DMA2D_IFCR_CTCIF       (1 << 1)
#define DMA2D_IFCR_ALL         0x0000003f

#define DMA2D_OPFCCR_ARGB8888  0

#define DMA2D_NLR_NL(n)        (((n) & 0xffff) << 0)
#define DMA2D_NLR_PL(n)        (((n) & 0x3fff) << 16)

#endif /* __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DMA2D_H */
