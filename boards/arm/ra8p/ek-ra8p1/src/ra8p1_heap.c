/****************************************************************************
 * boards/arm/ra8p/ek-ra8p1/src/ra8p1_heap.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>
#include <syslog.h>

#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/kmalloc.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "ra8p_mpuinit.h"

#include "ek-ra8p1.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_EK_RA8P1_SDRAM_HEAP) && CONFIG_MM_REGIONS < 2 && \
    (!defined(CONFIG_BUILD_PROTECTED) || !defined(CONFIG_MM_KERNEL_HEAP))
#  error CONFIG_MM_REGIONS must be at least 2 with EK_RA8P1_SDRAM_HEAP
#endif

#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP)
#  ifndef CONFIG_EK_RA8P1_PROTECTED_USRAM_BASE
#    error CONFIG_EK_RA8P1_PROTECTED_USRAM_BASE is required
#  endif
#  define RA8P1_KERNEL_HEAP_END CONFIG_EK_RA8P1_PROTECTED_USRAM_BASE
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_allocate_heap
 *
 * Description:
 *   In the protected KNSh configuration the populated external SDRAM is
 *   the user heap.  Internal SRAM is reserved for kernel sections, protected
 *   user .data/.bss, and the kernel heap.
 *
 ****************************************************************************/

#if defined(CONFIG_BUILD_PROTECTED) && defined(CONFIG_MM_KERNEL_HEAP) && \
    defined(CONFIG_EK_RA8P1_SDRAM_HEAP)
void up_allocate_heap(void **heap_start, size_t *heap_size)
{
  int ret;

  ret = ra8p1_sdram_initialize();
  if (ret < 0)
    {
      ferr("ERROR: EK-RA8P1 SDRAM user heap init failed: %d\n", ret);
      PANIC();
    }

  board_autoled_on(LED_HEAPALLOCATE);
  ra8p_mpu_uheap(RA8P_SDRAM_START, RA8P_SDRAM_SIZE);

  *heap_start = (FAR void *)RA8P_SDRAM_START;
  *heap_size  = RA8P_SDRAM_SIZE;

  syslog(LOG_INFO,
         "ra8p1: user SDRAM heap base=0x%08" PRIx32
         " size=0x%08" PRIx32 "\n",
         (uint32_t)RA8P_SDRAM_START, (uint32_t)RA8P_SDRAM_SIZE);
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
  DEBUGASSERT(g_idle_topstack < RA8P1_KERNEL_HEAP_END);

  board_autoled_on(LED_HEAPALLOCATE);
  *heap_start = (FAR void *)g_idle_topstack;
  *heap_size  = RA8P1_KERNEL_HEAP_END - g_idle_topstack;

  syslog(LOG_INFO,
         "ra8p1: kernel SRAM heap base=0x%08" PRIx32
         " size=0x%08" PRIx32 " end=0x%08" PRIx32 "\n",
         (uint32_t)g_idle_topstack, (uint32_t)*heap_size,
         (uint32_t)RA8P1_KERNEL_HEAP_END);
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
#if defined(CONFIG_EK_RA8P1_SDRAM_HEAP) && \
    (!defined(CONFIG_BUILD_PROTECTED) || !defined(CONFIG_MM_KERNEL_HEAP))
  int ret;

  ret = ra8p1_sdram_initialize();
  if (ret < 0)
    {
      ferr("ERROR: EK-RA8P1 SDRAM heap init failed: %d\n", ret);
      return;
    }

  kumm_addregion((FAR void *)RA8P_SDRAM_START, RA8P_SDRAM_SIZE);
  syslog(LOG_INFO,
         "ra8p1: added SDRAM heap base=0x%08" PRIx32
         " size=0x%08" PRIx32 "\n",
         (uint32_t)RA8P_SDRAM_START, (uint32_t)RA8P_SDRAM_SIZE);
#endif
}
#endif
