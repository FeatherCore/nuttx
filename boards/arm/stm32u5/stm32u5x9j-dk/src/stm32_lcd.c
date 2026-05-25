/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32_lcd.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <sys/param.h>
#include <syslog.h>

#include <nuttx/arch.h>
#include <nuttx/cache.h>
#include <nuttx/lcd/hx8379c.h>
#include <nuttx/video/fb.h>

#include "stm32_gpio.h"
#include "stm32_dma2d.h"
#include "stm32_dsi.h"
#include "stm32_gfxmmu.h"
#include "stm32_ltdc.h"
#include "stm32_mpuinit.h"
#include "stm32u5x9j-dk.h"

#ifdef CONFIG_STM32U5X9J_DK_LCD

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32U5X9J_LCD_XRES      480
#define STM32U5X9J_LCD_YRES      480
#define STM32U5X9J_LCD_VACT      481
#ifdef CONFIG_STM32U5X9J_DK_LCD_XRGB8888
#  define STM32U5X9J_LCD_BPP     32
#  define STM32U5X9J_LCD_FORMAT_NAME "XRGB8888"
#else
#  define STM32U5X9J_LCD_BPP     16
#  define STM32U5X9J_LCD_FORMAT_NAME "RGB565"
#endif
#define STM32U5X9J_LCD_DSI_BPP     32
#define STM32U5X9J_LCD_DSI_FORMAT_NAME "RGB888"
#define STM32U5X9J_LCD_PANEL_COLMOD HX8379C_PIXEL_FORMAT_RGB888
#define STM32U5X9J_LCD_BYTESPP   (STM32U5X9J_LCD_BPP / 8)
#define STM32U5X9J_LCD_PHYS_STRIDE (STM32U5X9J_LCD_XRES * \
                                    STM32U5X9J_LCD_BYTESPP)

#ifdef CONFIG_STM32U5X9J_DK_LCD_FB_SRAM
#  define STM32U5X9J_LCD_USE_GFXMMU 1
#endif

/* GFXMMU is useful when the framebuffer is carved from scarce internal SRAM,
 * because it packs the visible 480x480 circular panel storage.  It is not used
 * for PSRAM framebuffer mode: direct linear PSRAM scanout avoids the extra
 * translation latency that caused LTDC FIFO underruns during LVGL refresh.
 */

#ifdef STM32U5X9J_LCD_USE_GFXMMU
#  define STM32U5X9J_LCD_GFXMMU_BLOCKS_PER_LINE 192
#  define STM32U5X9J_LCD_GFXMMU_BLOCK_BYTES 16
#  define STM32U5X9J_LCD_LAYER_STRIDE \
          (STM32U5X9J_LCD_GFXMMU_BLOCKS_PER_LINE * \
           STM32U5X9J_LCD_GFXMMU_BLOCK_BYTES)
#  define STM32U5X9J_LCD_VFBLEN       (STM32U5X9J_LCD_LAYER_STRIDE * \
                                       STM32U5X9J_LCD_YRES)
#  define STM32U5X9J_LCD_STORAGE_FBLEN \
          STM32U5X9J_LCD_GFXMMU_FRAME_SIZE
#  define STM32U5X9J_LCD_FB_MAP_NAME  "sram-gfxmmu"
#else
#  define STM32U5X9J_LCD_LAYER_STRIDE STM32U5X9J_LCD_PHYS_STRIDE
#  define STM32U5X9J_LCD_VFBLEN       STM32U5X9J_LCD_FBLEN
#  define STM32U5X9J_LCD_STORAGE_FBLEN STM32U5X9J_LCD_FBLEN
#  define STM32U5X9J_LCD_FB_MAP_NAME  "direct"
#endif

#define STM32U5X9J_LCD_LAYER_PIXELS \
  (STM32U5X9J_LCD_LAYER_STRIDE / STM32U5X9J_LCD_BYTESPP)
#define STM32U5X9J_LCD_LAYER_Y0  1
#define STM32U5X9J_LCD_FBLEN     (STM32U5X9J_LCD_PHYS_STRIDE * \
                                  STM32U5X9J_LCD_YRES)

#define STM32U5X9J_LCD_FB_SIZE   (STM32U5X9J_LCD_STORAGE_FBLEN * \
                                  STM32U5X9J_LCD_FB_COUNT)

