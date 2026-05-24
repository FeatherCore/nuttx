/****************************************************************************
 * arch/arm/src/stm32u5/stm32_icache.c
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
#include <nuttx/cache.h>
#include <nuttx/compiler.h>

#include <arch/barriers.h>

#include <stdbool.h>
#include <stdint.h>

#include "stm32.h"
#include "stm32_icache.h"
#include "hardware/stm32_icache.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32U5_ICACHE_SIZE            8192

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_icache_initialized;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32_icache_initialize(void)
{
  uint32_t regval;

  /* CubeU5 STM32U5x9J examples select ICACHE_1WAY before HAL_ICACHE_Enable().
   */

  regval = getreg32(STM32_ICACHE_CR);

#ifdef CONFIG_STM32U5_ICACHE_DIRECT
  regval &= ~ICACHE_CR_WAYSEL;
#else
  regval |= ICACHE_CR_WAYSEL;
#endif

  putreg32(regval, STM32_ICACHE_CR);
  putreg32(ICACHE_FCR_CBSYENDF | ICACHE_FCR_CERRF, STM32_ICACHE_FCR);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

size_t stm32_get_icache_linesize(void)
{
  return STM32U5_ICACHE_LINESIZE;
}

size_t stm32_get_icache_size(void)
{
  return STM32U5_ICACHE_SIZE;
}

void stm32_enable_icache(void)
{
  if (!g_icache_initialized)
    {
      stm32_icache_initialize();
      g_icache_initialized = true;
    }

  modifyreg32(STM32_ICACHE_CR, 0, ICACHE_CR_EN);
  UP_MB();
}

void stm32_disable_icache(void)
{
  putreg32(ICACHE_FCR_CBSYENDF, STM32_ICACHE_FCR);
  modifyreg32(STM32_ICACHE_CR, ICACHE_CR_EN, 0);

  while ((getreg32(STM32_ICACHE_CR) & ICACHE_CR_EN) != 0)
    {
    }

  UP_MB();
}

void stm32_invalidate_icache(void)
{
  putreg32(ICACHE_FCR_CBSYENDF, STM32_ICACHE_FCR);
  modifyreg32(STM32_ICACHE_CR, 0, ICACHE_CR_CACHEINV);

  while ((getreg32(STM32_ICACHE_SR) & ICACHE_SR_BUSYF) != 0)
    {
    }

  putreg32(ICACHE_FCR_CBSYENDF, STM32_ICACHE_FCR);
  UP_MB();
}

size_t up_get_icache_linesize(void)
{
  return stm32_get_icache_linesize();
}

size_t up_get_icache_size(void)
{
  return stm32_get_icache_size();
}

void up_enable_icache(void)
{
  stm32_enable_icache();
}

void up_disable_icache(void)
{
  stm32_disable_icache();
}

void up_invalidate_icache(uintptr_t start, uintptr_t end)
{
  UNUSED(start);
  UNUSED(end);
  stm32_invalidate_icache();
}

void up_invalidate_icache_all(void)
{
  stm32_invalidate_icache();
}

void up_coherent_dcache(uintptr_t addr, size_t len)
{
  if (len > 0)
    {
      up_clean_dcache(addr, addr + len);
      up_invalidate_icache_all();
    }
}
