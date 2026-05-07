/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32n6_pwr.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_PWR_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_PWR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32n6_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_PWR_SVMCR3             (STM32N6_PWR_BASE + 0x003cu)

#define PWR_SVMCR3_VDDIO2VMEN          (1u << 0)
#define PWR_SVMCR3_VDDIO3VMEN          (1u << 1)
#define PWR_SVMCR3_VDDIO2SV            (1u << 8)
#define PWR_SVMCR3_VDDIO3SV            (1u << 9)
#define PWR_SVMCR3_VDDIO2RDY           (1u << 16)
#define PWR_SVMCR3_VDDIO3RDY           (1u << 17)
#define PWR_SVMCR3_VDDIO2VRSEL         (1u << 25)
#define PWR_SVMCR3_VDDIO3VRSEL         (1u << 26)

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_PWR_H */
