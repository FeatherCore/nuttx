/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/src/stm32h7s78_lcd.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <syslog.h>

#include <arch/board/board.h>
#include <nuttx/arch.h>
#include <nuttx/video/fb.h>

#include "arm_internal.h"

#include "hardware/stm32h7rs_gpio.h"
#include "hardware/stm32h7rs_lptim.h"
#include "hardware/stm32h7rs_rcc.h"

#include "stm32h7rs_ltdc.h"
#include "stm32h7rs_mpuinit.h"

#include "stm32h7s78-dk.h"

#ifdef CONFIG_STM32H7S78_DK_LCD

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LCD_POWER_DELAY_MS       20u
#define LCD_DISP_EN_PIN          15u
#define LCD_BACKLIGHT_PIN        15u
#define LCD_BACKLIGHT_BRIGHTNESS 40u
#define LCD_LPTIM_PERIOD         10000u
#define LCD_LPTIM_TIMEOUT        100000u

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_lcd_initialized;
static bool g_lcd_backlight_on;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32h7s78_gpio_write(uintptr_t portbase, uint32_t pin,
                                  bool value)
{
  if (value)
    {
      putreg32(1u << pin, portbase + STM32H7RS_GPIO_BSRR_OFFSET);
    }
  else
    {
      putreg32(1u << (pin + 16), portbase + STM32H7RS_GPIO_BSRR_OFFSET);
    }
}

static void stm32h7s78_gpio_output(uintptr_t portbase, uint32_t pin,
                                   bool value, uint32_t pupd)
{
  uint32_t regval;

  stm32h7s78_gpio_write(portbase, pin, value);

  regval = getreg32(portbase + STM32H7RS_GPIO_MODER_OFFSET);
  regval &= ~GPIO_MODE_MASK(pin);
  regval |= GPIO_MODE_OUTPUT << GPIO_MODE_SHIFT(pin);
  putreg32(regval, portbase + STM32H7RS_GPIO_MODER_OFFSET);

  modifyreg32(portbase + STM32H7RS_GPIO_OTYPER_OFFSET, 1u << pin, 0);

  regval = getreg32(portbase + STM32H7RS_GPIO_OSPEEDR_OFFSET);
  regval &= ~GPIO_SPEED_MASK(pin);
  regval |= GPIO_SPEED_HIGH << GPIO_SPEED_SHIFT(pin);
  putreg32(regval, portbase + STM32H7RS_GPIO_OSPEEDR_OFFSET);

  regval = getreg32(portbase + STM32H7RS_GPIO_PUPDR_OFFSET);
  regval &= ~GPIO_PUPD_MASK(pin);
  regval |= pupd << GPIO_PUPD_SHIFT(pin);
  putreg32(regval, portbase + STM32H7RS_GPIO_PUPDR_OFFSET);
}

static void stm32h7s78_gpio_alt(uintptr_t portbase, uint32_t pin,
                                uint32_t af)
{
  uintptr_t afr;
  uint32_t regval;

  regval = getreg32(portbase + STM32H7RS_GPIO_MODER_OFFSET);
  regval &= ~GPIO_MODE_MASK(pin);
  regval |= GPIO_MODE_ALT << GPIO_MODE_SHIFT(pin);
  putreg32(regval, portbase + STM32H7RS_GPIO_MODER_OFFSET);

  modifyreg32(portbase + STM32H7RS_GPIO_OTYPER_OFFSET, 1u << pin, 0);

  regval = getreg32(portbase + STM32H7RS_GPIO_OSPEEDR_OFFSET);
  regval &= ~GPIO_SPEED_MASK(pin);
  regval |= GPIO_SPEED_HIGH << GPIO_SPEED_SHIFT(pin);
  putreg32(regval, portbase + STM32H7RS_GPIO_OSPEEDR_OFFSET);

  regval = getreg32(portbase + STM32H7RS_GPIO_PUPDR_OFFSET);
  regval &= ~GPIO_PUPD_MASK(pin);
  putreg32(regval, portbase + STM32H7RS_GPIO_PUPDR_OFFSET);

  afr = portbase + (pin < 8 ? STM32H7RS_GPIO_AFRL_OFFSET :
                              STM32H7RS_GPIO_AFRH_OFFSET);
  regval = getreg32(afr);
  regval &= ~GPIO_AFR_MASK(pin);
  regval |= af << GPIO_AFR_SHIFT(pin);
  putreg32(regval, afr);
}

