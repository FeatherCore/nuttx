/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32h7rs_lptim.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_LPTIM_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_LPTIM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32h7rs_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32H7RS_LPTIM_ISR_OFFSET       0x0000u
#define STM32H7RS_LPTIM_ICR_OFFSET       0x0004u
#define STM32H7RS_LPTIM_DIER_OFFSET      0x0008u
#define STM32H7RS_LPTIM_CFGR_OFFSET      0x000cu
#define STM32H7RS_LPTIM_CR_OFFSET        0x0010u
#define STM32H7RS_LPTIM_CCR1_OFFSET      0x0014u
#define STM32H7RS_LPTIM_ARR_OFFSET       0x0018u
#define STM32H7RS_LPTIM_CNT_OFFSET       0x001cu
#define STM32H7RS_LPTIM_CFGR2_OFFSET     0x0024u
#define STM32H7RS_LPTIM_RCR_OFFSET       0x0028u
#define STM32H7RS_LPTIM_CCMR1_OFFSET     0x002cu
#define STM32H7RS_LPTIM_CCR2_OFFSET      0x0034u

#define STM32H7RS_LPTIM1_ISR             (STM32H7RS_LPTIM1_BASE + \
                                          STM32H7RS_LPTIM_ISR_OFFSET)
#define STM32H7RS_LPTIM1_ICR             (STM32H7RS_LPTIM1_BASE + \
                                          STM32H7RS_LPTIM_ICR_OFFSET)
#define STM32H7RS_LPTIM1_DIER            (STM32H7RS_LPTIM1_BASE + \
                                          STM32H7RS_LPTIM_DIER_OFFSET)
#define STM32H7RS_LPTIM1_CFGR            (STM32H7RS_LPTIM1_BASE + \
                                          STM32H7RS_LPTIM_CFGR_OFFSET)
#define STM32H7RS_LPTIM1_CR              (STM32H7RS_LPTIM1_BASE + \
                                          STM32H7RS_LPTIM_CR_OFFSET)
#define STM32H7RS_LPTIM1_CCR1            (STM32H7RS_LPTIM1_BASE + \
                                          STM32H7RS_LPTIM_CCR1_OFFSET)
#define STM32H7RS_LPTIM1_ARR             (STM32H7RS_LPTIM1_BASE + \
                                          STM32H7RS_LPTIM_ARR_OFFSET)
#define STM32H7RS_LPTIM1_CNT             (STM32H7RS_LPTIM1_BASE + \
                                          STM32H7RS_LPTIM_CNT_OFFSET)
#define STM32H7RS_LPTIM1_CFGR2           (STM32H7RS_LPTIM1_BASE + \
                                          STM32H7RS_LPTIM_CFGR2_OFFSET)
#define STM32H7RS_LPTIM1_RCR             (STM32H7RS_LPTIM1_BASE + \
                                          STM32H7RS_LPTIM_RCR_OFFSET)
#define STM32H7RS_LPTIM1_CCMR1           (STM32H7RS_LPTIM1_BASE + \
                                          STM32H7RS_LPTIM_CCMR1_OFFSET)
#define STM32H7RS_LPTIM1_CCR2            (STM32H7RS_LPTIM1_BASE + \
                                          STM32H7RS_LPTIM_CCR2_OFFSET)

#define LPTIM_ISR_ARROK                  (1u << 4)
#define LPTIM_ISR_REPOK                  (1u << 8)
#define LPTIM_ISR_CMP2OK                 (1u << 19)

#define LPTIM_ICR_ARROKCF                (1u << 4)
#define LPTIM_ICR_REPOKCF                (1u << 8)
#define LPTIM_ICR_CMP2OKCF               (1u << 19)

#define LPTIM_CR_ENABLE                  (1u << 0)
#define LPTIM_CR_CNTSTRT                 (1u << 2)

#define LPTIM_CFGR_WAVE                  (1u << 20)

#define LPTIM_CCMR1_CC2SEL               (1u << 16)
#define LPTIM_CCMR1_CC2E                 (1u << 17)
#define LPTIM_CCMR1_CC2P_SHIFT           18
#define LPTIM_CCMR1_CC2P_MASK            (3u << LPTIM_CCMR1_CC2P_SHIFT)
#define LPTIM_CCMR1_CC2P_LOW             (1u << LPTIM_CCMR1_CC2P_SHIFT)

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_LPTIM_H */
