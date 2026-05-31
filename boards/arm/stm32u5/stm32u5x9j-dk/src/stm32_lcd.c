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
#if defined(CONFIG_STM32U5X9J_DK_LCD_CO5300)
#include <nuttx/lcd/co5300.h>
#elif defined(CONFIG_STM32U5X9J_DK_LCD_ST7801)
#include <nuttx/lcd/st7801.h>
#else
#include <nuttx/lcd/hx8379c.h>
#endif
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

#if defined(CONFIG_STM32U5X9J_DK_LCD_CO5300) || \
    defined(CONFIG_STM32U5X9J_DK_LCD_ST7801)
#  define BOARD_LCD_EXTERNAL_AMOLED 1
#endif

#ifdef CONFIG_STM32U5X9J_DK_LCD_CO5300
#  define BOARD_LCD_XRES      466
#  define BOARD_LCD_YRES      466
#  define BOARD_LCD_VACT      466
#  define BOARD_LCD_DSI_LANES 1
#  define BOARD_LCD_DSI_BPP   16
#  define BOARD_LCD_DSI_BTA   1
#  define BOARD_LCD_DSI_LP_CMD 1
#  define BOARD_LCD_DSI_HSCALE 2
#  define BOARD_LCD_DSI_HSCALE_NUM 0
#  define BOARD_LCD_DSI_HSCALE_DEN 0
#  define BOARD_LCD_DSI_PIXEL_PLL_N 120
#  define BOARD_LCD_DSI_PIXEL_PLL_R 30
#  define BOARD_LCD_DSI_PHY_NDIV 100
#  define BOARD_LCD_DSI_PHY_FRANGE STM32_DSI_DPHY_FRANGE_390MHZ_450MHZ
#  define BOARD_LCD_DSI_PHY_SWAP STM32_DSI_PHY_SWAP_CLK
#  define BOARD_LCD_DSI_VIDEO_MODE STM32_DSI_VIDEO_MODE_BURST
#  define BOARD_LCD_DSI_FORMAT_NAME "RGB565"
#  define BOARD_LCD_PANEL_COLMOD CO5300_PIXEL_FORMAT_RGB565
#  define BOARD_LCD_PANEL_NAME "CO5300"
#  define BOARD_LCD_LAYER_Y0  0
#  define BOARD_LCD_VSYNC     4
#  define BOARD_LCD_VBP       12
#  define BOARD_LCD_VFP       18
#  define BOARD_LCD_HSYNC     4
#  define BOARD_LCD_HBP       32
#  define BOARD_LCD_HFP       32
#elif defined(CONFIG_STM32U5X9J_DK_LCD_ST7801)
#  define BOARD_LCD_XRES      410
#  define BOARD_LCD_YRES      502
#  define BOARD_LCD_VACT      502
#  define BOARD_LCD_DSI_LANES 1
#  define BOARD_LCD_DSI_BPP   16
#  define BOARD_LCD_DSI_BTA   1
#  define BOARD_LCD_DSI_LP_CMD 1
#  define BOARD_LCD_DSI_HSCALE 0
#  define BOARD_LCD_DSI_HSCALE_NUM 7
#  define BOARD_LCD_DSI_HSCALE_DEN 2
#  define BOARD_LCD_DSI_PIXEL_PLL_N 120
#  define BOARD_LCD_DSI_PIXEL_PLL_R 32
#  define BOARD_LCD_DSI_PHY_NDIV 105
#  define BOARD_LCD_DSI_PHY_FRANGE STM32_DSI_DPHY_FRANGE_390MHZ_450MHZ
#  define BOARD_LCD_DSI_PHY_SWAP STM32_DSI_PHY_SWAP_CLK
#  define BOARD_LCD_DSI_VIDEO_MODE STM32_DSI_VIDEO_MODE_SYNC_PULSES
#  define BOARD_LCD_DSI_FORMAT_NAME "RGB565"
#  define BOARD_LCD_PANEL_COLMOD ST7801_PIXEL_FORMAT_RGB565
#  define BOARD_LCD_PANEL_NAME "ST7801"
#  define BOARD_LCD_LAYER_Y0  0
#  define BOARD_LCD_VSYNC     4
#  define BOARD_LCD_VBP       20
#  define BOARD_LCD_VFP       20
#  define BOARD_LCD_HSYNC     4
#  define BOARD_LCD_HBP       20
#  define BOARD_LCD_HFP       20
#else
#  define BOARD_LCD_XRES      480
#  define BOARD_LCD_YRES      480
#  define BOARD_LCD_VACT      481
#  define BOARD_LCD_DSI_LANES 2
#  define BOARD_LCD_DSI_BPP   32
#  define BOARD_LCD_DSI_BTA   1
#  define BOARD_LCD_DSI_LP_CMD 0
#  define BOARD_LCD_DSI_HSCALE 0
#  define BOARD_LCD_DSI_HSCALE_NUM 0
#  define BOARD_LCD_DSI_HSCALE_DEN 0
#  define BOARD_LCD_DSI_PIXEL_PLL_N 0
#  define BOARD_LCD_DSI_PIXEL_PLL_R 0
#  define BOARD_LCD_DSI_PHY_NDIV 0
#  define BOARD_LCD_DSI_PHY_FRANGE 0
#  define BOARD_LCD_DSI_PHY_SWAP 0
#  define BOARD_LCD_DSI_VIDEO_MODE STM32_DSI_VIDEO_MODE_BURST
#  define BOARD_LCD_DSI_FORMAT_NAME "RGB888"
#  define BOARD_LCD_PANEL_COLMOD HX8379C_PIXEL_FORMAT_RGB888
#  define BOARD_LCD_PANEL_NAME "HX8379C"
#  define BOARD_LCD_LAYER_Y0  1
#  define BOARD_LCD_VSYNC     1
#  define BOARD_LCD_VBP       12
#  define BOARD_LCD_VFP       50
#  define BOARD_LCD_HSYNC     2
#  define BOARD_LCD_HBP       1
#  define BOARD_LCD_HFP       1
#endif

