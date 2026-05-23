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
#include <nuttx/irq.h>

#include <arch/barriers.h>

#include "arm_internal.h"
#include "chip.h"
#ifdef CONFIG_ARM_MPU
#  include "mpu.h"
#endif
#if defined(CONFIG_ARCH_HAVE_TRUSTZONE) && defined(CONFIG_ARCH_TRUSTZONE_DISABLED)
#  include "sau.h"
#endif
#include "hardware/stm32n6_memorymap.h"
#include "stm32n6.h"
#include "stm32n6_mpuinit.h"
#include "stm32n6_userspace.h"

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

#  ifdef CONFIG_STM32N6_PSRAM_MPU_SHARE_NONE
#    define STM32N6_PSRAM_MPU_SHARE MPU_RBAR_SH_NO
#  else
#    define STM32N6_PSRAM_MPU_SHARE MPU_RBAR_SH_OUTER
#  endif

#  ifdef CONFIG_STM32N6_PSRAM_MPU_NONCACHEABLE
#    define STM32N6_PSRAM_MPU_ATTR MPU_RLAR_NONCACHEABLE
#  elif defined(CONFIG_STM32N6_PSRAM_MPU_WRITE_THROUGH)
#    ifdef CONFIG_STM32N6_PSRAM_MPU_NO_WRITE_ALLOCATE
#      define STM32N6_PSRAM_MPU_ATTR MPU_RLAR_WRITE_THROUGH_NWA
#    else
#      define STM32N6_PSRAM_MPU_ATTR MPU_RLAR_WRITE_THROUGH
#    endif
#  else
#    ifdef CONFIG_STM32N6_PSRAM_MPU_NO_WRITE_ALLOCATE
#      define STM32N6_PSRAM_MPU_ATTR MPU_RLAR_WRITE_BACK_NWA
#    else
#      define STM32N6_PSRAM_MPU_ATTR MPU_RLAR_WRITE_BACK
#    endif
#  endif

#  ifdef CONFIG_NXBOOT_BOOTLOADER
#    define STM32N6_RAM_MPU_FLAGS STM32N6_NORMAL_CACHEABLE
#  else
#    define STM32N6_RAM_MPU_FLAGS STM32N6_NORMAL_CACHEABLE_XN
#  endif

#  ifdef CONFIG_ARCH_RAMFUNCS
#    define STM32N6_RAMFUNC_MPU_BASE ((uintptr_t)_sramfuncs)
#    define STM32N6_RAMFUNC_MPU_SIZE \
       ((uintptr_t)_eramfuncs - (uintptr_t)_sramfuncs)
#    define STM32N6_RAMFUNC_MPU_END ((uintptr_t)_eramfuncs)
#    define STM32N6_RAM_AFTER_RAMFUNC_MPU_SIZE \
       (CONFIG_RAM_START + CONFIG_RAM_SIZE - STM32N6_RAMFUNC_MPU_END)
#  endif

#  ifdef CONFIG_BUILD_PROTECTED
#    define STM32N6_XSPI2_MPU_BASE \
       (STM32N6_XSPI2_MEM_BASE + CONFIG_STM32N6_OTA_PRIMARY_SLOT_OFFSET + \
        CONFIG_NXBOOT_HEADER_SIZE)
#    define STM32N6_XSPI2_MPU_SIZE \
       (CONFIG_NUTTX_USERSPACE - STM32N6_XSPI2_MPU_BASE)
#    define STM32N6_PSRAM_MPU_FLAGS1 \
       (MPU_RBAR_AP_RWNO | STM32N6_PSRAM_MPU_SHARE | MPU_RBAR_XN)
#    define STM32N6_PSRAM_MPU_FLAGS2 \
       (STM32N6_PSRAM_MPU_ATTR | MPU_RLAR_PXN)
#    define STM32N6_USER_PSRAM_MPU_FLAGS1 \
       (MPU_RBAR_AP_RWRW | STM32N6_PSRAM_MPU_SHARE | MPU_RBAR_XN)
