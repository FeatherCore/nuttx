/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/include/board.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __BOARDS_ARM_STM32N6_STM32N6570_DK_INCLUDE_BOARD_H
#define __BOARDS_ARM_STM32N6_STM32N6570_DK_INCLUDE_BOARD_H

/* Target clock profile from STM32CubeN6 Template FSBL 800 MHz setup:
 * Cortex-M55 at 800 MHz, with bus/peripheral clocks in the 400/200 MHz
 * class.  The first code drop exposes these as fixed bring-up constants;
 * the RCC sequence is translated in arch/arm/src/stm32n6/stm32n6_rcc.c.
 */

#define STM32N6_SYSCLK_FREQUENCY       400000000ul
#define STM32N6_CPUCLK_FREQUENCY       800000000ul
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

#define BOARD_LTDC_WIDTH               800u
#define BOARD_LTDC_HEIGHT              480u
#define BOARD_LTDC_BPP                 16u
#define BOARD_LTDC_STRIDE              (BOARD_LTDC_WIDTH * 2u)
#define BOARD_LTDC_FB_COUNT            2u
#define BOARD_LTDC_FRAME_SIZE          \
  (BOARD_LTDC_STRIDE * BOARD_LTDC_HEIGHT)
#define BOARD_LTDC_FB_SIZE             \
  (BOARD_LTDC_FRAME_SIZE * BOARD_LTDC_FB_COUNT)
#define BOARD_LTDC_FB_BASE             STM32N6_XSPI1_PSRAM_BASE
#define BOARD_LTDC_HSYNC               4u
#define BOARD_LTDC_HBP                 4u
#define BOARD_LTDC_HFP                 4u
#define BOARD_LTDC_VSYNC               4u
#define BOARD_LTDC_VBP                 4u
#define BOARD_LTDC_VFP                 4u
#define BOARD_LTDC_CLOCK_DIV           32u

#define STM32N6570_LCD_WIDTH           BOARD_LTDC_WIDTH
#define STM32N6570_LCD_HEIGHT          BOARD_LTDC_HEIGHT
#define STM32N6570_LCD_BPP             BOARD_LTDC_BPP
#define STM32N6570_LCD_STRIDE          BOARD_LTDC_STRIDE
#define STM32N6570_LCD_FB_BASE         BOARD_LTDC_FB_BASE
#define STM32N6570_LCD_FB_SIZE         BOARD_LTDC_FB_SIZE
#define STM32N6570_LCD_FB_RESERVE      BOARD_LTDC_FB_SIZE

/* GPIO pin selections ******************************************************/

#define GPIO_LTDC_PINCFG(p)            (GPIO_ALT | \
                                        GPIO_AF14 | \
                                        GPIO_PUSHPULL | \
                                        GPIO_SPEED_50MHZ | \
                                        GPIO_FLOAT | (p))

#define GPIO_LTDC_R0                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTG | \
                                        GPIO_PIN0)
#define GPIO_LTDC_R1                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTD | \
                                        GPIO_PIN9)
#define GPIO_LTDC_R2                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTD | \
                                        GPIO_PIN15)
#define GPIO_LTDC_R3                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTB | \
                                        GPIO_PIN4)
#define GPIO_LTDC_R4                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTH | \
                                        GPIO_PIN4)
#define GPIO_LTDC_R5                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTA | \
                                        GPIO_PIN15)
#define GPIO_LTDC_R6                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTG | \
                                        GPIO_PIN11)
#define GPIO_LTDC_R7                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTD | \
                                        GPIO_PIN8)

#define GPIO_LTDC_G0                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTG | \
                                        GPIO_PIN12)
#define GPIO_LTDC_G1                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTG | \
                                        GPIO_PIN1)
#define GPIO_LTDC_G2                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTA | \
                                        GPIO_PIN1)
#define GPIO_LTDC_G3                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTA | \
                                        GPIO_PIN0)
#define GPIO_LTDC_G4                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTB | \
                                        GPIO_PIN15)
