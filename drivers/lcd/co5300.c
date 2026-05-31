/****************************************************************************
 * drivers/lcd/co5300.c
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
#include <nuttx/lcd/co5300.h>

#ifdef CONFIG_LCD_CO5300

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MIPI_DSI_DCS_SHORT_WRITE0 0x05
#define MIPI_DSI_DCS_SHORT_READ   0x06
#define MIPI_DSI_DCS_SHORT_WRITE1 0x15

#define CO5300_DCS_SET_COLUMN     0x2a
#define CO5300_DCS_SET_PAGE       0x2b
#define CO5300_DCS_SOFT_RESET     0x01
#define CO5300_DCS_SLEEP_OUT      0x11
#define CO5300_DCS_DISPLAY_ON     0x29
#define CO5300_DCS_READ_ID        0x04
#define CO5300_DCS_READ_STATUS    0x0a
#define CO5300_DCS_SET_TEAR_ON    0x35
#define CO5300_DCS_SET_ADDRESS    0x36
#define CO5300_DCS_SET_PIXEL_FMT  0x3a
#define CO5300_DCS_WRITE_BRIGHT   0x51
#define CO5300_DCS_CTRL_DISPLAY   0x53
#define CO5300_DCS_EXT_VIDEO_MODE 0xc2

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct co5300_cmd_s
{
  uint8_t cmd;
  uint8_t len;
  uint16_t delay;
  FAR const uint8_t *data;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint8_t g_co5300_fe_20[] =
{
  0x20
};

static const uint8_t g_co5300_f4[] =
{
  0x5a
};

static const uint8_t g_co5300_f5[] =
{
  0x59
};

static const uint8_t g_co5300_fe_80[] =
{
  0x80
};

static const uint8_t g_co5300_03[] =
{
  0x00
};

static const uint8_t g_co5300_fe_00[] =
{
  0x00
};

static const uint8_t g_co5300_te_on[] =
{
  0x00
};

static const uint8_t g_co5300_ctrl_display[] =
{
  0x20
};

static const uint8_t g_co5300_ext_video_mode[] =
{
  0x03
};

static const uint8_t g_co5300_brightness[] =
{
  0xff
};

static const uint8_t g_co5300_63[] =
{
  0xab
};

static const uint8_t g_co5300_caset[] =
{
  0x00, 0x06, 0x01, 0xd7
};

static const uint8_t g_co5300_raset[] =
{
  0x00, 0x00, 0x01, 0xd1
};

/* Porting note, 2026-05-31:
 *
 * This table is intentionally small and follows the YuYing ESP32-P4 CO5300
 * reference plus the STM32CubeU5 CO5300 diagnostic direction.  The driver
 * owns only panel DCS command sequencing; the board DSI host still has to
 * provide the matching 466x466 RGB565 1-lane video configuration.
 *
 * Important hardware findings from the STM32U5x9J-DK bring-up:
 *
 * - DCS reads can pass while the HS video stream is still black.  CO5300
 *   ID 33 11 00 and display status 0x9c prove the LP command/BTA path and
 *   the panel state, but they do not prove LTDC->DSI scanout.
 * - The known-good board baseline uses clock-lane P/N swap only
 *   (STM32_DSI_PHY_SWAP_CLK).  Swapping data lane D0 is not part of the
 *   validated CO5300 path.
 * - Keep the board-side horizontal scale at hscale=2 for this panel.  The
 *   later ST7801 fractional-hscale fix must not be copied here.
 * - Normal validation uses the LTDC framebuffer/colorbar path.  The DSI
 *   wrapper pattern generator was only a diagnostic tool and produced
 *   misleading artifacts during bring-up.
 */

