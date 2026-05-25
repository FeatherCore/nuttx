/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32_mpuinit.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <sys/param.h>

#ifdef CONFIG_BUILD_PROTECTED
#  include <nuttx/userspace.h>
#endif

#include "mpu.h"
#include "stm32_mpuinit.h"

#ifdef CONFIG_ARM_MPU

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_USER_FLASH \
  (MPU_RASR_TEX_NOR | MPU_RASR_C | MPU_RASR_B | MPU_RASR_AP_RORO)

#define STM32_USER_INTSRAM \
  (MPU_RASR_TEX_NOR | MPU_RASR_C | MPU_RASR_B | \
   MPU_RASR_AP_RWRW | MPU_RASR_XN)

#define STM32_USER_EXTSRAM \
  (MPU_RASR_TEX_NOR | MPU_RASR_C | MPU_RASR_B | \
   MPU_RASR_AP_RWRW | MPU_RASR_XN)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_mpuinitialize
 *
 * Description:
 *   Add protected build user regions after the generic STM32H7RS cache/MPU
 *   setup installed the privileged default memory map.
 *
 ****************************************************************************/

void stm32_mpuinitialize(void)
{
#ifdef CONFIG_BUILD_PROTECTED
  uintptr_t datastart = MIN(USERSPACE->us_datastart, USERSPACE->us_bssstart);
  uintptr_t dataend   = MAX(USERSPACE->us_dataend, USERSPACE->us_bssend);

  DEBUGASSERT(USERSPACE->us_textend >= USERSPACE->us_textstart &&
              dataend >= datastart);

  mpu_configure_region(USERSPACE->us_textstart,
                       USERSPACE->us_textend - USERSPACE->us_textstart,
                       STM32_USER_FLASH);
  mpu_configure_region(datastart, dataend - datastart,
                       STM32_USER_INTSRAM);
#endif
}

#ifdef CONFIG_BUILD_PROTECTED
/****************************************************************************
 * Name: stm32_mpu_uheap
 *
 * Description:
 *   Map a user heap region.  STM32H7S78-DK uses this for XSPI1 PSRAM.
 *
 ****************************************************************************/

void stm32_mpu_uheap(uintptr_t start, size_t size)
{
  mpu_configure_region(start, size, STM32_USER_EXTSRAM);
}
#endif

#endif /* CONFIG_ARM_MPU */
