/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/src/stm32_touchscreen.c
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

#include <arch/board/board.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/input/gt911.h>
#include <nuttx/input/touchscreen.h>

#include "arm_internal.h"

#include "hardware/stm32_gpio.h"
#include "hardware/stm32_rcc.h"
#include "stm32_i2c.h"

#include "stm32h7s78-dk.h"

#ifdef CONFIG_STM32H7S78_DK_GT911

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BOARD_GT911_ADDR           0x5du  /* Cube TS_I2C_ADDRESS 0xba >> 1 */
#define BOARD_GT911_I2C_FREQUENCY  100000u
#define BOARD_GT911_INT_PIN        3u

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct gt911_config_s g_gt911_config =
{
  .address   = BOARD_GT911_ADDR,
  .frequency = BOARD_GT911_I2C_FREQUENCY,
  .xres      = BOARD_LTDC_WIDTH,
  .yres      = BOARD_LTDC_HEIGHT,
  .poll_ms   = CONFIG_STM32H7S78_DK_GT911_POLL_MS,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32_gt911_int_config(void)
{
  uint32_t regval;

  modifyreg32(STM32_RCC_AHB4ENR, 0, RCC_AHB4ENR_GPIOEEN);
  (void)getreg32(STM32_RCC_AHB4ENR);

  regval = getreg32(STM32_GPIOE_MODER);
  regval &= ~GPIO_MODE_MASK(BOARD_GT911_INT_PIN);
  regval |= GPIO_MODE_INPUT << GPIO_MODE_SHIFT(BOARD_GT911_INT_PIN);
  putreg32(regval, STM32_GPIOE_MODER);

  modifyreg32(STM32_GPIOE_OTYPER, 1u << BOARD_GT911_INT_PIN, 0);

  regval = getreg32(STM32_GPIOE_PUPDR);
  regval &= ~GPIO_PUPD_MASK(BOARD_GT911_INT_PIN);
  regval |= GPIO_PULLUP << GPIO_PUPD_SHIFT(BOARD_GT911_INT_PIN);
  putreg32(regval, STM32_GPIOE_PUPDR);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_touchscreen_setup(void)
{
  FAR struct i2c_master_s *i2c;
  int ret;

  stm32_gt911_int_config();

  i2c = stm32_i2cbus_initialize(1);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "stm32h7s78-dk: I2C1 init failed\n");
      return -ENODEV;
    }

  ret = gt911_register(i2c, &g_gt911_config, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32h7s78-dk: GT911 register failed: %d\n", ret);
      (void)stm32_i2cbus_uninitialize(i2c);
    }
  else
    {
      syslog(LOG_INFO, "stm32h7s78-dk: GT911 /dev/input0 registered\n");
    }

  return ret;
}

#endif /* CONFIG_STM32H7S78_DK_GT911 */
