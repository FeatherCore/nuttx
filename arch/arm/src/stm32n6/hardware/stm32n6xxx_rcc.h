/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32n6xxx_rcc.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6XXX_RCC_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6XXX_RCC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include "hardware/stm32n6xxx_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register Offsets *********************************************************/

#define STM32_RCC_CR_OFFSET           0x0000  /* Clock control register */
#define STM32_RCC_SR_OFFSET           0x0004  /* Clock status register */
#define STM32_RCC_CFGR1_OFFSET        0x0020  /* Clock configuration register 1 */
#define STM32_RCC_CFGR2_OFFSET        0x0024  /* Clock configuration register 2 */
#define STM32_RCC_HSICFGR_OFFSET      0x0048  /* HSI configuration register */
#define STM32_RCC_PLL1CFGR1_OFFSET    0x0080  /* PLL1 configuration register 1 */
#define STM32_RCC_PLL1CFGR2_OFFSET    0x0084  /* PLL1 configuration register 2 */
#define STM32_RCC_PLL1CFGR3_OFFSET    0x0088  /* PLL1 configuration register 3 */
#define STM32_RCC_IC1CFGR_OFFSET      0x00c4  /* IC1 configuration register */
#define STM32_RCC_IC2CFGR_OFFSET      0x00c8  /* IC2 configuration register */
#define STM32_RCC_IC3CFGR_OFFSET      0x00cc  /* IC3 configuration register */
#define STM32_RCC_IC6CFGR_OFFSET      0x00d8  /* IC6 configuration register */
#define STM32_RCC_IC11CFGR_OFFSET     0x00ec  /* IC11 configuration register */
#define STM32_RCC_IC16CFGR_OFFSET     0x0100  /* IC16 configuration register */
#define STM32_RCC_CCIPR4_OFFSET       0x0150  /* Peripheral kernel clock select register 4 */
#define STM32_RCC_CCIPR6_OFFSET       0x0158  /* Peripheral kernel clock select register 6 */
#define STM32_RCC_CCIPR13_OFFSET      0x0174  /* Peripheral kernel clock select register 13 */

#define STM32_RCC_AHB5RSTR_OFFSET     0x0220  /* AHB5 peripheral reset register */
#define STM32_RCC_APB1RSTR1_OFFSET    0x0224  /* APB1 peripheral reset register 1 */
#define STM32_RCC_APB2RSTR_OFFSET     0x022c  /* APB2 peripheral reset register */
#define STM32_RCC_APB5RSTR_OFFSET     0x023c  /* APB5 peripheral reset register */

/* Peripheral clock enable / set / clear register offsets.  Each enable
 * register (xxxENR) has a paired set register (xxxENSR) that performs an
 * atomic OR and a clear register (xxxENCR) that performs an atomic AND
 * (Reference: RM0486 14.5).  The same triplet pattern applies to the LPENR
 * (sleep mode) registers.
 */

#define STM32_RCC_MEMENR_OFFSET       0x024c  /* AXI/AHB SRAM clock enable register */
#define STM32_RCC_BUSENR_OFFSET       0x0244  /* Bus clock enable register */
#define STM32_RCC_MISCENR_OFFSET      0x0248  /* Miscellaneous clock enable register */
#define STM32_RCC_AHB3ENR_OFFSET      0x0258  /* AHB3 peripheral clock enable register */
#define STM32_RCC_AHB4ENR_OFFSET      0x025c  /* AHB4 peripheral clock enable register */
#define STM32_RCC_AHB5ENR_OFFSET      0x0260  /* AHB5 peripheral clock enable register */
#define STM32_RCC_APB1LENR_OFFSET     0x0264  /* APB1 peripheral clock enable register 1 */
#define STM32_RCC_APB2ENR_OFFSET      0x026c  /* APB2 peripheral clock enable register */
#define STM32_RCC_APB4HENR_OFFSET     0x0278  /* APB4 peripheral clock enable register 2 */
#define STM32_RCC_APB5ENR_OFFSET      0x027c  /* APB5 peripheral clock enable register */
#define STM32_RCC_BUSLPENR_OFFSET     0x0284  /* Bus clocks enable in Sleep mode */
#define STM32_RCC_MEMLPENR_OFFSET     0x028c  /* SRAM clocks enable in Sleep mode */
#define STM32_RCC_APB1LLPENR_OFFSET   0x02a4  /* APB1 LP clock enable register 1 */
#define STM32_RCC_APB2LPENR_OFFSET    0x02ac  /* APB2 LP clock enable register */

#define STM32_RCC_DIVENR_OFFSET       0x0240  /* IC divider enable register */
#define STM32_RCC_DIVENSR_OFFSET      0x0a40  /* IC divider enable set register */

