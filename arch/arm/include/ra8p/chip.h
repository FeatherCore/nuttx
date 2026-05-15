/****************************************************************************
 * arch/arm/include/ra8p/chip.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_INCLUDE_RA8P_CHIP_H
#define __ARCH_ARM_INCLUDE_RA8P_CHIP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RA8P_MRAM_BASE              0x02000000
#define RA8P_MRAM_SIZE              0x00100000
#define RA8P_SRAM_BASE              0x22000000
#define RA8P_SRAM_SIZE              0x001d4000
#define RA8P_SDRAM_BASE             0x68000000
#define RA8P_SDRAM_SIZE             0x08000000
#define RA8P_OSPI0_CS0_BASE         0x80000000
#define RA8P_OSPI0_CS0_SIZE         0x10000000
#define RA8P_OSPI0_CS1_BASE         0x90000000
#define RA8P_OSPI0_CS1_SIZE         0x10000000
#define RA8P_OSPI1_CS0_BASE         0x70000000
#define RA8P_OSPI1_CS0_SIZE         0x08000000
#define RA8P_OSPI1_CS1_BASE         0x78000000
#define RA8P_OSPI1_CS1_SIZE         0x08000000

/* The EK-RA8P1 mounted OSPI NOR is 512 Mbit/64 MiB.  The OSPI aperture is
 * larger than the physical flash.
 */

#define RA8P_EK_OSPI_FLASH_BASE     RA8P_OSPI0_CS1_BASE
#define RA8P_EK_OSPI_FLASH_SIZE     0x04000000

/* NVIC priority levels.  Cortex-M85 implements priority fields in the high
 * bits of each priority byte.
 */

#define NVIC_SYSH_PRIORITY_MIN      0xf0
#define NVIC_SYSH_PRIORITY_DEFAULT  0x80
#define NVIC_SYSH_PRIORITY_MAX      0x00
#define NVIC_SYSH_PRIORITY_STEP     0x10

#endif /* __ARCH_ARM_INCLUDE_RA8P_CHIP_H */
