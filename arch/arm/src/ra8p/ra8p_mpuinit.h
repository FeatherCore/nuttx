/****************************************************************************
 * arch/arm/src/ra8p/ra8p_mpuinit.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_RA8P_RA8P_MPUINIT_H
#define __ARCH_ARM_SRC_RA8P_RA8P_MPUINIT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <sys/types.h>

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
void ra8p_mpuinitialize(void);

#  ifdef CONFIG_BUILD_PROTECTED
void ra8p_mpu_uheap(uintptr_t start, size_t size);
#  else
#    define ra8p_mpu_uheap(start, size)
#  endif
#else
#  define ra8p_mpuinitialize()
#  define ra8p_mpu_uheap(start, size)
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_ARM_SRC_RA8P_RA8P_MPUINIT_H */