#define STM32_RCC_AHB5RSTSR_OFFSET    0x0a20  /* AHB5 peripheral reset set register */
#define STM32_RCC_APB1RSTSR1_OFFSET   0x0a24  /* APB1 peripheral reset set register 1 */
#define STM32_RCC_APB2RSTSR_OFFSET    0x0a2c  /* APB2 peripheral reset set register */
#define STM32_RCC_APB5RSTSR_OFFSET    0x0a3c  /* APB5 peripheral reset set register */
#define STM32_RCC_BUSENSR_OFFSET      0x0a44  /* Bus clock enable set register */
#define STM32_RCC_MISCENSR_OFFSET     0x0a48  /* Miscellaneous clock enable set register */
#define STM32_RCC_MEMENSR_OFFSET      0x0a4c  /* SRAM clock enable set register */
#define STM32_RCC_AHB3ENSR_OFFSET     0x0a58  /* AHB3 clock enable set register */
#define STM32_RCC_AHB4ENSR_OFFSET     0x0a5c  /* AHB4 clock enable set register */
#define STM32_RCC_AHB5ENSR_OFFSET     0x0a60  /* AHB5 clock enable set register */
#define STM32_RCC_APB1LENSR_OFFSET    0x0a64  /* APB1 clock enable set register 1 */
#define STM32_RCC_APB2ENSR_OFFSET     0x0a6c  /* APB2 clock enable set register */
#define STM32_RCC_APB4HENSR_OFFSET    0x0a78  /* APB4 clock enable set register 2 */
#define STM32_RCC_APB5ENSR_OFFSET     0x0a7c  /* APB5 clock enable set register */
#define STM32_RCC_BUSLPENSR_OFFSET    0x0a84  /* Bus LP clock enable set register */
#define STM32_RCC_MEMLPENSR_OFFSET    0x0a8c  /* SRAM LP clock enable set register */
#define STM32_RCC_APB1LLPENSR_OFFSET  0x0aa4  /* APB1 LP clock enable set register 1 */
#define STM32_RCC_APB2LPENSR_OFFSET   0x0aac  /* APB2 LP clock enable set register */

#define STM32_RCC_CCR_OFFSET          0x1000  /* Clock control clear register */
#define STM32_RCC_AHB5RSTCR_OFFSET    0x1220  /* AHB5 peripheral reset clear register */
#define STM32_RCC_APB1RSTCR1_OFFSET   0x1224  /* APB1 peripheral reset clear register 1 */
#define STM32_RCC_APB2RSTCR_OFFSET    0x122c  /* APB2 peripheral reset clear register */
#define STM32_RCC_APB5RSTCR_OFFSET    0x123c  /* APB5 peripheral reset clear register */
#define STM32_RCC_APB1ENCR1_OFFSET    0x1264  /* APB1 clock enable clear register 1 */
#define STM32_RCC_APB2ENCR_OFFSET     0x126c  /* APB2 clock enable clear register */

#define STM32_RCC_CSR_OFFSET          0x0800  /* Clock status (set) register */

/* Register Addresses *******************************************************/

#define STM32_RCC_CR                  (STM32_RCC_BASE + STM32_RCC_CR_OFFSET)
#define STM32_RCC_SR                  (STM32_RCC_BASE + STM32_RCC_SR_OFFSET)
#define STM32_RCC_CFGR1               (STM32_RCC_BASE + STM32_RCC_CFGR1_OFFSET)
#define STM32_RCC_CFGR2               (STM32_RCC_BASE + STM32_RCC_CFGR2_OFFSET)
#define STM32_RCC_HSICFGR             (STM32_RCC_BASE + STM32_RCC_HSICFGR_OFFSET)
#define STM32_RCC_PLL1CFGR1           (STM32_RCC_BASE + STM32_RCC_PLL1CFGR1_OFFSET)
#define STM32_RCC_PLL1CFGR2           (STM32_RCC_BASE + STM32_RCC_PLL1CFGR2_OFFSET)
#define STM32_RCC_PLL1CFGR3           (STM32_RCC_BASE + STM32_RCC_PLL1CFGR3_OFFSET)
#define STM32_RCC_IC1CFGR             (STM32_RCC_BASE + STM32_RCC_IC1CFGR_OFFSET)
#define STM32_RCC_IC2CFGR             (STM32_RCC_BASE + STM32_RCC_IC2CFGR_OFFSET)
#define STM32_RCC_IC3CFGR             (STM32_RCC_BASE + STM32_RCC_IC3CFGR_OFFSET)
#define STM32_RCC_IC6CFGR             (STM32_RCC_BASE + STM32_RCC_IC6CFGR_OFFSET)
#define STM32_RCC_IC11CFGR            (STM32_RCC_BASE + STM32_RCC_IC11CFGR_OFFSET)
#define STM32_RCC_IC16CFGR            (STM32_RCC_BASE + STM32_RCC_IC16CFGR_OFFSET)
#define STM32_RCC_CCIPR4              (STM32_RCC_BASE + STM32_RCC_CCIPR4_OFFSET)
#define STM32_RCC_CCIPR6              (STM32_RCC_BASE + STM32_RCC_CCIPR6_OFFSET)
#define STM32_RCC_CCIPR13             (STM32_RCC_BASE + STM32_RCC_CCIPR13_OFFSET)

