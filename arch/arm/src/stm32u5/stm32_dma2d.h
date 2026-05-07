/****************************************************************************
 * arch/arm/src/stm32u5/stm32_dma2d.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_STM32_DMA2D_H
#define __ARCH_ARM_SRC_STM32U5_STM32_DMA2D_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_STM32U5_DMA2D
int stm32_dma2dinitialize(void);
int stm32_dma2dfill(uintptr_t dest, uint32_t color, uint16_t width,
                    uint16_t height, uint16_t stride);
#endif

#endif /* __ARCH_ARM_SRC_STM32U5_STM32_DMA2D_H */
