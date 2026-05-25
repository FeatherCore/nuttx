/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/include/board.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __BOARDS_ARM_STM32H7RS_STM32H7S78_DK_INCLUDE_BOARD_H
#define __BOARDS_ARM_STM32H7RS_STM32H7S78_DK_INCLUDE_BOARD_H

/* Clocking follows the STM32CubeH7RS STM32H7S78-DK LL boot template:
 * HSI / 32 * 300 -> PLL1P, SYSCLK = CPUCLK = 600 MHz, AHB = SYSCLK / 2
 * and APB1/APB2/APB4/APB5 = AHB / 2.  PLL2S is 200 MHz and feeds
 * XSPI1/XSPI2, matching Cube's STM32H7S78-DK XIP boot template.
 */

#define STM32_SYSCLK_FREQUENCY 600000000ul
#define STM32_CPUCLK_FREQUENCY 600000000ul
#define STM32_HCLK_FREQUENCY   300000000ul
#define STM32_PCLK1_FREQUENCY  150000000ul
#define STM32_PCLK2_FREQUENCY  150000000ul
#define STM32_PCLK4_FREQUENCY  150000000ul
#define STM32_PCLK5_FREQUENCY  150000000ul
#define STM32_PLL2S_FREQUENCY  200000000ul

#define STM32_UART4_BAUD        115200ul

#define BOARD_LTDC_WIDTH            800
#define BOARD_LTDC_HEIGHT           480
#define BOARD_LTDC_BPP              16
#define BOARD_LTDC_STRIDE           ((BOARD_LTDC_WIDTH * BOARD_LTDC_BPP) / 8)
#define BOARD_LTDC_FRAME_SIZE       (BOARD_LTDC_STRIDE * BOARD_LTDC_HEIGHT)
#define BOARD_LTDC_FB_COUNT         2
#define BOARD_LTDC_FB_SIZE          (BOARD_LTDC_FRAME_SIZE * \
                                     BOARD_LTDC_FB_COUNT)
#define BOARD_LTDC_FB_BASE          0x90000000ul

/* RK050HR18 timing, matching STM32CubeH7RS STM32H7S78-DK BSP. */

#define BOARD_LTDC_HSYNC            4
#define BOARD_LTDC_HBP              8
#define BOARD_LTDC_HFP              8
#define BOARD_LTDC_VSYNC            4
#define BOARD_LTDC_VBP              8
#define BOARD_LTDC_VFP              8

#define LED_STARTED        0
#define LED_HEAPALLOCATE   1
#define LED_IRQSENABLED    2
#define LED_STACKCREATED   3
#define LED_INIRQ          4
#define LED_SIGNAL         5
#define LED_ASSERTION      6
#define LED_PANIC          7

#endif /* __BOARDS_ARM_STM32H7RS_STM32H7S78_DK_INCLUDE_BOARD_H */