#define STM32_RCC_AHB5RSTR            (STM32_RCC_BASE + STM32_RCC_AHB5RSTR_OFFSET)
#define STM32_RCC_APB1RSTR1           (STM32_RCC_BASE + STM32_RCC_APB1RSTR1_OFFSET)
#define STM32_RCC_APB2RSTR            (STM32_RCC_BASE + STM32_RCC_APB2RSTR_OFFSET)
#define STM32_RCC_APB5RSTR            (STM32_RCC_BASE + STM32_RCC_APB5RSTR_OFFSET)

#define STM32_RCC_DIVENR              (STM32_RCC_BASE + STM32_RCC_DIVENR_OFFSET)
#define STM32_RCC_DIVENSR             (STM32_RCC_BASE + STM32_RCC_DIVENSR_OFFSET)

#define STM32_RCC_BUSENR              (STM32_RCC_BASE + STM32_RCC_BUSENR_OFFSET)
#define STM32_RCC_MISCENR             (STM32_RCC_BASE + STM32_RCC_MISCENR_OFFSET)
#define STM32_RCC_MEMENR              (STM32_RCC_BASE + STM32_RCC_MEMENR_OFFSET)
#define STM32_RCC_AHB3ENR             (STM32_RCC_BASE + STM32_RCC_AHB3ENR_OFFSET)
#define STM32_RCC_AHB4ENR             (STM32_RCC_BASE + STM32_RCC_AHB4ENR_OFFSET)
#define STM32_RCC_AHB5ENR             (STM32_RCC_BASE + STM32_RCC_AHB5ENR_OFFSET)
#define STM32_RCC_APB1LENR            (STM32_RCC_BASE + STM32_RCC_APB1LENR_OFFSET)
#define STM32_RCC_APB1ENR1            STM32_RCC_APB1LENR
#define STM32_RCC_APB2ENR             (STM32_RCC_BASE + STM32_RCC_APB2ENR_OFFSET)
#define STM32_RCC_APB4HENR            (STM32_RCC_BASE + STM32_RCC_APB4HENR_OFFSET)
#define STM32_RCC_APB4ENR2            STM32_RCC_APB4HENR
#define STM32_RCC_APB5ENR             (STM32_RCC_BASE + STM32_RCC_APB5ENR_OFFSET)
#define STM32_RCC_BUSLPENR            (STM32_RCC_BASE + STM32_RCC_BUSLPENR_OFFSET)
#define STM32_RCC_MEMLPENR            (STM32_RCC_BASE + STM32_RCC_MEMLPENR_OFFSET)
#define STM32_RCC_APB1LLPENR          (STM32_RCC_BASE + STM32_RCC_APB1LLPENR_OFFSET)
#define STM32_RCC_APB2LPENR           (STM32_RCC_BASE + STM32_RCC_APB2LPENR_OFFSET)

#define STM32_RCC_AHB5RSTSR           (STM32_RCC_BASE + STM32_RCC_AHB5RSTSR_OFFSET)
#define STM32_RCC_APB1RSTSR1          (STM32_RCC_BASE + STM32_RCC_APB1RSTSR1_OFFSET)
#define STM32_RCC_APB2RSTSR           (STM32_RCC_BASE + STM32_RCC_APB2RSTSR_OFFSET)
#define STM32_RCC_APB5RSTSR           (STM32_RCC_BASE + STM32_RCC_APB5RSTSR_OFFSET)
#define STM32_RCC_BUSENSR             (STM32_RCC_BASE + STM32_RCC_BUSENSR_OFFSET)
#define STM32_RCC_MISCENSR            (STM32_RCC_BASE + STM32_RCC_MISCENSR_OFFSET)
#define STM32_RCC_MEMENSR             (STM32_RCC_BASE + STM32_RCC_MEMENSR_OFFSET)
#define STM32_RCC_AHB3ENSR            (STM32_RCC_BASE + STM32_RCC_AHB3ENSR_OFFSET)
#define STM32_RCC_AHB4ENSR            (STM32_RCC_BASE + STM32_RCC_AHB4ENSR_OFFSET)
#define STM32_RCC_AHB5ENSR            (STM32_RCC_BASE + STM32_RCC_AHB5ENSR_OFFSET)
#define STM32_RCC_APB1LENSR           (STM32_RCC_BASE + STM32_RCC_APB1LENSR_OFFSET)
#define STM32_RCC_APB1ENSR1           STM32_RCC_APB1LENSR
#define STM32_RCC_APB2ENSR            (STM32_RCC_BASE + STM32_RCC_APB2ENSR_OFFSET)
#define STM32_RCC_APB4HENSR           (STM32_RCC_BASE + STM32_RCC_APB4HENSR_OFFSET)
#define STM32_RCC_APB4ENSR2           STM32_RCC_APB4HENSR
#define STM32_RCC_APB5ENSR            (STM32_RCC_BASE + STM32_RCC_APB5ENSR_OFFSET)
#define STM32_RCC_BUSLPENSR           (STM32_RCC_BASE + STM32_RCC_BUSLPENSR_OFFSET)
#define STM32_RCC_MEMLPENSR           (STM32_RCC_BASE + STM32_RCC_MEMLPENSR_OFFSET)
#define STM32_RCC_APB1LLPENSR         (STM32_RCC_BASE + STM32_RCC_APB1LLPENSR_OFFSET)
#define STM32_RCC_APB2LPENSR          (STM32_RCC_BASE + STM32_RCC_APB2LPENSR_OFFSET)

