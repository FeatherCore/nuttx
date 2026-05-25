/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32_ltdc.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_LTDC_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_LTDC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32_memorymap.h"

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

#define LTDC_SSCR_VSH(n)          (((n) & 0x7ffu) << 0)
#define LTDC_SSCR_HSW(n)          (((n) & 0xfffu) << 16)

#define LTDC_BPCR_AVBP(n)         (((n) & 0x7ffu) << 0)
#define LTDC_BPCR_AHBP(n)         (((n) & 0xfffu) << 16)

#define LTDC_AWCR_AAH(n)          (((n) & 0x7ffu) << 0)
#define LTDC_AWCR_AAW(n)          (((n) & 0xfffu) << 16)

#define LTDC_TWCR_TOTALH(n)       (((n) & 0x7ffu) << 0)
#define LTDC_TWCR_TOTALW(n)       (((n) & 0xfffu) << 16)

#define LTDC_GCR_LTDCEN           (1u << 0)
#define LTDC_GCR_PCPOL            (1u << 28)
#define LTDC_GCR_DEPOL            (1u << 29)
#define LTDC_GCR_VSPOL            (1u << 30)
#define LTDC_GCR_HSPOL            (1u << 31)

#define LTDC_SRCR_IMR             (1u << 0)
#define LTDC_SRCR_VBR             (1u << 1)

#define LTDC_LXCR_LEN             (1u << 0)

#define LTDC_LXWHPCR_WHST(n)      (((n) & 0xfffu) << 0)
#define LTDC_LXWHPCR_WHSP(n)      (((n) & 0xfffu) << 16)

#define LTDC_LXWVPCR_WVST(n)      (((n) & 0x7ffu) << 0)
#define LTDC_LXWVPCR_WVSP(n)      (((n) & 0x7ffu) << 16)

#define LTDC_LXPFCR_RGB565        2u

#define LTDC_LXCACR_CONSTA(n)     (((n) & 0xffu) << 0)

#define LTDC_LXBFCR_BF1(n)        (((n) & 0x7u) << 8)
#define LTDC_LXBFCR_BF2(n)        (((n) & 0x7u) << 0)
#define LTDC_LXBFCR_PAXCA         6u

#define LTDC_LXCFBLR_CFBLL(n)     (((n) & 0x1fffu) << 0)
#define LTDC_LXCFBLR_CFBP(n)      (((n) & 0x1fffu) << 16)

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_LTDC_H */
