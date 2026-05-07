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
#include <nuttx/video/fb.h>

#include "stm32_gpio.h"
#include "stm32_dma2d.h"
#include "stm32_dsi.h"
#include "stm32_gfxmmu.h"
#include "stm32_ltdc.h"
#include "stm32u5x9j-dk.h"

#include "hardware/stm32_memorymap.h"

#ifdef CONFIG_STM32U5X9J_DK_LCD

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32U5X9J_LCD_XRES      480
#define STM32U5X9J_LCD_YRES      480
#define STM32U5X9J_LCD_VACT      481
#define STM32U5X9J_LCD_BPP       32
#define STM32U5X9J_LCD_STRIDE    (STM32U5X9J_LCD_XRES * 4)
#define STM32U5X9J_LCD_FBLEN     (STM32U5X9J_LCD_STRIDE * \
                                  STM32U5X9J_LCD_YRES)
#define STM32U5X9J_LCD_FB        ((uintptr_t)STM32_HSPI1_BANK)

#define STM32U5X9J_LCD_VSYNC     1
#define STM32U5X9J_LCD_VBP       12
#define STM32U5X9J_LCD_VFP       50
#define STM32U5X9J_LCD_HSYNC     2
#define STM32U5X9J_LCD_HBP       1
#define STM32U5X9J_LCD_HFP       1

/****************************************************************************
 * Private Data
 ****************************************************************************/

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
};

struct stm32u5x9j_lcd_cmd_s
{
  uint8_t cmd;
  uint8_t len;
  uint16_t delay;
  FAR const uint8_t *data;
};

static const uint8_t g_lcd_b9[] =
{
  0xff, 0x83, 0x79
};

static const uint8_t g_lcd_00[] = { 0x00 };
static const uint8_t g_lcd_01[] = { 0x01 };
static const uint8_t g_lcd_02[] = { 0x02 };
static const uint8_t g_lcd_77[] = { 0x77 };

static const uint8_t g_lcd_b1[] =
{
  0x44, 0x1c, 0x1c, 0x37, 0x57, 0x90, 0xd0, 0xe2,
  0x58, 0x80, 0x38, 0x38, 0xf8, 0x33, 0x34, 0x42
};

static const uint8_t g_lcd_b2[] =
{
  0x80, 0x14, 0x0c, 0x30, 0x20, 0x50, 0x11, 0x42,
  0x1d
};

static const uint8_t g_lcd_b4[] =
{
  0x01, 0xaa, 0x01, 0xaf, 0x01, 0xaf, 0x10, 0xea,
  0x1c, 0xea
};

static const uint8_t g_lcd_c7[] =
{
  0x00, 0x00, 0x00, 0xc0
};

static const uint8_t g_lcd_d3[] =
{
  0x00, 0x07, 0x00, 0x00, 0x00, 0x08, 0x08, 0x32,
  0x10, 0x01, 0x00, 0x01, 0x03, 0x72, 0x03, 0x72,
  0x00, 0x08, 0x00, 0x08, 0x33, 0x33, 0x05, 0x05,
  0x37, 0x05, 0x05, 0x37, 0x0a, 0x00, 0x00, 0x00,
  0x0a, 0x00, 0x01, 0x00, 0x0e
};

static const uint8_t g_lcd_d5[] =
{
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
  0x19, 0x19, 0x18, 0x18, 0x18, 0x18, 0x19, 0x19,
  0x01, 0x00, 0x03, 0x02, 0x05, 0x04, 0x07, 0x06,
  0x23, 0x22, 0x21, 0x20, 0x18, 0x18, 0x18, 0x18,
  0x00, 0x00
};

static const uint8_t g_lcd_d6[] =
{
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
  0x19, 0x19, 0x18, 0x18, 0x19, 0x19, 0x18, 0x18,
  0x06, 0x07, 0x04, 0x05, 0x02, 0x03, 0x00, 0x01,
  0x20, 0x21, 0x22, 0x23, 0x18, 0x18, 0x18, 0x18
};

