/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32_heap.c
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
#include <nuttx/mm/mm.h>
#include <nuttx/nuttx.h>
#include <nuttx/userspace.h>

#include <arch/board/board.h>
#include <arch/stm32u5/chip.h>

#include "arm_internal.h"
#include "hardware/stm32_memorymap.h"
#include "stm32.h"
#include "stm32_mpuinit.h"

#include "stm32u5x9j-dk.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BOARD_PSRAM_HEAP_BASE        BOARD_HSPI1_PSRAM_HEAP_BASE
#define BOARD_PSRAM_HEAP_SIZE        BOARD_HSPI1_PSRAM_HEAP_SIZE
#define BOARD_PROTECTED_USRAM_END \
  (CONFIG_STM32U5X9J_PROTECTED_USRAM_BASE + \
   CONFIG_STM32U5X9J_PROTECTED_USRAM_SIZE)

#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP)
#  ifndef CONFIG_STM32U5X9J_PROTECTED_USRAM_BASE
#    error CONFIG_STM32U5X9J_PROTECTED_USRAM_BASE is required
#  endif
#  ifdef CONFIG_STM32U5X9J_DK_LCD_FB_SRAM
#    define BOARD_KERNEL_HEAP_END BOARD_INTERNAL_SRAM_FB_BASE
#  else
#    define BOARD_KERNEL_HEAP_END CONFIG_STM32U5X9J_PROTECTED_USRAM_BASE
#  endif
#  define BOARD_BOOTSTRAP_UHEAP_SIZE \
          CONFIG_STM32U5X9J_PROTECTED_UHEAP_SIZE
#  if BOARD_BOOTSTRAP_UHEAP_SIZE == 0
#    error STM32U5x9J-DK protected build requires an internal bootstrap user heap
#  endif
#endif

#if defined(CONFIG_STM32U5X9J_DK_HSPI_HEAP) && CONFIG_MM_REGIONS < 2
#  error CONFIG_MM_REGIONS must be at least 2 with STM32U5X9J_DK_HSPI_HEAP
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_allocate_heap
 *
 * Description:
 *   In the protected KNSh PSRAM-heap configuration the initial user heap is a
 *   small internal SRAM bootstrap region.  HSPI1 PSRAM is added later from
 *   arm_addregion().
 *
 *   Keeping struct mm_heap_s and the earliest allocator state in internal
 *   SRAM avoids depending on memory-mapped PSRAM while nx_start() is still
 *   constructing the protected user and kernel heaps.
 *
 *   If the PSRAM heap is disabled, fall back to the remaining protected user
 *   SRAM window so macro-disabled configurations still have a usable heap.
 *
 ****************************************************************************/

#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP)
void up_allocate_heap(void **heap_start, size_t *heap_size)
{
#ifdef CONFIG_STM32U5X9J_DK_HSPI_HEAP
  uintptr_t heap_end = USERSPACE->us_bssend;
  uintptr_t heap_base = heap_end - BOARD_BOOTSTRAP_UHEAP_SIZE;
#else
  uintptr_t heap_base = USERSPACE->us_bssend;
  uintptr_t heap_end = BOARD_PROTECTED_USRAM_END;
#endif

  DEBUGASSERT(heap_end > heap_base);
  DEBUGASSERT(heap_base >= CONFIG_STM32U5X9J_PROTECTED_USRAM_BASE);
  DEBUGASSERT(heap_end <= BOARD_PROTECTED_USRAM_END);

  board_autoled_on(LED_HEAPALLOCATE);
  *heap_start = (FAR void *)heap_base;
#ifdef CONFIG_STM32U5X9J_DK_HSPI_HEAP
  *heap_size  = BOARD_BOOTSTRAP_UHEAP_SIZE;

  finfo("user SRAM bootstrap heap base=0x%08" PRIx32
        " size=0x%08" PRIx32 " psram=0x%08" PRIx32
        " psram-size=0x%08" PRIx32 "\n",
        (uint32_t)heap_base, (uint32_t)*heap_size,
        (uint32_t)BOARD_PSRAM_HEAP_BASE,
        (uint32_t)BOARD_PSRAM_HEAP_SIZE);
#else
  *heap_size = heap_end - heap_base;
  stm32_mpu_uheap(heap_base, *heap_size);

  finfo("user SRAM heap base=0x%08" PRIx32 " size=0x%08" PRIx32 "\n",
        (uint32_t)heap_base, (uint32_t)*heap_size);
#endif
}

/****************************************************************************
 * Name: up_allocate_kheap
 *
 * Description:
 *   Give the remaining internal SRAM below the protected user static data
 *   window to the kernel heap.
 *
 ****************************************************************************/

void up_allocate_kheap(void **heap_start, size_t *heap_size)
{
  DEBUGASSERT(g_idle_topstack < BOARD_KERNEL_HEAP_END);

  board_autoled_on(LED_HEAPALLOCATE);
  *heap_start = (FAR void *)g_idle_topstack;
  *heap_size  = BOARD_KERNEL_HEAP_END - g_idle_topstack;

  finfo("kernel SRAM heap base=0x%08" PRIx32
        " size=0x%08" PRIx32 " end=0x%08" PRIx32 "\n",
        (uint32_t)g_idle_topstack, (uint32_t)*heap_size,
        (uint32_t)BOARD_KERNEL_HEAP_END);
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
  stm32_addregion();

#if defined(CONFIG_STM32U5X9J_DK_HSPI_HEAP)
  int ret;

  ret = stm32_hspi1_psram_initialize();
  if (ret < 0)
    {
      ferr("ERROR: STM32U5x9J-DK PSRAM heap init failed: %d\n", ret);
      return;
    }

#  if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP)
  stm32_mpu_uheap(BOARD_PSRAM_HEAP_BASE, BOARD_PSRAM_HEAP_SIZE);
#  endif

  kumm_addregion((FAR void *)BOARD_PSRAM_HEAP_BASE,
                 BOARD_PSRAM_HEAP_SIZE);
  finfo("added PSRAM heap base=0x%08" PRIx32 " size=0x%08" PRIx32 "\n",
        (uint32_t)BOARD_PSRAM_HEAP_BASE,
        (uint32_t)BOARD_PSRAM_HEAP_SIZE);
#endif
}
#endif
