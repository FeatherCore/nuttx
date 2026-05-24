/****************************************************************************
 * drivers/lcd/hx8379c.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/param.h>

#include <errno.h>
#include <stdint.h>

#include <nuttx/arch.h>
#include <nuttx/debug.h>
#include <nuttx/lcd/hx8379c.h>

#ifdef CONFIG_LCD_HX8379C

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MIPI_DSI_DCS_SHORT_WRITE0 0x05
#define MIPI_DSI_DCS_SHORT_WRITE1 0x15

#define HX8379C_DCS_SET_PIXEL_FORMAT 0x3a
#define HX8379C_DCS_SLEEP_OUT        0x11
#define HX8379C_DCS_DISPLAY_ON       0x29

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct hx8379c_cmd_s
{
  uint8_t cmd;
  uint8_t len;
  uint16_t delay;
  FAR const uint8_t *data;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint8_t g_hx8379c_b9[] =
{
  0xff, 0x83, 0x79
};

static const uint8_t g_hx8379c_00[] = { 0x00 };
static const uint8_t g_hx8379c_01[] = { 0x01 };
static const uint8_t g_hx8379c_02[] = { 0x02 };
static const uint8_t g_hx8379c_77[] = { 0x77 };

static const uint8_t g_hx8379c_b1[] =
{
  0x44, 0x1c, 0x1c, 0x37, 0x57, 0x90, 0xd0, 0xe2,
  0x58, 0x80, 0x38, 0x38, 0xf8, 0x33, 0x34, 0x42
};

static const uint8_t g_hx8379c_b2[] =
{
  0x80, 0x14, 0x0c, 0x30, 0x20, 0x50, 0x11, 0x42,
  0x1d
};

static const uint8_t g_hx8379c_b4[] =
{
  0x01, 0xaa, 0x01, 0xaf, 0x01, 0xaf, 0x10, 0xea,
  0x1c, 0xea
};

static const uint8_t g_hx8379c_c7[] =
{
  0x00, 0x00, 0x00, 0xc0
};

static const uint8_t g_hx8379c_d3[] =
{
  0x00, 0x07, 0x00, 0x00, 0x00, 0x08, 0x08, 0x32,
  0x10, 0x01, 0x00, 0x01, 0x03, 0x72, 0x03, 0x72,
  0x00, 0x08, 0x00, 0x08, 0x33, 0x33, 0x05, 0x05,
  0x37, 0x05, 0x05, 0x37, 0x0a, 0x00, 0x00, 0x00,
  0x0a, 0x00, 0x01, 0x00, 0x0e
};

static const uint8_t g_hx8379c_d5[] =
{
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
  0x19, 0x19, 0x18, 0x18, 0x18, 0x18, 0x19, 0x19,
  0x01, 0x00, 0x03, 0x02, 0x05, 0x04, 0x07, 0x06,
  0x23, 0x22, 0x21, 0x20, 0x18, 0x18, 0x18, 0x18,
  0x00, 0x00
};

static const uint8_t g_hx8379c_d6[] =
{
  0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
  0x19, 0x19, 0x18, 0x18, 0x19, 0x19, 0x18, 0x18,
  0x06, 0x07, 0x04, 0x05, 0x02, 0x03, 0x00, 0x01,
  0x20, 0x21, 0x22, 0x23, 0x18, 0x18, 0x18, 0x18
};

static const uint8_t g_hx8379c_e0[] =
{
  0x00, 0x16, 0x1b, 0x30, 0x36, 0x3f, 0x24, 0x40,
  0x09, 0x0d, 0x0f, 0x18, 0x0e, 0x11, 0x12, 0x11,
  0x14, 0x07, 0x12, 0x13, 0x18, 0x00, 0x17, 0x1c,
  0x30, 0x36, 0x3f, 0x24, 0x40, 0x09, 0x0c, 0x0f,
  0x18, 0x0e, 0x11, 0x14, 0x11, 0x12, 0x07, 0x12,
  0x14, 0x18
};

static const uint8_t g_hx8379c_b6[] =
{
  0x2c, 0x2c, 0x00
};

static const uint8_t g_hx8379c_c1_0[] =
{
  0x01, 0x00, 0x07, 0x0f, 0x16, 0x1f, 0x27, 0x30,
  0x38, 0x40, 0x47, 0x4e, 0x56, 0x5d, 0x65, 0x6d,
  0x74, 0x7d, 0x84, 0x8a, 0x90, 0x99, 0xa1, 0xa9,
  0xb0, 0xb6, 0xbd, 0xc4, 0xcd, 0xd4, 0xdd, 0xe5,
  0xec, 0xf3, 0x36, 0x07, 0x1c, 0xc0, 0x1b, 0x01,
  0xf1, 0x34
};

static const uint8_t g_hx8379c_c1_1[] =
{
  0x00, 0x08, 0x0f, 0x16, 0x1f, 0x28, 0x31, 0x39,
  0x41, 0x48, 0x51, 0x59, 0x60, 0x68, 0x70, 0x78,
  0x7f, 0x87, 0x8d, 0x94, 0x9c, 0xa3, 0xab, 0xb3,
  0xb9, 0xc1, 0xc8, 0xd0, 0xd8, 0xe0, 0xe8, 0xee,
  0xf5, 0x3b, 0x1a, 0xb6, 0xa0, 0x07, 0x45, 0xc5,
  0x37, 0x00
};

static const uint8_t g_hx8379c_c1_2[] =
{
  0x00, 0x09, 0x0f, 0x18, 0x21, 0x2a, 0x34, 0x3c,
  0x45, 0x4c, 0x56, 0x5e, 0x66, 0x6e, 0x76, 0x7e,
  0x87, 0x8e, 0x95, 0x9d, 0xa6, 0xaf, 0xb7, 0xbd,
  0xc5, 0xce, 0xd5, 0xdf, 0xe7, 0xee, 0xf4, 0xfa,
  0xff, 0x0c, 0x31, 0x83, 0x3c, 0x5b, 0x56, 0x1e,
  0x5a, 0xff
};

static const struct hx8379c_cmd_s g_hx8379c_init[] =
{
  { 0xb9, nitems(g_hx8379c_b9),   0,   g_hx8379c_b9 },
  { 0xb1, nitems(g_hx8379c_b1),   0,   g_hx8379c_b1 },
  { 0xb2, nitems(g_hx8379c_b2),   0,   g_hx8379c_b2 },
  { 0xb4, nitems(g_hx8379c_b4),   0,   g_hx8379c_b4 },
  { 0xc7, nitems(g_hx8379c_c7),   0,   g_hx8379c_c7 },
  { 0xcc, nitems(g_hx8379c_02),   0,   g_hx8379c_02 },
  { 0xd2, nitems(g_hx8379c_77),   0,   g_hx8379c_77 },
  { 0xd3, nitems(g_hx8379c_d3),   0,   g_hx8379c_d3 },
  { 0xd5, nitems(g_hx8379c_d5),   0,   g_hx8379c_d5 },
  { 0xd6, nitems(g_hx8379c_d6),   0,   g_hx8379c_d6 },
  { 0xe0, nitems(g_hx8379c_e0),   0,   g_hx8379c_e0 },
  { 0xb6, nitems(g_hx8379c_b6),   0,   g_hx8379c_b6 },
  { 0xbd, nitems(g_hx8379c_00),   0,   g_hx8379c_00 },
  { 0xc1, nitems(g_hx8379c_c1_0), 0,   g_hx8379c_c1_0 },
  { 0xbd, nitems(g_hx8379c_01),   0,   g_hx8379c_01 },
  { 0xc1, nitems(g_hx8379c_c1_1), 0,   g_hx8379c_c1_1 },
  { 0xbd, nitems(g_hx8379c_02),   0,   g_hx8379c_02 },
  { 0xc1, nitems(g_hx8379c_c1_2), 0,   g_hx8379c_c1_2 },
  { 0xbd, nitems(g_hx8379c_00),   0,   g_hx8379c_00 },
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int hx8379c_initialize(FAR const struct hx8379c_dsi_ops_s *ops)
{
  FAR const struct hx8379c_cmd_s *cmd;
  uint32_t i;
  uint8_t pixel_format;
  int ret;

  if (ops == NULL || ops->short_write == NULL || ops->long_write == NULL)
    {
      return -EINVAL;
    }

  for (i = 0; i < nitems(g_hx8379c_init); i++)
    {
      cmd = &g_hx8379c_init[i];
      if (cmd->len == 0)
        {
          ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE0, cmd->cmd, 0);
        }
      else if (cmd->len == 1)
        {
          ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE1, cmd->cmd,
                                 cmd->data[0]);
        }
      else
        {
          ret = ops->long_write(cmd->cmd, cmd->data, cmd->len);
        }

      if (ret < 0)
        {
          lcderr("HX8379C DSI cmd 0x%02x failed: %d\n", cmd->cmd, ret);
          return ret;
        }

      if (cmd->delay > 0)
        {
          if (ops->delay_ms != NULL)
            {
              ops->delay_ms(cmd->delay);
            }
          else
            {
              up_mdelay(cmd->delay);
            }
        }
    }

  pixel_format = ops->pixel_format != 0 ?
                 ops->pixel_format : HX8379C_PIXEL_FORMAT_RGB888;

  ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE1,
                         HX8379C_DCS_SET_PIXEL_FORMAT, pixel_format);
  if (ret < 0)
    {
      lcderr("HX8379C DSI cmd 0x%02x failed: %d\n",
             HX8379C_DCS_SET_PIXEL_FORMAT, ret);
      return ret;
    }

  ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE0,
                         HX8379C_DCS_SLEEP_OUT, 0);
  if (ret < 0)
    {
      lcderr("HX8379C DSI cmd 0x%02x failed: %d\n",
             HX8379C_DCS_SLEEP_OUT, ret);
      return ret;
    }

  if (ops->delay_ms != NULL)
    {
      ops->delay_ms(120);
    }
  else
    {
      up_mdelay(120);
    }

  ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE0,
                         HX8379C_DCS_DISPLAY_ON, 0);
  if (ret < 0)
    {
      lcderr("HX8379C DSI cmd 0x%02x failed: %d\n",
             HX8379C_DCS_DISPLAY_ON, ret);
      return ret;
    }

  if (ops->delay_ms != NULL)
    {
      ops->delay_ms(120);
    }
  else
    {
      up_mdelay(120);
    }

  return OK;
}

#endif /* CONFIG_LCD_HX8379C */
