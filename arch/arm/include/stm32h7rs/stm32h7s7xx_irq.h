/****************************************************************************
 * arch/arm/include/stm32h7rs/stm32h7s7xx_irq.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_INCLUDE_STM32H7RS_STM32H7S7XX_IRQ_H
#define __ARCH_ARM_INCLUDE_STM32H7RS_STM32H7S7XX_IRQ_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* STM32H7S7 external interrupt numbers follow the CMSIS IRQn values in
 * STM32CubeH7RS Drivers/CMSIS/Device/ST/STM32H7RSxx/Include/stm32h7s7xx.h.
 * Only commonly referenced names are aliased here; NR_IRQS covers the whole
 * published table through FDCAN2_IT1_IRQn = 155.
 */

#define STM32_IRQ_PVDPVM          (STM32_IRQ_FIRST + 0)
#define STM32_IRQ_RTC             (STM32_IRQ_FIRST + 1)
#define STM32_IRQ_RTCWKUP         (STM32_IRQ_FIRST + 2)
#define STM32_IRQ_RTCSSRU         (STM32_IRQ_FIRST + 3)
#define STM32_IRQ_IWDG            (STM32_IRQ_FIRST + 4)
#define STM32_IRQ_ICACHE          (STM32_IRQ_FIRST + 151)

#define STM32_IRQ_EXTI0           (STM32_IRQ_FIRST + 16)
#define STM32_IRQ_EXTI1           (STM32_IRQ_FIRST + 17)
#define STM32_IRQ_EXTI2           (STM32_IRQ_FIRST + 18)
#define STM32_IRQ_EXTI3           (STM32_IRQ_FIRST + 19)
#define STM32_IRQ_EXTI4           (STM32_IRQ_FIRST + 20)
#define STM32_IRQ_EXTI5           (STM32_IRQ_FIRST + 21)
#define STM32_IRQ_EXTI6           (STM32_IRQ_FIRST + 22)
#define STM32_IRQ_EXTI7           (STM32_IRQ_FIRST + 23)
#define STM32_IRQ_EXTI8           (STM32_IRQ_FIRST + 24)
#define STM32_IRQ_EXTI9           (STM32_IRQ_FIRST + 25)
#define STM32_IRQ_EXTI10          (STM32_IRQ_FIRST + 26)
#define STM32_IRQ_EXTI11          (STM32_IRQ_FIRST + 27)
#define STM32_IRQ_EXTI12          (STM32_IRQ_FIRST + 28)
#define STM32_IRQ_EXTI13          (STM32_IRQ_FIRST + 29)
#define STM32_IRQ_EXTI14          (STM32_IRQ_FIRST + 30)
#define STM32_IRQ_EXTI15          (STM32_IRQ_FIRST + 31)

#define STM32_IRQ_USART1          (STM32_IRQ_FIRST + 82)
#define STM32_IRQ_USART2          (STM32_IRQ_FIRST + 83)
#define STM32_IRQ_USART3          (STM32_IRQ_FIRST + 84)
#define STM32_IRQ_UART4           (STM32_IRQ_FIRST + 85)
#define STM32_IRQ_UART5           (STM32_IRQ_FIRST + 86)
#define STM32_IRQ_UART7           (STM32_IRQ_FIRST + 87)
#define STM32_IRQ_UART8           (STM32_IRQ_FIRST + 88)
#define STM32_IRQ_LPUART1         (STM32_IRQ_FIRST + 131)

#define STM32_IRQ_XSPI1           (STM32_IRQ_FIRST + 105)
#define STM32_IRQ_XSPI2           (STM32_IRQ_FIRST + 106)

#define STM32_IRQ_SDMMC1          (STM32_IRQ_FIRST + 108)
#define STM32_IRQ_SDMMC2          (STM32_IRQ_FIRST + 109)

#define STM32_IRQ_GPU2D           (STM32_IRQ_FIRST + 149)
#define STM32_IRQ_GPU2D_ER        (STM32_IRQ_FIRST + 150)
#define STM32_IRQ_NEXTINTS        156
#define NR_IRQS                   (STM32_IRQ_FIRST + STM32_IRQ_NEXTINTS)

#endif /* __ARCH_ARM_INCLUDE_STM32H7RS_STM32H7S7XX_IRQ_H */
