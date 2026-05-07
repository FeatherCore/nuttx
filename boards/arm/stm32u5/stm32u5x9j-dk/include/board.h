/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/include/board.h
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

#ifndef __BOARDS_ARM_STM32U5_STM32U5X9J_DK_INCLUDE_BOARD_H
#define __BOARDS_ARM_STM32U5_STM32U5X9J_DK_INCLUDE_BOARD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#ifndef __ASSEMBLY__
#  include <stdint.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Clocking *****************************************************************/

/* The STM32U5x9J-DK Cube LL template configures the maximum system clock
 * from MSIS and PLL1. Keep the first NuttX bring-up on the same 160 MHz path.
 *
 *   System Clock source : PLL (MSIS)
 *   SYSCLK(Hz)          : 160000000   Determined by PLL configuration
 *   HCLK(Hz)            : 160000000
 *   AHB Prescaler       : 1            (STM32_RCC_CFGR2_HPRE)  (160MHz)
 *   APB1 Prescaler      : 1            (STM32_RCC_CFGR2_PPRE1) (160MHz)
 *   APB2 Prescaler      : 1            (STM32_RCC_CFGR2_PPRE2) (160MHz)
 *   APB3 Prescaler      : 1            (STM32_RCC_CFGR3_PPRE3) (160MHz)
 *   MSIS Frequency(Hz)  : 4000000      (nominal)
 *   MSIK Frequency(Hz)  : 4000000      (nominal)
 *   PLL_MBOOST          : 1            (Embedded power distribution booster)
 *   PLLM                : 1            (STM32_PLLCFG_PLLM)
 *   PLLN                : 80           (STM32_PLLCFG_PLLN)
 *   PLLP                : 2            (STM32_PLLCFG_PLLP)
 *   PLLQ                : 2            (STM32_PLLCFG_PLLQ)
 *   PLLR                : 2            (STM32_PLLCFG_PLLR)
 *   Flash Latency(WS)   : 4
 */

/* HSI - 16 MHz RC factory-trimmed
 * LSI - 32 KHz RC
 * MSI - 4 MHz, autotrimmed via LSE
 * HSE - not installed
 * LSE - 32.768 kHz installed
 */

#define STM32_HSI_FREQUENCY     16000000ul
#define STM32_LSI_FREQUENCY     32000
#define STM32_LSE_FREQUENCY     32768

#define STM32_BOARD_USEMSIS     1
#define STM32_BOARD_MSISRANGE   RCC_ICSCR1_MSISRANGE_4MHZ
#define STM32_BOARD_MSIKRANGE   RCC_ICSCR1_MSIKRANGE_4MHZ

/* PLL1 config; we use this to generate our system clock */

#define STM32_RCC_PLL1CFGR_PLL1M          RCC_PLL1CFGR_PLL1M(1)
#define STM32_RCC_PLL1DIVR_PLL1N          RCC_PLL1DIVR_PLL1N(80)
#define STM32_RCC_PLL1DIVR_PLL1P          0
#undef  STM32_RCC_PLL1CFGR_PLL1P_ENABLED
#define STM32_RCC_PLL1DIVR_PLL1Q          0
#undef  STM32_RCC_PLL1CFGR_PLL1Q_ENABLED
#define STM32_RCC_PLL1DIVR_PLL1R          RCC_PLL1DIVR_PLL1R(2)
#define STM32_RCC_PLL1CFGR_PLL1R_ENABLED

#define STM32_SYSCLK_FREQUENCY  160000000ul

/* Enable LSE (for the RTC and for MSIS autotrimming) */

#define STM32_USE_LSE           1

/* Configure the HCLK divisor (for the AHB bus, core, memory, and DMA */

#define STM32_RCC_CFGR2_HPRE    RCC_CFGR2_HPRE_SYSCLK     /* HCLK  = SYSCLK / 1 */
#define STM32_HCLK_FREQUENCY    STM32_SYSCLK_FREQUENCY

/* Configure the APB1 prescaler */