#ifdef STM32U5X9J_LCD_USE_GFXMMU
#  define STM32U5X9J_LCD_FB_PHYS \
          ((uintptr_t)STM32U5X9J_INTERNAL_SRAM_FB_BASE)
#  define STM32U5X9J_LCD_FB      ((uintptr_t)stm32_gfxmmu_buffer0())
#  define STM32U5X9J_LCD_FB1     ((uintptr_t)stm32_gfxmmu_buffer(1))
#else
#  define STM32U5X9J_LCD_FB_PHYS \
          ((uintptr_t)STM32U5X9J_HSPI1_PSRAM_MEM_BASE)
#  define STM32U5X9J_LCD_FB      STM32U5X9J_LCD_FB_PHYS
#  define STM32U5X9J_LCD_FB1     (STM32U5X9J_LCD_FB_PHYS + \
                                  STM32U5X9J_LCD_STORAGE_FBLEN)
#endif

#ifdef CONFIG_STM32U5X9J_DK_LCD_FB_PSRAM
#if STM32U5X9J_LCD_FB_SIZE > STM32U5X9J_HSPI1_PSRAM_FB_RESERVED_SIZE
#  error STM32U5x9J-DK PSRAM framebuffer reserve is too small
#endif
#endif

#ifdef CONFIG_STM32U5X9J_DK_LCD_FB_SRAM
#if STM32U5X9J_LCD_FB_SIZE > STM32U5X9J_INTERNAL_SRAM_FB_SIZE
#  error STM32U5x9J-DK internal SRAM framebuffer reserve is too small
#endif
#endif

#define STM32U5X9J_LCD_VSYNC     1
#define STM32U5X9J_LCD_VBP       12
#define STM32U5X9J_LCD_VFP       50
#define STM32U5X9J_LCD_HSYNC     2
#define STM32U5X9J_LCD_HBP       1
#define STM32U5X9J_LCD_HFP       1

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if STM32U5X9J_LCD_BPP == 16
typedef uint16_t stm32_lcd_pixel_t;
#else
typedef uint32_t stm32_lcd_pixel_t;
#endif

