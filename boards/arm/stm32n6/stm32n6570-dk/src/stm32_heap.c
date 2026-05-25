/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32_heap.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/compiler.h>
#include <nuttx/kmalloc.h>
#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP) && \
    defined(CONFIG_STM32N6_PSRAM_HEAP)
#  include <nuttx/userspace.h>
#endif

#include <arch/board/board.h>

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

#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP)
#  ifndef CONFIG_STM32N6_PROTECTED_USRAM_BASE
#    error CONFIG_STM32N6_PROTECTED_USRAM_BASE is required
#  endif
#  define STM32N6570_KERNEL_HEAP_END CONFIG_STM32N6_PROTECTED_USRAM_BASE
#  define STM32N6570_BOOTSTRAP_UHEAP_SIZE \
          CONFIG_STM32N6_PROTECTED_UHEAP_SIZE
#  if defined(CONFIG_STM32N6_PSRAM_HEAP) && \
      STM32N6570_BOOTSTRAP_UHEAP_SIZE == 0
#    error STM32N6570-DK protected PSRAM heap requires an internal bootstrap user heap
#  endif
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_heap_link
 *
 * Description:
 *   Linker anchor used by nxboot-app.ld so this board heap object is pulled
 *   even when only arm_addregion() is needed from it.
 *
 ****************************************************************************/

void stm32_heap_link(void)
{
}

/****************************************************************************
 * Name: up_allocate_heap
 *
 * Description:
 *   In the protected KNSh configuration the initial user heap is a small
 *   internal user SRAM bootstrap region.  The XSPI1 PSRAM window is added
 *   later from arm_addregion().
 *
 *   Keeping struct mm_heap_s and the earliest allocator state in internal
 *   SRAM avoids depending on memory-mapped PSRAM while nx_start() is still
 *   constructing the protected user and kernel heaps.
 *
 ****************************************************************************/

#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP) && \
    defined(CONFIG_STM32N6_PSRAM_HEAP)
void up_allocate_heap(void **heap_start, size_t *heap_size)
{
  uintptr_t heap_end = USERSPACE->us_bssend;
  uintptr_t heap_base = heap_end - STM32N6570_BOOTSTRAP_UHEAP_SIZE;

  board_autoled_on(LED_HEAPALLOCATE);

  DEBUGASSERT(heap_end > heap_base);
  DEBUGASSERT(heap_base >= CONFIG_STM32N6_PROTECTED_USRAM_BASE);
  DEBUGASSERT(heap_end <= CONFIG_STM32N6_PROTECTED_USRAM_BASE +
                          CONFIG_STM32N6_PROTECTED_USRAM_SIZE);

  *heap_start = (FAR void *)heap_base;
  *heap_size  = STM32N6570_BOOTSTRAP_UHEAP_SIZE;

  finfo("user SRAM bootstrap heap base=0x%08" PRIx32
        " size=0x%08" PRIx32 " psram=0x%08" PRIx32
        " psram-size=0x%08" PRIx32 "\n",
        (uint32_t)heap_base, (uint32_t)*heap_size,
        (uint32_t)STM32N6570_PSRAM_HEAP_BASE,
        (uint32_t)STM32N6570_PSRAM_HEAP_SIZE);
}
#endif

/****************************************************************************
 * Name: up_allocate_kheap
 *
 * Description:
 *   Give the remaining internal AXI SRAM below the protected user static
 *   data window to the kernel heap.
 *
 ****************************************************************************/

#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP)
void up_allocate_kheap(void **heap_start, size_t *heap_size)
{
  DEBUGASSERT(g_idle_topstack < STM32N6570_KERNEL_HEAP_END);

  board_autoled_on(LED_HEAPALLOCATE);
  *heap_start = (FAR void *)g_idle_topstack;
  *heap_size  = STM32N6570_KERNEL_HEAP_END - g_idle_topstack;

  finfo("kernel SRAM heap base=0x%08" PRIx32
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

  ret = stm32_xspi1_psram_initialize();
  if (ret < 0)
    {
      ferr("ERROR: STM32N6570-DK PSRAM heap init failed: %d\n", ret);
      return;
    }

  stm32n6_mpu_uheap(STM32N6570_PSRAM_HEAP_BASE,
                    STM32N6570_PSRAM_HEAP_SIZE);
  kumm_addregion((FAR void *)STM32N6570_PSRAM_HEAP_BASE,
                 STM32N6570_PSRAM_HEAP_SIZE);
  finfo("added PSRAM heap base=0x%08" PRIx32 " size=0x%08" PRIx32 "\n",
        (uint32_t)STM32N6570_PSRAM_HEAP_BASE,
        (uint32_t)STM32N6570_PSRAM_HEAP_SIZE);
#endif
}
#endif