#  else
#    define STM32N6_XSPI2_MPU_BASE STM32N6_XSPI2_MEM_BASE
#    define STM32N6_XSPI2_MPU_SIZE STM32N6_XSPI2_NOR_SIZE
#    define STM32N6_PSRAM_MPU_FLAGS1 \
       (MPU_RBAR_AP_RWNO | STM32N6_PSRAM_MPU_SHARE | MPU_RBAR_XN)
#    define STM32N6_PSRAM_MPU_FLAGS2 \
       (STM32N6_PSRAM_MPU_ATTR | MPU_RLAR_PXN)
#  endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32n6_sau_reset(void)
{
#if defined(CONFIG_ARCH_HAVE_TRUSTZONE) && defined(CONFIG_ARCH_TRUSTZONE_DISABLED)
  uint32_t regions;
  uint32_t region;

  /* The NuttX STM32N6 port runs with TrustZone disabled, so do not inherit
   * a secure attribution layout from the first-stage image.
   */

  putreg32(0, SAU_CTRL);

  regions = (getreg32(SAU_TYPE) & SAU_TYPE_SREGION_MASK) >>
            SAU_TYPE_SREGION_SHIFT;

  for (region = 0; region < regions; region++)
    {
      putreg32(region, SAU_RNR);
      putreg32(0, SAU_RBAR);
      putreg32(0, SAU_RLAR);
    }

  putreg32(SAU_SFSR_MASK, SAU_SFSR);
  putreg32(0, SAU_CTRL);

  UP_DSB();
  UP_ISB();
#endif
}

static void stm32n6_cache_setup(void)
{
#ifdef CONFIG_ARM_MPU
  const struct mpu_region_s regions[] =
    {
#if defined(CONFIG_ARCH_RAMFUNCS) && !defined(CONFIG_NXBOOT_BOOTLOADER)
      {
        .base   = STM32N6_RAMFUNC_MPU_BASE,
        .size   = STM32N6_RAMFUNC_MPU_SIZE,
        .flags1 = STM32N6_NORMAL_CACHEABLE,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
      {
        .base   = STM32N6_RAMFUNC_MPU_END,
        .size   = STM32N6_RAM_AFTER_RAMFUNC_MPU_SIZE,
        .flags1 = STM32N6_RAM_MPU_FLAGS,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
#else
      {
        .base   = CONFIG_RAM_START,
        .size   = CONFIG_RAM_SIZE,
        .flags1 = STM32N6_RAM_MPU_FLAGS,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
#endif
      {
        .base   = STM32N6_XSPI2_MPU_BASE,
        .size   = STM32N6_XSPI2_MPU_SIZE,
        .flags1 = STM32N6_NORMAL_CACHEABLE_RO,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
      {
        .base   = STM32N6_XSPI1_MEM_BASE,
        .size   = STM32N6_XSPI1_PSRAM_SIZE,
        .flags1 = STM32N6_PSRAM_MPU_FLAGS1,
        .flags2 = STM32N6_PSRAM_MPU_FLAGS2
      },
    };

#ifdef CONFIG_ARMV8M_DCACHE
  up_disable_dcache();
#endif
#ifdef CONFIG_ARMV8M_ICACHE
  up_disable_icache();
#endif

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

static void stm32n6_early_fault_setup(void)
{
#ifdef CONFIG_DEBUG_HARDFAULT_ALERT
  irq_attach(STM32_IRQ_HARDFAULT, arm_hardfault, NULL);
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

  stm32n6_sau_reset();

  stm32n6_clockconfig();

  arm_fpuconfig();

  stm32n6_lowsetup();

  stm32n6_cache_setup();

  stm32n6_early_fault_setup();

#ifdef CONFIG_BUILD_PROTECTED
  stm32n6_userspace();

  stm32n6_mpuinitialize();
#endif

  nx_start();

  for (; ; );
}