static const struct co5300_cmd_s g_co5300_init[] =
{
  { 0xfe, nitems(g_co5300_fe_20),       0,   g_co5300_fe_20 },
  { 0xf4, nitems(g_co5300_f4),          0,   g_co5300_f4 },
  { 0xf5, nitems(g_co5300_f5),          0,   g_co5300_f5 },
  { 0xfe, nitems(g_co5300_fe_80),       0,   g_co5300_fe_80 },
  { 0x03, nitems(g_co5300_03),          0,   g_co5300_03 },
  { 0xfe, nitems(g_co5300_fe_00),       0,   g_co5300_fe_00 },
  { CO5300_DCS_SET_TEAR_ON,             1,   0,   g_co5300_te_on },
  { CO5300_DCS_EXT_VIDEO_MODE,          1,   0,   g_co5300_ext_video_mode },
  { CO5300_DCS_CTRL_DISPLAY,            1,   0,   g_co5300_ctrl_display },
  { CO5300_DCS_WRITE_BRIGHT,            1,   0,   g_co5300_brightness },
  { 0x63, nitems(g_co5300_63),          0,   g_co5300_63 },
  { CO5300_DCS_SET_COLUMN,              4,   0,   g_co5300_caset },
  { CO5300_DCS_SET_PAGE,                4,   0,   g_co5300_raset },
  { CO5300_DCS_SLEEP_OUT,               0,   400, NULL },
  { CO5300_DCS_DISPLAY_ON,              0,   0,   NULL },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void co5300_delay(FAR const struct co5300_dsi_ops_s *ops,
                         unsigned int delay_ms)
{
  if (ops->delay_ms != NULL)
    {
      ops->delay_ms(delay_ms);
    }
  else
    {
      up_mdelay(delay_ms);
    }
}

static int co5300_send(FAR const struct co5300_dsi_ops_s *ops,
                       FAR const struct co5300_cmd_s *cmd)
{
  int ret;

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
      lcderr("CO5300 DSI cmd 0x%02x failed: %d\n", cmd->cmd, ret);
      return ret;
    }

  if (cmd->delay > 0)
    {
      co5300_delay(ops, cmd->delay);
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* CO5300 init/report notes:
 *
 * The software reset and ID read at the start are deliberate.  They catch
 * broken reset/power/LP-command wiring early before the long display path is
 * blamed.  A successful read should be treated as a command-path checkpoint;
 * final display validation still requires a real framebuffer pattern after
 * DSI video mode is enabled by the board code.
 */

int co5300_initialize(FAR const struct co5300_dsi_ops_s *ops)
{
  FAR const struct co5300_cmd_s *cmd;
  uint8_t id[3];
  uint8_t status;
  uint8_t pixel_format;
  uint8_t brightness;
  uint32_t i;
  int ret;

  if (ops == NULL || ops->short_write == NULL || ops->long_write == NULL)
    {
      return -EINVAL;
    }

  pixel_format = ops->pixel_format != 0 ?
                 ops->pixel_format : CO5300_PIXEL_FORMAT_RGB565;
  brightness = ops->brightness != 0 ?
               ops->brightness : CO5300_BRIGHTNESS_MAX;

  if (ops->read != NULL)
    {
      ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE0,
                             CO5300_DCS_SOFT_RESET, 0);
      if (ret < 0)
        {
          lcderr("CO5300 DSI cmd 0x%02x failed: %d\n",
                 CO5300_DCS_SOFT_RESET, ret);
          return ret;
        }

      co5300_delay(ops, 120);

      ret = ops->read(MIPI_DSI_DCS_SHORT_READ, CO5300_DCS_READ_ID,
                      id, sizeof(id));
      if (ret < 0)
        {
          lcderr("CO5300 DSI read ID failed: %d\n", ret);
          return ret;
        }

      lcdinfo("CO5300 ID: %02x %02x %02x\n", id[0], id[1], id[2]);
    }

  ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE1,
                         CO5300_DCS_SET_ADDRESS, ops->madctl);
  if (ret < 0)
    {
      lcderr("CO5300 DSI cmd 0x%02x failed: %d\n",
             CO5300_DCS_SET_ADDRESS, ret);
      return ret;
    }

  ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE1,
                         CO5300_DCS_SET_PIXEL_FMT, pixel_format);
  if (ret < 0)
    {
      lcderr("CO5300 DSI cmd 0x%02x failed: %d\n",
             CO5300_DCS_SET_PIXEL_FMT, ret);
      return ret;
    }

  for (i = 0; i < nitems(g_co5300_init); i++)
    {
      cmd = &g_co5300_init[i];

      if (cmd->cmd == CO5300_DCS_WRITE_BRIGHT &&
          brightness != CO5300_BRIGHTNESS_MAX)
        {
          ret = co5300_set_brightness(ops, brightness);
          if (ret < 0)
            {
              return ret;
            }

          continue;
        }

      ret = co5300_send(ops, cmd);
      if (ret < 0)
        {
          return ret;
        }
    }

  if (ops->read != NULL)
    {
      ret = ops->read(MIPI_DSI_DCS_SHORT_READ, CO5300_DCS_READ_STATUS,
                      &status, sizeof(status));
      if (ret < 0)
        {
          lcdwarn("CO5300 DSI read display status failed: %d\n", ret);
        }
      else
        {
          lcdinfo("CO5300 display status: %02x BSTON=%u NORON=%u "
                  "DISPON=%u\n",
                  status, (status >> 7) & 1, (status >> 3) & 1,
                  (status >> 2) & 1);
        }
    }

  return OK;
}

int co5300_set_brightness(FAR const struct co5300_dsi_ops_s *ops,
                          uint8_t brightness)
{
  int ret;

  if (ops == NULL || ops->short_write == NULL)
    {
      return -EINVAL;
    }

  ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE1,
                         CO5300_DCS_WRITE_BRIGHT, brightness);
  if (ret < 0)
    {
      lcderr("CO5300 DSI cmd 0x%02x failed: %d\n",
             CO5300_DCS_WRITE_BRIGHT, ret);
      return ret;
    }

  return OK;
}

#endif /* CONFIG_LCD_CO5300 */
