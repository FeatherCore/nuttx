/****************************************************************************
 * include/nuttx/sensors/lsm6dsox.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_SENSORS_LSM6DSOX_H
#define __INCLUDE_NUTTX_SENSORS_LSM6DSOX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/sensors/ioctl.h>

#include <stdint.h>

#if defined(CONFIG_I2C) && defined(CONFIG_SENSORS_LSM6DSOX)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LSM6DSOXIOC_READ_ID _SNIOC(0x60)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct i2c_master_s;

struct lsm6dsox_config_s
{
  uint8_t address;
  uint32_t frequency;
  FAR const char *devpath;
};

struct lsm6dsox_id_s
{
  uint8_t whoami;
};

struct lsm6dsox_sample_s
{
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
  int16_t temperature;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int lsm6dsox_register(FAR struct i2c_master_s *i2c,
                      FAR const struct lsm6dsox_config_s *config);

#endif /* CONFIG_I2C && CONFIG_SENSORS_LSM6DSOX */
#endif /* __INCLUDE_NUTTX_SENSORS_LSM6DSOX_H */
