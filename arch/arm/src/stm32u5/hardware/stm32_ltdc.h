/****************************************************************************
 * arch/arm/src/stm32u5/hardware/stm32_ltdc.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_LTDC_H
#define __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_LTDC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_LTDC_SSCR       (STM32_LTDC_BASE + 0x0008)
#define STM32_LTDC_BPCR       (STM32_LTDC_BASE + 0x000c)
#define STM32_LTDC_AWCR       (STM32_LTDC_BASE + 0x0010)
#define STM32_LTDC_TWCR       (STM32_LTDC_BASE + 0x0014)
#define STM32_LTDC_GCR        (STM32_LTDC_BASE + 0x0018)
#define STM32_LTDC_SRCR       (STM32_LTDC_BASE + 0x0024)
#define STM32_LTDC_BCCR       (STM32_LTDC_BASE + 0x002c)
#define STM32_LTDC_IER        (STM32_LTDC_BASE + 0x0034)
#define STM32_LTDC_ISR        (STM32_LTDC_BASE + 0x0038)
#define STM32_LTDC_ICR        (STM32_LTDC_BASE + 0x003c)
#define STM32_LTDC_LIPCR      (STM32_LTDC_BASE + 0x0040)
#define STM32_LTDC_CPSR       (STM32_LTDC_BASE + 0x0044)
#define STM32_LTDC_CDSR       (STM32_LTDC_BASE + 0x0048)

#define STM32_LTDC_L1CR       (STM32_LTDC_BASE + 0x0084)
#define STM32_LTDC_L1WHPCR    (STM32_LTDC_BASE + 0x0088)
#define STM32_LTDC_L1WVPCR    (STM32_LTDC_BASE + 0x008c)
#define STM32_LTDC_L1CKCR     (STM32_LTDC_BASE + 0x0090)
#define STM32_LTDC_L1PFCR     (STM32_LTDC_BASE + 0x0094)
#define STM32_LTDC_L1CACR     (STM32_LTDC_BASE + 0x0098)
#define STM32_LTDC_L1DCCR     (STM32_LTDC_BASE + 0x009c)
#define STM32_LTDC_L1BFCR     (STM32_LTDC_BASE + 0x00a0)
#define STM32_LTDC_L1CFBAR    (STM32_LTDC_BASE + 0x00ac)
#define STM32_LTDC_L1CFBLR    (STM32_LTDC_BASE + 0x00b0)
#define STM32_LTDC_L1CFBLNR   (STM32_LTDC_BASE + 0x00b4)

#define LTDC_SSCR_VSH(n)      (((n) & 0x7ff) << 0)
#define LTDC_SSCR_HSW(n)      (((n) & 0xfff) << 16)

#define LTDC_BPCR_AVBP(n)     (((n) & 0x7ff) << 0)
#define LTDC_BPCR_AHBP(n)     (((n) & 0xfff) << 16)

#define LTDC_AWCR_AAH(n)      (((n) & 0x7ff) << 0)
#define LTDC_AWCR_AAW(n)      (((n) & 0xfff) << 16)

#define LTDC_TWCR_TOTALH(n)   (((n) & 0x7ff) << 0)
#define LTDC_TWCR_TOTALW(n)   (((n) & 0xfff) << 16)

#define LTDC_GCR_LTDCEN       (1 << 0)
#define LTDC_GCR_PCPOL        (1 << 28)
#define LTDC_GCR_DEPOL        (1 << 29)
#define LTDC_GCR_VSPOL        (1 << 30)
#define LTDC_GCR_HSPOL        (1 << 31)

#define LTDC_SRCR_IMR         (1 << 0)
#define LTDC_SRCR_VBR         (1 << 1)

#define LTDC_IER_LIE          (1 << 0)
#define LTDC_IER_FUIE         (1 << 1)
#define LTDC_IER_TERRIE       (1 << 2)
#define LTDC_IER_RRIE         (1 << 3)

#define LTDC_ISR_LIF          (1 << 0)
#define LTDC_ISR_FUIF         (1 << 1)
#define LTDC_ISR_TERRIF       (1 << 2)
#define LTDC_ISR_RRIF         (1 << 3)

#define LTDC_ICR_CLIF         (1 << 0)
#define LTDC_ICR_CFUIF        (1 << 1)
#define LTDC_ICR_CTERRIF      (1 << 2)
#define LTDC_ICR_CRRIF        (1 << 3)

#define LTDC_LIPCR_LIPOS(n)   (((n) & 0x7ff) << 0)

#define LTDC_CPSR_CYPOS_SHIFT 0
#define LTDC_CPSR_CYPOS_MASK  (0xffff << LTDC_CPSR_CYPOS_SHIFT)
#define LTDC_CPSR_CXPOS_SHIFT 16
#define LTDC_CPSR_CXPOS_MASK  (0xffff << LTDC_CPSR_CXPOS_SHIFT)

#define LTDC_CDSR_VDES        (1 << 0)
#define LTDC_CDSR_HDES        (1 << 1)
#define LTDC_CDSR_VSYNCS      (1 << 2)
#define LTDC_CDSR_HSYNCS      (1 << 3)

#define LTDC_LXCR_LEN         (1 << 0)

#define LTDC_LXWHPCR_WHST(n)  (((n) & 0xfff) << 0)
#define LTDC_LXWHPCR_WHSP(n)  (((n) & 0xfff) << 16)

#define LTDC_LXWVPCR_WVST(n)  (((n) & 0xfff) << 0)
#define LTDC_LXWVPCR_WVSP(n)  (((n) & 0xfff) << 16)

#define LTDC_LXPFCR_ARGB8888  0
#define LTDC_LXPFCR_RGB888    1
#define LTDC_LXPFCR_RGB565    2

#define LTDC_LXCACR_CONSTA(n) (((n) & 0xff) << 0)

#define LTDC_LXBFCR_BF1(n)    (((n) & 0x7) << 8)
#define LTDC_LXBFCR_BF2(n)    (((n) & 0x7) << 0)
#define LTDC_LXBFCR_CA        4
#define LTDC_LXBFCR_1CA       5
#define LTDC_LXBFCR_PAXCA     6
#define LTDC_LXBFCR_1PAXCA    7

#define LTDC_LXCFBLR_CFBLL(n) (((n) & 0x1fff) << 0)
#define LTDC_LXCFBLR_CFBP(n)  (((n) & 0x1fff) << 16)

#endif /* __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_LTDC_H */
