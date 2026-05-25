/****************************************************************************
 * arch/arm/src/stm32u5/stm32_addregion.c
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

#include <sys/types.h>
#include <stdint.h>
#include <string.h>

#include <nuttx/compiler.h>
#include <nuttx/kmalloc.h>

#include "arm_internal.h"
#include "stm32_mpuinit.h"
#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_STM32U5_FSMC
#  undef CONFIG_STM32U5_FSMC_SRAM
#endif

/* Set the range of SRAM2 as well, requires a second memory region */

#define SRAM2_START  STM32_SRAM2_BASE
#define SRAM2_END    (SRAM2_START + STM32_SRAM2_SIZE)

/* Set the range of SRAM3, requiring a third memory region */

#ifdef STM32_SRAM3_SIZE
#  define SRAM3_START  STM32_SRAM3_BASE
#  define SRAM3_END    (SRAM3_START + STM32_SRAM3_SIZE)
#endif

#ifdef STM32_SRAM5_SIZE
#  define SRAM5_START  STM32_SRAM5_BASE
#  define SRAM5_END    (SRAM5_START + STM32_SRAM5_SIZE)
#endif

/* Some sanity checking.  If multiple memory regions are defined, verify
 * that CONFIG_MM_REGIONS is set to match at least the STM32U5 common regions
 * selected here.  Boards may override arm_addregion() and add their own
 * external memory regions after calling stm32_addregion().
 */

#if CONFIG_MM_REGIONS < defined(CONFIG_STM32U5_SRAM2_HEAP) + \
                        defined(CONFIG_STM32U5_SRAM3_HEAP) + \
                        defined(CONFIG_STM32U5_SRAM5_HEAP) + \
                        defined(CONFIG_STM32U5_FSMC_SRAM_HEAP) + 1
#  error "You need more memory manager regions to support selected heap components"
#endif

/* If FSMC SRAM is going to be used as heap, then verify that the starting
 * address and size of the external SRAM region has been provided in the
 * configuration (as CONFIG_HEAP2_BASE and CONFIG_HEAP2_SIZE).
 */

#ifdef CONFIG_STM32U5_FSMC_SRAM
#  if !defined(CONFIG_HEAP2_BASE) || !defined(CONFIG_HEAP2_SIZE)
#    error "CONFIG_HEAP2_BASE and CONFIG_HEAP2_SIZE must be provided"
#    undef CONFIG_STM32U5_FSMC_SRAM
#  endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_heap_color
 *
 * Description:
 *   Set heap memory to a known, non-zero state to checking heap usage.
 *
 ****************************************************************************/

#ifdef CONFIG_HEAP_COLORATION
static inline void up_heap_color(void *start, size_t size)
{
  memset(start, HEAP_COLOR, size);
}
#else
#  define up_heap_color(start, size)
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_addregion
 *
 * Description:
 *   Add STM32U5 common non-contiguous heap regions.
 *
 ****************************************************************************/

#if CONFIG_MM_REGIONS > 1
void stm32_addregion(void)
{
#ifdef CONFIG_STM32U5_SRAM2_HEAP

#  if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP)

  /* Allow user-mode access to the SRAM2 heap */

  stm32_mpu_uheap((uintptr_t)SRAM2_START, SRAM2_END - SRAM2_START);

#  endif

  /* Colorize the heap for debug */

  up_heap_color((void *)SRAM2_START, SRAM2_END - SRAM2_START);

  /* Add the SRAM2 user heap region. */

  kumm_addregion((void *)SRAM2_START, SRAM2_END - SRAM2_START);

#endif /* SRAM2 */

#ifdef CONFIG_STM32U5_SRAM3_HEAP

#  if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP)

  /* Allow user-mode access to the SRAM3 heap */

  stm32_mpu_uheap((uintptr_t)SRAM3_START, SRAM3_END - SRAM3_START);

#  endif

  /* Colorize the heap for debug */

  up_heap_color((void *)SRAM3_START, SRAM3_END - SRAM3_START);

  /* Add the SRAM3 user heap region. */

  kumm_addregion((void *)SRAM3_START, SRAM3_END - SRAM3_START);

#endif /* SRAM3 */

#ifdef CONFIG_STM32U5_SRAM5_HEAP

#  if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP)

  /* Allow user-mode access to the SRAM5 heap */

  stm32_mpu_uheap((uintptr_t)SRAM5_START, STM32_SRAM5_SIZE);

#  endif

  /* Colorize the heap for debug */

  up_heap_color((void *)SRAM5_START, STM32_SRAM5_SIZE);

  /* Add the SRAM5 user heap region. */

  kumm_addregion((void *)SRAM5_START, STM32_SRAM5_SIZE);

#endif /* SRAM5 */

#ifdef CONFIG_STM32U5_FSMC_SRAM_HEAP
#  if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP)

  /* Allow user-mode access to the FSMC SRAM user heap memory */

  stm32_mpu_uheap((uintptr_t)CONFIG_HEAP2_BASE, CONFIG_HEAP2_SIZE);

#  endif

  /* Colorize the heap for debug */

  up_heap_color((void *)CONFIG_HEAP2_BASE, CONFIG_HEAP2_SIZE);

  /* Add the external FSMC SRAM user heap region. */

  kumm_addregion((void *)CONFIG_HEAP2_BASE, CONFIG_HEAP2_SIZE);
#endif
}

/****************************************************************************
 * Name: arm_addregion
 *
 * Description:
 *   Add non-contiguous heap regions.  Boards with external RAM may override
 *   this weak default from their board source directory and call
 *   stm32_addregion() for the common STM32U5 regions.
 *
 ****************************************************************************/

void weak_function arm_addregion(void)
{
  stm32_addregion();
}
#endif
