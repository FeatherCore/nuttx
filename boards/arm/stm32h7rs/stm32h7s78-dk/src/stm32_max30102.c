/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/src/stm32_max30102.c
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
#include <nuttx/sensors/max30102.h>

#include "arm_internal.h"

#include "hardware/stm32_gpio.h"
#include "hardware/stm32_rcc.h"
#include "stm32_i2c.h"
#include "stm32h7s78-dk.h"

#ifdef CONFIG_STM32H7S78_DK_MAX30102

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BOARD_MAX30102_INT_PIN 1u /* ARDUINO D2 / CN14 pin 3 / PF1 */

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct max30102_config_s g_max30102_config =
{
  .address   = CONFIG_MAX30102_I2C_ADDRESS,
  .frequency = CONFIG_MAX30102_I2C_FREQUENCY,
  .devpath   = CONFIG_MAX30102_DEVPATH,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32_max30102_int_config(void)
{
  uint32_t regval;

  modifyreg32(STM32_RCC_AHB4ENR, 0, RCC_AHB4ENR_GPIOFEN);
  (void)getreg32(STM32_RCC_AHB4ENR);

  regval = getreg32(STM32_GPIOF_MODER);
  regval &= ~GPIO_MODE_MASK(BOARD_MAX30102_INT_PIN);
  regval |= GPIO_MODE_INPUT << GPIO_MODE_SHIFT(BOARD_MAX30102_INT_PIN);
  putreg32(regval, STM32_GPIOF_MODER);

  modifyreg32(STM32_GPIOF_OTYPER, 1u << BOARD_MAX30102_INT_PIN, 0);

  regval = getreg32(STM32_GPIOF_PUPDR);
  regval &= ~GPIO_PUPD_MASK(BOARD_MAX30102_INT_PIN);
  regval |= GPIO_PULLUP << GPIO_PUPD_SHIFT(BOARD_MAX30102_INT_PIN);
  putreg32(regval, STM32_GPIOF_PUPDR);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_max30102_setup(void)
{
  FAR struct i2c_master_s *i2c;
  bool i2cdev_registered = false;
  int ret;

  stm32_max30102_int_config();

  i2c = stm32_i2cbus_initialize(1);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "stm32h7s78-dk: MAX30102 I2C1 init failed\n");
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

  syslog(LOG_INFO, "stm32h7s78-dk: probing MAX30102 at I2C1 addr 0x%02x\n",
         CONFIG_MAX30102_I2C_ADDRESS);

  ret = max30102_register(i2c, &g_max30102_config);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32h7s78-dk: MAX30102 register failed: %d\n",
             ret);

      if (!i2cdev_registered)
        {
          (void)stm32_i2cbus_uninitialize(i2c);
        }

      return ret;
    }

  syslog(LOG_INFO,
         "stm32h7s78-dk: MAX30102 %s registered on I2C1 "
         "SCL=D15/PB6 SDA=D14/PB9 INT=D2/PF1\n",
         CONFIG_MAX30102_DEVPATH);
  return OK;
}

#endif /* CONFIG_STM32H7S78_DK_MAX30102 */
