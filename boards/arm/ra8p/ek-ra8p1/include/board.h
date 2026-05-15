/****************************************************************************
 * boards/arm/ra8p/ek-ra8p1/include/board.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __BOARDS_ARM_RA8P_EK_RA8P1_INCLUDE_BOARD_H
#define __BOARDS_ARM_RA8P_EK_RA8P1_INCLUDE_BOARD_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* FSP quickstart clock reference:
 *
 *   XTAL:   24 MHz
 *   PLL1P:  1 GHz, CPUCLK /1
 *   ICLK:   CPUCLK /4
 *   PCLKA:  CPUCLK /8
 *   BCLK:   CPUCLK /8
 *   SCICLK: 96 MHz
 */

#define RA8P_XTAL_FREQUENCY      24000000
#define RA8P_CPUCLK_FREQUENCY    1000000000
#define RA8P_ICLK_FREQUENCY      250000000
#define RA8P_PCLKA_FREQUENCY     125000000
#define RA8P_BCLK_FREQUENCY      125000000
#define RA8P_SCICLK_FREQUENCY    96000000

#define RA8P_MRAM_START          0x02000000
#define RA8P_MRAM_SIZE           0x00100000
#define RA8P_SRAM_START          0x22000000
#define RA8P_SRAM_SIZE           0x001d4000
#define RA8P_SDRAM_START         0x68000000
#define RA8P_SDRAM_SIZE          0x04000000
#define RA8P_OSPI0_CS1_START     0x90000000
#define RA8P_OSPI_NOR_SIZE       0x04000000

#define LED_STARTED              0
#define LED_HEAPALLOCATE         0
#define LED_IRQSENABLED          0
#define LED_STACKCREATED         0
#define LED_INIRQ                0
#define LED_SIGNAL               0
#define LED_ASSERTION            0
#define LED_PANIC                0

#endif /* __BOARDS_ARM_RA8P_EK_RA8P1_INCLUDE_BOARD_H */