static int stm32h7s78_lptim1_wait(uint32_t flag)
{
  uint32_t timeout = LCD_LPTIM_TIMEOUT;

  while ((getreg32(STM32H7RS_LPTIM1_ISR) & flag) == 0)
    {
      if (timeout-- == 0)
        {
          return -ETIMEDOUT;
        }
    }

  return OK;
}

static uint32_t stm32h7s78_lcd_backlight_pulse(uint32_t brightness)
{
  if (brightness > 100u)
    {
      brightness = 100u;
    }

  if (brightness == 0)
    {
      return 0;
    }

  return (((LCD_LPTIM_PERIOD + 1u) * brightness) / 100u) - 1u;
}

static void stm32h7s78_lptim1_clock_enable(void)
{
  modifyreg32(STM32H7RS_RCC_CCIPR2, RCC_CCIPR2_LPTIM1SEL_MASK,
              RCC_CCIPR2_LPTIM1SEL_PCLK);
  modifyreg32(STM32H7RS_RCC_APB1ENR1, 0, RCC_APB1ENR1_LPTIM1EN);
  (void)getreg32(STM32H7RS_RCC_APB1ENR1);

  modifyreg32(STM32H7RS_RCC_APB1RSTR1, 0, RCC_APB1RSTR1_LPTIM1RST);
  modifyreg32(STM32H7RS_RCC_APB1RSTR1, RCC_APB1RSTR1_LPTIM1RST, 0);
}

static int stm32h7s78_lcd_backlight_start(uint32_t brightness)
{
  uint32_t pulse = stm32h7s78_lcd_backlight_pulse(brightness);
  int ret;

  if (g_lcd_backlight_on)
    {
      return OK;
    }

  stm32h7s78_gpio_alt(STM32H7RS_GPIOG_BASE, LCD_BACKLIGHT_PIN,
                      GPIO_AF_LPTIM1);
  stm32h7s78_lptim1_clock_enable();

  putreg32(0, STM32H7RS_LPTIM1_CR);
  putreg32(0, STM32H7RS_LPTIM1_CFGR);
  putreg32(0, STM32H7RS_LPTIM1_CFGR2);
  putreg32(0, STM32H7RS_LPTIM1_DIER);
  putreg32(0, STM32H7RS_LPTIM1_CCMR1);
  putreg32(LPTIM_ICR_REPOKCF | LPTIM_ICR_ARROKCF | LPTIM_ICR_CMP2OKCF,
           STM32H7RS_LPTIM1_ICR);

  putreg32(LPTIM_CR_ENABLE, STM32H7RS_LPTIM1_CR);

  putreg32(LPTIM_ICR_REPOKCF, STM32H7RS_LPTIM1_ICR);
  putreg32(0, STM32H7RS_LPTIM1_RCR);
  ret = stm32h7s78_lptim1_wait(LPTIM_ISR_REPOK);
  if (ret < 0)
    {
      goto fail;
    }

  putreg32(LPTIM_ICR_ARROKCF, STM32H7RS_LPTIM1_ICR);
  putreg32(LCD_LPTIM_PERIOD, STM32H7RS_LPTIM1_ARR);
  ret = stm32h7s78_lptim1_wait(LPTIM_ISR_ARROK);
  if (ret < 0)
    {
      goto fail;
    }

  putreg32(0, STM32H7RS_LPTIM1_CR);
  putreg32(0, STM32H7RS_LPTIM1_CFGR);
  putreg32(0, STM32H7RS_LPTIM1_CFGR2);

  putreg32(LPTIM_CR_ENABLE, STM32H7RS_LPTIM1_CR);
  putreg32(LPTIM_ICR_CMP2OKCF, STM32H7RS_LPTIM1_ICR);
  putreg32(pulse, STM32H7RS_LPTIM1_CCR2);
  ret = stm32h7s78_lptim1_wait(LPTIM_ISR_CMP2OK);
  if (ret < 0)
    {
      goto fail;
    }

  putreg32(0, STM32H7RS_LPTIM1_CR);
  modifyreg32(STM32H7RS_LPTIM1_CCMR1,
              LPTIM_CCMR1_CC2SEL | LPTIM_CCMR1_CC2P_MASK,
              LPTIM_CCMR1_CC2P_LOW);
  modifyreg32(STM32H7RS_LPTIM1_CFGR, LPTIM_CFGR_WAVE, 0);

  putreg32(LPTIM_CR_ENABLE, STM32H7RS_LPTIM1_CR);
  modifyreg32(STM32H7RS_LPTIM1_CCMR1, 0, LPTIM_CCMR1_CC2E);
  modifyreg32(STM32H7RS_LPTIM1_CR, 0, LPTIM_CR_CNTSTRT);

  g_lcd_backlight_on = true;
  return OK;

fail:
  putreg32(0, STM32H7RS_LPTIM1_CR);
  modifyreg32(STM32H7RS_RCC_APB1ENR1, RCC_APB1ENR1_LPTIM1EN, 0);
  stm32h7s78_gpio_output(STM32H7RS_GPIOG_BASE, LCD_BACKLIGHT_PIN, true,
                         GPIO_FLOAT);
  return ret;
}

