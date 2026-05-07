/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/include/board.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __BOARDS_ARM_STM32N6_STM32N6570_DK_INCLUDE_BOARD_H
#define __BOARDS_ARM_STM32N6_STM32N6570_DK_INCLUDE_BOARD_H

/* Target clock profile from STM32CubeN6 Template_FSBL_XIP_Custom:
 * Cortex-M55 at 600 MHz, with bus/peripheral clocks in the 400/200 MHz
 * class.  The first code drop exposes these as fixed bring-up constants;
 * the RCC sequence is translated in arch/arm/src/stm32n6/stm32n6_rcc.c.
 */

#define STM32N6_SYSCLK_FREQUENCY       400000000ul
#define STM32N6_CPUCLK_FREQUENCY       600000000ul
#define STM32N6_SYSBUS_FREQUENCY       400000000ul
#define STM32N6_HCLK_FREQUENCY         200000000ul
#define STM32N6_PCLK1_FREQUENCY        200000000ul
#define STM32N6_PCLK2_FREQUENCY        200000000ul
#define STM32N6_XSPI_FREQUENCY         200000000ul

#define STM32N6_USART1_BAUD            115200ul

#define STM32N6_FSBL_LOAD_BASE         0x34180400u
#define STM32N6_APP_RAM_BASE           0x34000000u
#define STM32N6_XSPI2_NOR_BASE         0x70000000u
#define STM32N6_XSPI1_PSRAM_BASE       0x90000000u
#define STM32N6_NXBOOT_APP_OFFSET      0x00100000u
#define STM32N6_NXBOOT_APP_BASE        0x70100000u
#define STM32N6_NXBOOT_HEADER_SIZE     0x00000400u
#define STM32N6_NXBOOT_APP_VECTOR      0x70100400u

#define LED_STARTED        0
#define LED_HEAPALLOCATE   1
#define LED_IRQSENABLED    2
#define LED_STACKCREATED   3
#define LED_INIRQ          4
#define LED_SIGNAL         5
#define LED_ASSERTION      6
#define LED_PANIC          7

#endif /* __BOARDS_ARM_STM32N6_STM32N6570_DK_INCLUDE_BOARD_H */
