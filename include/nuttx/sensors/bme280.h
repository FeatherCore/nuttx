/****************************************************************************
 * include/nuttx/sensors/bme280.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_SENSORS_BME280_H
#define __INCLUDE_NUTTX_SENSORS_BME280_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/fs/ioctl.h>

#if defined(CONFIG_I2C) && defined(CONFIG_SENSORS_BME280)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BME280IOC_READ_ID _SNIOC(0x40)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct i2c_master_s;

struct bme280_config_s
{
  uint8_t address;
  uint32_t frequency;
  FAR const char *devpath;
};

struct bme280_sample_s
{
  int32_t   temperature; /* 0.01 degrees Celsius */
  uint32_t  pressure;    /* Q24.8 Pascal */
  uint32_t  humidity;    /* Q22.10 relative humidity percent */
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

int bme280_register(FAR struct i2c_master_s *i2c,
                    FAR const struct bme280_config_s *config);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_I2C && CONFIG_SENSORS_BME280 */
#endif /* __INCLUDE_NUTTX_SENSORS_BME280_H */
