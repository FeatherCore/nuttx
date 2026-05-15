/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32n6570_heap.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <syslog.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/compiler.h>
#include <nuttx/kmalloc.h>
#include <nuttx/userspace.h>

#include <arch/board/board.h>
#include <arch/stm32n6/chip.h>

#include "arm_internal.h"
#include "hardware/stm32n6_memorymap.h"
#include "stm32n6_mpuinit.h"

#include "stm32n6570-dk.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_STM32N6_PSRAM_HEAP) && CONFIG_MM_REGIONS < 2
#  error CONFIG_MM_REGIONS must be at least 2 with STM32N6_PSRAM_HEAP
#endif

#ifdef CONFIG_STM32N6_PSRAM_HEAP
#  if CONFIG_STM32N6_PSRAM_HEAP_OFFSET >= STM32N6_XSPI1_PSRAM_SIZE
#    error CONFIG_STM32N6_PSRAM_HEAP_OFFSET must be smaller than PSRAM size
#  endif
#  define STM32N6570_PSRAM_HEAP_BASE \
          (STM32N6_XSPI1_MEM_BASE + CONFIG_STM32N6_PSRAM_HEAP_OFFSET)
#  define STM32N6570_PSRAM_HEAP_SIZE \
          (STM32N6_XSPI1_PSRAM_SIZE - CONFIG_STM32N6_PSRAM_HEAP_OFFSET)
#endif

#define STM32N6570_USER_HEAP_BOOT_SIZE 0x2000

#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP)
#  ifndef CONFIG_STM32N6_PROTECTED_USRAM_BASE
#    error CONFIG_STM32N6_PROTECTED_USRAM_BASE is required
#  endif
#  define STM32N6570_KERNEL_HEAP_END CONFIG_STM32N6_PROTECTED_USRAM_BASE
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_allocate_heap
 *
 * Description:
 *   In the protected KNSh configuration the initial user heap is a small
 *   user SRAM bootstrap region reserved by user-space.ld.  The XSPI PSRAM is
 *   added later as a second user heap region from arm_addregion().
 *
 *   Keeping struct mm_heap_s in internal user SRAM avoids taking allocator
 *   mutex atomics on the memory-mapped XSPI PSRAM window.
 *
 ****************************************************************************/

#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP) && \
    defined(CONFIG_STM32N6_PSRAM_HEAP)
void up_allocate_heap(void **heap_start, size_t *heap_size)
{
  uintptr_t boot_heap;
  int ret;

  ret = stm32n6570_xspi1_psram_initialize();
  if (ret < 0)
    {
      ferr("ERROR: STM32N6570-DK PSRAM user heap init failed: %d\n", ret);
      PANIC();
    }

  board_autoled_on(LED_HEAPALLOCATE);

  DEBUGASSERT(USERSPACE->us_bssend >=
              CONFIG_STM32N6_PROTECTED_USRAM_BASE +
              STM32N6570_USER_HEAP_BOOT_SIZE);

  boot_heap = USERSPACE->us_bssend - STM32N6570_USER_HEAP_BOOT_SIZE;

  *heap_start = (FAR void *)boot_heap;
  *heap_size  = STM32N6570_USER_HEAP_BOOT_SIZE;

  syslog(LOG_INFO,
         "stm32n6: user SRAM bootstrap heap base=0x%08" PRIx32
         " size=0x%08" PRIx32 " psram=0x%08" PRIx32
         " psram-size=0x%08" PRIx32 "\n",
         (uint32_t)boot_heap, (uint32_t)STM32N6570_USER_HEAP_BOOT_SIZE,
         (uint32_t)STM32N6570_PSRAM_HEAP_BASE,
         (uint32_t)STM32N6570_PSRAM_HEAP_SIZE);
}

/****************************************************************************
 * Name: up_allocate_kheap
 *
 * Description:
 *   Give the remaining internal AXI SRAM below the protected user static
 *   data window to the kernel heap.
 *
 ****************************************************************************/

void up_allocate_kheap(void **heap_start, size_t *heap_size)
{
  DEBUGASSERT(g_idle_topstack < STM32N6570_KERNEL_HEAP_END);

  board_autoled_on(LED_HEAPALLOCATE);
  *heap_start = (FAR void *)g_idle_topstack;
  *heap_size  = STM32N6570_KERNEL_HEAP_END - g_idle_topstack;

  syslog(LOG_INFO,
         "stm32n6: kernel SRAM heap base=0x%08" PRIx32
         " size=0x%08" PRIx32 " end=0x%08" PRIx32 "\n",
         (uint32_t)g_idle_topstack, (uint32_t)*heap_size,
         (uint32_t)STM32N6570_KERNEL_HEAP_END);
}
#endif

/****************************************************************************
 * Name: arm_addregion
 *
 * Description:
 *   Add board-specific non-contiguous heap regions.
 *
 ****************************************************************************/

#if CONFIG_MM_REGIONS > 1
void arm_addregion(void)
{
#if defined(CONFIG_STM32N6_PSRAM_HEAP)
  int ret;

  ret = stm32n6570_xspi1_psram_initialize();
  if (ret < 0)
    {
      ferr("ERROR: STM32N6570-DK PSRAM heap init failed: %d\n", ret);
      return;
    }

  kumm_addregion((FAR void *)STM32N6570_PSRAM_HEAP_BASE,
                 STM32N6570_PSRAM_HEAP_SIZE);
  syslog(LOG_INFO,
         "stm32n6: added PSRAM heap base=0x%08" PRIx32
         " size=0x%08" PRIx32 "\n",
         (uint32_t)STM32N6570_PSRAM_HEAP_BASE,
         (uint32_t)STM32N6570_PSRAM_HEAP_SIZE);
#endif
}
#endif
