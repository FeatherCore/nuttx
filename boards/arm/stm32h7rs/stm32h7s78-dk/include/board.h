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

#define STM32H7RS_SYSCLK_FREQUENCY 600000000ul
#define STM32H7RS_CPUCLK_FREQUENCY 600000000ul
#define STM32H7RS_HCLK_FREQUENCY   300000000ul
#define STM32H7RS_PCLK1_FREQUENCY  150000000ul
#define STM32H7RS_PCLK2_FREQUENCY  150000000ul
#define STM32H7RS_PCLK4_FREQUENCY  150000000ul
#define STM32H7RS_PCLK5_FREQUENCY  150000000ul
#define STM32H7RS_PLL2S_FREQUENCY  200000000ul

#define STM32H7RS_UART4_BAUD        115200ul

#define LED_STARTED        0
#define LED_HEAPALLOCATE   1
#define LED_IRQSENABLED    2
#define LED_STACKCREATED   3
#define LED_INIRQ          4
#define LED_SIGNAL         5
#define LED_ASSERTION      6
#define LED_PANIC          7

#endif /* __BOARDS_ARM_STM32H7RS_STM32H7S78_DK_INCLUDE_BOARD_H */
