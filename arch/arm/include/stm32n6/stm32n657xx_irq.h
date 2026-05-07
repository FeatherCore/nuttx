/****************************************************************************
 * arch/arm/include/stm32n6/stm32n657xx_irq.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_INCLUDE_STM32N6_STM32N657XX_IRQ_H
#define __ARCH_ARM_INCLUDE_STM32N6_STM32N657XX_IRQ_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* STM32N657 external interrupt numbers follow the CMSIS IRQn values in
 * STM32CubeN6 Drivers/CMSIS/Device/ST/STM32N6xx/Include/stm32n657xx.h.
 * Only the names needed by the first bring-up are aliased here; NR_IRQS
 * covers the complete published table through LTDC_UP_ERR_IRQn = 194.
 */

#define STM32_IRQ_PVDPVM          (STM32_IRQ_FIRST + 0)
#define STM32_IRQ_RCC             (STM32_IRQ_FIRST + 3)
#define STM32_IRQ_EXTI0           (STM32_IRQ_FIRST + 20)
#define STM32_IRQ_EXTI1           (STM32_IRQ_FIRST + 21)
#define STM32_IRQ_EXTI2           (STM32_IRQ_FIRST + 22)
#define STM32_IRQ_EXTI3           (STM32_IRQ_FIRST + 23)
#define STM32_IRQ_EXTI4           (STM32_IRQ_FIRST + 24)
#define STM32_IRQ_DCMIPP          (STM32_IRQ_FIRST + 48)
#define STM32_IRQ_LTDC_LO         (STM32_IRQ_FIRST + 58)
#define STM32_IRQ_DMA2D           (STM32_IRQ_FIRST + 60)
#define STM32_IRQ_GFXMMU          (STM32_IRQ_FIRST + 63)
#define STM32_IRQ_I2C1EV          (STM32_IRQ_FIRST + 100)
#define STM32_IRQ_I2C1ER          (STM32_IRQ_FIRST + 101)
#define STM32_IRQ_I2C2EV          (STM32_IRQ_FIRST + 102)
#define STM32_IRQ_I2C2ER          (STM32_IRQ_FIRST + 103)
#define STM32_IRQ_I2C3EV          (STM32_IRQ_FIRST + 104)
#define STM32_IRQ_I2C3ER          (STM32_IRQ_FIRST + 105)
#define STM32_IRQ_I2C4EV          (STM32_IRQ_FIRST + 106)
#define STM32_IRQ_I2C4ER          (STM32_IRQ_FIRST + 107)
#define STM32_IRQ_USART1          (STM32_IRQ_FIRST + 159)
#define STM32_IRQ_XSPI1           (STM32_IRQ_FIRST + 170)
#define STM32_IRQ_XSPI2           (STM32_IRQ_FIRST + 171)
#define STM32_IRQ_SDMMC1          (STM32_IRQ_FIRST + 174)
#define STM32_IRQ_UCPD1           (STM32_IRQ_FIRST + 176)
#define STM32_IRQ_LTDC_UP         (STM32_IRQ_FIRST + 193)

#define STM32_IRQ_NEXTINTS        195
#define NR_IRQS                   (STM32_IRQ_FIRST + STM32_IRQ_NEXTINTS)

#endif /* __ARCH_ARM_INCLUDE_STM32N6_STM32N657XX_IRQ_H */