#define STM32_RCC_CFGR2_PPRE1   RCC_CFGR2_PPRE1_HCLK      /* PCLK1 = HCLK / 1 */
#define STM32_PCLK1_FREQUENCY   (STM32_HCLK_FREQUENCY / 1)

#define STM32_APB1_TIM2_CLKIN   (STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM3_CLKIN   (STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM4_CLKIN   (STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM5_CLKIN   (STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM6_CLKIN   (STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM7_CLKIN   (STM32_PCLK1_FREQUENCY)

/* Configure the APB2 prescaler */

#define STM32_RCC_CFGR2_PPRE2   RCC_CFGR2_PPRE2_HCLK       /* PCLK2 = HCLK / 1 */
#define STM32_PCLK2_FREQUENCY   (STM32_HCLK_FREQUENCY / 1)

#define STM32_APB2_TIM1_CLKIN   (STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM15_CLKIN  (STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM16_CLKIN  (STM32_PCLK2_FREQUENCY)

/* Configure the APB3 prescaler */

#define STM32_RCC_CFGR3_PPRE3   RCC_CFGR3_PPRE3_HCLK       /* PCLK3 = HCLK / 1 */
#define STM32_PCLK3_FREQUENCY   (STM32_HCLK_FREQUENCY / 1)

/* The timer clock frequencies are automatically defined by hardware.  If the
 * APB prescaler equals 1, the timer clock frequencies are set to the same
 * frequency as that of the APB domain. Otherwise they are set to twice.
 * Note: TIM1,15,16 are on APB2, others on APB1
 */

#define BOARD_TIM1_FREQUENCY    STM32_HCLK_FREQUENCY
#define BOARD_TIM2_FREQUENCY    STM32_HCLK_FREQUENCY
#define BOARD_TIM3_FREQUENCY    STM32_HCLK_FREQUENCY
#define BOARD_TIM4_FREQUENCY    STM32_HCLK_FREQUENCY
#define BOARD_TIM5_FREQUENCY    STM32_HCLK_FREQUENCY
#define BOARD_TIM6_FREQUENCY    STM32_HCLK_FREQUENCY
#define BOARD_TIM7_FREQUENCY    STM32_HCLK_FREQUENCY
#define BOARD_TIM15_FREQUENCY   STM32_HCLK_FREQUENCY
#define BOARD_TIM16_FREQUENCY   STM32_HCLK_FREQUENCY
#define BOARD_LPTIM1_FREQUENCY  STM32_HCLK_FREQUENCY
#define BOARD_LPTIM2_FREQUENCY  STM32_HCLK_FREQUENCY

/* DMA Channel/Stream Selections ********************************************/

/* Alternate function pin selections ****************************************/

/* USART1: Connected to STLINK-V3EC VCP. */

#define GPIO_USART1_RX   GPIO_USART1_RX_1    /* PA10 */
#define GPIO_USART1_TX   GPIO_USART1_TX_1    /* PA9  */

/* I2C buses from the STM32U5x9J-DK Cube BSP. */

#define GPIO_I2C2_SCL    (GPIO_I2C2_SCL_3 | GPIO_SPEED_50MHZ | \
                          GPIO_OPENDRAIN | GPIO_OUTPUT_SET) /* PF1 */
#define GPIO_I2C2_SDA    (GPIO_I2C2_SDA_3 | GPIO_SPEED_50MHZ | \
                          GPIO_OPENDRAIN | GPIO_OUTPUT_SET) /* PF0 */

#define GPIO_I2C3_SCL    (GPIO_I2C3_SCL_4 | GPIO_SPEED_50MHZ | \
                          GPIO_OPENDRAIN | GPIO_OUTPUT_SET) /* PH7 */
#define GPIO_I2C3_SDA    (GPIO_I2C3_SDA_4 | GPIO_SPEED_50MHZ | \
                          GPIO_OPENDRAIN | GPIO_OUTPUT_SET) /* PH8 */

#define GPIO_I2C4_SCL    (GPIO_I2C4_SCL_1 | GPIO_SPEED_50MHZ | \
                          GPIO_OPENDRAIN | GPIO_OUTPUT_SET) /* PB6 */