#ifdef CONFIG_STM32U5X9J_DK_LCD_XRGB8888
#  define BOARD_LCD_BPP     32
#  define BOARD_LCD_FORMAT_NAME "XRGB8888"
#else
#  define BOARD_LCD_BPP     16
#  define BOARD_LCD_FORMAT_NAME "RGB565"
#endif
#define BOARD_LCD_BYTESPP   (BOARD_LCD_BPP / 8)
#define BOARD_LCD_PHYS_STRIDE (BOARD_LCD_XRES * \
                                    BOARD_LCD_BYTESPP)

#ifdef CONFIG_STM32U5X9J_DK_LCD_FB_SRAM
#  define BOARD_LCD_USE_GFXMMU 1
#endif

/* GFXMMU is useful when the framebuffer is carved from scarce internal
 * SRAM, because it packs the visible 480x480 circular panel storage.  It is
 * not used for PSRAM framebuffer mode: direct linear PSRAM scanout avoids
 * the extra translation latency that caused LTDC FIFO underruns.
 */

#ifdef BOARD_LCD_USE_GFXMMU
#  define BOARD_LCD_GFXMMU_BLOCKS_PER_LINE 192
#  define BOARD_LCD_GFXMMU_BLOCK_BYTES 16
#  define BOARD_LCD_LAYER_STRIDE \
          (BOARD_LCD_GFXMMU_BLOCKS_PER_LINE * \
           BOARD_LCD_GFXMMU_BLOCK_BYTES)
#  define BOARD_LCD_VFBLEN       (BOARD_LCD_LAYER_STRIDE * \
                                       BOARD_LCD_YRES)
#  define BOARD_LCD_STORAGE_FBLEN \
          BOARD_LCD_GFXMMU_FRAME_SIZE
#  define BOARD_LCD_FB_MAP_NAME  "sram-gfxmmu"
#else
#  define BOARD_LCD_LAYER_STRIDE BOARD_LCD_PHYS_STRIDE
#  define BOARD_LCD_VFBLEN       BOARD_LCD_FBLEN
#  define BOARD_LCD_STORAGE_FBLEN BOARD_LCD_FBLEN
#  define BOARD_LCD_FB_MAP_NAME  "direct"
#endif

#define BOARD_LCD_LAYER_PIXELS \
  (BOARD_LCD_LAYER_STRIDE / BOARD_LCD_BYTESPP)
#define BOARD_LCD_FBLEN     (BOARD_LCD_PHYS_STRIDE * \
                                  BOARD_LCD_YRES)

#define BOARD_LCD_FB_SIZE   (BOARD_LCD_STORAGE_FBLEN * \
                                  BOARD_LCD_FB_COUNT)

#ifdef BOARD_LCD_USE_GFXMMU
#  define BOARD_LCD_FB_PHYS \
          ((uintptr_t)BOARD_INTERNAL_SRAM_FB_BASE)
