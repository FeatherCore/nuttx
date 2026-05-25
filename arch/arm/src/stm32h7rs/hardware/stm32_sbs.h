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

#define STM32_SBS_CCCSR              (STM32_SBS_BASE + 0x0110u)
#define STM32_SBS_CCVALR             (STM32_SBS_BASE + 0x0114u)
#define STM32_SBS_CCSWVALR           (STM32_SBS_BASE + 0x0118u)

#define SBS_CCCSR_XSPI1_COMP_EN          (1u << 2)
#define SBS_CCCSR_XSPI1_COMP_CODESEL     (1u << 3)
#define SBS_CCCSR_XSPI2_COMP_EN          (1u << 4)
#define SBS_CCCSR_XSPI2_COMP_CODESEL     (1u << 5)
#define SBS_CCCSR_XSPI1_COMP_RDY         (1u << 9)
#define SBS_CCCSR_XSPI2_COMP_RDY         (1u << 10)
#define SBS_CCCSR_XSPI1_IOHSLV           (1u << 17)
#define SBS_CCCSR_XSPI2_IOHSLV           (1u << 18)

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_SBS_H */
