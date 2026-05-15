/****************************************************************************
 * arch/arm/src/ra8p/ra8p_allocateheap.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <nuttx/arch.h>
#include <nuttx/compiler.h>
#include <arch/board/board.h>

#include "arm_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void weak_function up_allocate_heap(void **heap_start, size_t *heap_size)
{
  *heap_start = (void *)g_idle_topstack;
  *heap_size  = CONFIG_RAM_END - g_idle_topstack;
}

/****************************************************************************
 * Name: arm_addregion
 *
 * Description:
 *   Add non-contiguous heap regions.  Boards with external RAM may override
 *   this weak default from their board source directory.
 *
 ****************************************************************************/

#if CONFIG_MM_REGIONS > 1
void weak_function arm_addregion(void)
{
}
#endif
