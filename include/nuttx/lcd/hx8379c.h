/****************************************************************************
 * include/nuttx/lcd/hx8379c.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_LCD_HX8379C_H
#define __INCLUDE_NUTTX_LCD_HX8379C_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <stdint.h>

#ifdef CONFIG_LCD_HX8379C

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HX8379C_PIXEL_FORMAT_RGB565  0x55
#define HX8379C_PIXEL_FORMAT_RGB888  0x77

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct hx8379c_dsi_ops_s
{
  CODE int (*short_write)(uint8_t datatype, uint8_t command,
                          uint8_t param);
  CODE int (*long_write)(uint8_t command, FAR const uint8_t *data,
                         size_t len);
  CODE void (*delay_ms)(unsigned int delay_ms);
  uint8_t pixel_format;
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
 * Name: hx8379c_initialize
 *
 * Description:
 *   Send the Himax HX8379C-compatible DSI panel initialization sequence.
 *
 * Input Parameters:
 *   ops - Board-provided DSI packet and delay operations.
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int hx8379c_initialize(FAR const struct hx8379c_dsi_ops_s *ops);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_LCD_HX8379C */
#endif /* __INCLUDE_NUTTX_LCD_HX8379C_H */