#define STM32_RCC_CCR                 (STM32_RCC_BASE + STM32_RCC_CCR_OFFSET)
#define STM32_RCC_AHB5RSTCR           (STM32_RCC_BASE + STM32_RCC_AHB5RSTCR_OFFSET)
#define STM32_RCC_APB1RSTCR1          (STM32_RCC_BASE + STM32_RCC_APB1RSTCR1_OFFSET)
#define STM32_RCC_APB2RSTCR           (STM32_RCC_BASE + STM32_RCC_APB2RSTCR_OFFSET)
#define STM32_RCC_APB5RSTCR           (STM32_RCC_BASE + STM32_RCC_APB5RSTCR_OFFSET)
#define STM32_RCC_APB1ENCR1           (STM32_RCC_BASE + STM32_RCC_APB1ENCR1_OFFSET)
#define STM32_RCC_APB2ENCR            (STM32_RCC_BASE + STM32_RCC_APB2ENCR_OFFSET)

#define STM32_RCC_CSR                 (STM32_RCC_BASE + STM32_RCC_CSR_OFFSET)

/* Register Bitfield Definitions ********************************************/

/* Clock control register */

#define RCC_CR_PLL1ON                 (1 << 8)  /* Bit 8:  PLL1 enable */
#define RCC_CR_HSION                  (1 << 3)  /* Bit 3:  HSI enable */

/* Clock status register */

#define RCC_SR_PLL1RDY                (1 << 8)  /* Bit 8:  PLL1 clock ready */
#define RCC_SR_HSIRDY                 (1 << 3)  /* Bit 3:  HSI clock ready */

/* Clock control set/clear registers */

#define RCC_CSR_PLL1ONS               (1 << 8)  /* Bit 8:  PLL1 enable set */
#define RCC_CSR_HSIONS                (1 << 3)  /* Bit 3:  HSI enable set */
#define RCC_CCR_PLL1ONC               (1 << 8)  /* Bit 8:  PLL1 enable clear */
#define RCC_CCR_HSIONC                (1 << 3)  /* Bit 3:  HSI enable clear */

/* Clock configuration register 1.  SYSSW = 0b11 selects three IC dividers
 * (IC2 for SYSCLK, IC6 for AHB, IC11 for APB) -- the SVD names this state
 * after the first IC only.
 */

#define RCC_CFGR1_SYSSWS_SHIFT        (28)
#define RCC_CFGR1_SYSSWS_MASK         (0x3 << RCC_CFGR1_SYSSWS_SHIFT)
#define RCC_CFGR1_SYSSWS_HSI          (0 << RCC_CFGR1_SYSSWS_SHIFT)
#define RCC_CFGR1_SYSSWS_IC2_IC6_IC11 (3 << RCC_CFGR1_SYSSWS_SHIFT)
#define RCC_CFGR1_SYSSW_SHIFT         (24)
#define RCC_CFGR1_SYSSW_MASK          (0x3 << RCC_CFGR1_SYSSW_SHIFT)
#define RCC_CFGR1_SYSSW_HSI           (0 << RCC_CFGR1_SYSSW_SHIFT)
#define RCC_CFGR1_SYSSW_IC2_IC6_IC11  (3 << RCC_CFGR1_SYSSW_SHIFT)
#define RCC_CFGR1_CPUSWS_SHIFT        (20)
#define RCC_CFGR1_CPUSWS_MASK         (0x3 << RCC_CFGR1_CPUSWS_SHIFT)
#define RCC_CFGR1_CPUSWS_HSI          (0 << RCC_CFGR1_CPUSWS_SHIFT)
#define RCC_CFGR1_CPUSWS_IC1          (3 << RCC_CFGR1_CPUSWS_SHIFT)
#define RCC_CFGR1_CPUSW_SHIFT         (16)
#define RCC_CFGR1_CPUSW_MASK          (0x3 << RCC_CFGR1_CPUSW_SHIFT)
#define RCC_CFGR1_CPUSW_HSI           (0 << RCC_CFGR1_CPUSW_SHIFT)
#define RCC_CFGR1_CPUSW_IC1           (3 << RCC_CFGR1_CPUSW_SHIFT)

/* Clock configuration register 2.  HPRE divides the SYSCLK fed to AHB
 * before it reaches the CPU/peripheral bus matrix.
 */

#define RCC_CFGR2_HPRE_SHIFT          (20)
#define RCC_CFGR2_HPRE_MASK           (0x7 << RCC_CFGR2_HPRE_SHIFT)
#define RCC_CFGR2_HPRE_SYSCLK         (0 << RCC_CFGR2_HPRE_SHIFT)
#define RCC_CFGR2_HPRE_SYSCLKd2       (1 << RCC_CFGR2_HPRE_SHIFT)
#define RCC_CFGR2_HPRE_SYSCLKd4       (2 << RCC_CFGR2_HPRE_SHIFT)
#define RCC_CFGR2_HPRE_SYSCLKd8       (3 << RCC_CFGR2_HPRE_SHIFT)
#define RCC_CFGR2_HPRE_SYSCLKd16      (4 << RCC_CFGR2_HPRE_SHIFT)

