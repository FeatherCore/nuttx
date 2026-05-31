/****************************************************************************
 * drivers/lcd/st7801.c
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
#include <nuttx/lcd/st7801.h>

#ifdef CONFIG_LCD_ST7801

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MIPI_DSI_DCS_SHORT_WRITE0  0x05
#define MIPI_DSI_DCS_SHORT_READ    0x06
#define MIPI_DSI_DCS_SHORT_WRITE1  0x15

#define ST7801_DCS_SOFT_RESET      0x01
#define ST7801_DCS_READ_ID         0x04
#define ST7801_DCS_READ_STATUS     0x0a
#define ST7801_DCS_SLEEP_OUT       0x11
#define ST7801_DCS_DISPLAY_ON      0x29
#define ST7801_DCS_SET_COLUMN      0x2a
#define ST7801_DCS_SET_PAGE        0x2b
#define ST7801_DCS_SET_TEAR_ON     0x35
#define ST7801_DCS_SET_ADDRESS     0x36
#define ST7801_DCS_SET_PIXEL_FMT   0x3a
#define ST7801_DCS_WRITE_BRIGHT    0x51
#define ST7801_DCS_CTRL_DISPLAY    0x53

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct st7801_cmd_s
{
  uint8_t cmd;
  uint8_t len;
  uint16_t delay;
  FAR const uint8_t *data;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint8_t g_st7801_caset[] =
{
  0x00, 0x00, 0x01, 0x99
};

static const uint8_t g_st7801_raset[] =
{
  0x00, 0x00, 0x01, 0xf5
};

static const uint8_t g_st7801_te_on[] =
{
  0x00
};

static const uint8_t g_st7801_colmod_vendor[] =
{
  ST7801_PIXEL_FORMAT_RGB888
};

static const uint8_t g_st7801_brightness_vendor[] =
{
  ST7801_BRIGHTNESS_MAX
};

static const uint8_t g_st7801_ctrl_display[] =
{
  0x20
};

/* Porting note, 2026-05-31:
 *
 * This table is based on the YuYing ESP32-P4 ST7801 reference, but the final
 * STM32U5x9J-DK board path uses RGB565 scanout.  The vendor sequence carries
 * a RGB888 COLMOD value, so st7801_initialize() rewrites 0x3a to the active
 * board pixel format when needed.  Do not remove that rewrite unless the
 * LTDC/DSI stream is also changed to match.
 *
 * Important hardware findings from the STM32U5x9J-DK bring-up:
 *
 * - ST7801 ID 78 01 00 and display status 0x9c validate the DCS/BTA
 *   path, but they do not prove HS video timing.  Several failing builds
 *   could read ID/status while the panel stayed black or showed artifacts.
 * - The known-good board baseline uses clock-lane P/N swap only
 *   (STM32_DSI_PHY_SWAP_CLK).  Swapping D0 caused command/read timeouts and
 *   is not part of the validated path.
 * - The decisive video fix lives in the board/DSI timing, not in this DCS
 *   table: ST7801 needs fractional horizontal timing scale 7/2 (hscale=3.5)
 *   for the current 420 Mbps / 15 MHz reference relationship.  Integer
 *   hscale=2 produced visible grid/block artifacts even with a solid-red
 *   framebuffer.
 * - The validated STM32 video mode is sync-pulse mode with the normal LTDC
 *   framebuffer path.  DSI wrapper pattern output is diagnostic-only.
 */