static const uint8_t g_lcd_e0[] =
{
  0x00, 0x16, 0x1b, 0x30, 0x36, 0x3f, 0x24, 0x40,
  0x09, 0x0d, 0x0f, 0x18, 0x0e, 0x11, 0x12, 0x11,
  0x14, 0x07, 0x12, 0x13, 0x18, 0x00, 0x17, 0x1c,
  0x30, 0x36, 0x3f, 0x24, 0x40, 0x09, 0x0c, 0x0f,
  0x18, 0x0e, 0x11, 0x14, 0x11, 0x12, 0x07, 0x12,
  0x14, 0x18
};

static const uint8_t g_lcd_b6[] =
{
  0x2c, 0x2c, 0x00
};

static const uint8_t g_lcd_c1_0[] =
{
  0x01, 0x00, 0x07, 0x0f, 0x16, 0x1f, 0x27, 0x30,
  0x38, 0x40, 0x47, 0x4e, 0x56, 0x5d, 0x65, 0x6d,
  0x74, 0x7d, 0x84, 0x8a, 0x90, 0x99, 0xa1, 0xa9,
  0xb0, 0xb6, 0xbd, 0xc4, 0xcd, 0xd4, 0xdd, 0xe5,
  0xec, 0xf3, 0x36, 0x07, 0x1c, 0xc0, 0x1b, 0x01,
  0xf1, 0x34
};

static const uint8_t g_lcd_c1_1[] =
{
  0x00, 0x08, 0x0f, 0x16, 0x1f, 0x28, 0x31, 0x39,
  0x41, 0x48, 0x51, 0x59, 0x60, 0x68, 0x70, 0x78,
  0x7f, 0x87, 0x8d, 0x94, 0x9c, 0xa3, 0xab, 0xb3,
  0xb9, 0xc1, 0xc8, 0xd0, 0xd8, 0xe0, 0xe8, 0xee,
  0xf5, 0x3b, 0x1a, 0xb6, 0xa0, 0x07, 0x45, 0xc5,
  0x37, 0x00
};

static const uint8_t g_lcd_c1_2[] =
{
  0x00, 0x09, 0x0f, 0x18, 0x21, 0x2a, 0x34, 0x3c,
  0x45, 0x4c, 0x56, 0x5e, 0x66, 0x6e, 0x76, 0x7e,
  0x87, 0x8e, 0x95, 0x9d, 0xa6, 0xaf, 0xb7, 0xbd,
  0xc5, 0xce, 0xd5, 0xdf, 0xe7, 0xee, 0xf4, 0xfa,
  0xff, 0x0c, 0x31, 0x83, 0x3c, 0x5b, 0x56, 0x1e,
  0x5a, 0xff
};