#  define BOARD_LCD_FB      ((uintptr_t)stm32_gfxmmu_buffer0())
#  define BOARD_LCD_FB1     ((uintptr_t)stm32_gfxmmu_buffer(1))
#else
#  define BOARD_LCD_FB_PHYS \
          ((uintptr_t)BOARD_HSPI1_PSRAM_MEM_BASE)
#  define BOARD_LCD_FB      BOARD_LCD_FB_PHYS
#  define BOARD_LCD_FB1     (BOARD_LCD_FB_PHYS + \
                                  BOARD_LCD_STORAGE_FBLEN)
#endif

#ifdef CONFIG_STM32U5X9J_DK_LCD_FB_PSRAM
#if BOARD_LCD_FB_SIZE > BOARD_HSPI1_PSRAM_FB_RESERVED_SIZE
#  error STM32U5x9J-DK PSRAM framebuffer reserve is too small
#endif
#endif

#ifdef CONFIG_STM32U5X9J_DK_LCD_FB_SRAM
#if BOARD_LCD_FB_SIZE > BOARD_INTERNAL_SRAM_FB_SIZE
#  error STM32U5x9J-DK internal SRAM framebuffer reserve is too small
#endif
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if BOARD_LCD_BPP == 16
typedef uint16_t stm32_lcd_pixel_t;
#else
typedef uint32_t stm32_lcd_pixel_t;
#endif

static const struct stm32_dsi_video_config_s g_dsi_config =
{
  .width   = BOARD_LCD_XRES,
  .hsync   = BOARD_LCD_HSYNC,
  .hbp     = BOARD_LCD_HBP,
  .hfp     = BOARD_LCD_HFP,
  .vsync   = BOARD_LCD_VSYNC,
  .vbp     = BOARD_LCD_VBP,
  .vfp     = BOARD_LCD_VFP,
  .vactive = BOARD_LCD_VACT,
  .lanes   = BOARD_LCD_DSI_LANES,
  .bpp     = BOARD_LCD_DSI_BPP,
  .hscale  = BOARD_LCD_DSI_HSCALE,
  .hscale_num = BOARD_LCD_DSI_HSCALE_NUM,
  .hscale_den = BOARD_LCD_DSI_HSCALE_DEN,
  .pixel_pll_n = BOARD_LCD_DSI_PIXEL_PLL_N,
  .pixel_pll_r = BOARD_LCD_DSI_PIXEL_PLL_R,
  .phy_ndiv = BOARD_LCD_DSI_PHY_NDIV,
  .phy_frange = BOARD_LCD_DSI_PHY_FRANGE,
  .phy_swap = BOARD_LCD_DSI_PHY_SWAP,
  .video_mode = BOARD_LCD_DSI_VIDEO_MODE,
  .bta     = BOARD_LCD_DSI_BTA,
  .lp_cmd  = BOARD_LCD_DSI_LP_CMD,
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
#if BOARD_LCD_BPP == 16
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
#ifdef BOARD_LCD_USE_GFXMMU
  return (uintptr_t)stm32_gfxmmu_buffer(frame);
#else
  return BOARD_LCD_FB_PHYS +
         (uintptr_t)frame * BOARD_LCD_STORAGE_FBLEN;
#endif
}

static void stm32_lcd_clean_framebuffer(void)
{
  uint32_t i;

  for (i = 0; i < BOARD_LCD_FB_COUNT; i++)
    {
      up_clean_dcache(stm32_lcd_fb(i),
                      stm32_lcd_fb(i) + BOARD_LCD_VFBLEN);
    }

#ifdef BOARD_LCD_USE_GFXMMU
  up_clean_dcache(BOARD_LCD_FB_PHYS,
                  BOARD_LCD_FB_PHYS + BOARD_LCD_FB_SIZE);
#endif
}

#ifndef CONFIG_STM32U5X9J_DK_LCD_COLORBAR
static void stm32_lcd_fill_frame(uintptr_t fbaddr, uint32_t color)
{
  FAR stm32_lcd_pixel_t *fb =
    (FAR stm32_lcd_pixel_t *)fbaddr;
  stm32_lcd_pixel_t pixel = stm32_lcd_color(color);
  uint32_t x;
  uint32_t y;

  for (y = 0; y < BOARD_LCD_YRES; y++)
    {
      for (x = 0; x < BOARD_LCD_XRES; x++)
        {
          fb[y * BOARD_LCD_LAYER_PIXELS + x] = pixel;
        }
    }
}