#define RCC_CFGR2_PPRE1_MASK          (0x7 << 0)
#define RCC_CFGR2_PPRE2_MASK          (0x7 << 4)
#define RCC_CFGR2_PPRE4_MASK          (0x7 << 12)
#define RCC_CFGR2_PPRE5_MASK          (0x7 << 16)
#define RCC_CFGR2_HPRE_DIV2           RCC_CFGR2_HPRE_SYSCLKd2

#define RCC_CFGR2                     STM32_RCC_CFGR2

/* PLL1 configuration register 1 */

#define RCC_PLL1CFGR1_SEL_SHIFT       (28)
#define RCC_PLL1CFGR1_SEL_MASK        (0x7 << RCC_PLL1CFGR1_SEL_SHIFT)
#define RCC_PLL1CFGR1_SEL_HSI         (0 << RCC_PLL1CFGR1_SEL_SHIFT)
#define RCC_PLL1CFGR1_PLL1SEL_MASK    RCC_PLL1CFGR1_SEL_MASK
#define RCC_PLL1CFGR1_PLL1SEL_HSI     RCC_PLL1CFGR1_SEL_HSI
#define RCC_PLL1CFGR1_DIVM_SHIFT      (20)       /* Bits 25-20: Reference divider */
#define RCC_PLL1CFGR1_DIVM_MASK       (0x3f << RCC_PLL1CFGR1_DIVM_SHIFT)
#define RCC_PLL1CFGR1_DIVN_SHIFT      (8)        /* Bits 19-8: Feedback divider */
#define RCC_PLL1CFGR1_DIVN_MASK       (0xfff << RCC_PLL1CFGR1_DIVN_SHIFT)
#define RCC_PLL1CFGR1_DIVM(n)         ((uint32_t)(n) << \
                                       RCC_PLL1CFGR1_DIVM_SHIFT)
#define RCC_PLL1CFGR1_DIVN(n)         ((uint32_t)(n) << \
                                       RCC_PLL1CFGR1_DIVN_SHIFT)

/* HSI configuration register */

#define RCC_HSICFGR_HSIDIV_MASK       (0x3 << 7)
#define RCC_HSICFGR_HSIDIV_DIV1       (0 << 7)

/* PLL1 configuration register 3 */

#define RCC_PLL1CFGR3_PDIVEN          (1 << 30)  /* Bit 30: Post-divider and PLL output enable */
#define RCC_PLL1CFGR3_PDIV1_SHIFT     (27)       /* Bits 29-27: Post-divider 1 */
#define RCC_PLL1CFGR3_PDIV1_MASK      (0x7 << RCC_PLL1CFGR3_PDIV1_SHIFT)
#define RCC_PLL1CFGR3_PDIV2_SHIFT     (24)       /* Bits 26-24: Post-divider 2 */
#define RCC_PLL1CFGR3_PDIV2_MASK      (0x7 << RCC_PLL1CFGR3_PDIV2_SHIFT)
#define RCC_PLL1CFGR3_MODSSRST        (1 << 0)   /* Bit 0:  Modulation spread spectrum reset */
#define RCC_PLL1CFGR3_MODSSDIS        (1 << 2)   /* Bit 2:  Modulation spread spectrum disable */
#define RCC_PLL1CFGR3_PDIV1(n)        ((uint32_t)(n) << \
                                       RCC_PLL1CFGR3_PDIV1_SHIFT)
#define RCC_PLL1CFGR3_PDIV2(n)        ((uint32_t)(n) << \
                                       RCC_PLL1CFGR3_PDIV2_SHIFT)

/* IC1..IC20 configuration registers -- all share the same layout.  Field
 * SEL selects the PLL source (PLL1..PLL4); field INT is an 8-bit integer
 * divider where INT[7:0] = N-1 yielding a divide ratio of N.
 */

#define RCC_ICCFGR_SEL_SHIFT          (28)
#define RCC_ICCFGR_SEL_MASK           (0x3 << RCC_ICCFGR_SEL_SHIFT)
#define RCC_ICCFGR_SEL_PLL1           (0 << RCC_ICCFGR_SEL_SHIFT)
#define RCC_ICCFGR_INT_SHIFT          (16)
#define RCC_ICCFGR_INT_MASK           (0xff << RCC_ICCFGR_INT_SHIFT)
#define RCC_ICCFGR_INT(n)             ((uint32_t)((n) - 1) << \
                                       RCC_ICCFGR_INT_SHIFT)
#define RCC_ICCFGR(sel, div)          ((sel) | RCC_ICCFGR_INT(div))

/* IC divider enable register */

#define RCC_DIVENR_IC11EN             (1 << 10)  /* Bit 10: IC11 enable */
#define RCC_DIVENR_IC16EN             (1 << 15)  /* Bit 15: IC16 enable */
#define RCC_DIVENR_IC6EN              (1 << 5)   /* Bit 5:  IC6 enable */
#define RCC_DIVENR_IC3EN              (1 << 2)   /* Bit 2:  IC3 enable */
#define RCC_DIVENR_IC2EN              (1 << 1)   /* Bit 1:  IC2 enable */
#define RCC_DIVENR_IC1EN              (1 << 0)   /* Bit 0:  IC1 enable */

