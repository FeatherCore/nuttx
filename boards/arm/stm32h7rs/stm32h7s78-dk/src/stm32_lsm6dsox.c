/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/src/stm32_lsm6dsox.c
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
#include <nuttx/sensors/lsm6dsox.h>

#include "stm32_i2c.h"
#include "stm32h7s78-dk.h"

#ifdef CONFIG_STM32H7S78_DK_LSM6DSOX

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct lsm6dsox_config_s g_lsm6dsox_config =
{
  .address   = CONFIG_LSM6DSOX_I2C_ADDRESS,
  .frequency = CONFIG_LSM6DSOX_I2C_FREQUENCY,
  .devpath   = CONFIG_LSM6DSOX_DEVPATH,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_lsm6dsox_register_at(FAR struct i2c_master_s *i2c,
                                      uint8_t address)
{
  struct lsm6dsox_config_s config;

  config = g_lsm6dsox_config;
  config.address = address;

  syslog(LOG_INFO,
         "stm32h7s78-dk: probing LSM6DSOX at I2C1 addr 0x%02x\n",
         address);
  return lsm6dsox_register(i2c, &config);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_lsm6dsox_setup(void)
{
  FAR struct i2c_master_s *i2c;
  bool i2cdev_registered = false;
  int ret;

  i2c = stm32_i2cbus_initialize(1);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "stm32h7s78-dk: LSM6DSOX I2C1 init failed\n");
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

  ret = stm32_lsm6dsox_register_at(i2c, CONFIG_LSM6DSOX_I2C_ADDRESS);
  if (ret < 0 && CONFIG_LSM6DSOX_I2C_ADDRESS != 0x6a)
    {
      ret = stm32_lsm6dsox_register_at(i2c, 0x6a);
    }

  if (ret < 0 && CONFIG_LSM6DSOX_I2C_ADDRESS != 0x6b)
    {
      ret = stm32_lsm6dsox_register_at(i2c, 0x6b);
    }

  if (ret < 0)
    {
      syslog(LOG_ERR,
             "stm32h7s78-dk: LSM6DSOX not found at 0x6a/0x6b: %d\n",
             ret);

      if (!i2cdev_registered)
        {
          (void)stm32_i2cbus_uninitialize(i2c);
        }

      return ret;
    }

  syslog(LOG_INFO,
         "stm32h7s78-dk: LSM6DSOX %s registered on I2C1 "
         "SCL=D15/PB6 SDA=D14/PB9\n",
         CONFIG_LSM6DSOX_DEVPATH);
  return OK;
}

#endif /* CONFIG_STM32H7S78_DK_LSM6DSOX */
