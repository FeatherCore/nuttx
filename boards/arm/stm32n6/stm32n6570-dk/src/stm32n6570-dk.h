/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32n6570-dk.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __BOARDS_ARM_STM32N6_STM32N6570_DK_SRC_STM32N6570_DK_H
#define __BOARDS_ARM_STM32N6_STM32N6570_DK_SRC_STM32N6570_DK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#include <nuttx/compiler.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int stm32_xspi2_nor_initialize(void);
int stm32_xspi1_psram_initialize(void);
int stm32_xspi1_psram_map(void);
int stm32_xspi1_psram_test(uintptr_t base, size_t size);
int stm32_xspi2_nor_erase(uint32_t offset, size_t nbytes);
ssize_t stm32_xspi2_nor_write(uint32_t offset,
                                   FAR const uint8_t *buffer,
                                   size_t nbytes);
int stm32_extmem_initialize(void);
int stm32_touchscreen_setup(void);

#endif /* __BOARDS_ARM_STM32N6_STM32N6570_DK_SRC_STM32N6570_DK_H */