static const struct stm32_dsi_video_config_s g_dsi_config =
{
  .width   = STM32U5X9J_LCD_XRES,
  .hsync   = STM32U5X9J_LCD_HSYNC,
  .hbp     = STM32U5X9J_LCD_HBP,
  .hfp     = STM32U5X9J_LCD_HFP,
  .vsync   = STM32U5X9J_LCD_VSYNC,
  .vbp     = STM32U5X9J_LCD_VBP,
  .vfp     = STM32U5X9J_LCD_VFP,
  .vactive = STM32U5X9J_LCD_VACT,
  .lanes   = 2,
  .bpp     = STM32U5X9J_LCD_DSI_BPP,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32_lcd_prepare_panel(void)
{
  stm32_configgpio(GPIO_LCD_DSI_POWER);
  stm32_configgpio(GPIO_LCD_RESET);
  stm32_configgpio(GPIO_LCD_BACKLIGHT);

  stm32_gpiowrite(GPIO_LCD_DSI_POWER, true);
  stm32_gpiowrite(GPIO_LCD_RESET, false);
}

static void stm32_lcd_release_panel(void)
{
  up_mdelay(11);
  stm32_gpiowrite(GPIO_LCD_RESET, true);
  up_mdelay(150);
}

static stm32_lcd_pixel_t stm32_lcd_color(uint32_t color)
{
#if STM32U5X9J_LCD_BPP == 16
  uint32_t r = (color >> 16) & 0xff;
  uint32_t g = (color >> 8) & 0xff;
  uint32_t b = color & 0xff;

  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
#else
  return color;
#endif
}

static uintptr_t stm32_lcd_fb(uint32_t frame)
{
#ifdef STM32U5X9J_LCD_USE_GFXMMU
  return (uintptr_t)stm32_gfxmmu_buffer(frame);
#else
  return STM32U5X9J_LCD_FB_PHYS +
         (uintptr_t)frame * STM32U5X9J_LCD_STORAGE_FBLEN;
#endif
}

static void stm32_lcd_clean_framebuffer(void)
{
  uint32_t i;

  for (i = 0; i < STM32U5X9J_LCD_FB_COUNT; i++)
    {
      up_clean_dcache(stm32_lcd_fb(i),
                      stm32_lcd_fb(i) + STM32U5X9J_LCD_VFBLEN);
    }

#ifdef STM32U5X9J_LCD_USE_GFXMMU
  up_clean_dcache(STM32U5X9J_LCD_FB_PHYS,
                  STM32U5X9J_LCD_FB_PHYS + STM32U5X9J_LCD_FB_SIZE);
#endif
}

#if defined(CONFIG_STM32U5X9J_DK_LCD_PATTERN) || \
    !defined(CONFIG_STM32U5X9J_DK_LCD_COLORBAR)
static void stm32_lcd_fill_frame(uintptr_t fbaddr, uint32_t color)
{
  FAR stm32_lcd_pixel_t *fb =
    (FAR stm32_lcd_pixel_t *)fbaddr;
  stm32_lcd_pixel_t pixel = stm32_lcd_color(color);
  uint32_t x;
  uint32_t y;

  for (y = 0; y < STM32U5X9J_LCD_YRES; y++)
    {
      for (x = 0; x < STM32U5X9J_LCD_XRES; x++)
        {
          fb[y * STM32U5X9J_LCD_LAYER_PIXELS + x] = pixel;
        }
    }
}
#endif

#if !defined(CONFIG_STM32U5X9J_DK_LCD_COLORBAR) && \
    !defined(CONFIG_STM32U5X9J_DK_LCD_PATTERN)
static void stm32_lcd_clear_framebuffer(void)
{
  uint32_t i;

  for (i = 0; i < STM32U5X9J_LCD_FB_COUNT; i++)
    {
      stm32_lcd_fill_frame(stm32_lcd_fb(i),
                                0xff000000);
    }

  stm32_lcd_clean_framebuffer();
}
#endif

#if defined(CONFIG_STM32U5X9J_DK_LCD_COLORBAR) || \
    defined(CONFIG_STM32U5X9J_DK_LCD_PATTERN)
#ifdef CONFIG_STM32U5X9J_DK_LCD_PATTERN
static void stm32_lcd_fill_cpu(uint32_t color)
{
  stm32_lcd_fill_frame(STM32U5X9J_LCD_FB, color);
}

static void stm32_lcd_rect_cpu(uint32_t x0, uint32_t y0,
                                    uint32_t width, uint32_t height,
                                    uint32_t color)
{
  FAR stm32_lcd_pixel_t *fb =
    (FAR stm32_lcd_pixel_t *)STM32U5X9J_LCD_FB;
  stm32_lcd_pixel_t pixel = stm32_lcd_color(color);
  uint32_t x1 = MIN(x0 + width, STM32U5X9J_LCD_XRES);
  uint32_t y1 = MIN(y0 + height, STM32U5X9J_LCD_YRES);
  uint32_t x;
  uint32_t y;

  for (y = y0; y < y1; y++)
    {
      for (x = x0; x < x1; x++)
        {
          fb[y * STM32U5X9J_LCD_LAYER_PIXELS + x] = pixel;
        }
    }
}

static void stm32_lcd_solidfill(uint32_t color)
{
#if STM32U5X9J_LCD_BPP == 32
  int ret;

  ret = stm32_dma2dfill(STM32U5X9J_LCD_FB, color,
                        STM32U5X9J_LCD_XRES, STM32U5X9J_LCD_YRES,
                        STM32U5X9J_LCD_LAYER_PIXELS);
  if (ret < 0)
    {
      stm32_lcd_fill_cpu(color);
    }
#else
  stm32_lcd_fill_cpu(color);
#endif
}

static void stm32_lcd_rgbrect(void)
{
  stm32_lcd_rect_cpu(0, 0, 160, 160, 0xffff0000);
  stm32_lcd_rect_cpu(160, 160, 160, 160, 0xff00ff00);
  stm32_lcd_rect_cpu(320, 320, 160, 160, 0xff0000ff);
}
#endif

static void stm32_lcd_colorbar_frame(uintptr_t fbaddr)
{
  static const uint32_t colors[8] =
    {
      0xffffffff, 0xffffff00, 0xff00ffff, 0xff00ff00,
      0xffff00ff, 0xffff0000, 0xff0000ff, 0xff000000
    };
  FAR stm32_lcd_pixel_t *fb =
    (FAR stm32_lcd_pixel_t *)fbaddr;
  uint32_t x;
  uint32_t y;

  for (y = 0; y < STM32U5X9J_LCD_YRES; y++)
    {
      for (x = 0; x < STM32U5X9J_LCD_XRES; x++)
        {
          fb[y * STM32U5X9J_LCD_LAYER_PIXELS + x] =
            stm32_lcd_color(colors[(x * nitems(colors)) /
                                 STM32U5X9J_LCD_XRES]);
        }
    }
}

static void stm32_lcd_colorbar(void)
{
  uint32_t i;

  for (i = 0; i < STM32U5X9J_LCD_FB_COUNT; i++)
    {
      stm32_lcd_colorbar_frame(stm32_lcd_fb(i));
    }

  syslog(LOG_INFO, "stm32u5x9j: LCD framebuffer colorbar done\n");
}

#ifdef CONFIG_STM32U5X9J_DK_LCD_PATTERN
static void stm32_lcd_pattern(void)
{
  stm32_lcd_solidfill(0xff000000);
  stm32_lcd_colorbar();
  stm32_lcd_rgbrect();
  stm32_lcd_clean_framebuffer();
  syslog(LOG_INFO, "stm32u5x9j: LCD static pattern done\n");
}
#endif /* CONFIG_STM32U5X9J_DK_LCD_PATTERN */
#endif

static int stm32_lcd_panel_short_write(uint8_t datatype,
                                            uint8_t command,
                                            uint8_t param)
{
  return stm32_dsishortwrite(datatype, command, param);
}

static int stm32_lcd_panel_long_write(uint8_t command,
                                           FAR const uint8_t *data,
                                           size_t len)
{
  return stm32_dsilongwrite(command, data, len);
}

static void stm32_lcd_panel_delay(unsigned int delay_ms)
{
  up_mdelay(delay_ms);
}

static int stm32_lcd_panel_initialize(void)
{
  static const struct hx8379c_dsi_ops_s ops =
    {
      .short_write = stm32_lcd_panel_short_write,
      .long_write  = stm32_lcd_panel_long_write,
      .delay_ms    = stm32_lcd_panel_delay,
      .pixel_format = STM32U5X9J_LCD_PANEL_COLMOD,
    };
  int ret;

  ret = hx8379c_initialize(&ops);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: LCD panel init failed: %d\n", ret);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_lcd_initialize(void)
{
#ifdef STM32U5X9J_LCD_USE_GFXMMU
  struct stm32_gfxmmu_config_s gfxmmu;
#endif
  struct stm32_ltdc_config_s ltdc;
  struct fb_vtable_s *vtable;
#ifdef STM32U5X9J_LCD_USE_GFXMMU
  uint32_t i;
#endif
  int ret;

#ifdef CONFIG_STM32U5X9J_DK_HSPI_RAM
  ret = stm32_hspi1_psram_initialize();
  if (ret < 0)
    {
      return ret;
    }
#else
#  ifdef CONFIG_STM32U5X9J_DK_LCD_FB_PSRAM
  return -ENODEV;
#  endif
#endif

  stm32_lcd_prepare_panel();

  syslog(LOG_INFO,
         "stm32u5x9j: LCD fb-map=%s fb-format=%s fb-bpp=%u dsi-format=%s "
         "dsi-bpp=%u panel-colmod=0x%02x phys-stride=%u "
         "layer-stride=%u fb-count=%u\n",
         STM32U5X9J_LCD_FB_MAP_NAME, STM32U5X9J_LCD_FORMAT_NAME,
         (unsigned int)STM32U5X9J_LCD_BPP,
         STM32U5X9J_LCD_DSI_FORMAT_NAME,
         (unsigned int)STM32U5X9J_LCD_DSI_BPP,
         (unsigned int)STM32U5X9J_LCD_PANEL_COLMOD,
         (unsigned int)STM32U5X9J_LCD_PHYS_STRIDE,
         (unsigned int)STM32U5X9J_LCD_LAYER_STRIDE,
         (unsigned int)STM32U5X9J_LCD_FB_COUNT);

  ret = stm32_dma2dinitialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: DMA2D init failed: %d\n", ret);
      return ret;
    }

#ifdef STM32U5X9J_LCD_USE_GFXMMU
  memset(&gfxmmu, 0, sizeof(gfxmmu));
  gfxmmu.physical   = STM32U5X9J_LCD_FB_PHYS;
  gfxmmu.width      = STM32U5X9J_LCD_XRES;
  gfxmmu.height     = STM32U5X9J_LCD_YRES;
  gfxmmu.stride     = STM32U5X9J_LCD_PHYS_STRIDE;
  gfxmmu.frame_size = STM32U5X9J_LCD_STORAGE_FBLEN;
  gfxmmu.bpp        = STM32U5X9J_LCD_BPP;
  gfxmmu.frames     = STM32U5X9J_LCD_FB_COUNT;
  gfxmmu.circular   = true;

  ret = stm32_gfxmmuinitialize(&gfxmmu);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: GFXMMU init failed: %d\n", ret);
      return ret;
    }
#endif

  ret = stm32_dsiinitialize(&g_dsi_config);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: DSI host init failed: %d\n", ret);
      return ret;
    }

  stm32_lcd_release_panel();

  memset(&ltdc, 0, sizeof(ltdc));
  ltdc.fb_base       = STM32U5X9J_LCD_FB;
  ltdc.fb1_base      = STM32U5X9J_LCD_FB1;
