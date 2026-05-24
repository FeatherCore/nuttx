/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32u5x9j-dk.h
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

#ifndef __BOARDS_ARM_STM32U5_STM32U5X9J_DK_SRC_STM32U5X9J_DK_H
#define __BOARDS_ARM_STM32U5_STM32U5X9J_DK_SRC_STM32U5X9J_DK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <stdbool.h>
#include <stdint.h>

#include "hardware/stm32_memorymap.h"
#include "stm32_gpio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#define HAVE_PROC             1
#define HAVE_RTC_DRIVER       1
#define HAVE_I2C_DRIVER       1
#define HAVE_I2C_PROBES       1

#if !defined(CONFIG_FS_PROCFS)
#  undef HAVE_PROC
#endif

#if defined(HAVE_PROC) && defined(CONFIG_DISABLE_MOUNTPOINT)
#  warning Mountpoints disabled.  No procfs support
#  undef HAVE_PROC
#endif

/* Check if we can support the RTC driver */

#if !defined(CONFIG_RTC) || !defined(CONFIG_RTC_DRIVER)
#  undef HAVE_RTC_DRIVER
#endif

#if !defined(CONFIG_I2C) || !defined(CONFIG_I2C_DRIVER)
#  undef HAVE_I2C_DRIVER
#endif

#if !defined(CONFIG_STM32U5X9J_DK_I2C_PROBES) || !defined(HAVE_I2C_DRIVER)
#  undef HAVE_I2C_PROBES
#endif

/* STM32U5x9J-DK LEDs and user button. */

#define GPIO_LED_GREEN (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHZ | \
                        GPIO_OUTPUT_CLEAR | GPIO_PORTE | GPIO_PIN0)
#define GPIO_LED_RED   (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHZ | \
                        GPIO_OUTPUT_CLEAR | GPIO_PORTE | GPIO_PIN1)

#define GPIO_BTN_USER  (GPIO_INPUT | GPIO_FLOAT | GPIO_EXTI | \
                        GPIO_PORTC | GPIO_PIN13)

/* Board-local probe GPIOs from the STM32U5x9J-DK Cube BSP. */

#define GPIO_TS_INT       (GPIO_INPUT | GPIO_FLOAT | GPIO_EXTI | \
                           GPIO_PORTE | GPIO_PIN8)
#define GPIO_VL53L5CX_LP (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHZ | \
                           GPIO_OUTPUT_SET | GPIO_PORTE | GPIO_PIN14)
#define GPIO_VL53L5CX_INT (GPIO_INPUT | GPIO_FLOAT | GPIO_EXTI | \
                           GPIO_PORTB | GPIO_PIN5)

#define GPIO_LCD_DSI_POWER (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHZ | \
                            GPIO_OUTPUT_SET | GPIO_PORTI | GPIO_PIN5)
#define GPIO_LCD_RESET     (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_PULLDOWN | \
                            GPIO_SPEED_2MHZ | GPIO_OUTPUT_SET | \
                            GPIO_PORTD | GPIO_PIN5)
#define GPIO_LCD_BACKLIGHT (GPIO_INPUT | GPIO_PULLUP | GPIO_PORTI | \
                            GPIO_PIN6)

/* STM32U5x9J-DK external memories. */

#define STM32U5X9J_OSPI1_NOR_MEM_BASE        STM32_OCTOSPI1_BANK
#define STM32U5X9J_OSPI1_NOR_SIZE            (64u * 1024u * 1024u)

#define STM32U5X9J_HSPI1_PSRAM_MEM_BASE      STM32_HSPI1_BANK
#define STM32U5X9J_HSPI1_PSRAM_SIZE          (64u * 1024u * 1024u)

#ifdef CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER
#  define STM32U5X9J_LCD_FB_COUNT            2u
#else
#  define STM32U5X9J_LCD_FB_COUNT            1u
#endif

/* Internal-SRAM framebuffer mode uses the GFXMMU circular layout to reduce
 * the real SRAM storage consumed by each 480x480 frame.  PSRAM framebuffer
 * mode intentionally reserves a larger linear window, because the verified
 * stable path is direct LTDC scanout without GFXMMU translation.
 */