static void stm32h7s78_lcd_backlight_stop(void)
{
  if (g_lcd_backlight_on)
    {
      modifyreg32(STM32H7RS_LPTIM1_CCMR1, LPTIM_CCMR1_CC2E, 0);
      putreg32(0, STM32H7RS_LPTIM1_CR);
      g_lcd_backlight_on = false;
    }

  modifyreg32(STM32H7RS_RCC_APB1ENR1, RCC_APB1ENR1_LPTIM1EN, 0);

  /* The DK backlight is driven by Cube with low output polarity.  Keep the
   * pin high while the PWM is off.
   */

  stm32h7s78_gpio_output(STM32H7RS_GPIOG_BASE, LCD_BACKLIGHT_PIN, true,
                         GPIO_FLOAT);
}

static int stm32h7s78_lcd_panel_config(void)
{
  modifyreg32(STM32H7RS_RCC_AHB4ENR, 0,
              RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOGEN);
  (void)getreg32(STM32H7RS_RCC_AHB4ENR);

  stm32h7s78_gpio_output(STM32H7RS_GPIOE_BASE, LCD_DISP_EN_PIN, true,
                         GPIO_PULLUP);
  stm32h7s78_lcd_backlight_stop();
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void stm32h7rs_ltdcpower(bool on)
{
  if (on)
    {
      int ret;

      stm32h7s78_gpio_output(STM32H7RS_GPIOE_BASE, LCD_DISP_EN_PIN, true,
                             GPIO_PULLUP);

      ret = stm32h7s78_lcd_backlight_start(LCD_BACKLIGHT_BRIGHTNESS);
      if (ret < 0)
        {
          syslog(LOG_WARNING,
                 "stm32h7s78-dk: LCD backlight PWM failed: %d; "
                 "falling back to static PG15 low\n",
                 ret);
          stm32h7s78_gpio_output(STM32H7RS_GPIOG_BASE, LCD_BACKLIGHT_PIN,
                                 false, GPIO_FLOAT);
        }
    }
  else
    {
      stm32h7s78_lcd_backlight_stop();
      stm32h7s78_gpio_output(STM32H7RS_GPIOE_BASE, LCD_DISP_EN_PIN, false,
                             GPIO_PULLUP);
    }
}

int up_fbinitialize(int display)
{
  int ret;

  if (display != 0)
    {
      return -EINVAL;
    }

  if (g_lcd_initialized)
    {
      return OK;
    }

  ret = stm32h7s78_xspi1_psram_initialize();
  if (ret < 0)
    {
      return ret;
    }

  /* LVGL's NuttX fbdev driver mmaps /dev/fb0 from user space.  The
   * framebuffer sits before the configured PSRAM heap, so map it explicitly.
   */

  stm32h7rs_mpu_uheap(BOARD_LTDC_FB_BASE, BOARD_LTDC_FB_SIZE);

  ret = stm32h7s78_lcd_panel_config();
  if (ret < 0)
    {
      return ret;
    }

  up_mdelay(LCD_POWER_DELAY_MS);

  ret = stm32h7rs_ltdcinitialize();
  if (ret < 0)
    {
      return ret;
    }

  stm32h7rs_ltdcpower(true);

  g_lcd_initialized = true;
  syslog(LOG_INFO,
         "stm32h7s78-dk: LCD /dev/fb0 800x480 RGB565 "
         "fb=psram@0x%08" PRIx32 " size=%" PRIu32
         " backlight=lptim1-ch2\n",
         (uint32_t)BOARD_LTDC_FB_BASE, (uint32_t)BOARD_LTDC_FB_SIZE);
  return OK;
}

FAR struct fb_vtable_s *up_fbgetvplane(int display, int vplane)
{
  if (display != 0)
    {
      return NULL;
    }

  return stm32h7rs_ltdcgetvplane(vplane);
}

void up_fbuninitialize(int display)
{
  if (display == 0 && g_lcd_initialized)
    {
      stm32h7rs_ltdcuninitialize();
      g_lcd_initialized = false;
    }
}

#endif /* CONFIG_STM32H7S78_DK_LCD */