static void stm32_lcd_clear_framebuffer(void)
{
  uint32_t i;

  for (i = 0; i < BOARD_LCD_FB_COUNT; i++)
    {
      stm32_lcd_fill_frame(stm32_lcd_fb(i),
                                0xff000000);
    }

  stm32_lcd_clean_framebuffer();
}
#endif

#ifdef CONFIG_STM32U5X9J_DK_LCD_COLORBAR
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

  for (y = 0; y < BOARD_LCD_YRES; y++)
    {
      for (x = 0; x < BOARD_LCD_XRES; x++)
        {
          fb[y * BOARD_LCD_LAYER_PIXELS + x] =
            stm32_lcd_color(colors[(x * nitems(colors)) /
                                 BOARD_LCD_XRES]);
        }
    }
}

static void stm32_lcd_colorbar(void)
{
  uint32_t i;

  for (i = 0; i < BOARD_LCD_FB_COUNT; i++)
    {
      stm32_lcd_colorbar_frame(stm32_lcd_fb(i));
    }

  syslog(LOG_INFO, "stm32u5x9j: LCD framebuffer colorbar done\n");
}
#endif /* CONFIG_STM32U5X9J_DK_LCD_COLORBAR */

static int stm32_lcd_panel_short_write(uint8_t datatype,
                                            uint8_t command,
                                            uint8_t param)
{
  int ret;

  ret = stm32_dsishortwrite(datatype, command, param);
  return ret;
}

#ifdef BOARD_LCD_EXTERNAL_AMOLED
static int stm32_lcd_panel_read(uint8_t datatype, uint8_t command,
                                FAR uint8_t *buffer, size_t len)
{
  return stm32_dsiread(datatype, command, buffer, len);
}
#endif

static int stm32_lcd_panel_long_write(uint8_t command,
                                           FAR const uint8_t *data,
                                           size_t len)
{
  int ret;

  ret = stm32_dsilongwrite(command, data, len);
  return ret;
}

static void stm32_lcd_panel_delay(unsigned int delay_ms)
{
  up_mdelay(delay_ms);
}

static int stm32_lcd_panel_initialize(void)
{
#ifdef CONFIG_STM32U5X9J_DK_LCD_CO5300
  static const struct co5300_dsi_ops_s ops =
    {
      .short_write  = stm32_lcd_panel_short_write,
      .read         = stm32_lcd_panel_read,
      .long_write   = stm32_lcd_panel_long_write,
      .delay_ms     = stm32_lcd_panel_delay,
      .pixel_format = BOARD_LCD_PANEL_COLMOD,
      .madctl       = CO5300_MADCTL_RGB,
      .brightness   = CO5300_BRIGHTNESS_MAX,
    };
#elif defined(CONFIG_STM32U5X9J_DK_LCD_ST7801)
  static const struct st7801_dsi_ops_s ops =
    {
      .short_write  = stm32_lcd_panel_short_write,
      .read         = stm32_lcd_panel_read,
      .long_write   = stm32_lcd_panel_long_write,
      .delay_ms     = stm32_lcd_panel_delay,
      .pixel_format = BOARD_LCD_PANEL_COLMOD,
      .madctl       = ST7801_MADCTL_RGB,
      .brightness   = ST7801_BRIGHTNESS_50_PERCENT,
    };
#else
  static const struct hx8379c_dsi_ops_s ops =
    {
      .short_write = stm32_lcd_panel_short_write,
      .long_write  = stm32_lcd_panel_long_write,
      .delay_ms    = stm32_lcd_panel_delay,
      .pixel_format = BOARD_LCD_PANEL_COLMOD,
    };
#endif

  int ret;

#ifdef CONFIG_STM32U5X9J_DK_LCD_CO5300
  ret = co5300_initialize(&ops);
#elif defined(CONFIG_STM32U5X9J_DK_LCD_ST7801)
  ret = st7801_initialize(&ops);
#else
  ret = hx8379c_initialize(&ops);
#endif
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
#ifdef BOARD_LCD_USE_GFXMMU
  struct stm32_gfxmmu_config_s gfxmmu;
#endif
  struct stm32_ltdc_config_s ltdc;
  struct fb_vtable_s *vtable;
#ifdef BOARD_LCD_USE_GFXMMU
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
         "stm32u5x9j: LCD panel=%s fb-map=%s fb-format=%s fb-bpp=%u "
         "dsi-format=%s dsi-bpp=%u dsi-lanes=%u panel-colmod=0x%02x "
         "phys-stride=%u layer-stride=%u fb-count=%u\n",
         BOARD_LCD_PANEL_NAME, BOARD_LCD_FB_MAP_NAME, BOARD_LCD_FORMAT_NAME,
         (unsigned int)BOARD_LCD_BPP,
         BOARD_LCD_DSI_FORMAT_NAME,
         (unsigned int)BOARD_LCD_DSI_BPP,
         (unsigned int)BOARD_LCD_DSI_LANES,
         (unsigned int)BOARD_LCD_PANEL_COLMOD,
         (unsigned int)BOARD_LCD_PHYS_STRIDE,
         (unsigned int)BOARD_LCD_LAYER_STRIDE,
         (unsigned int)BOARD_LCD_FB_COUNT);

  ret = stm32_dma2dinitialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: DMA2D init failed: %d\n", ret);
      return ret;
    }

