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
#include <stdint.h>
#include <inttypes.h>

#include <debug.h>

#include <nuttx/compiler.h>
#include <nuttx/kmalloc.h>

#include <arch/stm32n6/chip.h>

#include "hardware/stm32n6_memorymap.h"
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
#  define STM32N6570_PSRAM_HEAP_END \
          (STM32N6_XSPI1_MEM_BASE + STM32N6_XSPI1_PSRAM_SIZE)
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

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
#ifdef CONFIG_STM32N6_PSRAM_HEAP
  size_t heapsize = CONFIG_STM32N6_PSRAM_HEAP_SIZE;
  int ret;

  if (STM32N6570_PSRAM_HEAP_BASE + heapsize > STM32N6570_PSRAM_HEAP_END)
    {
      ferr("ERROR: STM32N6570-DK PSRAM heap outside device "
           "base=0x%08" PRIx32 " size=0x%zx\n",
           (uint32_t)STM32N6570_PSRAM_HEAP_BASE, heapsize);
      return;
    }

  ret = stm32n6570_xspi1_psram_initialize();
  if (ret < 0)
    {
      ferr("ERROR: STM32N6570-DK PSRAM heap init failed: %d\n", ret);
      return;
    }

  kumm_addregion((FAR void *)STM32N6570_PSRAM_HEAP_BASE, heapsize);
  finfo("added PSRAM heap base=0x%08" PRIx32 " size=0x%zx\n",
        (uint32_t)STM32N6570_PSRAM_HEAP_BASE, heapsize);
#endif
}
#endif
