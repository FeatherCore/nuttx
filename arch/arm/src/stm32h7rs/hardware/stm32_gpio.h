/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32_gpio.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_GPIO_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_GPIO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_GPIO_MODER_OFFSET      0x0000u
#define STM32_GPIO_OTYPER_OFFSET     0x0004u
#define STM32_GPIO_OSPEEDR_OFFSET    0x0008u
#define STM32_GPIO_PUPDR_OFFSET      0x000cu
#define STM32_GPIO_AFRL_OFFSET       0x0020u
#define STM32_GPIO_AFRH_OFFSET       0x0024u

#define STM32_GPIO_IDR_OFFSET        0x0010u
#define STM32_GPIO_ODR_OFFSET        0x0014u
#define STM32_GPIO_BSRR_OFFSET       0x0018u

#define STM32_GPIOA_MODER            (STM32_GPIOA_BASE + \
                                           STM32_GPIO_MODER_OFFSET)
#define STM32_GPIOA_OTYPER           (STM32_GPIOA_BASE + \
                                           STM32_GPIO_OTYPER_OFFSET)
#define STM32_GPIOA_OSPEEDR          (STM32_GPIOA_BASE + \
                                           STM32_GPIO_OSPEEDR_OFFSET)
#define STM32_GPIOA_PUPDR            (STM32_GPIOA_BASE + \
                                           STM32_GPIO_PUPDR_OFFSET)
#define STM32_GPIOA_AFRL             (STM32_GPIOA_BASE + \
                                           STM32_GPIO_AFRL_OFFSET)
#define STM32_GPIOA_AFRH             (STM32_GPIOA_BASE + \
                                           STM32_GPIO_AFRH_OFFSET)
#define STM32_GPIOB_MODER            (STM32_GPIOB_BASE + \
                                           STM32_GPIO_MODER_OFFSET)
#define STM32_GPIOB_OTYPER           (STM32_GPIOB_BASE + \
                                           STM32_GPIO_OTYPER_OFFSET)
#define STM32_GPIOB_OSPEEDR          (STM32_GPIOB_BASE + \
                                           STM32_GPIO_OSPEEDR_OFFSET)
#define STM32_GPIOB_PUPDR            (STM32_GPIOB_BASE + \
                                           STM32_GPIO_PUPDR_OFFSET)
#define STM32_GPIOB_AFRL             (STM32_GPIOB_BASE + \
                                           STM32_GPIO_AFRL_OFFSET)
#define STM32_GPIOB_AFRH             (STM32_GPIOB_BASE + \
                                           STM32_GPIO_AFRH_OFFSET)
#define STM32_GPIOC_MODER            (STM32_GPIOC_BASE + \
                                           STM32_GPIO_MODER_OFFSET)
#define STM32_GPIOC_OTYPER           (STM32_GPIOC_BASE + \
                                           STM32_GPIO_OTYPER_OFFSET)
#define STM32_GPIOC_OSPEEDR          (STM32_GPIOC_BASE + \
                                           STM32_GPIO_OSPEEDR_OFFSET)
#define STM32_GPIOC_PUPDR            (STM32_GPIOC_BASE + \
                                           STM32_GPIO_PUPDR_OFFSET)
#define STM32_GPIOC_AFRL             (STM32_GPIOC_BASE + \
                                           STM32_GPIO_AFRL_OFFSET)
#define STM32_GPIOC_AFRH             (STM32_GPIOC_BASE + \
                                           STM32_GPIO_AFRH_OFFSET)
#define STM32_GPIOD_MODER            (STM32_GPIOD_BASE + \
                                           STM32_GPIO_MODER_OFFSET)
#define STM32_GPIOD_OTYPER           (STM32_GPIOD_BASE + \
                                           STM32_GPIO_OTYPER_OFFSET)
#define STM32_GPIOD_OSPEEDR          (STM32_GPIOD_BASE + \
                                           STM32_GPIO_OSPEEDR_OFFSET)
#define STM32_GPIOD_PUPDR            (STM32_GPIOD_BASE + \
                                           STM32_GPIO_PUPDR_OFFSET)
#define STM32_GPIOD_AFRL             (STM32_GPIOD_BASE + \
                                           STM32_GPIO_AFRL_OFFSET)
#define STM32_GPIOD_AFRH             (STM32_GPIOD_BASE + \
                                           STM32_GPIO_AFRH_OFFSET)
#define STM32_GPIOE_MODER            (STM32_GPIOE_BASE + \
                                           STM32_GPIO_MODER_OFFSET)
#define STM32_GPIOE_OTYPER           (STM32_GPIOE_BASE + \
                                           STM32_GPIO_OTYPER_OFFSET)
#define STM32_GPIOE_OSPEEDR          (STM32_GPIOE_BASE + \
                                           STM32_GPIO_OSPEEDR_OFFSET)
#define STM32_GPIOE_PUPDR            (STM32_GPIOE_BASE + \
                                           STM32_GPIO_PUPDR_OFFSET)
#define STM32_GPIOE_AFRL             (STM32_GPIOE_BASE + \
                                           STM32_GPIO_AFRL_OFFSET)
#define STM32_GPIOE_AFRH             (STM32_GPIOE_BASE + \
                                           STM32_GPIO_AFRH_OFFSET)
#define STM32_GPIOF_MODER            (STM32_GPIOF_BASE + \
                                           STM32_GPIO_MODER_OFFSET)
#define STM32_GPIOF_OTYPER           (STM32_GPIOF_BASE + \
                                           STM32_GPIO_OTYPER_OFFSET)
#define STM32_GPIOF_OSPEEDR          (STM32_GPIOF_BASE + \
                                           STM32_GPIO_OSPEEDR_OFFSET)
