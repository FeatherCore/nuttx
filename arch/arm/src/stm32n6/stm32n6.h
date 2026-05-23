/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void stm32n6_clockconfig(void);
void stm32n6_power_config(void);
void stm32n6_lowsetup(void);
void stm32n6_earlyputc(char ch);
void stm32n6_earlyputs(const char *str);
void stm32n6_usart1_wait_txcomplete(void);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_H */
