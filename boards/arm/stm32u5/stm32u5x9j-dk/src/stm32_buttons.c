/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32_buttons.c
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

#include <errno.h>
#include <stdint.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>

#include <arch/board/board.h>

#include "stm32_gpio.h"
#include "stm32u5x9j-dk.h"

#ifdef CONFIG_ARCH_BUTTONS

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint32_t g_buttons[NUM_BUTTONS] =
{
  GPIO_BTN_USER
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

uint32_t board_button_initialize(void)
{
  int i;

  for (i = 0; i < NUM_BUTTONS; i++)
    {
      stm32_configgpio(g_buttons[i]);
    }

  return NUM_BUTTONS;
}

uint32_t board_buttons(void)
{
  uint32_t ret = 0;
  int i;

  for (i = 0; i < NUM_BUTTONS; i++)
    {
      if (stm32_gpioread(g_buttons[i]))
        {
          ret |= (1 << i);
        }
    }

  return ret;
}

#ifdef CONFIG_ARCH_IRQBUTTONS
int board_button_irq(int id, xcpt_t irqhandler, void *arg)
{
  if (id < MIN_IRQBUTTON || id > MAX_IRQBUTTON)
    {
      return -EINVAL;
    }

  return stm32_gpiosetevent(g_buttons[id], true, true, true,
                            irqhandler, arg);
}
#endif

#endif /* CONFIG_ARCH_BUTTONS */
