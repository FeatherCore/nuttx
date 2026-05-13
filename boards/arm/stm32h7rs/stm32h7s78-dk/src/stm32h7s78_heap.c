/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/src/stm32h7s78_heap.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <debug.h>

#include <nuttx/compiler.h>
#include <nuttx/kmalloc.h>

#include "chip.h"
#include "hardware/stm32h7rs_memorymap.h"

#include "stm32h7s78-dk.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_STM32H7RS_PSRAM_HEAP) && CONFIG_MM_REGIONS < 2
#  error CONFIG_MM_REGIONS must be at least 2 with STM32H7RS_PSRAM_HEAP
#endif

#ifdef CONFIG_STM32H7RS_PSRAM_HEAP
#  if CONFIG_STM32H7RS_PSRAM_HEAP_OFFSET >= STM32H7RS_XSPI1_PSRAM_SIZE
#    error CONFIG_STM32H7RS_PSRAM_HEAP_OFFSET must be smaller than PSRAM size
#  endif
#  define STM32H7S78_PSRAM_HEAP_BASE \
          (STM32H7RS_XSPI1_MEM_BASE + CONFIG_STM32H7RS_PSRAM_HEAP_OFFSET)
#  define STM32H7S78_PSRAM_HEAP_SIZE \
          (STM32H7RS_XSPI1_PSRAM_SIZE - CONFIG_STM32H7RS_PSRAM_HEAP_OFFSET)
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
#ifdef CONFIG_STM32H7RS_PSRAM_HEAP
  int ret;

  ret = stm32h7s78_xspi1_psram_initialize();
  if (ret < 0)
    {
      ferr("ERROR: STM32H7S78-DK PSRAM heap init failed: %d\n", ret);
      return;
    }

  kumm_addregion((FAR void *)STM32H7S78_PSRAM_HEAP_BASE,
                 STM32H7S78_PSRAM_HEAP_SIZE);
  finfo("added PSRAM heap base=0x%08x size=0x%08x\n",
        STM32H7S78_PSRAM_HEAP_BASE, STM32H7S78_PSRAM_HEAP_SIZE);
#endif
}
#endif
