/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/mount.h>
#include <sys/types.h>
#include <syslog.h>
#include <nuttx/debug.h>

#include <nuttx/board.h>
#include <nuttx/input/buttons.h>
#include <nuttx/leds/userled.h>

#include "stm32u5x9j-dk.h"

#include <arch/board/board.h>

#ifdef HAVE_I2C_DRIVER
#  include "stm32_i2c.h"
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_STM32U5X9J_DK_I2C_BUSES
static FAR struct i2c_master_s *stm32_i2c_register_bus(int bus)
{
  FAR struct i2c_master_s *i2c;
  int ret;

  i2c = stm32_i2cbus_initialize(bus);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize I2C%d\n", bus);
      return NULL;
    }

  ret = i2c_register(i2c, bus);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to register /dev/i2c%d: %d\n",
             bus, ret);
      stm32_i2cbus_uninitialize(i2c);
      return NULL;
    }

  return i2c;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Should not be here, but there is a bug in clock_initalize logic that
 * prevents external RTC to be used when no internal RTC !
 * **************************************************************************/
#if defined(CONFIG_RTC) && defined(CONFIG_RTC_EXTERNAL)
int up_rtc_initialize(void)
{
  return OK;
}
#endif

/****************************************************************************
 * Name: stm32_bringup
 *
 * Description:
 *   Perform architecture-specific initialization
 *
 *   CONFIG_BOARD_LATE_INITIALIZE=y :
 *     Called from board_late_initialize().
 *
 ****************************************************************************/

int stm32_bringup(void)
{
  FAR struct i2c_master_s *i2c3 = NULL;
  FAR struct i2c_master_s *i2c5 = NULL;
  int ret = OK;

#ifdef CONFIG_FS_PROCFS
  /* Mount the procfs file system */

  ret = mount(NULL, "/proc", "procfs", 0, NULL);
  if (ret < 0)
    {
      ferr("ERROR: Failed to mount procfs at /proc: %d\n", ret);
    }
#endif

#if !defined(CONFIG_ARCH_LEDS) && defined(CONFIG_USERLED_LOWER)
  ret = userled_lower_initialize("/dev/userleds");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: userled_lower_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_INPUT_BUTTONS
#ifdef CONFIG_INPUT_BUTTONS_LOWER
  ret = btn_lower_initialize("/dev/buttons");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: btn_lower_initialize() failed: %d\n", ret);
    }
#else
  board_button_initialize();
#endif
#endif /* CONFIG_INPUT_BUTTONS */

#ifdef CONFIG_STM32U5X9J_DK_HSPI_RAM
  ret = stm32u5x9j_hspi_ram_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: HSPI PSRAM bring-up failed: %d\n", ret);
    }
#ifdef CONFIG_STM32U5X9J_DK_HSPI_DIAG
  else
    {
      ret = stm32u5x9j_hspi_diag_initialize();
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: HSPI PSRAM diag failed: %d\n", ret);
        }
    }
#endif
#endif

#ifdef CONFIG_STM32U5X9J_DK_OSPI_NOR
  ret = stm32u5x9j_flash_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: OSPI NOR bring-up failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_STM32U5X9J_DK_EMMC
  ret = stm32u5x9j_emmc_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: eMMC bring-up failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_STM32U5X9J_DK_LCD
  ret = stm32u5x9j_lcd_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: LCD bring-up failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_STM32U5X9J_DK_I2C_BUSES
#ifdef CONFIG_STM32U5_I2C2
  stm32_i2c_register_bus(2);
#endif
#ifdef CONFIG_STM32U5_I2C3
  i2c3 = stm32_i2c_register_bus(3);
#endif
#ifdef CONFIG_STM32U5_I2C4
  stm32_i2c_register_bus(4);
#endif
#ifdef CONFIG_STM32U5_I2C5
  i2c5 = stm32_i2c_register_bus(5);
#endif
#endif /* CONFIG_STM32U5X9J_DK_I2C_BUSES */

#ifdef CONFIG_STM32U5X9J_DK_TOUCH
  if (i2c5 != NULL)
    {
      ret = stm32u5x9j_touch_initialize(i2c5);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: touchscreen init failed: %d\n", ret);
        }
    }
  else
    {
      syslog(LOG_ERR, "ERROR: I2C5 missing, skip touchscreen\n");
    }
#endif

#ifdef HAVE_I2C_PROBES
  if (i2c3 != NULL && i2c5 != NULL)
    {
      ret = stm32u5x9j_i2c_probes_initialize(i2c3, i2c5);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: I2C board probes failed: %d\n", ret);
        }
    }
  else
    {
      syslog(LOG_ERR, "ERROR: I2C3/I2C5 missing, skip board probes\n");
    }
#endif

  UNUSED(ret);
  return OK;
}
