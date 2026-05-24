/****************************************************************************
 * arch/arm/src/stm32u5/stm32_dcache.c
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
#include <nuttx/irq.h>

#include <arch/barriers.h>

#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

#include "stm32.h"
#include "stm32_dcache.h"
#include "hardware/stm32_dcache.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32U5_DCACHE_SIZE            4096
#define STM32U5_DCACHE_TIMEOUT         1000000

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_dcache_wait_clear(uint32_t mask)
{
  uint32_t timeout = STM32U5_DCACHE_TIMEOUT;

  while ((getreg32(STM32_DCACHE1_SR) & mask) != 0)
    {
      if (timeout-- == 0)
        {
          return -ETIMEDOUT;
        }
    }

  return OK;
}

static int stm32_dcache_wait_set(uint32_t mask)
{
  uint32_t timeout = STM32U5_DCACHE_TIMEOUT;

  while ((getreg32(STM32_DCACHE1_SR) & mask) == 0)
    {
      if (timeout-- == 0)
        {
          return -ETIMEDOUT;
        }
    }

  return OK;
}

static int stm32_dcache_command_aligned(uint32_t command, uintptr_t start,
                                        uintptr_t endaddr)
{
  irqstate_t flags;
  int ret;

  flags = enter_critical_section();

  if ((getreg32(STM32_DCACHE1_CR) & DCACHE_CR_EN) == 0)
    {
      ret = OK;
      goto out;
    }

  ret = stm32_dcache_wait_clear(DCACHE_SR_BUSYF | DCACHE_SR_BUSYCMDF);
  if (ret < 0)
    {
      goto out;
    }

  putreg32(DCACHE_FCR_CBSYENDF | DCACHE_FCR_CCMDENDF | DCACHE_FCR_CERRF,
           STM32_DCACHE1_FCR);
  putreg32(start, STM32_DCACHE1_CMDRSADDRR);
  putreg32(endaddr, STM32_DCACHE1_CMDREADDRR);
  modifyreg32(STM32_DCACHE1_CR, DCACHE_CR_CACHECMD_MASK, command);
  modifyreg32(STM32_DCACHE1_IER, DCACHE_IER_CMDENDIE, 0);
  modifyreg32(STM32_DCACHE1_CR, 0, DCACHE_CR_STARTCMD);

  ret = stm32_dcache_wait_set(DCACHE_SR_CMDENDF);
  putreg32(DCACHE_FCR_CCMDENDF, STM32_DCACHE1_FCR);

out:
  leave_critical_section(flags);
  UP_MB();
  return ret;
}

static int stm32_dcache_command(uint32_t command, uintptr_t start,
                                uintptr_t end)
{
  if (end <= start)
    {
      return OK;
    }

  start &= ~(STM32U5_DCACHE_LINESIZE - 1);
  end = (end - 1) & ~(STM32U5_DCACHE_LINESIZE - 1);

  return stm32_dcache_command_aligned(command, start, end);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

size_t stm32_get_dcache_linesize(void)
{
  return STM32U5_DCACHE_LINESIZE;
}

size_t stm32_get_dcache_size(void)
{
  return STM32U5_DCACHE_SIZE;
}

void stm32_enable_dcache(void)
{
  irqstate_t flags;

  flags = enter_critical_section();

  if ((getreg32(STM32_DCACHE1_SR) &
       (DCACHE_SR_BUSYF | DCACHE_SR_BUSYCMDF)) == 0)
    {
#ifdef CONFIG_STM32U5_DCACHE1_READ_BURST_INCR
      modifyreg32(STM32_DCACHE1_CR, 0, DCACHE_CR_HBURST);
#else
      modifyreg32(STM32_DCACHE1_CR, DCACHE_CR_HBURST, 0);
#endif
      modifyreg32(STM32_DCACHE1_CR, 0, DCACHE_CR_EN);
    }

  leave_critical_section(flags);
  UP_MB();
}

void stm32_disable_dcache(void)
{
  irqstate_t flags;

  stm32_dcache_command_aligned(DCACHE_COMMAND_CLEAN_INVALIDATE, 0,
                               UINT32_MAX &
                               ~(STM32U5_DCACHE_LINESIZE - 1));

  flags = enter_critical_section();
  putreg32(DCACHE_FCR_CBSYENDF | DCACHE_FCR_CCMDENDF | DCACHE_FCR_CERRF,
           STM32_DCACHE1_FCR);
  modifyreg32(STM32_DCACHE1_CR, DCACHE_CR_EN, 0);
  stm32_dcache_wait_clear(DCACHE_SR_BUSYF | DCACHE_SR_BUSYCMDF);
  leave_critical_section(flags);
  UP_MB();
}

void stm32_invalidate_dcache(uintptr_t start, uintptr_t end)
{
  stm32_dcache_command(DCACHE_COMMAND_INVALIDATE, start, end);
}

void stm32_invalidate_dcache_all(void)
{
  irqstate_t flags;

  flags = enter_critical_section();

  if ((getreg32(STM32_DCACHE1_CR) & DCACHE_CR_EN) == 0)
    {
      leave_critical_section(flags);
      UP_MB();
      return;
    }

  if ((getreg32(STM32_DCACHE1_SR) &
       (DCACHE_SR_BUSYF | DCACHE_SR_BUSYCMDF)) == 0)
    {
      putreg32(DCACHE_FCR_CBSYENDF | DCACHE_FCR_CCMDENDF |
               DCACHE_FCR_CERRF,
               STM32_DCACHE1_FCR);
      modifyreg32(STM32_DCACHE1_CR, DCACHE_CR_CACHECMD_MASK, 0);
      modifyreg32(STM32_DCACHE1_CR, 0, DCACHE_CR_CACHEINV);
      stm32_dcache_wait_clear(DCACHE_SR_BUSYF);
      putreg32(DCACHE_FCR_CBSYENDF, STM32_DCACHE1_FCR);
    }

  leave_critical_section(flags);
  UP_MB();
}

void stm32_clean_dcache(uintptr_t start, uintptr_t end)
{
  stm32_dcache_command(DCACHE_COMMAND_CLEAN, start, end);
}

void stm32_flush_dcache(uintptr_t start, uintptr_t end)
{
  stm32_dcache_command(DCACHE_COMMAND_CLEAN_INVALIDATE, start, end);
}

size_t up_get_dcache_linesize(void)
{
  return stm32_get_dcache_linesize();
}

size_t up_get_dcache_size(void)
{
  return stm32_get_dcache_size();
}

void up_enable_dcache(void)
{
  stm32_enable_dcache();
}

void up_disable_dcache(void)
{
  stm32_disable_dcache();
}

void up_invalidate_dcache(uintptr_t start, uintptr_t end)
{
  stm32_invalidate_dcache(start, end);
}

void up_invalidate_dcache_all(void)
{
  stm32_invalidate_dcache_all();
}

void up_clean_dcache(uintptr_t start, uintptr_t end)
{
  stm32_clean_dcache(start, end);
}

void up_clean_dcache_all(void)
{
  stm32_dcache_command_aligned(DCACHE_COMMAND_CLEAN, 0,
                               UINT32_MAX &
                               ~(STM32U5_DCACHE_LINESIZE - 1));
}

void up_flush_dcache(uintptr_t start, uintptr_t end)
{
  stm32_flush_dcache(start, end);
}

void up_flush_dcache_all(void)
{
  stm32_dcache_command_aligned(DCACHE_COMMAND_CLEAN_INVALIDATE, 0,
                               UINT32_MAX &
                               ~(STM32U5_DCACHE_LINESIZE - 1));
}
