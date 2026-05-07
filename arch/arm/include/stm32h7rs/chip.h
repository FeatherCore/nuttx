/****************************************************************************
 * arch/arm/include/stm32h7rs/chip.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_INCLUDE_STM32H7RS_CHIP_H
#define __ARCH_ARM_INCLUDE_STM32H7RS_CHIP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if !defined(CONFIG_ARCH_CHIP_STM32H7S7L8)
#  error STM32H7RS chip not identified
#endif

/* STM32H7S7L8 resources used by the STM32H7S78-DK first-stage port.
 * The sizes come from the STM32H7RS CMSIS device header and the CubeH7RS
 * STM32H7S78-DK XIP template.
 */

#define STM32H7RS_FLASH_SIZE        (64 * 1024)
#define STM32H7RS_AXI_SRAM_SIZE     (456 * 1024)
#define STM32H7RS_AHB_SRAM_SIZE     (16 * 1024)
#define STM32H7RS_XSPI2_NOR_SIZE    (128 * 1024 * 1024)
#define STM32H7RS_XSPI1_PSRAM_SIZE  (32 * 1024 * 1024)

#define STM32H7RS_NGPIO             16
#define STM32H7RS_NUSART            3
#define STM32H7RS_NUART             5
#define STM32H7RS_NXSPI             2

#define NVIC_SYSH_PRIORITY_MIN      0xf0
#define NVIC_SYSH_PRIORITY_DEFAULT  0x80
#define NVIC_SYSH_PRIORITY_MAX      0x00
#define NVIC_SYSH_PRIORITY_STEP     0x10

#endif /* __ARCH_ARM_INCLUDE_STM32H7RS_CHIP_H */
