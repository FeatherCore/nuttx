/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_STM32_H
#define __ARCH_ARM_SRC_STM32H7RS_STM32_H

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void stm32_clockconfig(void);
void stm32_lowsetup(void);
void stm32_uart4_wait_txcomplete(void);

#endif /* __ARCH_ARM_SRC_STM32H7RS_STM32_H */
