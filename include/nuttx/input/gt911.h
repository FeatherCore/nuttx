/****************************************************************************
 * include/nuttx/input/gt911.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_INPUT_GT911_H
#define __INCLUDE_NUTTX_INPUT_GT911_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/i2c/i2c_master.h>
#include <nuttx/input/touchscreen.h>

#ifdef CONFIG_INPUT_GT911

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct gt911_config_s
{
  uint8_t  address;    /* 7-bit I2C address */
  uint8_t  flags;      /* TOUCH_FLAG_* */
  uint16_t xres;       /* Horizontal resolution in pixels */
  uint16_t yres;       /* Vertical resolution in pixels */
  uint16_t poll_ms;    /* Poll interval in milliseconds */
  uint32_t frequency;  /* I2C bus frequency */
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
 * Name: gt911_register
 *
 * Description:
 *   Register a Goodix GT911 touchscreen as /dev/inputN.
 *
 * Input Parameters:
 *   i2c    - I2C bus instance
 *   config - Board-provided touchscreen configuration
 *   minor  - Input device minor number
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int gt911_register(FAR struct i2c_master_s *i2c,
                   FAR const struct gt911_config_s *config, int minor);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_INPUT_GT911 */
#endif /* __INCLUDE_NUTTX_INPUT_GT911_H */
