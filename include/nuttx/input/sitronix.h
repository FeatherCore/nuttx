/****************************************************************************
 * include/nuttx/input/sitronix.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_INPUT_SITRONIX_H
#define __INCLUDE_NUTTX_INPUT_SITRONIX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <nuttx/i2c/i2c_master.h>
#include <nuttx/irq.h>

#ifdef CONFIG_INPUT_SITRONIX

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct sitronix_config_s
{
  uint8_t  address;    /* 7-bit I2C address */
  uint8_t  flags;      /* TOUCH_FLAG_* */
  uint16_t xres;       /* Horizontal resolution in pixels */
  uint16_t yres;       /* Vertical resolution in pixels */
  uint16_t poll_ms;    /* Poll interval in milliseconds */
  uint32_t frequency;  /* I2C bus frequency */

  CODE int (*attach)(FAR const struct sitronix_config_s *config,
                     xcpt_t isr, FAR void *arg);
  CODE void (*enable)(FAR const struct sitronix_config_s *config,
                      bool enable);
  CODE void (*clear)(FAR const struct sitronix_config_s *config);
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
 * Name: sitronix_register
 *
 * Description:
 *   Register a Sitronix I2C touchscreen as /dev/inputN.
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

int sitronix_register(FAR struct i2c_master_s *i2c,
                      FAR const struct sitronix_config_s *config,
                      int minor);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_INPUT_SITRONIX */
#endif /* __INCLUDE_NUTTX_INPUT_SITRONIX_H */
