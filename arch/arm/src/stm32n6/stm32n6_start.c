/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_start.c
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

#include "arm_internal.h"
#include "chip.h"
#ifdef CONFIG_ARM_MPU
#  include "mpu.h"
#endif
#include "hardware/stm32n6_memorymap.h"
#include "stm32n6.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_ARM_MPU
#  define STM32N6_NORMAL_CACHEABLE \
    (MPU_RBAR_AP_RWNO | MPU_RBAR_SH_NO)

#  define STM32N6_NORMAL_CACHEABLE_RO \
    (MPU_RBAR_AP_RONO | MPU_RBAR_SH_NO)

#  define STM32N6_NORMAL_CACHEABLE_XN \
    (STM32N6_NORMAL_CACHEABLE | MPU_RBAR_XN)
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32n6_cache_setup(void)
{
#ifdef CONFIG_ARM_MPU
  static const struct mpu_region_s regions[] =
    {
      {
        .base   = CONFIG_RAM_START,
        .size   = CONFIG_RAM_SIZE,
        .flags1 = STM32N6_NORMAL_CACHEABLE_XN,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
      {
        .base   = STM32N6_XSPI2_MEM_BASE,
        .size   = STM32N6_XSPI2_NOR_SIZE,
        .flags1 = STM32N6_NORMAL_CACHEABLE_RO,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
      {
        .base   = STM32N6_XSPI1_MEM_BASE,
        .size   = STM32N6_XSPI1_PSRAM_SIZE,
        .flags1 = STM32N6_NORMAL_CACHEABLE_XN,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
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

const uintptr_t g_idle_topstack =
  (uintptr_t)_ebss + CONFIG_IDLETHREAD_STACKSIZE;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

osentry_function
void __start(void)
{
  const uint32_t *src;
  uint32_t *dest;

  for (dest = (uint32_t *)_sbss; dest < (uint32_t *)_ebss; )
    {
      *dest++ = 0;
    }

  for (src = (const uint32_t *)_eronly,
       dest = (uint32_t *)_sdata; dest < (uint32_t *)_edata; )
    {
      *dest++ = *src++;
    }

#ifdef CONFIG_ARCH_RAMFUNCS
  for (src = (const uint32_t *)_framfuncs,
       dest = (uint32_t *)_sramfuncs; dest < (uint32_t *)_eramfuncs; )
    {
      *dest++ = *src++;
    }
#endif

  stm32n6_clockconfig();
  arm_fpuconfig();
  stm32n6_lowsetup();
  arm_lowputc('N');
  arm_lowputc('6');
  arm_lowputc('\r');
  arm_lowputc('\n');
  stm32n6_clock_bootlog();
  stm32n6_cache_setup();

  nx_start();

  for (; ; );
}
