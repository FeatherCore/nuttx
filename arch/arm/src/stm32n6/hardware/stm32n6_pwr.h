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

#define STM32N6_PWR_CR1                (STM32N6_PWR_BASE + 0x0000u)
#define STM32N6_PWR_VOSCR              (STM32N6_PWR_BASE + 0x0020u)
#define STM32N6_PWR_SVMCR3             (STM32N6_PWR_BASE + 0x003cu)

#define PWR_CR1_SDEN                   (1u << 2)
#define PWR_SUPPLY_CONFIG_MASK         PWR_CR1_SDEN

#define PWR_VOSCR_VOS                  (1u << 0)
#define PWR_VOSCR_VOSRDY               (1u << 1)
#define PWR_VOSCR_ACTVOS               (1u << 16)
#define PWR_VOSCR_ACTVOSRDY            (1u << 17)

#define PWR_EXTERNAL_SOURCE_SUPPLY      0u
#define PWR_REGULATOR_VOLTAGE_SCALE0    PWR_VOSCR_VOS
#define PWR_REGULATOR_VOLTAGE_SCALE1    0u

#define PWR_SVMCR3_VDDIO2VMEN          (1u << 0)
#define PWR_SVMCR3_VDDIO3VMEN          (1u << 1)
#define PWR_SVMCR3_VDDIO2SV            (1u << 8)
#define PWR_SVMCR3_VDDIO3SV            (1u << 9)
#define PWR_SVMCR3_VDDIO2RDY           (1u << 16)
#define PWR_SVMCR3_VDDIO3RDY           (1u << 17)
#define PWR_SVMCR3_VDDIO2VRSEL         (1u << 25)
#define PWR_SVMCR3_VDDIO3VRSEL         (1u << 26)

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_PWR_H */
