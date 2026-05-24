/****************************************************************************
 * arch/arm/src/stm32u5/stm32_mpuinit.c
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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/param.h>

#include <nuttx/userspace.h>

#include "mpu.h"
#include "stm32_mpuinit.h"
#include "hardware/stm32_memorymap.h"

#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_ARM_MPU)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32U5_INTSRAM_BASE        ((uintptr_t)0x20000000)
#define STM32U5_INTSRAM_LIMIT       ((uintptr_t)0x30000000)

#ifdef CONFIG_STM32U5_PSRAM_MPU_SHARE_NONE
#  define STM32U5_USER_EXTSRAM_SHARE MPU_RBAR_SH_NO
#else
#  define STM32U5_USER_EXTSRAM_SHARE MPU_RBAR_SH_OUTER
#endif

#ifdef CONFIG_STM32U5_PSRAM_MPU_NONCACHEABLE
#  define STM32U5_USER_EXTSRAM_ATTR MPU_RLAR_NONCACHEABLE
#elif defined(CONFIG_STM32U5_PSRAM_MPU_WRITE_THROUGH)
#  ifdef CONFIG_STM32U5_PSRAM_MPU_NO_WRITE_ALLOCATE
#    define STM32U5_USER_EXTSRAM_ATTR MPU_RLAR_WRITE_THROUGH_NWA
#  else
#    define STM32U5_USER_EXTSRAM_ATTR MPU_RLAR_WRITE_THROUGH
#  endif
#else
#  ifdef CONFIG_STM32U5_PSRAM_MPU_NO_WRITE_ALLOCATE
#    define STM32U5_USER_EXTSRAM_ATTR MPU_RLAR_WRITE_BACK_NWA
#  else
#    define STM32U5_USER_EXTSRAM_ATTR MPU_RLAR_WRITE_BACK
#  endif
#endif

#define STM32U5_USER_EXTSRAM \
  (MPU_RBAR_AP_RWRW | STM32U5_USER_EXTSRAM_SHARE | MPU_RBAR_XN)

#define STM32U5_USER_EXTSRAM_RLAR \
  (STM32U5_USER_EXTSRAM_ATTR | MPU_RLAR_PXN)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool stm32_mpu_is_internal_sram(uintptr_t start, size_t size)
{
  uintptr_t end;

  if (size == 0)
    {
      return false;
    }

  end = start + size - 1;
  if (end < start)
    {
      return false;
    }

  return start >= STM32U5_INTSRAM_BASE && end < STM32U5_INTSRAM_LIMIT;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_mpuinitialize
 *
 * Description:
 *   Configure the MPU to permit user-space access to only restricted SAM3U
 *   resources.
 *
 ****************************************************************************/

void stm32_mpuinitialize(void)
{
  uintptr_t datastart = MIN(USERSPACE->us_datastart, USERSPACE->us_bssstart);
  uintptr_t dataend   = MAX(USERSPACE->us_dataend,   USERSPACE->us_bssend);

  DEBUGASSERT(USERSPACE->us_textend >= USERSPACE->us_textstart &&
              dataend >= datastart);

  /* Show MPU information */

  mpu_showtype();

  /* Configure user flash and SRAM space */

  mpu_user_flash(USERSPACE->us_textstart,
                 USERSPACE->us_textend - USERSPACE->us_textstart);

  mpu_user_intsram(datastart, dataend - datastart);

  /* Then enable the MPU */

  mpu_control(true, false, true);
}

/****************************************************************************
 * Name: stm32_mpu_uheap
 *
 * Description:
 *  Map the user-heap region.
 *
 *  This logic may need an extension to handle external SDRAM).
 *
 ****************************************************************************/

void stm32_mpu_uheap(uintptr_t start, size_t size)
{
  if (stm32_mpu_is_internal_sram(start, size))
    {
      mpu_user_intsram(start, size);
    }
  else
    {
      mpu_configure_region(start, size, STM32U5_USER_EXTSRAM,
                           STM32U5_USER_EXTSRAM_RLAR);
    }
}

/****************************************************************************
 * Name: stm32_mpu_ufbmem
 *
 * Description:
 *  Map a framebuffer region for user-space fbdev clients.
 *
 ****************************************************************************/

void stm32_mpu_ufbmem(uintptr_t start, size_t size)
{
  if (stm32_mpu_is_internal_sram(start, size))
    {
      mpu_user_intsram(start, size);
    }
  else
    {
      mpu_configure_region(start, size, STM32U5_USER_EXTSRAM,
                           STM32U5_USER_EXTSRAM_RLAR);
    }
}

#endif /* CONFIG_BUILD_PROTECTED && CONFIG_ARM_MPU */
