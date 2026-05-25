/****************************************************************************
 * arch/arm/src/stm32n6/stm32_mpuinit.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <stdint.h>
#include <sys/param.h>

#ifdef CONFIG_BUILD_PROTECTED
#  include <nuttx/userspace.h>
#endif

#include "mpu.h"
#include "arm_internal.h"
#include "stm32_mpuinit.h"

#ifdef CONFIG_ARM_MPU

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_USER_FLASH \
  (MPU_RBAR_AP_RORO | MPU_RBAR_SH_NO)

#define STM32_USER_INTSRAM \
  (MPU_RBAR_AP_RWRW | MPU_RBAR_SH_NO | MPU_RBAR_XN)

#ifdef CONFIG_STM32N6_PSRAM_MPU_SHARE_NONE
#  define STM32_USER_EXTSRAM_SHARE MPU_RBAR_SH_NO
#else
#  define STM32_USER_EXTSRAM_SHARE MPU_RBAR_SH_OUTER
#endif

/* Cortex-M55 treats D-side Shareable Normal memory as effectively
 * non-cacheable.  The fast PSRAM policy for CPU-owned heap and stacks is
 * therefore Non-shareable + cacheable, with optional no-write-allocate MAIR.
 * Shared windows used by LTDC/DMA still need cache maintenance or a separate
 * non-cacheable MPU override.
 */

#ifdef CONFIG_STM32N6_PSRAM_MPU_NONCACHEABLE
#  define STM32_USER_EXTSRAM_ATTR MPU_RLAR_NONCACHEABLE
#elif defined(CONFIG_STM32N6_PSRAM_MPU_WRITE_THROUGH)
#  ifdef CONFIG_STM32N6_PSRAM_MPU_NO_WRITE_ALLOCATE
#    define STM32_USER_EXTSRAM_ATTR MPU_RLAR_WRITE_THROUGH_NWA
#  else
#    define STM32_USER_EXTSRAM_ATTR MPU_RLAR_WRITE_THROUGH
#  endif
#else
#  ifdef CONFIG_STM32N6_PSRAM_MPU_NO_WRITE_ALLOCATE
#    define STM32_USER_EXTSRAM_ATTR MPU_RLAR_WRITE_BACK_NWA
#  else
#    define STM32_USER_EXTSRAM_ATTR MPU_RLAR_WRITE_BACK
#  endif
#endif

#define STM32_USER_EXTSRAM \
  (MPU_RBAR_AP_RWRW | STM32_USER_EXTSRAM_SHARE | MPU_RBAR_XN)

#define STM32_USER_EXTSRAM_RLAR \
  (STM32_USER_EXTSRAM_ATTR | MPU_RLAR_PXN)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_mpuinitialize
 *
 * Description:
 *   Add protected build user regions after the generic STM32N6 cache/MPU
 *   setup installed the privileged default memory map.
 *
 ****************************************************************************/

void stm32_mpuinitialize(void)
{
#ifdef CONFIG_BUILD_PROTECTED
  uintptr_t textstart = CONFIG_NUTTX_USERSPACE;
  uintptr_t datastart = MIN(USERSPACE->us_datastart, USERSPACE->us_bssstart);
  uintptr_t dataend   = MAX(USERSPACE->us_dataend, USERSPACE->us_bssend);

  DEBUGASSERT(USERSPACE->us_textstart >= textstart &&
              USERSPACE->us_textend >= USERSPACE->us_textstart &&
              dataend >= datastart);

  mpu_configure_region(textstart, USERSPACE->us_textend - textstart,
                       STM32_USER_FLASH, MPU_RLAR_WRITE_BACK);
  mpu_configure_region(datastart, dataend - datastart,
                       STM32_USER_INTSRAM, MPU_RLAR_WRITE_BACK);
#endif
}

#ifdef CONFIG_BUILD_PROTECTED
/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_mpu_find_covering_region(uintptr_t start, size_t size,
                                           uintptr_t *region_base,
                                           size_t *region_size)
{
  uintptr_t end = start + size - 1;
  unsigned int region;

  DEBUGASSERT(size > 0);
  DEBUGASSERT(end >= start);

  for (region = 0; region < CONFIG_ARM_MPU_NREGIONS; region++)
    {
      uintptr_t base;
      uintptr_t limit;
      uint32_t rbar;
      uint32_t rlar;

      putreg32(region, MPU_RNR);
      rlar = getreg32(MPU_RLAR);

      if ((rlar & MPU_RLAR_ENABLE) == 0)
        {
          continue;
        }

      rbar = getreg32(MPU_RBAR);
      base = rbar & MPU_RBAR_BASE_MASK;
      limit = (rlar & MPU_RLAR_LIMIT_MASK) | ~MPU_RLAR_LIMIT_MASK;

      if (base <= start && limit >= end)
        {
          /* The default external-SRAM map can already cover PSRAM.  Reusing
           * that region avoids overlapping enabled regions where effective
           * permissions depend on MPU region priority.
           */

          *region_base = base;
          *region_size = limit - base + 1;
          return region;
        }
    }

  return -1;
}

/****************************************************************************
 * Name: stm32_mpu_uheap
 *
 * Description:
 *   Map a user heap region.  STM32N6570-DK uses this for XSPI1 PSRAM.
 *
 ****************************************************************************/

void stm32_mpu_uheap(uintptr_t start, size_t size)
{
  uintptr_t region_base;
  size_t region_size;
  int region;

  region = stm32_mpu_find_covering_region(start, size, &region_base,
                                          &region_size);
  if (region >= 0)
    {
      /* Convert the existing covering region to user-accessible PSRAM with
       * the selected cache/shareability policy instead of stacking a second
       * MPU entry over the same address range.
       */

      mpu_modify_region((unsigned int)region, region_base, region_size,
                        STM32_USER_EXTSRAM,
                        STM32_USER_EXTSRAM_RLAR);
    }
  else
    {
      mpu_configure_region(start, size, STM32_USER_EXTSRAM,
                           STM32_USER_EXTSRAM_RLAR);
    }
}
#endif

#endif /* CONFIG_ARM_MPU */
