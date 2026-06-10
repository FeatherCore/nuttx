/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32_exti.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_EXTI_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_EXTI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_EXTI_MASK(n)              (1u << (n))

#define STM32_EXTI_RTSR1_OFFSET         0x0000u
#define STM32_EXTI_FTSR1_OFFSET         0x0004u
#define STM32_EXTI_SWIER1_OFFSET        0x0008u
#define STM32_EXTI_CPUIMR1_OFFSET       0x0080u
#define STM32_EXTI_CPUEMR1_OFFSET       0x0084u
#define STM32_EXTI_CPUPR1_OFFSET        0x0088u

#define STM32_EXTI_RTSR1                (STM32_EXTI_BASE + \
                                         STM32_EXTI_RTSR1_OFFSET)
#define STM32_EXTI_FTSR1                (STM32_EXTI_BASE + \
                                         STM32_EXTI_FTSR1_OFFSET)
#define STM32_EXTI_SWIER1               (STM32_EXTI_BASE + \
                                         STM32_EXTI_SWIER1_OFFSET)
#define STM32_EXTI_CPUIMR1              (STM32_EXTI_BASE + \
                                         STM32_EXTI_CPUIMR1_OFFSET)
#define STM32_EXTI_CPUEMR1              (STM32_EXTI_BASE + \
                                         STM32_EXTI_CPUEMR1_OFFSET)
#define STM32_EXTI_CPUPR1               (STM32_EXTI_BASE + \
                                         STM32_EXTI_CPUPR1_OFFSET)

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_EXTI_H */
