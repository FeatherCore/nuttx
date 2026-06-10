/****************************************************************************
 * include/nuttx/sensors/max30102.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_SENSORS_MAX30102_H
#define __INCLUDE_NUTTX_SENSORS_MAX30102_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/fs/ioctl.h>

#if defined(CONFIG_I2C) && defined(CONFIG_SENSORS_MAX30102)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX30102IOC_READ_ID     _SNIOC(0x50)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct i2c_master_s;

struct max30102_config_s
{
  uint8_t   address;
  uint32_t  frequency;
  FAR const char *devpath;
};

struct max30102_id_s
{
  uint8_t   part_id;
  uint8_t   revision_id;
};

struct max30102_sample_s
{
  uint32_t  red;
  uint32_t  ir;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int max30102_register(FAR struct i2c_master_s *i2c,
                      FAR const struct max30102_config_s *config);

#endif /* CONFIG_I2C && CONFIG_SENSORS_MAX30102 */
#endif /* __INCLUDE_NUTTX_SENSORS_MAX30102_H */
