/****************************************************************************
 * arch/arm/src/ra8p/ra8p_start.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/cache.h>
#include <nuttx/init.h>
#include <arch/board/board.h>

#include "arm_internal.h"
#ifdef CONFIG_ARM_MPU
#  include "mpu.h"
#endif
#include "ra8p_clockconfig.h"
#ifdef CONFIG_BUILD_PROTECTED
#  include "ra8p_mpuinit.h"
#  include "ra8p_userspace.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HEAP_BASE ((uintptr_t)_ebss + CONFIG_IDLETHREAD_STACKSIZE)

#ifdef CONFIG_ARM_MPU
#  define RA8P_NORMAL_CACHEABLE \
     (MPU_RBAR_AP_RWNO | MPU_RBAR_SH_NO)

#  define RA8P_NORMAL_CACHEABLE_RO \
     (MPU_RBAR_AP_RONO | MPU_RBAR_SH_NO)

#  define RA8P_NORMAL_CACHEABLE_XN \
     (RA8P_NORMAL_CACHEABLE | MPU_RBAR_XN)

#  ifdef CONFIG_BUILD_PROTECTED
#    define RA8P_SDRAM_MPU_FLAGS1 \
       (MPU_RBAR_AP_RWNO | MPU_RBAR_SH_NO | MPU_RBAR_XN)
#    define RA8P_SDRAM_MPU_FLAGS2 \
       (MPU_RLAR_WRITE_BACK | MPU_RLAR_PXN)
#  else
#    define RA8P_SDRAM_MPU_FLAGS1 RA8P_NORMAL_CACHEABLE_XN
#    define RA8P_SDRAM_MPU_FLAGS2 MPU_RLAR_WRITE_BACK
#  endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void ra8p_cache_setup(void)
{
#ifdef CONFIG_ARM_MPU
  static const struct mpu_region_s regions[] =
    {
      {
        .base   = CONFIG_RAM_START,
        .size   = CONFIG_RAM_SIZE,
        .flags1 = RA8P_NORMAL_CACHEABLE_XN,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
      {
        .base   = RA8P_MRAM_START,
        .size   = RA8P_MRAM_SIZE,
        .flags1 = RA8P_NORMAL_CACHEABLE_RO,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
#ifdef CONFIG_RA8P_OSPI_B
      {
        .base   = RA8P_OSPI0_CS1_START,
        .size   = RA8P_OSPI_NOR_SIZE,
        .flags1 = RA8P_NORMAL_CACHEABLE_RO,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
#endif
#ifdef CONFIG_RA8P_SDRAM
      {
        .base   = RA8P_SDRAM_START,
        .size   = RA8P_SDRAM_SIZE,
        .flags1 = RA8P_SDRAM_MPU_FLAGS1,
        .flags2 = RA8P_SDRAM_MPU_FLAGS2
      },
#endif
    };

  mpu_reset();
  mpu_initialize(regions, sizeof(regions) / sizeof(regions[0]),
                 false, true);
#endif

#ifdef CONFIG_ARMV8M_ICACHE
  up_enable_icache();
#endif
#ifdef CONFIG_ARMV8M_DCACHE
  up_enable_dcache();
#endif
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

const uintptr_t g_idle_topstack = HEAP_BASE;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void ra8p_boardinitialize(void);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_ARMV8M_STACKCHECK
void __start(void) noinstrument_function;
#endif

void __start(void)
{
  const uint32_t *src;
  uint32_t *dest;

#ifdef CONFIG_ARMV8M_STACKCHECK
  __asm__ volatile
    ("sub r10, sp, %0" : : "r" (CONFIG_IDLETHREAD_STACKSIZE - 64) :);
#endif

  for (dest = (uint32_t *)_sbss; dest < (uint32_t *)_ebss; )
    {
      *dest++ = 0;
    }

  for (src = (const uint32_t *)_eronly,
       dest = (uint32_t *)_sdata; dest < (uint32_t *)_edata; )
    {
      *dest++ = *src++;
    }

  ra8p_clockconfig();
  arm_fpuconfig();
  ra8p_cache_setup();

#ifdef CONFIG_ARMV8M_STACKCHECK
  arm_stack_check_init();
#endif

  ra8p_boardinitialize();

#ifdef CONFIG_BUILD_PROTECTED
  ra8p_userspace();
  ra8p_mpuinitialize();
#endif

  nx_start();

  for (; ; )
    {
    }
}