#define GPIO_I2C4_SDA    (GPIO_I2C4_SDA_1 | GPIO_SPEED_50MHZ | \
                          GPIO_OPENDRAIN | GPIO_OUTPUT_SET) /* PB7 */

#define GPIO_I2C5_SCL    (GPIO_I2C5_SCL_1 | GPIO_SPEED_50MHZ | \
                          GPIO_OPENDRAIN | GPIO_OUTPUT_SET) /* PH5 */
#define GPIO_I2C5_SDA    (GPIO_I2C5_SDA_1 | GPIO_SPEED_50MHZ | \
                          GPIO_OPENDRAIN | GPIO_OUTPUT_SET) /* PH4 */

/* OSPI1 NOR: MX25UM51245G on OCTOSPIM port 1, mapped at 0x90000000. */

#define GPIO_OSPI1_CLK   (GPIO_OCTOSPIM_P1_CLK_1 | GPIO_SPEED_100MHZ) /* PF10 */
#define GPIO_OSPI1_DQS   (GPIO_OCTOSPIM_P1_DQS_3 | GPIO_SPEED_100MHZ) /* PA1 */
#define GPIO_OSPI1_NCS   (GPIO_OCTOSPIM_P1_NCS_4 | GPIO_SPEED_100MHZ) /* PA2 */
#define GPIO_OSPI1_IO0   (GPIO_OCTOSPIM_P1_IO0_3 | GPIO_SPEED_100MHZ) /* PF8 */
#define GPIO_OSPI1_IO1   (GPIO_OCTOSPIM_P1_IO1_3 | GPIO_SPEED_100MHZ) /* PF9 */
#define GPIO_OSPI1_IO2   (GPIO_OCTOSPIM_P1_IO2_3 | GPIO_SPEED_100MHZ) /* PF7 */
#define GPIO_OSPI1_IO3   (GPIO_OCTOSPIM_P1_IO3_3 | GPIO_SPEED_100MHZ) /* PF6 */
#define GPIO_OSPI1_IO4   (GPIO_OCTOSPIM_P1_IO4_2 | GPIO_SPEED_100MHZ) /* PC1 */
#define GPIO_OSPI1_IO5   (GPIO_OCTOSPIM_P1_IO5_3 | GPIO_SPEED_100MHZ) /* PC2 */
#define GPIO_OSPI1_IO6   (GPIO_OCTOSPIM_P1_IO6_1 | GPIO_SPEED_100MHZ) /* PC3 */
#define GPIO_OSPI1_IO7   (GPIO_OCTOSPIM_P1_IO7_1 | GPIO_SPEED_100MHZ) /* PC0 */

/* HSPI1 PSRAM: APS512xx 16-bit APMEM, mapped at 0xa0000000. */

#define GPIO_HSPI1_CLK   (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN3 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_NCLK  (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN4 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_DQS0  (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN2 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_DQS1  (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN8 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_NCS   (GPIO_ALT | GPIO_AF8 | GPIO_PORTH | GPIO_PIN9 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D0    (GPIO_ALT | GPIO_AF8 | GPIO_PORTH | GPIO_PIN10 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D1    (GPIO_ALT | GPIO_AF8 | GPIO_PORTH | GPIO_PIN11 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D2    (GPIO_ALT | GPIO_AF8 | GPIO_PORTH | GPIO_PIN12 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D3    (GPIO_ALT | GPIO_AF8 | GPIO_PORTH | GPIO_PIN13 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D4    (GPIO_ALT | GPIO_AF8 | GPIO_PORTH | GPIO_PIN14 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D5    (GPIO_ALT | GPIO_AF8 | GPIO_PORTH | GPIO_PIN15 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D6    (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN0 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D7    (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN1 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D8    (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN9 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D9    (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN10 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D10   (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN11 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D11   (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN12 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D12   (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN13 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D13   (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN14 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D14   (GPIO_ALT | GPIO_AF8 | GPIO_PORTI | GPIO_PIN15 | \
                          GPIO_SPEED_100MHZ)