#define RCC_DIVENSR_IC1ENS            RCC_DIVENR_IC1EN
#define RCC_DIVENSR_IC2ENS            RCC_DIVENR_IC2EN
#define RCC_DIVENSR_IC3ENS            RCC_DIVENR_IC3EN
#define RCC_DIVENSR_IC6ENS            RCC_DIVENR_IC6EN
#define RCC_DIVENSR_IC11ENS           RCC_DIVENR_IC11EN
#define RCC_DIVENSR_IC16ENS           RCC_DIVENR_IC16EN

/* SRAM clock enable register */

#define RCC_MEMENR_CACHEAXIRAMEN      (1 << 10)  /* Bit 10: CACHEAXIRAM enable */
#define RCC_MEMENR_AXISRAM2EN         (1 << 8)   /* Bit 8:  AXISRAM2 enable */
#define RCC_MEMENR_AXISRAM1EN         (1 << 7)   /* Bit 7:  AXISRAM1 enable */
#define RCC_MEMENR_AXISRAM6EN         (1 << 3)   /* Bit 3:  AXISRAM6 enable */
#define RCC_MEMENR_AXISRAM5EN         (1 << 2)   /* Bit 2:  AXISRAM5 enable */
#define RCC_MEMENR_AXISRAM4EN         (1 << 1)   /* Bit 1:  AXISRAM4 enable */
#define RCC_MEMENR_AXISRAM3EN         (1 << 0)   /* Bit 0:  AXISRAM3 enable */

#define RCC_MEMENR_ALLAXISRAM         (RCC_MEMENR_AXISRAM1EN | RCC_MEMENR_AXISRAM2EN | \
                                       RCC_MEMENR_AXISRAM3EN | RCC_MEMENR_AXISRAM4EN | \
                                       RCC_MEMENR_AXISRAM5EN | RCC_MEMENR_AXISRAM6EN)
#define RCC_MEMENR_AXISRAM            RCC_MEMENR_ALLAXISRAM
#define RCC_MEMENR_AHBSRAM            ((1 << 4) | (1 << 5))

#define RCC_MEMENSR_AXISRAM           RCC_MEMENR_AXISRAM
#define RCC_MEMENSR_AHBSRAM           RCC_MEMENR_AHBSRAM

/* Bus and miscellaneous clock enable registers */

#define RCC_BUSENR_ALL_BUS            ((1 << 0) | (1 << 1) | (1 << 2) | \
                                       (1 << 3) | (1 << 4) | (1 << 5) | \
                                       (1 << 6) | (1 << 7) | (1 << 8) | \
                                       (1 << 9) | (1 << 10) | \
                                       (1 << 11) | (1 << 12))

#define RCC_BUSENSR_ALL_BUS           RCC_BUSENR_ALL_BUS

#define RCC_MISCENR_XSPIPHYCOMPEN     (1 << 3)
#define RCC_MISCENSR_XSPIPHYCOMPENS   RCC_MISCENR_XSPIPHYCOMPEN

/* AHB3 peripheral clock enable register */

#define RCC_AHB3ENR_RIFSCEN           (1 << 9)
#define RCC_AHB3ENSR_RIFSCENS         RCC_AHB3ENR_RIFSCEN

/* AHB4 peripheral clock enable register */

#define RCC_AHB4ENR_PWREN             (1 << 18)  /* Bit 18: PWR enable */
#define RCC_AHB4ENR_GPIOQEN           (1 << 16)  /* Bit 16: GPIOQ enable */
#define RCC_AHB4ENR_GPIOPEN           (1 << 15)  /* Bit 15: GPIOP enable */
#define RCC_AHB4ENR_GPIOOEN           (1 << 14)  /* Bit 14: GPIOO enable */
#define RCC_AHB4ENR_GPIONEN           (1 << 13)  /* Bit 13: GPION enable */
#define RCC_AHB4ENR_GPIOHEN           (1 << 7)   /* Bit 7:  GPIOH enable */
#define RCC_AHB4ENR_GPIOGEN           (1 << 6)   /* Bit 6:  GPIOG enable */
#define RCC_AHB4ENR_GPIOFEN           (1 << 5)   /* Bit 5:  GPIOF enable */
#define RCC_AHB4ENR_GPIOEEN           (1 << 4)   /* Bit 4:  GPIOE enable */
#define RCC_AHB4ENR_GPIODEN           (1 << 3)   /* Bit 3:  GPIOD enable */
#define RCC_AHB4ENR_GPIOCEN           (1 << 2)   /* Bit 2:  GPIOC enable */
#define RCC_AHB4ENR_GPIOBEN           (1 << 1)   /* Bit 1:  GPIOB enable */
#define RCC_AHB4ENR_GPIOAEN           (1 << 0)   /* Bit 0:  GPIOA enable */

