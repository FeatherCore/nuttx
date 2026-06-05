/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/src/stm32_aw8697.c
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
#include <nuttx/input/aw8697.h>

#include "arm_internal.h"

#include "hardware/stm32_gpio.h"
#include "hardware/stm32_rcc.h"
#include "stm32_i2c.h"

#include "stm32h7s78-dk.h"

#ifdef CONFIG_STM32H7S78_DK_AW8697

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BOARD_AW8697_INT_PIN 12u /* ARDUINO D3 / CN14 pin 4 / PD12 */
#define BOARD_AW8697_RST_PIN 1u  /* ARDUINO D2 / CN14 pin 3 / PF1 */

#define STM32_GPIOF_BSRR     (STM32_GPIOF_BASE + STM32_GPIO_BSRR_OFFSET)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct aw8697_config_s g_aw8697_config =
{
  .address            = CONFIG_FF_AW8697_I2C_ADDRESS,
  .frequency          = CONFIG_FF_AW8697_I2C_FREQUENCY,
  .rated_frequency_hz = CONFIG_FF_AW8697_RATED_FREQUENCY,
  .devpath            = CONFIG_FF_AW8697_DEVPATH,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_aw8697_register_at(FAR struct i2c_master_s *i2c,
                                    uint8_t address)
{
  struct aw8697_config_s config;

  config = g_aw8697_config;
  config.address = address;

  syslog(LOG_INFO, "stm32h7s78-dk: probing AW8697 at I2C1 addr 0x%02x\n",
         address);
  return aw8697_register(i2c, &config);
}

static void stm32_aw8697_int_config(void)
{
  uint32_t regval;

  modifyreg32(STM32_RCC_AHB4ENR, 0, RCC_AHB4ENR_GPIODEN);
  (void)getreg32(STM32_RCC_AHB4ENR);

  regval = getreg32(STM32_GPIOD_MODER);
  regval &= ~GPIO_MODE_MASK(BOARD_AW8697_INT_PIN);
  regval |= GPIO_MODE_INPUT << GPIO_MODE_SHIFT(BOARD_AW8697_INT_PIN);
  putreg32(regval, STM32_GPIOD_MODER);

  modifyreg32(STM32_GPIOD_OTYPER, 1u << BOARD_AW8697_INT_PIN, 0);

  regval = getreg32(STM32_GPIOD_PUPDR);
  regval &= ~GPIO_PUPD_MASK(BOARD_AW8697_INT_PIN);
  regval |= GPIO_PULLUP << GPIO_PUPD_SHIFT(BOARD_AW8697_INT_PIN);
  putreg32(regval, STM32_GPIOD_PUPDR);
}

static void stm32_aw8697_reset_config(void)
{
  uint32_t regval;

  modifyreg32(STM32_RCC_AHB4ENR, 0, RCC_AHB4ENR_GPIOFEN);
  (void)getreg32(STM32_RCC_AHB4ENR);

  putreg32(1u << (BOARD_AW8697_RST_PIN + 16u), STM32_GPIOF_BSRR);

  regval = getreg32(STM32_GPIOF_MODER);
  regval &= ~GPIO_MODE_MASK(BOARD_AW8697_RST_PIN);
  regval |= GPIO_MODE_OUTPUT << GPIO_MODE_SHIFT(BOARD_AW8697_RST_PIN);
  putreg32(regval, STM32_GPIOF_MODER);

  modifyreg32(STM32_GPIOF_OTYPER, 1u << BOARD_AW8697_RST_PIN, 0);

  regval = getreg32(STM32_GPIOF_OSPEEDR);
  regval &= ~GPIO_SPEED_MASK(BOARD_AW8697_RST_PIN);
  regval |= GPIO_SPEED_HIGH << GPIO_SPEED_SHIFT(BOARD_AW8697_RST_PIN);
  putreg32(regval, STM32_GPIOF_OSPEEDR);

  regval = getreg32(STM32_GPIOF_PUPDR);
  regval &= ~GPIO_PUPD_MASK(BOARD_AW8697_RST_PIN);
  regval |= GPIO_PULLUP << GPIO_PUPD_SHIFT(BOARD_AW8697_RST_PIN);
  putreg32(regval, STM32_GPIOF_PUPDR);

  up_mdelay(2);
  putreg32(1u << BOARD_AW8697_RST_PIN, STM32_GPIOF_BSRR);
  up_mdelay(5);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_aw8697_setup(void)
{
  FAR struct i2c_master_s *i2c;
  bool i2cdev_registered = false;
  int ret;

  stm32_aw8697_int_config();
  stm32_aw8697_reset_config();

  i2c = stm32_i2cbus_initialize(1);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "stm32h7s78-dk: AW8697 I2C1 init failed\n");
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

  ret = stm32_aw8697_register_at(i2c, CONFIG_FF_AW8697_I2C_ADDRESS);
  if (ret < 0 && CONFIG_FF_AW8697_I2C_ADDRESS != 0x5a)
    {
      ret = stm32_aw8697_register_at(i2c, 0x5a);
    }

  if (ret < 0 && CONFIG_FF_AW8697_I2C_ADDRESS != 0x5b)
    {
      ret = stm32_aw8697_register_at(i2c, 0x5b);
    }

  if (ret < 0)
    {
      syslog(LOG_ERR,
             "stm32h7s78-dk: AW8697 not found at 0x5a/0x5b: %d\n",
             ret);

      if (!i2cdev_registered)
        {
          (void)stm32_i2cbus_uninitialize(i2c);
        }

      return ret;
    }

  syslog(LOG_INFO,
         "stm32h7s78-dk: AW8697 %s registered on I2C1 "
         "SCL=D15/PB6 SDA=D14/PB9 INT=D3/PD12 RSTN=D2/PF1\n",
         CONFIG_FF_AW8697_DEVPATH);
  return OK;
}

#endif /* CONFIG_STM32H7S78_DK_AW8697 */
