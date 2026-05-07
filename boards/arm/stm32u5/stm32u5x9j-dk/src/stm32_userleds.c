/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32_userleds.c
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

#include <stdbool.h>
#include <sys/param.h>

#include <nuttx/board.h>

#include <arch/board/board.h>

#include "stm32_gpio.h"
#include "stm32u5x9j-dk.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint32_t g_ledcfg[BOARD_NLEDS] =
{
  GPIO_LED_GREEN,
  GPIO_LED_RED,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_ARCH_LEDS
void board_autoled_initialize(void)
{
  int i;

  for (i = 0; i < nitems(g_ledcfg); i++)
    {
      stm32_configgpio(g_ledcfg[i]);
    }
}

void board_autoled_on(int led)
{
  switch (led)
    {
      case LED_HEAPALLOCATE:
        stm32_gpiowrite(GPIO_LED_RED, true);
        stm32_gpiowrite(GPIO_LED_GREEN, false);
        break;

      case LED_IRQSENABLED:
        stm32_gpiowrite(GPIO_LED_RED, true);
        stm32_gpiowrite(GPIO_LED_GREEN, true);
        break;

      case LED_STACKCREATED:
      case LED_IDLE:
        stm32_gpiowrite(GPIO_LED_RED, false);
        stm32_gpiowrite(GPIO_LED_GREEN, true);
        break;

      case LED_INIRQ:
      case LED_SIGNAL:
      case LED_ASSERTION:
      case LED_PANIC:
        stm32_gpiowrite(GPIO_LED_RED, true);
        break;

      default:
        break;
    }
}

void board_autoled_off(int led)
{
  switch (led)
    {
      case LED_INIRQ:
      case LED_SIGNAL:
      case LED_ASSERTION:
        stm32_gpiowrite(GPIO_LED_RED, false);
        break;

      case LED_PANIC:
        stm32_gpiowrite(GPIO_LED_RED, false);
        break;

      default:
        break;
    }
}
#endif /* CONFIG_ARCH_LEDS */

#ifndef CONFIG_ARCH_LEDS

uint32_t board_userled_initialize(void)
{
  int i;

  for (i = 0; i < nitems(g_ledcfg); i++)
    {
      stm32_configgpio(g_ledcfg[i]);
    }

  return BOARD_NLEDS;
}

void board_userled(int led, bool ledon)
{
  if ((unsigned int)led < nitems(g_ledcfg))
    {
      stm32_gpiowrite(g_ledcfg[led], ledon);
    }
}

void board_userled_all(uint32_t ledset)
{
  int i;

  for (i = 0; i < nitems(g_ledcfg); i++)
    {
      stm32_gpiowrite(g_ledcfg[i], (ledset & (1 << i)) != 0);
    }
}

#endif /* !CONFIG_ARCH_LEDS */
