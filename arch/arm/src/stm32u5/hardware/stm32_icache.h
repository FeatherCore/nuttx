/****************************************************************************
 * arch/arm/src/stm32u5/hardware/stm32_icache.h
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

#ifndef __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_ICACHE_H
#define __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_ICACHE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "chip.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register Offsets *********************************************************/

#define STM32_ICACHE_CR_OFFSET        0x00 /* Control Register */
#define STM32_ICACHE_SR_OFFSET        0x04 /* Status Register */
#define STM32_ICACHE_IER_OFFSET       0x08 /* Interrupt Enable Register */
#define STM32_ICACHE_FCR_OFFSET       0x0c /* Flag Clear Register */
#define STM32_ICACHE_HMONR_OFFSET     0x10 /* Hit Monitor Register */
#define STM32_ICACHE_MMONR_OFFSET     0x14 /* Miss Monitor Register */
#define STM32_ICACHE_CRR_OFFSET(n)   (0x20 + ((n) << 2))

/* Register Addresses *******************************************************/

#define STM32_ICACHE_CR \
  (STM32_ICACHE_BASE + STM32_ICACHE_CR_OFFSET)
#define STM32_ICACHE_SR \
  (STM32_ICACHE_BASE + STM32_ICACHE_SR_OFFSET)
#define STM32_ICACHE_IER \
  (STM32_ICACHE_BASE + STM32_ICACHE_IER_OFFSET)
#define STM32_ICACHE_FCR \
  (STM32_ICACHE_BASE + STM32_ICACHE_FCR_OFFSET)
#define STM32_ICACHE_HMONR \
  (STM32_ICACHE_BASE + STM32_ICACHE_HMONR_OFFSET)
#define STM32_ICACHE_MMONR \
  (STM32_ICACHE_BASE + STM32_ICACHE_MMONR_OFFSET)
#define STM32_ICACHE_CRR(n) \
  (STM32_ICACHE_BASE + STM32_ICACHE_CRR_OFFSET(n))

/* Register Bitfield Definitions ********************************************/

/* Control Register */

#define ICACHE_CR_EN                 (1 << 0)  /* Enable */
#define ICACHE_CR_CACHEINV           (1 << 1)  /* Cache invalidation */
#define ICACHE_CR_WAYSEL             (1 << 2)  /* 0=1-way, 1=2-way */
#define ICACHE_CR_HITMEN             (1 << 16) /* Hit monitor enable */
#define ICACHE_CR_MISSMEN            (1 << 17) /* Miss monitor enable */
#define ICACHE_CR_HITMRST            (1 << 18) /* Hit monitor reset */
#define ICACHE_CR_MISSMRST           (1 << 19) /* Miss monitor reset */

/* Status Register */

#define ICACHE_SR_BUSYF              (1 << 0)  /* Busy flag */
#define ICACHE_SR_BSYENDF            (1 << 1)  /* Busy end flag */
#define ICACHE_SR_ERRF               (1 << 2)  /* Cache error flag */

/* Interrupt Enable Register */

#define ICACHE_IER_BSYENDIE          (1 << 1)  /* Busy end interrupt enable */
#define ICACHE_IER_ERRIE             (1 << 2)  /* Error interrupt enable */
#define ICACHE_IER_ALLINTS           (ICACHE_IER_BSYENDIE | ICACHE_IER_ERRIE)

/* Flag Clear Register */

#define ICACHE_FCR_CBSYENDF          (1 << 1)  /* Clear busy end flag */
#define ICACHE_FCR_CERRF             (1 << 2)  /* Clear cache error flag */

/* Region x Configuration Register */

#define ICACHE_CRR_BASEADDR_SHIFT    (0)
#define ICACHE_CRR_BASEADDR_MASK     (0xff << ICACHE_CRR_BASEADDR_SHIFT)
#define ICACHE_CRR_RSIZE_SHIFT       (9)
#define ICACHE_CRR_RSIZE_MASK        (0x7 << ICACHE_CRR_RSIZE_SHIFT)
#define ICACHE_CRR_REN               (1 << 15)
#define ICACHE_CRR_REMAPADDR_SHIFT   (16)
#define ICACHE_CRR_REMAPADDR_MASK    (0x7ff << ICACHE_CRR_REMAPADDR_SHIFT)
#define ICACHE_CRR_MSTSEL_SHIFT      (28)
#define ICACHE_CRR_MSTSEL            (1 << ICACHE_CRR_MSTSEL_SHIFT)
#define ICACHE_CRR_HBURST_SHIFT      (31)
#define ICACHE_CRR_HBURST            (1 << ICACHE_CRR_HBURST_SHIFT)

#endif /* __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_ICACHE_H */
