/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32h7rs_gpio.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_GPIO_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_GPIO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32h7rs_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32H7RS_GPIO_MODER_OFFSET      0x0000u
#define STM32H7RS_GPIO_OTYPER_OFFSET     0x0004u
#define STM32H7RS_GPIO_OSPEEDR_OFFSET    0x0008u
#define STM32H7RS_GPIO_PUPDR_OFFSET      0x000cu
#define STM32H7RS_GPIO_AFRL_OFFSET       0x0020u
#define STM32H7RS_GPIO_AFRH_OFFSET       0x0024u

#define STM32H7RS_GPIO_IDR_OFFSET        0x0010u
#define STM32H7RS_GPIO_ODR_OFFSET        0x0014u
#define STM32H7RS_GPIO_BSRR_OFFSET       0x0018u

#define STM32H7RS_GPIOA_MODER            (STM32H7RS_GPIOA_BASE + \
                                           STM32H7RS_GPIO_MODER_OFFSET)
#define STM32H7RS_GPIOA_OTYPER           (STM32H7RS_GPIOA_BASE + \
                                           STM32H7RS_GPIO_OTYPER_OFFSET)
#define STM32H7RS_GPIOA_OSPEEDR          (STM32H7RS_GPIOA_BASE + \
                                           STM32H7RS_GPIO_OSPEEDR_OFFSET)
#define STM32H7RS_GPIOA_PUPDR            (STM32H7RS_GPIOA_BASE + \
                                           STM32H7RS_GPIO_PUPDR_OFFSET)
#define STM32H7RS_GPIOA_AFRL             (STM32H7RS_GPIOA_BASE + \
                                           STM32H7RS_GPIO_AFRL_OFFSET)
#define STM32H7RS_GPIOA_AFRH             (STM32H7RS_GPIOA_BASE + \
                                           STM32H7RS_GPIO_AFRH_OFFSET)
#define STM32H7RS_GPIOB_MODER            (STM32H7RS_GPIOB_BASE + \
                                           STM32H7RS_GPIO_MODER_OFFSET)
#define STM32H7RS_GPIOB_OTYPER           (STM32H7RS_GPIOB_BASE + \
                                           STM32H7RS_GPIO_OTYPER_OFFSET)
#define STM32H7RS_GPIOB_OSPEEDR          (STM32H7RS_GPIOB_BASE + \
                                           STM32H7RS_GPIO_OSPEEDR_OFFSET)
#define STM32H7RS_GPIOB_PUPDR            (STM32H7RS_GPIOB_BASE + \
                                           STM32H7RS_GPIO_PUPDR_OFFSET)
#define STM32H7RS_GPIOB_AFRL             (STM32H7RS_GPIOB_BASE + \
                                           STM32H7RS_GPIO_AFRL_OFFSET)
#define STM32H7RS_GPIOB_AFRH             (STM32H7RS_GPIOB_BASE + \
                                           STM32H7RS_GPIO_AFRH_OFFSET)
#define STM32H7RS_GPIOC_MODER            (STM32H7RS_GPIOC_BASE + \
                                           STM32H7RS_GPIO_MODER_OFFSET)
#define STM32H7RS_GPIOC_OTYPER           (STM32H7RS_GPIOC_BASE + \
                                           STM32H7RS_GPIO_OTYPER_OFFSET)
#define STM32H7RS_GPIOC_OSPEEDR          (STM32H7RS_GPIOC_BASE + \
                                           STM32H7RS_GPIO_OSPEEDR_OFFSET)
#define STM32H7RS_GPIOC_PUPDR            (STM32H7RS_GPIOC_BASE + \
                                           STM32H7RS_GPIO_PUPDR_OFFSET)
#define STM32H7RS_GPIOC_AFRL             (STM32H7RS_GPIOC_BASE + \
                                           STM32H7RS_GPIO_AFRL_OFFSET)
#define STM32H7RS_GPIOC_AFRH             (STM32H7RS_GPIOC_BASE + \
                                           STM32H7RS_GPIO_AFRH_OFFSET)
#define STM32H7RS_GPIOD_MODER            (STM32H7RS_GPIOD_BASE + \
                                           STM32H7RS_GPIO_MODER_OFFSET)
#define STM32H7RS_GPIOD_OTYPER           (STM32H7RS_GPIOD_BASE + \
                                           STM32H7RS_GPIO_OTYPER_OFFSET)
#define STM32H7RS_GPIOD_OSPEEDR          (STM32H7RS_GPIOD_BASE + \
                                           STM32H7RS_GPIO_OSPEEDR_OFFSET)
#define STM32H7RS_GPIOD_PUPDR            (STM32H7RS_GPIOD_BASE + \
                                           STM32H7RS_GPIO_PUPDR_OFFSET)
#define STM32H7RS_GPIOD_AFRL             (STM32H7RS_GPIOD_BASE + \
                                           STM32H7RS_GPIO_AFRL_OFFSET)
#define STM32H7RS_GPIOD_AFRH             (STM32H7RS_GPIOD_BASE + \
                                           STM32H7RS_GPIO_AFRH_OFFSET)
#define STM32H7RS_GPIOE_MODER            (STM32H7RS_GPIOE_BASE + \
                                           STM32H7RS_GPIO_MODER_OFFSET)
#define STM32H7RS_GPIOE_OTYPER           (STM32H7RS_GPIOE_BASE + \
                                           STM32H7RS_GPIO_OTYPER_OFFSET)
#define STM32H7RS_GPIOE_OSPEEDR          (STM32H7RS_GPIOE_BASE + \
                                           STM32H7RS_GPIO_OSPEEDR_OFFSET)
#define STM32H7RS_GPIOE_PUPDR            (STM32H7RS_GPIOE_BASE + \
                                           STM32H7RS_GPIO_PUPDR_OFFSET)