#ifdef CONFIG_STM32U5X9J_DK_LCD_XRGB8888
#  define STM32U5X9J_LCD_GFXMMU_FRAME_SIZE   0x000b20c0u
#  define STM32U5X9J_LCD_PSRAM_FB_RESERVED_SIZE \
          (2u * 1024u * 1024u)
#else
#  define STM32U5X9J_LCD_GFXMMU_FRAME_SIZE   0x00059d80u
#  define STM32U5X9J_LCD_PSRAM_FB_RESERVED_SIZE \
          (1u * 1024u * 1024u)
#endif

#if defined(CONFIG_STM32U5X9J_DK_LCD_FB_PSRAM)
#  define STM32U5X9J_HSPI1_PSRAM_FB_RESERVED_SIZE \
          STM32U5X9J_LCD_PSRAM_FB_RESERVED_SIZE
#else
#  define STM32U5X9J_HSPI1_PSRAM_FB_RESERVED_SIZE 0u
#endif

#if defined(CONFIG_STM32U5X9J_DK_LCD_FB_SRAM)
#  define STM32U5X9J_INTERNAL_SRAM_FB_SIZE   \
          (STM32U5X9J_LCD_GFXMMU_FRAME_SIZE * STM32U5X9J_LCD_FB_COUNT)
#  define STM32U5X9J_INTERNAL_SRAM_FB_BASE   \
          (CONFIG_STM32U5X9J_PROTECTED_USRAM_BASE - \
           STM32U5X9J_INTERNAL_SRAM_FB_SIZE)
#endif

#define STM32U5X9J_HSPI1_PSRAM_HEAP_BASE     \
  (STM32U5X9J_HSPI1_PSRAM_MEM_BASE + \
   STM32U5X9J_HSPI1_PSRAM_FB_RESERVED_SIZE)
#define STM32U5X9J_HSPI1_PSRAM_HEAP_SIZE     \
  (STM32U5X9J_HSPI1_PSRAM_SIZE - \
   STM32U5X9J_HSPI1_PSRAM_FB_RESERVED_SIZE)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

/****************************************************************************
 * Public Function Declarations
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_bringup
 *
 * Description:
 *   Perform architecture-specific initialization
 *
 *   CONFIG_BOARD_LATE_INITIALIZE=y :
 *     Called from board_late_initialize().
 *
 ****************************************************************************/

int stm32_bringup(void);

#if defined(CONFIG_STM32U5X9J_DK_OSPI_NOR) || \
    defined(CONFIG_STM32U5X9J_DK_HSPI_RAM)
int stm32u5x9j_extmem_initialize(void);
#endif

#ifdef CONFIG_STM32U5X9J_DK_OSPI_NOR
int stm32u5x9j_ospi1_nor_initialize(void);
#ifdef CONFIG_STM32U5X9J_DK_OSPI_DIAG
int stm32u5x9j_ospi_diag_initialize(void);
#endif
#endif

#ifdef CONFIG_STM32U5X9J_DK_HSPI_RAM
int stm32u5x9j_hspi1_psram_initialize(void);
bool stm32u5x9j_hspi1_psram_is_mapped(void);
#ifdef CONFIG_STM32U5X9J_DK_HSPI_DIAG
int stm32u5x9j_hspi_diag_initialize(void);
#endif
#endif

#ifdef CONFIG_STM32U5X9J_DK_EMMC
int stm32u5x9j_emmc_initialize(void);
#endif

#ifdef CONFIG_STM32U5X9J_DK_LCD
int stm32u5x9j_lcd_initialize(void);
#endif

#ifdef HAVE_I2C_PROBES
struct i2c_master_s;
int stm32u5x9j_i2c_probes_initialize(FAR struct i2c_master_s *i2c3,
                                     FAR struct i2c_master_s *i2c5);
#endif

#ifdef CONFIG_STM32U5X9J_DK_TOUCH
struct i2c_master_s;
int stm32u5x9j_touch_initialize(FAR struct i2c_master_s *i2c5);
#endif

#endif /* __ASSEMBLY__ */
#endif /* __BOARDS_ARM_STM32U5_STM32U5X9J_DK_SRC_STM32U5X9J_DK_H */
