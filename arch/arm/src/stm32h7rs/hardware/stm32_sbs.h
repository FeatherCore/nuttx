/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32_sbs.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_SBS_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_SBS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_SBS_CCCSR_OFFSET           0x0110u
#define STM32_SBS_EXTICR_OFFSET(n)       (0x0130u + (((n) >> 2) << 2))

#define STM32_SBS_CCCSR                  (STM32_SBS_BASE + \
                                         STM32_SBS_CCCSR_OFFSET)
#define STM32_SBS_EXTICR(n)              (STM32_SBS_BASE + \
                                         STM32_SBS_EXTICR_OFFSET(n))

#define SBS_CCCSR_XSPI1_COMP_EN          (1u << 2)
#define SBS_CCCSR_XSPI1_COMP_CODESEL     (1u << 3)
#define SBS_CCCSR_XSPI2_COMP_EN          (1u << 4)
#define SBS_CCCSR_XSPI2_COMP_CODESEL     (1u << 5)
#define SBS_CCCSR_XSPI1_COMP_RDY         (1u << 9)
#define SBS_CCCSR_XSPI2_COMP_RDY         (1u << 10)
#define SBS_CCCSR_XSPI1_IOHSLV           (1u << 17)
#define SBS_CCCSR_XSPI2_IOHSLV           (1u << 18)

#define SBS_EXTICR_PORTA                 0u
#define SBS_EXTICR_PORTB                 1u
#define SBS_EXTICR_PORTC                 2u
#define SBS_EXTICR_PORTD                 3u
#define SBS_EXTICR_PORTE                 4u
#define SBS_EXTICR_PORTF                 5u
#define SBS_EXTICR_PORTG                 6u
#define SBS_EXTICR_PORTH                 7u
#define SBS_EXTICR_PORTM                 12u
#define SBS_EXTICR_PORTN                 13u
#define SBS_EXTICR_PORTO                 14u
#define SBS_EXTICR_PORTP                 15u

#define SBS_EXTICR_SHIFT(n)              (((n) & 3u) << 2)
#define SBS_EXTICR_MASK(n)               (15u << SBS_EXTICR_SHIFT(n))
#define SBS_EXTICR_PORT(n, p)            ((uint32_t)(p) << SBS_EXTICR_SHIFT(n))

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_SBS_H */
