/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/src/stm32_bme280.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdint.h>
#include <syslog.h>

#include <nuttx/i2c/i2c_master.h>
#include <nuttx/sensors/bme280.h>

#include "stm32_i2c.h"
#include "stm32h7s78-dk.h"

#ifdef CONFIG_STM32H7S78_DK_BME280

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct bme280_config_s g_bme280_config =
{
  .address   = CONFIG_BME280_I2C_ADDRESS,
  .frequency = CONFIG_BME280_I2C_FREQUENCY,
  .devpath   = CONFIG_BME280_DEVPATH,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_bme280_register_at(FAR struct i2c_master_s *i2c,
                                    uint8_t address)
{
  struct bme280_config_s config;

  config = g_bme280_config;
  config.address = address;

  syslog(LOG_INFO, "stm32h7s78-dk: probing BME280 at I2C1 addr 0x%02x\n",
         address);
  return bme280_register(i2c, &config);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_bme280_setup(void)
{
  FAR struct i2c_master_s *i2c;
  bool i2cdev_registered = false;
  int ret;

  i2c = stm32_i2cbus_initialize(1);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "stm32h7s78-dk: BME280 I2C1 init failed\n");
      return -ENODEV;
    }

#ifdef CONFIG_I2C_DRIVER
  ret = i2c_register(i2c, 1);
  if (ret < 0 && ret != -EEXIST)
    {
      syslog(LOG_ERR, "stm32h7s78-dk: /dev/i2c1 register failed: %d\n",
             ret);
    }
  else
    {
      i2cdev_registered = true;
    }
#endif

  ret = stm32_bme280_register_at(i2c, CONFIG_BME280_I2C_ADDRESS);
  if (ret < 0 && CONFIG_BME280_I2C_ADDRESS != 0x76)
    {
      ret = stm32_bme280_register_at(i2c, 0x76);
    }

  if (ret < 0 && CONFIG_BME280_I2C_ADDRESS != 0x77)
    {
      ret = stm32_bme280_register_at(i2c, 0x77);
    }

  if (ret < 0)
    {
      syslog(LOG_ERR,
             "stm32h7s78-dk: BME280 not found at 0x76/0x77: %d\n",
             ret);

      if (!i2cdev_registered)
        {
          (void)stm32_i2cbus_uninitialize(i2c);
        }

      return ret;
    }

  syslog(LOG_INFO,
         "stm32h7s78-dk: BME280 %s registered on I2C1 "
         "SCL=D15/PB6 SDA=D14/PB9\n",
         CONFIG_BME280_DEVPATH);
  return OK;
}

#endif /* CONFIG_STM32H7S78_DK_BME280 */
