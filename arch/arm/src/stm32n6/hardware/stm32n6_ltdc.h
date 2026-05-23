/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32n6_ltdc.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_LTDC_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_LTDC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "hardware/stm32n6_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_LTDC_SSCR               (STM32N6_LTDC_BASE + 0x0008)
#define STM32N6_LTDC_BPCR               (STM32N6_LTDC_BASE + 0x000c)
#define STM32N6_LTDC_AWCR               (STM32N6_LTDC_BASE + 0x0010)
#define STM32N6_LTDC_TWCR               (STM32N6_LTDC_BASE + 0x0014)
#define STM32N6_LTDC_GCR                (STM32N6_LTDC_BASE + 0x0018)
#define STM32N6_LTDC_SRCR               (STM32N6_LTDC_BASE + 0x0024)
#define STM32N6_LTDC_BCCR               (STM32N6_LTDC_BASE + 0x002c)
#define STM32N6_LTDC_IER                (STM32N6_LTDC_BASE + 0x0034)
#define STM32N6_LTDC_ISR                (STM32N6_LTDC_BASE + 0x0038)
#define STM32N6_LTDC_ICR                (STM32N6_LTDC_BASE + 0x003c)
#define STM32N6_LTDC_CPSR               (STM32N6_LTDC_BASE + 0x0044)
#define STM32N6_LTDC_CDSR               (STM32N6_LTDC_BASE + 0x0048)
#define STM32N6_LTDC_EDCR               (STM32N6_LTDC_BASE + 0x0060)
#define STM32N6_LTDC_IER2               (STM32N6_LTDC_BASE + 0x0064)
#define STM32N6_LTDC_ISR2               (STM32N6_LTDC_BASE + 0x0068)
#define STM32N6_LTDC_ICR2               (STM32N6_LTDC_BASE + 0x006c)
#define STM32N6_LTDC_ECRCR              (STM32N6_LTDC_BASE + 0x0078)
#define STM32N6_LTDC_CCRCR              (STM32N6_LTDC_BASE + 0x007c)
#define STM32N6_LTDC_FUTR               (STM32N6_LTDC_BASE + 0x0090)

#define STM32N6_LTDC_L1_BASE            (STM32N6_LTDC_BASE + 0x0100)
#define STM32N6_LTDC_LC0R               (STM32N6_LTDC_L1_BASE + 0x0000)
#define STM32N6_LTDC_LC1R               (STM32N6_LTDC_L1_BASE + 0x0004)
#define STM32N6_LTDC_LRCR               (STM32N6_LTDC_L1_BASE + 0x0008)
#define STM32N6_LTDC_LCR                (STM32N6_LTDC_L1_BASE + 0x000c)
#define STM32N6_LTDC_LWHPCR             (STM32N6_LTDC_L1_BASE + 0x0010)
#define STM32N6_LTDC_LWVPCR             (STM32N6_LTDC_L1_BASE + 0x0014)
#define STM32N6_LTDC_LCKCR              (STM32N6_LTDC_L1_BASE + 0x0018)
#define STM32N6_LTDC_LPFCR              (STM32N6_LTDC_L1_BASE + 0x001c)
#define STM32N6_LTDC_LCACR              (STM32N6_LTDC_L1_BASE + 0x0020)
#define STM32N6_LTDC_LDCCR              (STM32N6_LTDC_L1_BASE + 0x0024)
#define STM32N6_LTDC_LBFCR              (STM32N6_LTDC_L1_BASE + 0x0028)
#define STM32N6_LTDC_LBLCR              (STM32N6_LTDC_L1_BASE + 0x002c)
#define STM32N6_LTDC_LPCR               (STM32N6_LTDC_L1_BASE + 0x0030)
#define STM32N6_LTDC_LCFBAR             (STM32N6_LTDC_L1_BASE + 0x0034)
#define STM32N6_LTDC_LCFBLR             (STM32N6_LTDC_L1_BASE + 0x0038)
#define STM32N6_LTDC_LCFBLNR            (STM32N6_LTDC_L1_BASE + 0x003c)
#define STM32N6_LTDC_LFPF0R             (STM32N6_LTDC_L1_BASE + 0x0074)
#define STM32N6_LTDC_LFPF1R             (STM32N6_LTDC_L1_BASE + 0x0078)

#define LTDC_GCR_LTDCEN                 (1 << 0)
#define LTDC_GCR_BCKEN                  (1 << 17)
#define LTDC_GCR_CRCEN                  (1 << 19)
#define LTDC_SRCR_IMR                   (1 << 0)
#define LTDC_SRCR_VBR                   (1 << 1)
#define LTDC_LRCR_IMR                   (1 << 0)
#define LTDC_LRCR_VBR                   (1 << 1)
#define LTDC_LRCR_GRMSK                 (1 << 2)
#define LTDC_LCR_LEN                    (1 << 0)

#define LTDC_IER_LIE                    (1 << 0)
#define LTDC_IER_FUWIE                  (1 << 1)
#define LTDC_IER_TERRIE                 (1 << 2)
#define LTDC_IER_RRIE                   (1 << 3)
#define LTDC_IER_FUIE                   (1 << 6)
#define LTDC_IER_CRCIE                  (1 << 7)

#define LTDC_ISR_LIF                    (1 << 0)
#define LTDC_ISR_FUWIF                  (1 << 1)
#define LTDC_ISR_TERRIF                 (1 << 2)
#define LTDC_ISR_RRIF                   (1 << 3)
#define LTDC_ISR_FUIF                   (1 << 6)
#define LTDC_ISR_CRCIF                  (1 << 7)

#define LTDC_ICR_CLIF                   (1 << 0)
#define LTDC_ICR_CFUWIF                 (1 << 1)
#define LTDC_ICR_CTERRIF                (1 << 2)
#define LTDC_ICR_CRRIF                  (1 << 3)
#define LTDC_ICR_CFUIF                  (1 << 6)
#define LTDC_ICR_CCRCIF                 (1 << 7)

#define LTDC_PIXEL_FORMAT_RGB565        4

#define LTDC_BLENDING_FACTOR1_CA        (4 << 8)
#define LTDC_BLENDING_FACTOR2_CA        (5 << 0)

#define LTDC_FULL_ALPHA                 255

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_LTDC_H */
