/****************************************************************************
 * arch/arm/src/stm32u5/stm32_userspace.c
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

#include <stdint.h>
#include <assert.h>

#include <debug.h>

#include <nuttx/userspace.h>

#include "stm32_mpuinit.h"
#include "stm32_userspace.h"

#if defined(CONFIG_ARCH_BOARD_STM32U5X9J_DK) && \
    defined(CONFIG_STM32U5X9J_DK_HSPI_RAM)
extern int stm32u5x9j_hspi1_psram_initialize(void);
#endif

#ifdef CONFIG_BUILD_PROTECTED

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_userspace
 *
 * Description:
 *   For the case of the separate user-/kernel-space build, perform whatever
 *   platform specific initialization of the user memory is required.
 *   Normally this just means initializing the user space .data and .bss
 *   segments.
 *
 ****************************************************************************/

void stm32_userspace(void)
{
  uint8_t *src;
  uint8_t *dest;
  uint8_t *end;

#if defined(CONFIG_ARCH_BOARD_STM32U5X9J_DK) && \
    defined(CONFIG_STM32U5X9J_DK_HSPI_RAM)
  int ret;

  /* U5x9J-DK links user .data/.bss into HSPI1 PSRAM.  The copy/clear below
   * runs before stm32_board_initialize(), so map PSRAM here first.
   */

  ret = stm32u5x9j_hspi1_psram_initialize();
  if (ret < 0)
    {
      ferr("ERROR: STM32U5x9J-DK PSRAM userspace init failed: %d\n", ret);
      PANIC();
    }
#endif

  /* Clear all of user-space .bss */

  DEBUGASSERT(USERSPACE->us_bssstart != 0 && USERSPACE->us_bssend != 0 &&
              USERSPACE->us_bssstart <= USERSPACE->us_bssend);

  dest = (uint8_t *)USERSPACE->us_bssstart;
  end  = (uint8_t *)USERSPACE->us_bssend;

  while (dest != end)
    {
      *dest++ = 0;
    }

  /* Initialize all of user-space .data */

  DEBUGASSERT(USERSPACE->us_datasource != 0 &&
              USERSPACE->us_datastart != 0 && USERSPACE->us_dataend != 0 &&
              USERSPACE->us_datastart <= USERSPACE->us_dataend);

  src  = (uint8_t *)USERSPACE->us_datasource;
  dest = (uint8_t *)USERSPACE->us_datastart;
  end  = (uint8_t *)USERSPACE->us_dataend;

  while (dest != end)
    {
      *dest++ = *src++;
    }

  /* Configure the MPU to permit user-space access to its FLASH and RAM */

  stm32_mpuinitialize();
}

#endif /* CONFIG_BUILD_PROTECTED */
