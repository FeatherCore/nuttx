/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32_touchscreen.c
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

#include <nuttx/i2c/i2c_master.h>
#include <nuttx/input/sitronix.h>

#include "stm32_exti.h"
#include "stm32_gpio.h"
#include "stm32u5x9j-dk.h"

#ifdef CONFIG_STM32U5X9J_DK_TOUCH

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32U5X9J_SITRONIX_ADDR      0x70
#define STM32U5X9J_SITRONIX_POLL_MSEC 30
#define STM32U5X9J_SITRONIX_XRES      480
#define STM32U5X9J_SITRONIX_YRES      480

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32u5x9j_touch_attach(
                         FAR const struct sitronix_config_s *config,
                         xcpt_t isr, FAR void *arg)
{
  return stm32_gpiosetevent(GPIO_TS_INT, false, true, true, isr, arg);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32u5x9j_touch_initialize(FAR struct i2c_master_s *i2c5)
{
  static const struct sitronix_config_s config =
    {
      .address   = STM32U5X9J_SITRONIX_ADDR,
      .frequency = I2C_SPEED_FAST,
      .xres      = STM32U5X9J_SITRONIX_XRES,
      .yres      = STM32U5X9J_SITRONIX_YRES,
      .poll_ms   = STM32U5X9J_SITRONIX_POLL_MSEC,
      .attach    = stm32u5x9j_touch_attach,
    };

  if (i2c5 == NULL)
    {
      return -ENODEV;
    }

  stm32_configgpio(GPIO_TS_INT);
  return sitronix_register(i2c5, &config, 0);
}

#endif /* CONFIG_STM32U5X9J_DK_TOUCH */