#define RCC_AHB4ENSR_GPIOAENS         RCC_AHB4ENR_GPIOAEN
#define RCC_AHB4ENSR_GPIOBENS         RCC_AHB4ENR_GPIOBEN
#define RCC_AHB4ENSR_GPIOCENS         RCC_AHB4ENR_GPIOCEN
#define RCC_AHB4ENSR_GPIODENS         RCC_AHB4ENR_GPIODEN
#define RCC_AHB4ENSR_GPIOEENS         RCC_AHB4ENR_GPIOEEN
#define RCC_AHB4ENSR_GPIOFENS         RCC_AHB4ENR_GPIOFEN
#define RCC_AHB4ENSR_GPIOGENS         RCC_AHB4ENR_GPIOGEN
#define RCC_AHB4ENSR_GPIOHENS         RCC_AHB4ENR_GPIOHEN
#define RCC_AHB4ENSR_GPIONENS         RCC_AHB4ENR_GPIONEN
#define RCC_AHB4ENSR_GPIOOENS         RCC_AHB4ENR_GPIOOEN
#define RCC_AHB4ENSR_GPIOPENS         RCC_AHB4ENR_GPIOPEN
#define RCC_AHB4ENSR_GPIOQENS         RCC_AHB4ENR_GPIOQEN
#define RCC_AHB4ENSR_PWRENS           RCC_AHB4ENR_PWREN

/* AHB5 peripheral clock enable/reset registers */

#define RCC_AHB5ENR_XSPI1EN           (1 << 5)
#define RCC_AHB5ENR_XSPI2EN           (1 << 12)
#define RCC_AHB5ENR_XSPIMEN           (1 << 13)
#define RCC_AHB5ENR_GPU2DEN           (1 << 20)

#define RCC_AHB5ENSR_XSPI1ENS         RCC_AHB5ENR_XSPI1EN
#define RCC_AHB5ENSR_XSPI2ENS         RCC_AHB5ENR_XSPI2EN
#define RCC_AHB5ENSR_XSPIMENS         RCC_AHB5ENR_XSPIMEN
#define RCC_AHB5ENSR_GPU2DENS         RCC_AHB5ENR_GPU2DEN

#define RCC_AHB5RST_XSPI1             RCC_AHB5ENR_XSPI1EN
#define RCC_AHB5RST_XSPI2             RCC_AHB5ENR_XSPI2EN
#define RCC_AHB5RST_XSPIM             RCC_AHB5ENR_XSPIMEN
#define RCC_AHB5RST_GPU2D             RCC_AHB5ENR_GPU2DEN

#define RCC_AHB5RSTR_XSPI1RST         RCC_AHB5RST_XSPI1
#define RCC_AHB5RSTR_XSPI2RST         RCC_AHB5RST_XSPI2
#define RCC_AHB5RSTR_XSPIMRST         RCC_AHB5RST_XSPIM
#define RCC_AHB5RSTR_GPU2DRST         RCC_AHB5RST_GPU2D
#define RCC_AHB5RSTSR_GPU2DRSTS       RCC_AHB5RST_GPU2D
#define RCC_AHB5RSTCR_GPU2DRSTC       RCC_AHB5RST_GPU2D

/* APB1 peripheral clock enable register 1 */

#define RCC_APB1LENR_TIM2EN           (1 << 0)   /* Bit 0:  TIM2 enable */
#define RCC_APB1ENR1_I2C1EN           (1 << 21)
#define RCC_APB1ENR1_I2C2EN           (1 << 22)
#define RCC_APB1ENR1_I2C3EN           (1 << 23)

#define RCC_APB1ENSR1_I2C1ENS         RCC_APB1ENR1_I2C1EN
#define RCC_APB1ENSR1_I2C2ENS         RCC_APB1ENR1_I2C2EN
#define RCC_APB1ENSR1_I2C3ENS         RCC_APB1ENR1_I2C3EN

#define RCC_APB1ENCR1_I2C1ENC         RCC_APB1ENR1_I2C1EN
#define RCC_APB1ENCR1_I2C2ENC         RCC_APB1ENR1_I2C2EN
#define RCC_APB1ENCR1_I2C3ENC         RCC_APB1ENR1_I2C3EN

#define RCC_APB1RSTR1_I2C1RST         RCC_APB1ENR1_I2C1EN
#define RCC_APB1RSTR1_I2C2RST         RCC_APB1ENR1_I2C2EN
#define RCC_APB1RSTR1_I2C3RST         RCC_APB1ENR1_I2C3EN

#define RCC_APB1RSTCR1_I2C1RSTC       RCC_APB1RSTR1_I2C1RST
#define RCC_APB1RSTCR1_I2C2RSTC       RCC_APB1RSTR1_I2C2RST
#define RCC_APB1RSTCR1_I2C3RSTC       RCC_APB1RSTR1_I2C3RST

/* APB2 peripheral clock enable register */

#define RCC_APB2ENR_USART1EN          (1 << 4)   /* Bit 4:  USART1 enable */
#define RCC_APB2ENSR_USART1ENS        RCC_APB2ENR_USART1EN
#define RCC_APB2RSTR_USART1RST        RCC_APB2ENR_USART1EN