#define STM32H7RS_GPIOE_AFRL             (STM32H7RS_GPIOE_BASE + \
                                           STM32H7RS_GPIO_AFRL_OFFSET)
#define STM32H7RS_GPIOE_AFRH             (STM32H7RS_GPIOE_BASE + \
                                           STM32H7RS_GPIO_AFRH_OFFSET)
#define STM32H7RS_GPIOF_MODER            (STM32H7RS_GPIOF_BASE + \
                                           STM32H7RS_GPIO_MODER_OFFSET)
#define STM32H7RS_GPIOF_OTYPER           (STM32H7RS_GPIOF_BASE + \
                                           STM32H7RS_GPIO_OTYPER_OFFSET)
#define STM32H7RS_GPIOF_OSPEEDR          (STM32H7RS_GPIOF_BASE + \
                                           STM32H7RS_GPIO_OSPEEDR_OFFSET)
#define STM32H7RS_GPIOF_PUPDR            (STM32H7RS_GPIOF_BASE + \
                                           STM32H7RS_GPIO_PUPDR_OFFSET)
#define STM32H7RS_GPIOF_AFRL             (STM32H7RS_GPIOF_BASE + \
                                           STM32H7RS_GPIO_AFRL_OFFSET)
#define STM32H7RS_GPIOF_AFRH             (STM32H7RS_GPIOF_BASE + \
                                           STM32H7RS_GPIO_AFRH_OFFSET)
#define STM32H7RS_GPIOG_MODER            (STM32H7RS_GPIOG_BASE + \
                                           STM32H7RS_GPIO_MODER_OFFSET)
#define STM32H7RS_GPIOG_OTYPER           (STM32H7RS_GPIOG_BASE + \
                                           STM32H7RS_GPIO_OTYPER_OFFSET)
#define STM32H7RS_GPIOG_OSPEEDR          (STM32H7RS_GPIOG_BASE + \
                                           STM32H7RS_GPIO_OSPEEDR_OFFSET)
#define STM32H7RS_GPIOG_PUPDR            (STM32H7RS_GPIOG_BASE + \
                                           STM32H7RS_GPIO_PUPDR_OFFSET)
#define STM32H7RS_GPIOG_AFRL             (STM32H7RS_GPIOG_BASE + \
                                           STM32H7RS_GPIO_AFRL_OFFSET)
#define STM32H7RS_GPIOG_AFRH             (STM32H7RS_GPIOG_BASE + \
                                           STM32H7RS_GPIO_AFRH_OFFSET)
#define STM32H7RS_GPION_MODER            (STM32H7RS_GPION_BASE + \
                                           STM32H7RS_GPIO_MODER_OFFSET)
#define STM32H7RS_GPION_OTYPER           (STM32H7RS_GPION_BASE + \
                                           STM32H7RS_GPIO_OTYPER_OFFSET)
#define STM32H7RS_GPION_OSPEEDR          (STM32H7RS_GPION_BASE + \
                                           STM32H7RS_GPIO_OSPEEDR_OFFSET)
#define STM32H7RS_GPION_PUPDR            (STM32H7RS_GPION_BASE + \
                                           STM32H7RS_GPIO_PUPDR_OFFSET)
#define STM32H7RS_GPION_AFRL             (STM32H7RS_GPION_BASE + \
                                           STM32H7RS_GPIO_AFRL_OFFSET)
#define STM32H7RS_GPION_AFRH             (STM32H7RS_GPION_BASE + \
                                           STM32H7RS_GPIO_AFRH_OFFSET)
#define STM32H7RS_GPIOO_MODER            (STM32H7RS_GPIOO_BASE + \
                                           STM32H7RS_GPIO_MODER_OFFSET)
#define STM32H7RS_GPIOO_OTYPER           (STM32H7RS_GPIOO_BASE + \
                                           STM32H7RS_GPIO_OTYPER_OFFSET)
#define STM32H7RS_GPIOO_OSPEEDR          (STM32H7RS_GPIOO_BASE + \
                                           STM32H7RS_GPIO_OSPEEDR_OFFSET)
#define STM32H7RS_GPIOO_PUPDR            (STM32H7RS_GPIOO_BASE + \
                                           STM32H7RS_GPIO_PUPDR_OFFSET)
#define STM32H7RS_GPIOO_AFRL             (STM32H7RS_GPIOO_BASE + \
                                           STM32H7RS_GPIO_AFRL_OFFSET)
#define STM32H7RS_GPIOO_AFRH             (STM32H7RS_GPIOO_BASE + \
                                           STM32H7RS_GPIO_AFRH_OFFSET)
#define STM32H7RS_GPIOP_MODER            (STM32H7RS_GPIOP_BASE + \
                                           STM32H7RS_GPIO_MODER_OFFSET)
#define STM32H7RS_GPIOP_OTYPER           (STM32H7RS_GPIOP_BASE + \
                                           STM32H7RS_GPIO_OTYPER_OFFSET)
#define STM32H7RS_GPIOP_OSPEEDR          (STM32H7RS_GPIOP_BASE + \
                                           STM32H7RS_GPIO_OSPEEDR_OFFSET)
#define STM32H7RS_GPIOP_PUPDR            (STM32H7RS_GPIOP_BASE + \
                                           STM32H7RS_GPIO_PUPDR_OFFSET)
#define STM32H7RS_GPIOP_AFRL             (STM32H7RS_GPIOP_BASE + \
                                           STM32H7RS_GPIO_AFRL_OFFSET)
#define STM32H7RS_GPIOP_AFRH             (STM32H7RS_GPIOP_BASE + \
                                           STM32H7RS_GPIO_AFRH_OFFSET)

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

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_GPIO_H */
