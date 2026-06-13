/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_STM32_H
#define __ARCH_ARM_SRC_STM32H7RS_STM32_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include <nuttx/irq.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void stm32_clockconfig(void);
void stm32_lowsetup(void);
void stm32_uart4_wait_txcomplete(void);

int stm32_gpiosetevent(uintptr_t portbase, uint8_t pin, bool risingedge,
                       bool fallingedge, bool event, xcpt_t func,
                       void *arg);

#endif /* __ARCH_ARM_SRC_STM32H7RS_STM32_H */
