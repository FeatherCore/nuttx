/****************************************************************************
 * arch/arm/src/ra8p/ra8p_mpuinit.c
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
#include "ra8p_mpuinit.h"

#ifdef CONFIG_ARM_MPU

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RA8P_USER_FLASH \
  (MPU_RBAR_AP_RORO | MPU_RBAR_SH_NO)

#define RA8P_USER_INTSRAM \
  (MPU_RBAR_AP_RWRW | MPU_RBAR_SH_NO | MPU_RBAR_XN)

#define RA8P_USER_EXTSRAM \
  (MPU_RBAR_AP_RWRW | MPU_RBAR_SH_NO | MPU_RBAR_XN)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ra8p_mpuinitialize
 *
 * Description:
 *   Add protected build user regions after the generic RA8P MPU setup
 *   installed the privileged default memory map.
 *
 ****************************************************************************/

void ra8p_mpuinitialize(void)
{
#ifdef CONFIG_BUILD_PROTECTED
  uintptr_t datastart = MIN(USERSPACE->us_datastart, USERSPACE->us_bssstart);
  uintptr_t dataend   = MAX(USERSPACE->us_dataend, USERSPACE->us_bssend);

  DEBUGASSERT(USERSPACE->us_textend >= USERSPACE->us_textstart &&
              dataend >= datastart);

  mpu_configure_region(USERSPACE->us_textstart,
                       USERSPACE->us_textend - USERSPACE->us_textstart,
                       RA8P_USER_FLASH, MPU_RLAR_WRITE_BACK);
  mpu_configure_region(datastart, dataend - datastart,
                       RA8P_USER_INTSRAM, MPU_RLAR_WRITE_BACK);
#endif
}

#ifdef CONFIG_BUILD_PROTECTED
/****************************************************************************
 * Name: ra8p_mpu_uheap
 *
 * Description:
 *   Map a protected user heap region.  EK-RA8P1 uses this for SDRAM.
 *
 ****************************************************************************/

void ra8p_mpu_uheap(uintptr_t start, size_t size)
{
  mpu_configure_region(start, size, RA8P_USER_EXTSRAM,
                       MPU_RLAR_WRITE_BACK | MPU_RLAR_PXN);
}
#endif

#endif /* CONFIG_ARM_MPU */
