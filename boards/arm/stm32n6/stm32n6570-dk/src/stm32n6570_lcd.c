/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32n6570_lcd.c
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
#include <nuttx/cache.h>
#include <nuttx/video/fb.h>

#include "arm_internal.h"

#include "hardware/stm32n6_memorymap.h"
#include "stm32n6_gpio.h"
#include "stm32n6_ltdc.h"
#include "stm32n6_mpuinit.h"

#include "stm32n6570-dk.h"

#ifdef CONFIG_STM32N6570_DK_LCD

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LCD_POWER_DELAY_MS      20u

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_lcd_initialized;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32n6570_lcd_panel_config(void)
{
  int ret;

  ret = stm32n6_configgpio(GPIO_LCD_DISP_NRST);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32n6_configgpio(GPIO_LCD_DISP_ONOFF);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32n6_configgpio(GPIO_LCD_DISP_BL);
  if (ret < 0)
    {
      return ret;
    }

  return stm32n6_configgpio(GPIO_LCD_DISP_EN);
}

#ifdef CONFIG_STM32N6570_DK_LCD_COLORBAR
static void stm32n6570_lcd_colorbar(void)
{
  static const uint16_t colors[8] =
  {
    0xf800, 0x07e0, 0x001f, 0xffe0, 0xf81f, 0x07ff, 0xffff, 0x0000
  };

  FAR volatile uint16_t *fb = (FAR volatile uint16_t *)BOARD_LTDC_FB_BASE;
  unsigned int page;
  unsigned int x;
  unsigned int y;

  for (page = 0; page < BOARD_LTDC_FB_COUNT; page++)
    {
      FAR volatile uint16_t *pagefb =
        fb + page * (BOARD_LTDC_FRAME_SIZE / sizeof(uint16_t));

      for (y = 0; y < BOARD_LTDC_HEIGHT; y++)
        {
          for (x = 0; x < BOARD_LTDC_WIDTH; x++)
            {
              pagefb[y * BOARD_LTDC_WIDTH + x] =
                colors[(x * 8u) / BOARD_LTDC_WIDTH];
            }
        }
    }

  up_clean_dcache(BOARD_LTDC_FB_BASE,
                  BOARD_LTDC_FB_BASE + BOARD_LTDC_FB_SIZE);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void stm32n6_ltdcpower(bool on)
{
  stm32n6_gpiowrite(GPIO_LCD_DISP_EN, on);
  stm32n6_gpiowrite(GPIO_LCD_DISP_BL, on);
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

  ret = stm32n6570_xspi1_psram_map();
  if (ret < 0)
    {
      return ret;
    }

  /* LVGL uses mmap() on /dev/fb0 from user space.  The framebuffer sits
   * before the configured PSRAM heap, so map it explicitly.
   */

  stm32n6_mpu_uheap(BOARD_LTDC_FB_BASE, BOARD_LTDC_FB_SIZE);

  ret = stm32n6570_lcd_panel_config();
  if (ret < 0)
    {
      return ret;
    }

  up_mdelay(LCD_POWER_DELAY_MS);

  ret = stm32n6_ltdcinitialize();
  if (ret < 0)
    {
      return ret;
    }

  stm32n6_ltdcpower(true);

#ifdef CONFIG_STM32N6570_DK_LCD_COLORBAR
  stm32n6570_lcd_colorbar();
#endif

  g_lcd_initialized = true;
  syslog(LOG_INFO,
         "stm32n6570-dk: LCD /dev/fb0 800x480 RGB565 "
         "fb=%s@0x%08" PRIx32 " size=%" PRIu32
         " ltdc-sync=global-vbr-irq-v1\n",
         "psram", (uint32_t)BOARD_LTDC_FB_BASE,
         (uint32_t)BOARD_LTDC_FB_SIZE);
  return OK;
}

FAR struct fb_vtable_s *up_fbgetvplane(int display, int vplane)
{
  if (display != 0)
    {
      return NULL;
    }

  return stm32n6_ltdcgetvplane(vplane);
}

void up_fbuninitialize(int display)
{
  if (display == 0 && g_lcd_initialized)
    {
      stm32n6_ltdcuninitialize();
      g_lcd_initialized = false;
    }
}

#endif /* CONFIG_STM32N6570_DK_LCD */
