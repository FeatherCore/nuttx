/****************************************************************************
 * arch/arm/src/stm32n6/stm32_mpuinit.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32_MPUINIT_H
#define __ARCH_ARM_SRC_STM32N6_STM32_MPUINIT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

#ifdef CONFIG_ARM_MPU
void stm32_mpuinitialize(void);
#else
#  define stm32_mpuinitialize()
#endif

#ifdef CONFIG_BUILD_PROTECTED
void stm32_mpu_uheap(uintptr_t start, size_t size);
#else
#  define stm32_mpu_uheap(start, size)
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_ARM_SRC_STM32N6_STM32_MPUINIT_H */