static const struct st7801_cmd_s g_st7801_init[] =
{
  { ST7801_DCS_SLEEP_OUT,        0,   100, NULL },
  { ST7801_DCS_SET_COLUMN,       4,   0,   g_st7801_caset },
  { ST7801_DCS_SET_PAGE,         4,   0,   g_st7801_raset },
  { ST7801_DCS_SET_TEAR_ON,      1,   0,   g_st7801_te_on },
  { ST7801_DCS_SET_PIXEL_FMT,    1,   0,   g_st7801_colmod_vendor },
  { ST7801_DCS_WRITE_BRIGHT,     1,   0,   g_st7801_brightness_vendor },
  { ST7801_DCS_CTRL_DISPLAY,     1,   0,   g_st7801_ctrl_display },
  { ST7801_DCS_DISPLAY_ON,       0,   0,   NULL },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void st7801_delay(FAR const struct st7801_dsi_ops_s *ops,
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

static int st7801_send(FAR const struct st7801_dsi_ops_s *ops,
                       FAR const struct st7801_cmd_s *cmd)
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
      lcderr("ST7801 DSI cmd 0x%02x failed: %d\n", cmd->cmd, ret);
      return ret;
    }

  if (cmd->delay > 0)
    {
      st7801_delay(ops, cmd->delay);
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* ST7801 init/report notes:
 *
 * Keep the early reset/read-ID sequence as a command-path checkpoint.  A
 * failed ID read is allowed to continue so that board bring-up can still
 * test downstream DSI video experiments, but a release-quality hardware
 * baseline should read ID 78 01 00 and status 0x9c before trusting display
 * results.
 */

int st7801_initialize(FAR const struct st7801_dsi_ops_s *ops)
{
  FAR const struct st7801_cmd_s *cmd;
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
                 ops->pixel_format : ST7801_PIXEL_FORMAT_RGB565;
  brightness = ops->brightness != 0 ?
               ops->brightness : ST7801_BRIGHTNESS_MAX;

  if (ops->read != NULL)
    {
      ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE0,
                             ST7801_DCS_SOFT_RESET, 0);
      if (ret < 0)
        {
          lcderr("ST7801 DSI cmd 0x%02x failed: %d\n",
                 ST7801_DCS_SOFT_RESET, ret);
          return ret;
        }

      st7801_delay(ops, 120);

      ret = ops->read(MIPI_DSI_DCS_SHORT_READ, ST7801_DCS_READ_ID,
                      id, sizeof(id));
      if (ret < 0)
        {
          lcdwarn("ST7801 DSI read ID failed: %d; continuing to test "
                  "downstream init/video path\n", ret);
        }
      else
        {
          lcdinfo("ST7801 ID: %02x %02x %02x\n", id[0], id[1], id[2]);
        }
    }

  ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE1,
                         ST7801_DCS_SET_ADDRESS, ops->madctl);
  if (ret < 0)
    {
      lcderr("ST7801 DSI cmd 0x%02x failed: %d\n",
             ST7801_DCS_SET_ADDRESS, ret);
      return ret;
    }

  ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE1,
                         ST7801_DCS_SET_PIXEL_FMT, pixel_format);
  if (ret < 0)
    {
      lcderr("ST7801 DSI cmd 0x%02x failed: %d\n",
             ST7801_DCS_SET_PIXEL_FMT, ret);
      return ret;
    }

  for (i = 0; i < nitems(g_st7801_init); i++)
    {
      cmd = &g_st7801_init[i];

      if (cmd->cmd == ST7801_DCS_SET_PIXEL_FMT &&
          cmd->len == 1 && cmd->data[0] != pixel_format)
        {
          struct st7801_cmd_s fmtcmd =
            {
              ST7801_DCS_SET_PIXEL_FMT, 1, cmd->delay, &pixel_format
            };

          lcdwarn("ST7801 forcing vendor COLMOD 0x%02x to active "
                  "pixel format 0x%02x\n", cmd->data[0], pixel_format);

          ret = st7801_send(ops, &fmtcmd);
          if (ret < 0)
            {
              return ret;
            }

          continue;
        }

      if (cmd->cmd == ST7801_DCS_WRITE_BRIGHT &&
          brightness != ST7801_BRIGHTNESS_MAX)
        {
          ret = st7801_set_brightness(ops, brightness);
          if (ret < 0)
            {
              return ret;
            }

          continue;
        }

      ret = st7801_send(ops, cmd);
      if (ret < 0)
        {
          return ret;
        }
    }

  if (ops->read != NULL)
    {
      ret = ops->read(MIPI_DSI_DCS_SHORT_READ, ST7801_DCS_READ_STATUS,
                      &status, sizeof(status));
      if (ret < 0)
        {
          lcdwarn("ST7801 DSI read display status failed: %d\n", ret);
        }
      else
        {
          lcdinfo("ST7801 display status: %02x BSTON=%u NORON=%u "
                  "SLPOUT=%u DISPON=%u\n",
                  status, (status >> 7) & 1, (status >> 3) & 1,
                  ((status >> 4) & 1) == 0, (status >> 2) & 1);
        }
    }

  return OK;
}

int st7801_set_brightness(FAR const struct st7801_dsi_ops_s *ops,
                          uint8_t brightness)
{
  int ret;

  if (ops == NULL || ops->short_write == NULL)
    {
      return -EINVAL;
    }

  ret = ops->short_write(MIPI_DSI_DCS_SHORT_WRITE1,
                         ST7801_DCS_WRITE_BRIGHT, brightness);
  if (ret < 0)
    {
      lcderr("ST7801 DSI cmd 0x%02x failed: %d\n",
             ST7801_DCS_WRITE_BRIGHT, ret);
      return ret;
    }

  return OK;
}

#endif /* CONFIG_LCD_ST7801 */
