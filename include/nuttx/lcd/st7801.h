/****************************************************************************
 * include/nuttx/lcd/st7801.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_LCD_ST7801_H
#define __INCLUDE_NUTTX_LCD_ST7801_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <stdint.h>

#ifdef CONFIG_LCD_ST7801

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ST7801_PIXEL_FORMAT_RGB565       0x55
#define ST7801_PIXEL_FORMAT_RGB666       0x66
#define ST7801_PIXEL_FORMAT_RGB888       0x77

#define ST7801_MADCTL_RGB                0x00
#define ST7801_MADCTL_BGR                0x08

#define ST7801_BRIGHTNESS_MAX            0xff
#define ST7801_BRIGHTNESS_50_PERCENT     0x7f

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct st7801_dsi_ops_s
{
  CODE int (*short_write)(uint8_t datatype, uint8_t command,
                          uint8_t param);
  CODE int (*read)(uint8_t datatype, uint8_t command, FAR uint8_t *buffer,
                   size_t len);
  CODE int (*long_write)(uint8_t command, FAR const uint8_t *data,
                         size_t len);
  CODE void (*delay_ms)(unsigned int delay_ms);
  uint8_t pixel_format;
  uint8_t madctl;
  uint8_t brightness;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: st7801_initialize
 *
 * Description:
 *   Send the YuYing 2.13-inch 410x502 AMOLED ST7801 MIPI DSI panel
 *   initialization sequence.
 *
 * Input Parameters:
 *   ops - Board-provided DSI packet operations, optional delay operation,
 *         and optional panel format values.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int st7801_initialize(FAR const struct st7801_dsi_ops_s *ops);

/****************************************************************************
 * Name: st7801_set_brightness
 *
 * Description:
 *   Set the ST7801 display brightness through DCS command 0x51.
 *
 * Input Parameters:
 *   ops        - Board-provided DSI packet operations.
 *   brightness - Raw ST7801 brightness value, 0x00 to 0xff.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int st7801_set_brightness(FAR const struct st7801_dsi_ops_s *ops,
                          uint8_t brightness);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_LCD_ST7801 */
#endif /* __INCLUDE_NUTTX_LCD_ST7801_H */
