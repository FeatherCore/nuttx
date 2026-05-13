/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32h7rs_start.c
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

#include "arm_internal.h"
#include "chip.h"
#ifdef CONFIG_ARM_MPU
#  include "mpu.h"
#endif
#include "stm32h7rs.h"
#ifdef CONFIG_BUILD_PROTECTED
#  include "stm32h7rs_mpuinit.h"
#  include "stm32h7rs_userspace.h"
#endif
#include "hardware/stm32h7rs_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32H7RS_FLASH_SIZE        (64 * 1024)

#define STM32H7RS_NORMAL_CACHEABLE \
  (MPU_RASR_TEX_NOR | MPU_RASR_C | MPU_RASR_B | MPU_RASR_AP_RWNO)

#define STM32H7RS_NORMAL_CACHEABLE_XN \
  (STM32H7RS_NORMAL_CACHEABLE | MPU_RASR_XN)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32h7rs_cache_setup(void)
{
#ifdef CONFIG_ARM_MPU
  static const struct mpu_region_s regions[] =
    {
      {
        .base = STM32H7RS_FLASH_BASE,
        .size = STM32H7RS_FLASH_SIZE,
        .flags = STM32H7RS_NORMAL_CACHEABLE
      },
      {
        .base = STM32H7RS_AXI_SRAM_BASE,
        .size = CONFIG_RAM_SIZE,
        .flags = STM32H7RS_NORMAL_CACHEABLE_XN
      },
      {
        .base = STM32H7RS_XSPI2_MEM_BASE,
        .size = STM32H7RS_XSPI2_NOR_SIZE,
        .flags = STM32H7RS_NORMAL_CACHEABLE
      },
      {
        .base = STM32H7RS_XSPI1_MEM_BASE,
        .size = STM32H7RS_XSPI1_PSRAM_SIZE,
        .flags = STM32H7RS_NORMAL_CACHEABLE_XN
      },
    };

  mpu_reset();
  mpu_initialize(regions, sizeof(regions) / sizeof(regions[0]),
                 false, true);
#endif

#ifdef CONFIG_ARMV7M_ICACHE
  up_enable_icache();
#endif
#ifdef CONFIG_ARMV7M_DCACHE
  up_enable_dcache();
#endif
}

static void stm32h7rs_early_fault_setup(void)
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

  for (dest = (uint32_t *)_sbss; dest < (uint32_t *)_ebss; dest++)
    {
      *dest = 0;
    }

  for (src = (const uint32_t *)_eronly,
       dest = (uint32_t *)_sdata; dest < (uint32_t *)_edata;
       src++, dest++)
    {
      *dest = *src;
    }

  stm32h7rs_clockconfig();
  arm_fpuconfig();
  stm32h7rs_lowsetup();
  arm_lowputc('H');
  arm_lowputc('7');
  arm_lowputc('R');
  arm_lowputc('S');
  arm_lowputc('\r');
  arm_lowputc('\n');
  stm32h7rs_uart4_wait_txcomplete();
  stm32h7rs_cache_setup();
  stm32h7rs_early_fault_setup();

#ifdef CONFIG_BUILD_PROTECTED
  stm32h7rs_userspace();
  stm32h7rs_mpuinitialize();
#endif

  nx_start();

  for (; ; );
}
