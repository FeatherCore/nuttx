/****************************************************************************
 * arch/arm/src/stm32n6/stm32_allocateheap.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

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
