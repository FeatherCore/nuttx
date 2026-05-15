/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32u5x9j_heap.c
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
#include <nuttx/mm/mm.h>
#include <nuttx/nuttx.h>
#include <nuttx/userspace.h>

#include <arch/board/board.h>
#include <arch/stm32u5/chip.h>

#include "arm_internal.h"
#include "hardware/stm32_memorymap.h"
#include "stm32_mpuinit.h"

#include "stm32u5x9j-dk.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32U5X9J_INTERNAL_SRAM_END (STM32_SRAM5_BASE + STM32_SRAM5_SIZE)
#define STM32U5X9J_PSRAM_BASE        STM32U5X9J_HSPI1_PSRAM_MEM_BASE
#define STM32U5X9J_PSRAM_SIZE        STM32U5X9J_HSPI1_PSRAM_SIZE
#define STM32U5X9J_PSRAM_END         (STM32U5X9J_PSRAM_BASE + \
                                      STM32U5X9J_PSRAM_SIZE)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_allocate_heap
 *
 * Description:
 *   In the protected KNSh configuration the initial user heap is backed by
 *   HSPI1 PSRAM.  User .data/.bss are also linked into the bottom of PSRAM,
 *   so the heap begins after the user bss.
 *
 ****************************************************************************/

#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP) && \
    defined(CONFIG_STM32U5X9J_DK_HSPI_RAM)
void up_allocate_heap(void **heap_start, size_t *heap_size)
{
  uintptr_t ubase;
  int ret;

  ret = stm32u5x9j_hspi1_psram_initialize();
  if (ret < 0)
    {
      ferr("ERROR: STM32U5x9J-DK PSRAM user heap init failed: %d\n", ret);
      PANIC();
    }

  ubase = ALIGN_UP((uintptr_t)USERSPACE->us_bssend, MM_ALIGN);

  DEBUGASSERT(ubase >= STM32U5X9J_PSRAM_BASE);
  DEBUGASSERT(ubase < STM32U5X9J_PSRAM_END);

  board_autoled_on(LED_HEAPALLOCATE);
  stm32_mpu_uheap(STM32U5X9J_PSRAM_BASE, STM32U5X9J_PSRAM_SIZE);

  *heap_start = (FAR void *)ubase;
  *heap_size  = STM32U5X9J_PSRAM_END - ubase;

  finfo("user PSRAM heap base=0x%08" PRIx32 " size=0x%08" PRIx32 "\n",
        (uint32_t)ubase, (uint32_t)*heap_size);
}

/****************************************************************************
 * Name: up_allocate_kheap
 *
 * Description:
 *   Give the remaining contiguous internal SRAM1/2/3/5 window to the kernel
 *   heap.  PSRAM is reserved for user memory.
 *
 ****************************************************************************/

void up_allocate_kheap(void **heap_start, size_t *heap_size)
{
  DEBUGASSERT(g_idle_topstack < STM32U5X9J_INTERNAL_SRAM_END);

  board_autoled_on(LED_HEAPALLOCATE);
  *heap_start = (FAR void *)g_idle_topstack;
  *heap_size  = STM32U5X9J_INTERNAL_SRAM_END - g_idle_topstack;

  finfo("kernel SRAM heap base=0x%08" PRIx32 " size=0x%08" PRIx32 "\n",
        (uint32_t)g_idle_topstack, (uint32_t)*heap_size);
}
#endif
