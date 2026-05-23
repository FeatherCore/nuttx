/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_gpio.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_GPIO_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_GPIO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifndef __ASSEMBLY__
#  include <stdbool.h>
#  include <stdint.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Bit-encoded input to stm32n6_configgpio(). */

#define STM32N6_GPIO_MODE_SHIFT       18
#define STM32N6_GPIO_MODE_MASK        (3u << STM32N6_GPIO_MODE_SHIFT)
#define STM32N6_GPIO_INPUT            (0u << STM32N6_GPIO_MODE_SHIFT)
#define STM32N6_GPIO_OUTPUT           (1u << STM32N6_GPIO_MODE_SHIFT)
#define STM32N6_GPIO_ALT              (2u << STM32N6_GPIO_MODE_SHIFT)
#define STM32N6_GPIO_ANALOG           (3u << STM32N6_GPIO_MODE_SHIFT)

#define STM32N6_GPIO_PUPD_SHIFT       16
#define STM32N6_GPIO_PUPD_MASK        (3u << STM32N6_GPIO_PUPD_SHIFT)
#define STM32N6_GPIO_FLOAT            (0u << STM32N6_GPIO_PUPD_SHIFT)
#define STM32N6_GPIO_PULLUP           (1u << STM32N6_GPIO_PUPD_SHIFT)
#define STM32N6_GPIO_PULLDOWN         (2u << STM32N6_GPIO_PUPD_SHIFT)

#define STM32N6_GPIO_AF_SHIFT         12
#define STM32N6_GPIO_AF_MASK          (15u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF(n)            (((uint32_t)(n) & 15u) << \
                                       STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF0              (0u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF1              (1u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF2              (2u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF3              (3u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF4              (4u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF5              (5u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF6              (6u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF7              (7u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF8              (8u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF9              (9u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF10             (10u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF11             (11u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF12             (12u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF13             (13u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF14             (14u << STM32N6_GPIO_AF_SHIFT)
#define STM32N6_GPIO_AF15             (15u << STM32N6_GPIO_AF_SHIFT)

#define STM32N6_GPIO_SPEED_SHIFT      10
#define STM32N6_GPIO_SPEED_MASK       (3u << STM32N6_GPIO_SPEED_SHIFT)
#define STM32N6_GPIO_SPEED_LOW        (0u << STM32N6_GPIO_SPEED_SHIFT)
#define STM32N6_GPIO_SPEED_MEDIUM     (1u << STM32N6_GPIO_SPEED_SHIFT)
#define STM32N6_GPIO_SPEED_HIGH       (2u << STM32N6_GPIO_SPEED_SHIFT)
#define STM32N6_GPIO_SPEED_VERYHIGH   (3u << STM32N6_GPIO_SPEED_SHIFT)

#define STM32N6_GPIO_OPENDRAIN        (1u << 9)
#define STM32N6_GPIO_PUSHPULL         0

#define STM32N6_GPIO_OUTPUT_SET       (1u << 8)
#define STM32N6_GPIO_OUTPUT_CLEAR     0

#define STM32N6_GPIO_PORT_SHIFT       4
#define STM32N6_GPIO_PORT_MASK        (15u << STM32N6_GPIO_PORT_SHIFT)
#define STM32N6_GPIO_PORTA            (0u << STM32N6_GPIO_PORT_SHIFT)
#define STM32N6_GPIO_PORTB            (1u << STM32N6_GPIO_PORT_SHIFT)
#define STM32N6_GPIO_PORTC            (2u << STM32N6_GPIO_PORT_SHIFT)
#define STM32N6_GPIO_PORTD            (3u << STM32N6_GPIO_PORT_SHIFT)
#define STM32N6_GPIO_PORTE            (4u << STM32N6_GPIO_PORT_SHIFT)
#define STM32N6_GPIO_PORTF            (5u << STM32N6_GPIO_PORT_SHIFT)
#define STM32N6_GPIO_PORTG            (6u << STM32N6_GPIO_PORT_SHIFT)
#define STM32N6_GPIO_PORTH            (7u << STM32N6_GPIO_PORT_SHIFT)
#define STM32N6_GPIO_PORTN            (8u << STM32N6_GPIO_PORT_SHIFT)
#define STM32N6_GPIO_PORTO            (9u << STM32N6_GPIO_PORT_SHIFT)
#define STM32N6_GPIO_PORTP            (10u << STM32N6_GPIO_PORT_SHIFT)
#define STM32N6_GPIO_PORTQ            (11u << STM32N6_GPIO_PORT_SHIFT)

#define STM32N6_GPIO_PIN_SHIFT        0
#define STM32N6_GPIO_PIN_MASK         (15u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN(n)           (((uint32_t)(n) & 15u) << \
                                       STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN0             (0u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN1             (1u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN2             (2u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN3             (3u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN4             (4u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN5             (5u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN6             (6u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN7             (7u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN8             (8u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN9             (9u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN10            (10u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN11            (11u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN12            (12u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN13            (13u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN14            (14u << STM32N6_GPIO_PIN_SHIFT)
#define STM32N6_GPIO_PIN15            (15u << STM32N6_GPIO_PIN_SHIFT)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__
int stm32n6_configgpio(uint32_t cfgset);
int stm32n6_unconfiggpio(uint32_t cfgset);
void stm32n6_gpiowrite(uint32_t pinset, bool value);
bool stm32n6_gpioread(uint32_t pinset);
#endif

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_GPIO_H */