#define GPIO_LTDC_G5                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTB | \
                                        GPIO_PIN12)
#define GPIO_LTDC_G6                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTB | \
                                        GPIO_PIN11)
#define GPIO_LTDC_G7                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTG | \
                                        GPIO_PIN8)

#define GPIO_LTDC_B1                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTA | \
                                        GPIO_PIN7)
#define GPIO_LTDC_B2                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTB | \
                                        GPIO_PIN2)
#define GPIO_LTDC_B3                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTG | \
                                        GPIO_PIN6)
#define GPIO_LTDC_B4                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTH | \
                                        GPIO_PIN3)
#define GPIO_LTDC_B5                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTH | \
                                        GPIO_PIN6)
#define GPIO_LTDC_B6                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTA | \
                                        GPIO_PIN8)
#define GPIO_LTDC_B7                   GPIO_LTDC_PINCFG( \
                                        GPIO_PORTA | \
                                        GPIO_PIN2)

#define GPIO_LTDC_HSYNC                GPIO_LTDC_PINCFG( \
                                        GPIO_PORTB | \
                                        GPIO_PIN14)
#define GPIO_LTDC_VSYNC                GPIO_LTDC_PINCFG( \
                                        GPIO_PORTE | \
                                        GPIO_PIN11)
#define GPIO_LTDC_CLK                  GPIO_LTDC_PINCFG( \
                                        GPIO_PORTB | \
                                        GPIO_PIN13)

#define GPIO_LCD_OUTCFG_VALUE(p, v)    (GPIO_OUTPUT | \
                                        GPIO_PUSHPULL | \
                                        GPIO_SPEED_50MHZ | \
                                        GPIO_FLOAT | (v) | (p))
#define GPIO_LCD_OUTCFG(p)             GPIO_LCD_OUTCFG_VALUE( \
                                        p, GPIO_OUTPUT_SET)
#define GPIO_LCD_OUTCFG_OFF(p)         GPIO_LCD_OUTCFG_VALUE( \
                                        p, GPIO_OUTPUT_CLEAR)

#define GPIO_LCD_DISP_NRST             GPIO_LCD_OUTCFG( \
                                        GPIO_PORTE | \
                                        GPIO_PIN1)
#define GPIO_LCD_DISP_ONOFF            GPIO_LCD_OUTCFG( \
                                        GPIO_PORTQ | \
                                        GPIO_PIN3)
#define GPIO_LCD_DISP_BL               GPIO_LCD_OUTCFG_OFF( \
                                        GPIO_PORTQ | \
                                        GPIO_PIN6)
#define GPIO_LCD_DISP_EN               GPIO_LCD_OUTCFG_OFF( \
                                        GPIO_PORTG | \
                                        GPIO_PIN13)

#define GPIO_I2C2_SCL                  (GPIO_ALT | \
                                        GPIO_AF4 | \
                                        GPIO_OPENDRAIN | \
                                        GPIO_SPEED_50MHZ | \
                                        GPIO_FLOAT | \
                                        GPIO_PORTD | \
                                        GPIO_PIN14)
#define GPIO_I2C2_SDA                  (GPIO_ALT | \
                                        GPIO_AF4 | \
                                        GPIO_OPENDRAIN | \
                                        GPIO_SPEED_50MHZ | \
                                        GPIO_FLOAT | \
                                        GPIO_PORTD | \
                                        GPIO_PIN4)
#define GPIO_GT911_INT                 (GPIO_INPUT | \
                                        GPIO_FLOAT | \
                                        GPIO_PORTQ | \
                                        GPIO_PIN4)
#define GPIO_GT911_NRST                GPIO_LCD_DISP_NRST

#define LED_STARTED        0
#define LED_HEAPALLOCATE   1
#define LED_IRQSENABLED    2
#define LED_STACKCREATED   3
#define LED_INIRQ          4
#define LED_SIGNAL         5
#define LED_ASSERTION      6
#define LED_PANIC          7

#endif /* __BOARDS_ARM_STM32N6_STM32N6570_DK_INCLUDE_BOARD_H */
