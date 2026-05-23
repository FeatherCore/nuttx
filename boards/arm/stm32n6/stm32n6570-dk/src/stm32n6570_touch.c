/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32n6570_touch.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <syslog.h>

#include <arch/board/board.h>
#include <nuttx/arch.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/input/gt911.h>
#include <nuttx/input/touchscreen.h>

#include "arm_internal.h"

#include "stm32n6_gpio.h"
#include "stm32n6_i2c.h"
#include "stm32n6570-dk.h"

#ifdef CONFIG_STM32N6570_DK_GT911

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GT911_ADDR             0x5du  /* Cube TS_I2C_ADDRESS 0xba >> 1 */
#define GT911_I2C_FREQUENCY    100000u

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct gt911_config_s g_gt911_config =
{
  .address   = GT911_ADDR,
  .frequency = GT911_I2C_FREQUENCY,
  .xres      = STM32N6570_LCD_WIDTH,
  .yres      = STM32N6570_LCD_HEIGHT,
  .poll_ms   = CONFIG_STM32N6570_DK_GT911_POLL_MS,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32n6570_gt911_gpio_config(void)
{
  int ret;

  ret = stm32n6_configgpio(GPIO_I2C2_SCL);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32n6_configgpio(GPIO_I2C2_SDA);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32n6_configgpio(GPIO_GT911_INT);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32n6_configgpio(GPIO_GT911_NRST);
  if (ret < 0)
    {
      return ret;
    }

  up_mdelay(50);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6570_touch_setup(void)
{
  FAR struct i2c_master_s *i2c;
  int ret;

  ret = stm32n6570_gt911_gpio_config();
  if (ret < 0)
    {
      return ret;
    }

  i2c = stm32n6_i2cbus_initialize(2);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "stm32n6570-dk: I2C2 init failed\n");
      return -ENODEV;
    }

  ret = gt911_register(i2c, &g_gt911_config, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32n6570-dk: GT911 register failed: %d\n", ret);
      (void)stm32n6_i2cbus_uninitialize(i2c);
    }
  else
    {
      syslog(LOG_INFO, "stm32n6570-dk: GT911 /dev/input0 registered\n");
    }

  return ret;
}

#endif /* CONFIG_STM32N6570_DK_GT911 */
