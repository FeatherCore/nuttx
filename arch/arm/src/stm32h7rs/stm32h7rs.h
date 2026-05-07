/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32h7rs.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_STM32H7RS_H
#define __ARCH_ARM_SRC_STM32H7RS_STM32H7RS_H

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void stm32h7rs_clockconfig(void);
void stm32h7rs_lowsetup(void);
int stm32h7rs_xspi2_nor_initialize(void);
int stm32h7rs_xspi1_psram_initialize(void);

#endif /* __ARCH_ARM_SRC_STM32H7RS_STM32H7RS_H */
