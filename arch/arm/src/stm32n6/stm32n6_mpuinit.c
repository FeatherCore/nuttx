/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_mpuinit.c
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
#include "stm32n6_mpuinit.h"

#ifdef CONFIG_ARM_MPU

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_USER_FLASH \
  (MPU_RBAR_AP_RORO | MPU_RBAR_SH_NO)

#define STM32N6_USER_INTSRAM \
  (MPU_RBAR_AP_RWRW | MPU_RBAR_SH_NO | MPU_RBAR_XN)

#define STM32N6_USER_EXTSRAM \
  (MPU_RBAR_AP_RWRW | MPU_RBAR_SH_NO | MPU_RBAR_XN)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_mpuinitialize
 *
 * Description:
 *   Add protected build user regions after the generic STM32N6 cache/MPU
 *   setup installed the privileged default memory map.
 *
 ****************************************************************************/

void stm32n6_mpuinitialize(void)
{
#ifdef CONFIG_BUILD_PROTECTED
  uintptr_t textstart = CONFIG_NUTTX_USERSPACE;
  uintptr_t datastart = MIN(USERSPACE->us_datastart, USERSPACE->us_bssstart);
  uintptr_t dataend   = MAX(USERSPACE->us_dataend, USERSPACE->us_bssend);

  DEBUGASSERT(USERSPACE->us_textstart >= textstart &&
              USERSPACE->us_textend >= USERSPACE->us_textstart &&
              dataend >= datastart);

  mpu_configure_region(textstart, USERSPACE->us_textend - textstart,
                       STM32N6_USER_FLASH, MPU_RLAR_WRITE_BACK);
  mpu_configure_region(datastart, dataend - datastart,
                       STM32N6_USER_INTSRAM, MPU_RLAR_WRITE_BACK);
#endif
}

#ifdef CONFIG_BUILD_PROTECTED
/****************************************************************************
 * Name: stm32n6_mpu_uheap
 *
 * Description:
 *   Map a user heap region.  STM32N6570-DK uses this for XSPI1 PSRAM.
 *
 ****************************************************************************/

void stm32n6_mpu_uheap(uintptr_t start, size_t size)
{
  mpu_configure_region(start, size, STM32N6_USER_EXTSRAM,
                       MPU_RLAR_WRITE_BACK | MPU_RLAR_PXN);
}
#endif

#endif /* CONFIG_ARM_MPU */
