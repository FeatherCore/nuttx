/****************************************************************************
 * arch/arm/src/stm32u5/stm32_dcache.h
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

#ifndef __ARCH_ARM_SRC_STM32U5_STM32_DCACHE_H
#define __ARCH_ARM_SRC_STM32U5_STM32_DCACHE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

size_t stm32_get_dcache_linesize(void);
size_t stm32_get_dcache_size(void);
void stm32_enable_dcache(void);
void stm32_disable_dcache(void);
void stm32_invalidate_dcache(uintptr_t start, uintptr_t end);
void stm32_invalidate_dcache_all(void);
void stm32_clean_dcache(uintptr_t start, uintptr_t end);
void stm32_flush_dcache(uintptr_t start, uintptr_t end);

#endif /* __ARCH_ARM_SRC_STM32U5_STM32_DCACHE_H */
