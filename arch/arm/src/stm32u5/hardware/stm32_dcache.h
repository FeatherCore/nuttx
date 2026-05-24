/****************************************************************************
 * arch/arm/src/stm32u5/hardware/stm32_dcache.h
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

#ifndef __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DCACHE_H
#define __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DCACHE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "chip.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register Offsets *********************************************************/

#define STM32_DCACHE_CR_OFFSET          0x00 /* Control Register */
#define STM32_DCACHE_SR_OFFSET          0x04 /* Status Register */
#define STM32_DCACHE_IER_OFFSET         0x08 /* Interrupt Enable Register */
#define STM32_DCACHE_FCR_OFFSET         0x0c /* Flag Clear Register */
#define STM32_DCACHE_RHMONR_OFFSET      0x10 /* Read Hit Monitor Register */
#define STM32_DCACHE_RMMONR_OFFSET      0x14 /* Read Miss Monitor Register */
#define STM32_DCACHE_WHMONR_OFFSET      0x20 /* Write Hit Monitor Register */
#define STM32_DCACHE_WMMONR_OFFSET      0x24 /* Write Miss Monitor Register */
#define STM32_DCACHE_CMDRSADDRR_OFFSET  0x28 /* Command Start Address */
#define STM32_DCACHE_CMDREADDRR_OFFSET  0x2c /* Command End Address */

/* Register Addresses *******************************************************/

#define STM32_DCACHE1_CR \
  (STM32_DCACHE1_BASE + STM32_DCACHE_CR_OFFSET)
#define STM32_DCACHE1_SR \
  (STM32_DCACHE1_BASE + STM32_DCACHE_SR_OFFSET)
#define STM32_DCACHE1_IER \
  (STM32_DCACHE1_BASE + STM32_DCACHE_IER_OFFSET)
#define STM32_DCACHE1_FCR \
  (STM32_DCACHE1_BASE + STM32_DCACHE_FCR_OFFSET)
#define STM32_DCACHE1_RHMONR \
  (STM32_DCACHE1_BASE + STM32_DCACHE_RHMONR_OFFSET)
#define STM32_DCACHE1_RMMONR \
  (STM32_DCACHE1_BASE + STM32_DCACHE_RMMONR_OFFSET)
#define STM32_DCACHE1_WHMONR \
  (STM32_DCACHE1_BASE + STM32_DCACHE_WHMONR_OFFSET)
#define STM32_DCACHE1_WMMONR \
  (STM32_DCACHE1_BASE + STM32_DCACHE_WMMONR_OFFSET)
#define STM32_DCACHE1_CMDRSADDRR \
  (STM32_DCACHE1_BASE + STM32_DCACHE_CMDRSADDRR_OFFSET)
#define STM32_DCACHE1_CMDREADDRR \
  (STM32_DCACHE1_BASE + STM32_DCACHE_CMDREADDRR_OFFSET)

/* Register Bitfield Definitions ********************************************/

/* Control Register */

#define DCACHE_CR_EN                    (1 << 0)  /* Enable */
#define DCACHE_CR_CACHEINV              (1 << 1)  /* Full cache invalidation */
#define DCACHE_CR_CACHECMD_SHIFT        (8)
#define DCACHE_CR_CACHECMD_MASK         (0x7 << DCACHE_CR_CACHECMD_SHIFT)
#define DCACHE_CR_CACHECMD_0            (1 << DCACHE_CR_CACHECMD_SHIFT)
#define DCACHE_CR_CACHECMD_1            (2 << DCACHE_CR_CACHECMD_SHIFT)
#define DCACHE_CR_CACHECMD_2            (4 << DCACHE_CR_CACHECMD_SHIFT)
#define DCACHE_CR_STARTCMD              (1 << 11) /* Start command */
#define DCACHE_CR_RHITMEN               (1 << 16) /* Read hit monitor enable */
#define DCACHE_CR_RMISSMEN              (1 << 17) /* Read miss monitor enable */
#define DCACHE_CR_RHITMRST              (1 << 18) /* Read hit monitor reset */
#define DCACHE_CR_RMISSMRST             (1 << 19) /* Read miss monitor reset */
#define DCACHE_CR_WHITMEN               (1 << 20) /* Write hit monitor enable */
#define DCACHE_CR_WMISSMEN              (1 << 21) /* Write miss monitor */
#define DCACHE_CR_WHITMRST              (1 << 22) /* Write hit monitor reset */
#define DCACHE_CR_WMISSMRST             (1 << 23) /* Write miss monitor reset */
#define DCACHE_CR_HBURST                (1 << 31) /* 0=WRAP, 1=INCR */

/* Status Register */

#define DCACHE_SR_BUSYF                 (1 << 0)  /* Busy flag */
#define DCACHE_SR_BSYENDF               (1 << 1)  /* Busy end flag */
#define DCACHE_SR_ERRF                  (1 << 2)  /* Cache error flag */
#define DCACHE_SR_BUSYCMDF              (1 << 3)  /* Busy command flag */
#define DCACHE_SR_CMDENDF               (1 << 4)  /* Command end flag */

/* Interrupt Enable Register */

#define DCACHE_IER_BSYENDIE             (1 << 1)  /* Busy end interrupt */
#define DCACHE_IER_ERRIE                (1 << 2)  /* Error interrupt */
#define DCACHE_IER_CMDENDIE             (1 << 4)  /* Command end interrupt */

/* Flag Clear Register */

#define DCACHE_FCR_CBSYENDF             (1 << 1)  /* Clear busy end flag */
#define DCACHE_FCR_CERRF                (1 << 2)  /* Clear cache error flag */
#define DCACHE_FCR_CCMDENDF             (1 << 4)  /* Clear command end flag */

/* Command Address Registers */

#define DCACHE_CMDRSADDRR_CMDSTARTADDR_MASK  0xffffffe0
#define DCACHE_CMDREADDRR_CMDENDADDR_MASK    0xffffffe0

/* Cache commands */

#define DCACHE_COMMAND_CLEAN \
  DCACHE_CR_CACHECMD_0
#define DCACHE_COMMAND_INVALIDATE \
  DCACHE_CR_CACHECMD_1
#define DCACHE_COMMAND_CLEAN_INVALIDATE \
  (DCACHE_CR_CACHECMD_0 | DCACHE_CR_CACHECMD_1)

#endif /* __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DCACHE_H */
