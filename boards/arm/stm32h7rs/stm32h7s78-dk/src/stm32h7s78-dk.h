/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/src/stm32h7s78-dk.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __BOARDS_ARM_STM32H7RS_STM32H7S78_DK_SRC_STM32H7S78_DK_H
#define __BOARDS_ARM_STM32H7RS_STM32H7S78_DK_SRC_STM32H7S78_DK_H

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int stm32_extmem_initialize(void);
int stm32_xspi2_nor_initialize(void);
int stm32_xspi1_psram_initialize(void);
int stm32_touchscreen_setup(void);
int stm32_aw8697_setup(void);
int stm32_bme280_setup(void);
int stm32_max30102_setup(void);
int stm32_lsm6dsox_setup(void);

#endif /* __BOARDS_ARM_STM32H7RS_STM32H7S78_DK_SRC_STM32H7S78_DK_H */