#define STM32_GPIOF_PUPDR            (STM32_GPIOF_BASE + \
                                           STM32_GPIO_PUPDR_OFFSET)
#define STM32_GPIOF_AFRL             (STM32_GPIOF_BASE + \
                                           STM32_GPIO_AFRL_OFFSET)
#define STM32_GPIOF_AFRH             (STM32_GPIOF_BASE + \
                                           STM32_GPIO_AFRH_OFFSET)
#define STM32_GPIOG_MODER            (STM32_GPIOG_BASE + \
                                           STM32_GPIO_MODER_OFFSET)
#define STM32_GPIOG_OTYPER           (STM32_GPIOG_BASE + \
                                           STM32_GPIO_OTYPER_OFFSET)
#define STM32_GPIOG_OSPEEDR          (STM32_GPIOG_BASE + \
                                           STM32_GPIO_OSPEEDR_OFFSET)
#define STM32_GPIOG_PUPDR            (STM32_GPIOG_BASE + \
                                           STM32_GPIO_PUPDR_OFFSET)
#define STM32_GPIOG_AFRL             (STM32_GPIOG_BASE + \
                                           STM32_GPIO_AFRL_OFFSET)
#define STM32_GPIOG_AFRH             (STM32_GPIOG_BASE + \
                                           STM32_GPIO_AFRH_OFFSET)
#define STM32_GPION_MODER            (STM32_GPION_BASE + \
                                           STM32_GPIO_MODER_OFFSET)
#define STM32_GPION_OTYPER           (STM32_GPION_BASE + \
                                           STM32_GPIO_OTYPER_OFFSET)
#define STM32_GPION_OSPEEDR          (STM32_GPION_BASE + \
                                           STM32_GPIO_OSPEEDR_OFFSET)
#define STM32_GPION_PUPDR            (STM32_GPION_BASE + \
                                           STM32_GPIO_PUPDR_OFFSET)
#define STM32_GPION_AFRL             (STM32_GPION_BASE + \
                                           STM32_GPIO_AFRL_OFFSET)
#define STM32_GPION_AFRH             (STM32_GPION_BASE + \
                                           STM32_GPIO_AFRH_OFFSET)
#define STM32_GPIOO_MODER            (STM32_GPIOO_BASE + \
                                           STM32_GPIO_MODER_OFFSET)
#define STM32_GPIOO_OTYPER           (STM32_GPIOO_BASE + \
                                           STM32_GPIO_OTYPER_OFFSET)
#define STM32_GPIOO_OSPEEDR          (STM32_GPIOO_BASE + \
                                           STM32_GPIO_OSPEEDR_OFFSET)
#define STM32_GPIOO_PUPDR            (STM32_GPIOO_BASE + \
                                           STM32_GPIO_PUPDR_OFFSET)
#define STM32_GPIOO_AFRL             (STM32_GPIOO_BASE + \
                                           STM32_GPIO_AFRL_OFFSET)
#define STM32_GPIOO_AFRH             (STM32_GPIOO_BASE + \
                                           STM32_GPIO_AFRH_OFFSET)
#define STM32_GPIOP_MODER            (STM32_GPIOP_BASE + \
                                           STM32_GPIO_MODER_OFFSET)
#define STM32_GPIOP_OTYPER           (STM32_GPIOP_BASE + \
                                           STM32_GPIO_OTYPER_OFFSET)
#define STM32_GPIOP_OSPEEDR          (STM32_GPIOP_BASE + \
                                           STM32_GPIO_OSPEEDR_OFFSET)
#define STM32_GPIOP_PUPDR            (STM32_GPIOP_BASE + \
                                           STM32_GPIO_PUPDR_OFFSET)
#define STM32_GPIOP_AFRL             (STM32_GPIOP_BASE + \
                                           STM32_GPIO_AFRL_OFFSET)
#define STM32_GPIOP_AFRH             (STM32_GPIOP_BASE + \
                                           STM32_GPIO_AFRH_OFFSET)

#define GPIO_MODE_SHIFT(n)               ((n) << 1)
#define GPIO_MODE_MASK(n)                (3u << GPIO_MODE_SHIFT(n))
#define GPIO_MODE_INPUT                  0u
#define GPIO_MODE_OUTPUT                 1u
#define GPIO_MODE_ALT                    2u

#define GPIO_SPEED_SHIFT(n)              ((n) << 1)
#define GPIO_SPEED_MASK(n)               (3u << GPIO_SPEED_SHIFT(n))
#define GPIO_SPEED_HIGH                  2u
#define GPIO_SPEED_VERYHIGH              3u

#define GPIO_PUPD_SHIFT(n)               ((n) << 1)
#define GPIO_PUPD_MASK(n)                (3u << GPIO_PUPD_SHIFT(n))
#define GPIO_FLOAT                       0u
#define GPIO_PULLUP                      1u
#define GPIO_PULLDOWN                    2u

#define GPIO_AFR_SHIFT(n)                (((n) & 7u) << 2)
#define GPIO_AFR_MASK(n)                 (15u << GPIO_AFR_SHIFT(n))
#define GPIO_AF_LPTIM1                   1u
#define GPIO_AF_I2C1                     4u
#define GPIO_AF_UART4                    8u
#define GPIO_AF_XSPIM_P1                 9u
#define GPIO_AF_XSPIM_P2                 9u
#define GPIO_AF_LTDC10                   10u
#define GPIO_AF_LTDC11                   11u
#define GPIO_AF_LTDC12                   12u
#define GPIO_AF_LTDC13                   13u
#define GPIO_AF_LTDC14                   14u

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_GPIO_H */