/* APB4 peripheral clock enable register 2 */

#define RCC_APB4HENR_BSECEN           (1 << 1)   /* Bit 1:  BSEC enable */
#define RCC_APB4HENR_SYSCFGEN         (1 << 0)   /* Bit 0:  SYSCFG enable */
#define RCC_APB4ENR2_BSECEN           RCC_APB4HENR_BSECEN
#define RCC_APB4ENR2_SYSCFGEN         RCC_APB4HENR_SYSCFGEN
#define RCC_APB4ENSR2_BSECENS         RCC_APB4HENR_BSECEN
#define RCC_APB4ENSR2_SYSCFGENS       RCC_APB4HENR_SYSCFGEN

/* APB5 peripheral clock enable/reset registers */

#define RCC_APB5ENR_LTDCEN            (1 << 1)
#define RCC_APB5ENSR_LTDCENS          RCC_APB5ENR_LTDCEN
#define RCC_APB5RSTR_LTDCRST          RCC_APB5ENR_LTDCEN

/* Bus clock enable in Sleep mode */

#define RCC_BUSLPENR_ACLKNCLPEN       (1 << 1)   /* Bit 1:  ACLKNC clock enable in CSLEEP */
#define RCC_BUSLPENR_ACLKNLPEN        (1 << 0)   /* Bit 0:  ACLKN clock enable in CSLEEP */

/* SRAM clock enable in Sleep mode */

#define RCC_MEMLPENR_CACHEAXIRAMLPEN  (1 << 10)  /* Bit 10: CACHEAXIRAM enable in CSLEEP */
#define RCC_MEMLPENR_AXISRAM2LPEN     (1 << 8)
#define RCC_MEMLPENR_AXISRAM1LPEN     (1 << 7)
#define RCC_MEMLPENR_AXISRAM6LPEN     (1 << 3)
#define RCC_MEMLPENR_AXISRAM5LPEN     (1 << 2)
#define RCC_MEMLPENR_AXISRAM4LPEN     (1 << 1)
#define RCC_MEMLPENR_AXISRAM3LPEN     (1 << 0)

#define RCC_MEMLPENR_ALLAXISRAM       (RCC_MEMLPENR_AXISRAM1LPEN | RCC_MEMLPENR_AXISRAM2LPEN | \
                                       RCC_MEMLPENR_AXISRAM3LPEN | RCC_MEMLPENR_AXISRAM4LPEN | \
                                       RCC_MEMLPENR_AXISRAM5LPEN | RCC_MEMLPENR_AXISRAM6LPEN)

/* APB1 peripheral clock enable in Sleep mode (register 1) */

#define RCC_APB1LLPENR_TIM2LPEN       (1 << 0)   /* Bit 0:  TIM2 enable in CSLEEP */

/* APB2 peripheral clock enable in Sleep mode */

#define RCC_APB2LPENR_USART1LPEN      (1 << 4)   /* Bit 4:  USART1 enable in CSLEEP */

/* Peripheral kernel clock select register 4 */

#define RCC_CCIPR4_I2C2SEL_SHIFT      (4)
#define RCC_CCIPR4_I2C2SEL_MASK       (0x7 << RCC_CCIPR4_I2C2SEL_SHIFT)
#define RCC_CCIPR4_I2C2SEL_PCLK1      (0 << RCC_CCIPR4_I2C2SEL_SHIFT)
#define RCC_CCIPR4_LTDCSEL_SHIFT      (24)
#define RCC_CCIPR4_LTDCSEL_MASK       (0x3 << RCC_CCIPR4_LTDCSEL_SHIFT)
#define RCC_CCIPR4_LTDCSEL_IC16       (2 << RCC_CCIPR4_LTDCSEL_SHIFT)

/* Peripheral kernel clock select register 6 */

#define RCC_CCIPR6_XSPI1SEL_SHIFT     (0)
#define RCC_CCIPR6_XSPI1SEL_MASK      (0x3 << RCC_CCIPR6_XSPI1SEL_SHIFT)
#define RCC_CCIPR6_XSPI1SEL_HCLK      (0 << RCC_CCIPR6_XSPI1SEL_SHIFT)
#define RCC_CCIPR6_XSPI2SEL_SHIFT     (4)
#define RCC_CCIPR6_XSPI2SEL_MASK      (0x3 << RCC_CCIPR6_XSPI2SEL_SHIFT)
#define RCC_CCIPR6_XSPI2SEL_IC3       (2 << RCC_CCIPR6_XSPI2SEL_SHIFT)

/* Peripheral kernel clock select register 13 */

#define RCC_CCIPR13_USART1SEL_SHIFT   (0)
#define RCC_CCIPR13_USART1SEL_MASK    (0x7 << RCC_CCIPR13_USART1SEL_SHIFT)
#define RCC_CCIPR13_USART1SEL_PCLK2   (0 << RCC_CCIPR13_USART1SEL_SHIFT)
#define RCC_CCIPR13_USART1SEL_HSI     (6 << RCC_CCIPR13_USART1SEL_SHIFT)

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6XXX_RCC_H */