#define GPIO_HSPI1_D15   (GPIO_ALT | GPIO_AF8 | GPIO_PORTJ | GPIO_PIN0 | \
                          GPIO_SPEED_100MHZ)

/* SDMMC1 eMMC: 8-bit bus. */

#define GPIO_SDMMC1_CK   (GPIO_SDMMC1_CK_1  | GPIO_SPEED_100MHZ)
#define GPIO_SDMMC1_CMD  (GPIO_SDMMC1_CMD_1 | GPIO_SPEED_100MHZ | GPIO_PULLUP)
#define GPIO_SDMMC1_D0   (GPIO_SDMMC1_D0_1  | GPIO_SPEED_100MHZ | GPIO_PULLUP)
#define GPIO_SDMMC1_D1   (GPIO_SDMMC1_D1_1  | GPIO_SPEED_100MHZ | GPIO_PULLUP)
#define GPIO_SDMMC1_D2   (GPIO_SDMMC1_D2_1  | GPIO_SPEED_100MHZ | GPIO_PULLUP)
#define GPIO_SDMMC1_D3   (GPIO_SDMMC1_D3_1  | GPIO_SPEED_100MHZ | GPIO_PULLUP)
#define GPIO_SDMMC1_D4   (GPIO_SDMMC1_D4_1  | GPIO_SPEED_100MHZ | GPIO_PULLUP)
#define GPIO_SDMMC1_D5   (GPIO_SDMMC1_D5_1  | GPIO_SPEED_100MHZ | GPIO_PULLUP)
#define GPIO_SDMMC1_D6   (GPIO_SDMMC1_D6_1  | GPIO_SPEED_100MHZ | GPIO_PULLUP)
#define GPIO_SDMMC1_D7   (GPIO_SDMMC1_D7_1  | GPIO_SPEED_100MHZ | GPIO_PULLUP)

/* SDMMC uses the 160 MHz SYSCLK kernel clock. The divisors are deliberately
 * conservative for first eMMC bring-up: 400 kHz during identification and
 * about 20 MHz for transfer.
 */

#define STM32_SDMMC_INIT_CLKDIV    200
#define STM32_SDMMC_MMCXFR_CLKDIV  4
#define STM32_SDMMC_SDXFR_CLKDIV   4

/* LED and button identifiers from the STM32U5x9J-DK BSP. GPIO definitions
 * live in the board-private header until user LED/button drivers are added.
 */

#define BOARD_LED3          0
#define BOARD_LED_GREEN     BOARD_LED3
#define BOARD_LED4          1
#define BOARD_LED_RED       BOARD_LED4
#define BOARD_NLEDS         2

#define BOARD_LED3_BIT      (1 << BOARD_LED3)
#define BOARD_LED_GREEN_BIT BOARD_LED3_BIT
#define BOARD_LED4_BIT      (1 << BOARD_LED4)
#define BOARD_LED_RED_BIT   BOARD_LED4_BIT

/* Auto-LED event IDs required by common NuttX code when the board advertises
 * LED capability. The STM32U5x9J-DK default NSH config leaves
 * CONFIG_ARCH_LEDS disabled and uses /dev/userleds instead.
 */

#define LED_STARTED        0
#define LED_HEAPALLOCATE   1
#define LED_IRQSENABLED    2
#define LED_STACKCREATED   3
#define LED_INIRQ          4
#define LED_SIGNAL         5
#define LED_ASSERTION      6
#define LED_PANIC          7
#define LED_IDLE           8

#define BUTTON_USER         0
#define NUM_BUTTONS         1
#define BUTTON_USER_BIT     (1 << BUTTON_USER)
#define MIN_IRQBUTTON       BUTTON_USER
#define MAX_IRQBUTTON       BUTTON_USER

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_board_initialize
 *
 * Description:
 *   All STM32 architectures must provide the following entry point.
 *   This entry point is called early in the initialization -- after all
 *   memory has been configured and mapped but before any devices
 *   have been initialized.
 *
 ****************************************************************************/

void stm32_board_initialize(void);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif  /* __BOARDS_ARM_STM32U5_STM32U5X9J_DK_INCLUDE_BOARD_H */