#ifdef CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER
  ltdc.layer_fb_base = STM32U5X9J_LCD_FB1;
#else
  ltdc.layer_fb_base = STM32U5X9J_LCD_FB;
#endif
  ltdc.fb_size       = STM32U5X9J_LCD_VFBLEN * STM32U5X9J_LCD_FB_COUNT;
  ltdc.xres          = STM32U5X9J_LCD_XRES;
  ltdc.yres          = STM32U5X9J_LCD_YRES;
  ltdc.stride        = STM32U5X9J_LCD_LAYER_STRIDE;
  ltdc.layer_stride  = STM32U5X9J_LCD_LAYER_STRIDE;
  ltdc.window_y0     = STM32U5X9J_LCD_LAYER_Y0;
  ltdc.hsync         = STM32U5X9J_LCD_HSYNC;
  ltdc.hbp           = STM32U5X9J_LCD_HBP;
  ltdc.hfp           = STM32U5X9J_LCD_HFP;
  ltdc.vsync         = STM32U5X9J_LCD_VSYNC;
  ltdc.vbp           = STM32U5X9J_LCD_VBP;
  ltdc.vfp           = STM32U5X9J_LCD_VFP;
  ltdc.bpp           = STM32U5X9J_LCD_BPP;

  ret = stm32_ltdcinitialize(&ltdc);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: LTDC init failed: %d\n", ret);
      return ret;
    }