#ifdef BOARD_LCD_USE_GFXMMU
  memset(&gfxmmu, 0, sizeof(gfxmmu));
  gfxmmu.physical   = BOARD_LCD_FB_PHYS;
  gfxmmu.width      = BOARD_LCD_XRES;
  gfxmmu.height     = BOARD_LCD_YRES;
  gfxmmu.stride     = BOARD_LCD_PHYS_STRIDE;
  gfxmmu.frame_size = BOARD_LCD_STORAGE_FBLEN;
  gfxmmu.bpp        = BOARD_LCD_BPP;
  gfxmmu.frames     = BOARD_LCD_FB_COUNT;
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
  ltdc.fb_base       = BOARD_LCD_FB;
  ltdc.fb1_base      = BOARD_LCD_FB1;
#ifdef CONFIG_STM32U5X9J_DK_LCD_ST7801
  ltdc.layer_fb_base = BOARD_LCD_FB;
#elif defined(CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER)
  ltdc.layer_fb_base = BOARD_LCD_FB1;
#else
  ltdc.layer_fb_base = BOARD_LCD_FB;
#endif
  ltdc.fb_size       = BOARD_LCD_VFBLEN * BOARD_LCD_FB_COUNT;
  ltdc.xres          = BOARD_LCD_XRES;
  ltdc.yres          = BOARD_LCD_YRES;
  ltdc.stride        = BOARD_LCD_LAYER_STRIDE;
  ltdc.layer_stride  = BOARD_LCD_LAYER_STRIDE;
  ltdc.window_y0     = BOARD_LCD_LAYER_Y0;
  ltdc.hsync         = BOARD_LCD_HSYNC;
  ltdc.hbp           = BOARD_LCD_HBP;
  ltdc.hfp           = BOARD_LCD_HFP;
  ltdc.vsync         = BOARD_LCD_VSYNC;
  ltdc.vbp           = BOARD_LCD_VBP;
  ltdc.vfp           = BOARD_LCD_VFP;
  ltdc.bpp           = BOARD_LCD_BPP;

  ret = stm32_ltdcinitialize(&ltdc);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: LTDC init failed: %d\n", ret);
      return ret;
    }

#ifndef BOARD_LCD_USE_GFXMMU
  stm32_mpu_ufbmem(BOARD_LCD_FB_PHYS, BOARD_LCD_FB_SIZE);
#else
  for (i = 0; i < BOARD_LCD_FB_COUNT; i++)
    {
      stm32_mpu_ufbmem(stm32_lcd_fb(i), BOARD_LCD_VFBLEN);
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

#ifndef CONFIG_STM32U5X9J_DK_LCD_COLORBAR
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

#ifdef CONFIG_STM32U5X9J_DK_LCD_COLORBAR
  stm32_lcd_colorbar();
  stm32_lcd_clean_framebuffer();
#endif

  syslog(LOG_INFO, "stm32u5x9j: /dev/fb0 framebuffer virt=0x%08" PRIxPTR
         " virt1=0x%08" PRIxPTR " phys=0x%08" PRIxPTR
         " size=%u count=%u\n", BOARD_LCD_FB, BOARD_LCD_FB1,
         BOARD_LCD_FB_PHYS, BOARD_LCD_FB_SIZE,
         BOARD_LCD_FB_COUNT);
  return OK;
}

#endif /* CONFIG_STM32U5X9J_DK_LCD */