static const struct stm32u5x9j_lcd_cmd_s g_lcd_init[] =
{
  { 0xb9, nitems(g_lcd_b9),   0,   g_lcd_b9 },
  { 0xb1, nitems(g_lcd_b1),   0,   g_lcd_b1 },
  { 0xb2, nitems(g_lcd_b2),   0,   g_lcd_b2 },
  { 0xb4, nitems(g_lcd_b4),   0,   g_lcd_b4 },
  { 0xc7, nitems(g_lcd_c7),   0,   g_lcd_c7 },
  { 0xcc, nitems(g_lcd_02),   0,   g_lcd_02 },
  { 0xd2, nitems(g_lcd_77),   0,   g_lcd_77 },
  { 0xd3, nitems(g_lcd_d3),   0,   g_lcd_d3 },
  { 0xd5, nitems(g_lcd_d5),   0,   g_lcd_d5 },
  { 0xd6, nitems(g_lcd_d6),   0,   g_lcd_d6 },
  { 0xe0, nitems(g_lcd_e0),   0,   g_lcd_e0 },
  { 0xb6, nitems(g_lcd_b6),   0,   g_lcd_b6 },
  { 0xbd, nitems(g_lcd_00),   0,   g_lcd_00 },
  { 0xc1, nitems(g_lcd_c1_0), 0,   g_lcd_c1_0 },
  { 0xbd, nitems(g_lcd_01),   0,   g_lcd_01 },
  { 0xc1, nitems(g_lcd_c1_1), 0,   g_lcd_c1_1 },
  { 0xbd, nitems(g_lcd_02),   0,   g_lcd_02 },
  { 0xc1, nitems(g_lcd_c1_2), 0,   g_lcd_c1_2 },
  { 0xbd, nitems(g_lcd_00),   0,   g_lcd_00 },
  { 0x11, 0,                  120, NULL },
  { 0x29, 0,                  120, NULL },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32u5x9j_lcd_reset_panel(void)
{
  stm32_configgpio(GPIO_LCD_DSI_POWER);
  stm32_configgpio(GPIO_LCD_RESET);

  stm32_gpiowrite(GPIO_LCD_DSI_POWER, true);
  stm32_gpiowrite(GPIO_LCD_RESET, false);
  up_mdelay(10);
  stm32_gpiowrite(GPIO_LCD_RESET, true);
  up_mdelay(120);
}

#ifdef CONFIG_STM32U5X9J_DK_LCD_COLORBAR
static void stm32u5x9j_lcd_fill_cpu(uint32_t color)
{
  FAR uint32_t *fb = (FAR uint32_t *)STM32U5X9J_LCD_FB;
  uint32_t i;

  for (i = 0; i < STM32U5X9J_LCD_XRES * STM32U5X9J_LCD_YRES; i++)
    {
      fb[i] = color;
    }
}

static void stm32u5x9j_lcd_rect_cpu(uint32_t x0, uint32_t y0,
                                    uint32_t width, uint32_t height,
                                    uint32_t color)
{
  FAR uint32_t *fb = (FAR uint32_t *)STM32U5X9J_LCD_FB;
  uint32_t x1 = MIN(x0 + width, STM32U5X9J_LCD_XRES);
  uint32_t y1 = MIN(y0 + height, STM32U5X9J_LCD_YRES);
  uint32_t x;
  uint32_t y;

  for (y = y0; y < y1; y++)
    {
      for (x = x0; x < x1; x++)
        {
          fb[y * STM32U5X9J_LCD_XRES + x] = color;
        }
    }
}

static void stm32u5x9j_lcd_solidfill(uint32_t color)
{
  int ret;

  ret = stm32_dma2dfill(STM32U5X9J_LCD_FB, color,
                        STM32U5X9J_LCD_XRES, STM32U5X9J_LCD_YRES,
                        STM32U5X9J_LCD_XRES);
  if (ret < 0)
    {
      stm32u5x9j_lcd_fill_cpu(color);
    }
}

static void stm32u5x9j_lcd_rgbrect(void)
{
  stm32u5x9j_lcd_rect_cpu(0, 0, 160, 160, 0x00ff0000);
  stm32u5x9j_lcd_rect_cpu(160, 160, 160, 160, 0x0000ff00);
  stm32u5x9j_lcd_rect_cpu(320, 320, 160, 160, 0x000000ff);
}

static void stm32u5x9j_lcd_colorbar_cpu(void)
{
  static const uint32_t colors[8] =
    {
      0x00ffffff, 0x00ffff00, 0x0000ffff, 0x0000ff00,
      0x00ff00ff, 0x00ff0000, 0x000000ff, 0x00000000
    };
  FAR uint32_t *fb = (FAR uint32_t *)STM32U5X9J_LCD_FB;
  uint32_t x;
  uint32_t y;

  for (y = 0; y < STM32U5X9J_LCD_YRES; y++)
    {
      for (x = 0; x < STM32U5X9J_LCD_XRES; x++)
        {
          fb[y * STM32U5X9J_LCD_XRES + x] =
            colors[(x * nitems(colors)) / STM32U5X9J_LCD_XRES];
        }
    }
}

static void stm32u5x9j_lcd_colorbar(void)
{
  static const uint32_t colors[8] =
    {
      0x00ffffff, 0x00ffff00, 0x0000ffff, 0x0000ff00,
      0x00ff00ff, 0x00ff0000, 0x000000ff, 0x00000000
    };
  uintptr_t fb = STM32U5X9J_LCD_FB;
  uint32_t x0;
  uint32_t x1;
  uint32_t i;
  int ret;

  for (i = 0; i < nitems(colors); i++)
    {
      x0 = (STM32U5X9J_LCD_XRES * i) / nitems(colors);
      x1 = (STM32U5X9J_LCD_XRES * (i + 1)) / nitems(colors);

      ret = stm32_dma2dfill(fb + x0 * 4, colors[i], x1 - x0,
                            STM32U5X9J_LCD_YRES, STM32U5X9J_LCD_XRES);
      if (ret < 0)
        {
          stm32u5x9j_lcd_colorbar_cpu();
          break;
        }
    }

  syslog(LOG_INFO, "stm32u5x9j: LCD framebuffer colorbar done\n");
}

#ifdef CONFIG_STM32U5X9J_DK_LCD_PATTERN
static void stm32u5x9j_lcd_pattern(void)
{
  stm32u5x9j_lcd_solidfill(0x00000000);
  stm32u5x9j_lcd_colorbar();
  stm32u5x9j_lcd_rgbrect();
  syslog(LOG_INFO, "stm32u5x9j: LCD static pattern done\n");
}
#endif
#endif

static int stm32u5x9j_lcd_panel_initialize(void)
{
  FAR const struct stm32u5x9j_lcd_cmd_s *cmd;
  uint32_t i;
  int ret;

  for (i = 0; i < nitems(g_lcd_init); i++)
    {
      cmd = &g_lcd_init[i];
      if (cmd->len == 0)
        {
          ret = stm32_dsishortwrite(STM32_DSI_DCS_SHORT_WRITE0,
                                    cmd->cmd, 0);
        }
      else if (cmd->len == 1)
        {
          ret = stm32_dsishortwrite(STM32_DSI_DCS_SHORT_WRITE1,
                                    cmd->cmd, cmd->data[0]);
        }
      else
        {
          ret = stm32_dsilongwrite(cmd->cmd, cmd->data, cmd->len);
        }

      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: LCD DSI cmd 0x%02x failed: %d\n",
                 cmd->cmd, ret);
          return ret;
        }

      if (cmd->delay > 0)
        {
          up_mdelay(cmd->delay);
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32u5x9j_lcd_initialize(void)
{
  struct stm32_gfxmmu_config_s gfxmmu;
  struct stm32_ltdc_config_s ltdc;
  struct fb_vtable_s *vtable;
  int ret;

#ifdef CONFIG_STM32U5X9J_DK_HSPI_RAM
  ret = stm32u5x9j_hspi_ram_initialize();
  if (ret < 0)
    {
      return ret;
    }
#else
  return -ENODEV;
#endif

  stm32u5x9j_lcd_reset_panel();

  ret = stm32_dma2dinitialize();
  if (ret < 0)
    {
      return ret;
    }

  memset(&gfxmmu, 0, sizeof(gfxmmu));
  gfxmmu.physical = STM32U5X9J_LCD_FB;
  gfxmmu.width    = STM32U5X9J_LCD_XRES;
  gfxmmu.height   = STM32U5X9J_LCD_YRES;
  gfxmmu.stride   = STM32U5X9J_LCD_STRIDE;
  gfxmmu.bpp      = STM32U5X9J_LCD_BPP;
  gfxmmu.circular = true;

  ret = stm32_gfxmmuinitialize(&gfxmmu);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_dsiinitialize(&g_dsi_config);
  if (ret < 0)
    {
      return ret;
    }

  memset(&ltdc, 0, sizeof(ltdc));
  ltdc.fb_base       = STM32U5X9J_LCD_FB;
  ltdc.layer_fb_base = stm32_gfxmmu_buffer0();
  ltdc.xres          = STM32U5X9J_LCD_XRES;
  ltdc.yres          = STM32U5X9J_LCD_YRES;
  ltdc.stride        = STM32U5X9J_LCD_STRIDE;
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
      return ret;
    }

  ret = stm32_dsistart();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32u5x9j_lcd_panel_initialize();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_dsienableltdc();
  if (ret < 0)
    {
      return ret;
    }

  vtable = stm32_ltdcgetvplane(0);
  if (vtable == NULL)
    {
      return -ENODEV;
    }

  ret = fb_register_device(0, 0, vtable);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_STM32U5X9J_DK_LCD_PATTERN
  stm32u5x9j_lcd_pattern();
#elif defined(CONFIG_STM32U5X9J_DK_LCD_COLORBAR)
  stm32u5x9j_lcd_colorbar();
#endif

  syslog(LOG_INFO, "stm32u5x9j: /dev/fb0 framebuffer at 0x%08" PRIxPTR
         " size=%u\n", STM32U5X9J_LCD_FB, STM32U5X9J_LCD_FBLEN);
  return OK;
}

#endif /* CONFIG_STM32U5X9J_DK_LCD */
