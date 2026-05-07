/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32h7rs_pwr.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_PWR_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_PWR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32h7rs_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32H7RS_PWR_CSR2               (STM32H7RS_PWR_BASE + 0x000cu)
#define STM32H7RS_PWR_CSR4               (STM32H7RS_PWR_BASE + 0x0014u)

#define PWR_CSR2_EN_XSPIM1               (1u << 14)
#define PWR_CSR2_EN_XSPIM2               (1u << 15)

#define PWR_CSR4_VOS                     (1u << 0)
#define PWR_CSR4_VOSRDY                  (1u << 1)

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_PWR_H */
