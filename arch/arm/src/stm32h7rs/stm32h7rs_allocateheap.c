/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32h7rs_allocateheap.c
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

#include <debug.h>

#include <nuttx/kmalloc.h>

#include "chip.h"
#include "stm32h7rs.h"
#include "hardware/stm32h7rs_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_STM32H7RS_PSRAM_HEAP) && CONFIG_MM_REGIONS < 2
#  error CONFIG_MM_REGIONS must be at least 2 when STM32H7RS_PSRAM_HEAP is enabled
#endif

#define STM32H7RS_PSRAM_HEAP_BASE STM32H7RS_XSPI1_MEM_BASE
#define STM32H7RS_PSRAM_HEAP_SIZE STM32H7RS_XSPI1_PSRAM_SIZE

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arm_addregion
 *
 * Description:
 *   Add non-contiguous heap regions.  The main AXI SRAM heap is provided by
 *   the common ARM allocator; the app may add the mapped XSPI1 PSRAM here.
 *
 ****************************************************************************/

#if CONFIG_MM_REGIONS > 1
void arm_addregion(void)
{
#ifdef CONFIG_STM32H7RS_PSRAM_HEAP
  int ret;

  ret = stm32h7rs_xspi1_psram_initialize();
  if (ret < 0)
    {
      ferr("ERROR: STM32H7RS PSRAM heap init failed: %d\n", ret);
      return;
    }

  kumm_addregion((FAR void *)STM32H7RS_PSRAM_HEAP_BASE,
                 STM32H7RS_PSRAM_HEAP_SIZE);
  finfo("added PSRAM heap base=0x%08x size=0x%08x\n",
        STM32H7RS_PSRAM_HEAP_BASE, STM32H7RS_PSRAM_HEAP_SIZE);
#endif
}
#endif