#ifndef STM32U5X9J_LCD_USE_GFXMMU
  stm32_mpu_ufbmem(STM32U5X9J_LCD_FB_PHYS, STM32U5X9J_LCD_FB_SIZE);
#else
  for (i = 0; i < STM32U5X9J_LCD_FB_COUNT; i++)
    {
      stm32_mpu_ufbmem(stm32_lcd_fb(i), STM32U5X9J_LCD_VFBLEN);
    }
#endif

  ret = stm32_dsistart();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: DSI start failed: %d\n", ret);
      return ret;
    }

  ret = stm32_lcd_panel_initialize();
  if (ret < 0)
    {
      return ret;
    }

#if !defined(CONFIG_STM32U5X9J_DK_LCD_COLORBAR) && \
    !defined(CONFIG_STM32U5X9J_DK_LCD_PATTERN)
  stm32_lcd_clear_framebuffer();
#endif

  ret = stm32_dsienableltdc();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: DSI LTDC enable failed: %d\n", ret);
      return ret;
    }

  vtable = stm32_ltdcgetvplane(0);
  if (vtable == NULL)
    {
      syslog(LOG_ERR, "ERROR: LTDC framebuffer plane missing\n");
      return -ENODEV;
    }

  ret = fb_register_device(0, 0, vtable);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fb_register_device failed: %d\n", ret);
      return ret;
    }

#ifdef CONFIG_STM32U5X9J_DK_LCD_PATTERN
  stm32_lcd_pattern();
#elif defined(CONFIG_STM32U5X9J_DK_LCD_COLORBAR)
  stm32_lcd_colorbar();
  stm32_lcd_clean_framebuffer();
#endif

  syslog(LOG_INFO, "stm32u5x9j: /dev/fb0 framebuffer virt=0x%08" PRIxPTR
         " virt1=0x%08" PRIxPTR " phys=0x%08" PRIxPTR
         " size=%u count=%u\n", STM32U5X9J_LCD_FB, STM32U5X9J_LCD_FB1,
         STM32U5X9J_LCD_FB_PHYS, STM32U5X9J_LCD_FB_SIZE,
         STM32U5X9J_LCD_FB_COUNT);
  return OK;
}

#endif /* CONFIG_STM32U5X9J_DK_LCD */
