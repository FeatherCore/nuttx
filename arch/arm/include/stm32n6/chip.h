/****************************************************************************
 * arch/arm/include/stm32n6/chip.h
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

#ifndef __ARCH_ARM_INCLUDE_STM32N6_CHIP_H
#define __ARCH_ARM_INCLUDE_STM32N6_CHIP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if !defined(CONFIG_ARCH_CHIP_STM32N657X0)
#  error STM32N6 chip not identified
#endif

/* Memory sizes - STM32N6 has no internal flash.  Code runs from the
 * on-chip AXI SRAM (loaded by the debugger in DEV mode, or by an FSBL
 * from external XSPI flash in normal boot).
 *
 * AXI SRAM layout (from STM32CubeN6 CMSIS stm32n657xx.h):
 *   AXISRAM1:     0x34000000  1 MB
 *   AXISRAM2:     0x34100000  1 MB
 *   AXISRAM3:     0x34200000  448 KB
 *   AXISRAM4:     0x34270000  448 KB
 *   AXISRAM5:     0x342E0000  448 KB
 *   AXISRAM6:     0x34350000  448 KB
 *   CACHEAXIRAM:  0x343C0000  256 KB
 *   -------------------------------------------
 *   Total:                    4 MB    (end = 0x34400000)
 *
 * VENCRAM (128 KB at 0x34400000) is excluded because it is reserved for the
 * video encoder.  The STM32N6570-DK NXboot flow uses a smaller 0x003ca000
 * AXI SRAM validation range and keeps the application window to 2 MiB to
 * match the STM32CubeN6 Template_FSBL_XIP_Custom layout.
 */

#define STM32_SRAM_SIZE           (4 * 1024 * 1024)  /* 4194304 bytes (4 MiB) */

#define STM32_AXISRAM_SIZE        (0x003ca000u)
#define STM32_AHBSRAM_SIZE        (32 * 1024u)
#define STM32_FSBL_RAM_SIZE       (511 * 1024u)
#define STM32_APP_RAM_SIZE        (2 * 1024 * 1024u)
#define STM32_XSPI2_NOR_SIZE      (128 * 1024 * 1024u)
#define STM32_XSPI1_PSRAM_SIZE    (32 * 1024 * 1024u)

#define STM32_NPORTS              (12) /* GPIO ports A-H and N-Q */
#define STM32_NGPIO               STM32_NPORTS
#define STM32_NUSART              (10)
#define STM32_NXSPI               (3)

/* NVIC priority levels *****************************************************/

/* 16 programmable interrupt levels (4-bit priority). */

#define NVIC_SYSH_PRIORITY_MIN      0xf0
#define NVIC_SYSH_PRIORITY_DEFAULT  0x80
#define NVIC_SYSH_PRIORITY_MAX      0x00
#define NVIC_SYSH_PRIORITY_STEP     0x10

#endif /* __ARCH_ARM_INCLUDE_STM32N6_CHIP_H */
