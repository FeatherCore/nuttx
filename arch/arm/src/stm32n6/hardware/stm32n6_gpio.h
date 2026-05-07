/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32n6_gpio.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_GPIO_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_GPIO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32n6_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_GPIO_MODER_OFFSET      0x0000u
#define STM32N6_GPIO_OTYPER_OFFSET     0x0004u
#define STM32N6_GPIO_OSPEEDR_OFFSET    0x0008u
#define STM32N6_GPIO_PUPDR_OFFSET      0x000cu
#define STM32N6_GPIO_AFRL_OFFSET       0x0020u
#define STM32N6_GPIO_AFRH_OFFSET       0x0024u

#define STM32N6_GPIO_MODER(b)          ((b) + STM32N6_GPIO_MODER_OFFSET)
#define STM32N6_GPIO_OTYPER(b)         ((b) + STM32N6_GPIO_OTYPER_OFFSET)
#define STM32N6_GPIO_OSPEEDR(b)        ((b) + STM32N6_GPIO_OSPEEDR_OFFSET)
#define STM32N6_GPIO_PUPDR(b)          ((b) + STM32N6_GPIO_PUPDR_OFFSET)
#define STM32N6_GPIO_AFRL(b)           ((b) + STM32N6_GPIO_AFRL_OFFSET)
#define STM32N6_GPIO_AFRH(b)           ((b) + STM32N6_GPIO_AFRH_OFFSET)

#define GPIO_MODE_SHIFT(n)             ((n) << 1)
#define GPIO_MODE_MASK(n)              (3u << GPIO_MODE_SHIFT(n))
#define GPIO_MODE_ALT                  2u

#define GPIO_SPEED_SHIFT(n)            ((n) << 1)
#define GPIO_SPEED_MASK(n)             (3u << GPIO_SPEED_SHIFT(n))
#define GPIO_SPEED_LOW                 0u
#define GPIO_SPEED_VERYHIGH            3u

#define GPIO_PUPD_SHIFT(n)             ((n) << 1)
#define GPIO_PUPD_MASK(n)              (3u << GPIO_PUPD_SHIFT(n))
#define GPIO_FLOAT                     0u

#define GPIO_AFR_SHIFT(n)              (((n) & 7u) << 2)
#define GPIO_AFR_MASK(n)               (15u << GPIO_AFR_SHIFT(n))
#define GPIO_AF_USART1                 7u
#define GPIO_AF_XSPIM                  9u

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_GPIO_H */
