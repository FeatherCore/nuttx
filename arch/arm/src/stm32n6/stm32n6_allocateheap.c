/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_allocateheap.c
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
#include <syslog.h>

#include <nuttx/kmalloc.h>

#include <arch/board/board.h>
#include <arch/stm32n6/chip.h>

#include "arm_internal.h"
#include "stm32n6.h"

#include "hardware/stm32n6_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_STM32N6_PSRAM_HEAP
#  define STM32N6_PSRAM_HEAP_BASE \
          (STM32N6_XSPI1_MEM_BASE + CONFIG_STM32N6_PSRAM_HEAP_OFFSET)
#  define STM32N6_PSRAM_HEAP_END \
          (STM32N6_XSPI1_MEM_BASE + STM32N6_XSPI1_PSRAM_SIZE)
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#if CONFIG_MM_REGIONS > 1
void arm_addregion(void)
{
#ifdef CONFIG_STM32N6_PSRAM_HEAP
  size_t heapsize = CONFIG_STM32N6_PSRAM_HEAP_SIZE;
  int ret;

  if (STM32N6_PSRAM_HEAP_BASE + heapsize > STM32N6_PSRAM_HEAP_END)
    {
      syslog(LOG_ERR,
             "stm32n6: PSRAM heap outside device base=0x%08x size=0x%zx\n",
             STM32N6_PSRAM_HEAP_BASE, heapsize);
      return;
    }

  ret = stm32n6_xspi1_psram_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32n6: PSRAM heap init failed: %d\n", ret);
      return;
    }

  kumm_addregion((FAR void *)STM32N6_PSRAM_HEAP_BASE, heapsize);
  syslog(LOG_INFO, "stm32n6: PSRAM heap added base=0x%08x size=0x%zx\n",
         STM32N6_PSRAM_HEAP_BASE, heapsize);
#endif
}
#endif
