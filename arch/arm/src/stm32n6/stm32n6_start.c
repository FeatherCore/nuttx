/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_start.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/init.h>

#include "arm_internal.h"
#include "stm32n6.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

const uintptr_t g_idle_topstack =
  (uintptr_t)_ebss + CONFIG_IDLETHREAD_STACKSIZE;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

osentry_function
void __start(void)
{
  const uint32_t *src;
  uint32_t *dest;

  for (dest = (uint32_t *)_sbss; dest < (uint32_t *)_ebss; )
    {
      *dest++ = 0;
    }

  for (src = (const uint32_t *)_eronly,
       dest = (uint32_t *)_sdata; dest < (uint32_t *)_edata; )
    {
      *dest++ = *src++;
    }

#ifdef CONFIG_ARCH_RAMFUNCS
  for (src = (const uint32_t *)_framfuncs,
       dest = (uint32_t *)_sramfuncs; dest < (uint32_t *)_eramfuncs; )
    {
      *dest++ = *src++;
    }
#endif

  stm32n6_clockconfig();
  arm_fpuconfig();
  stm32n6_lowsetup();
  arm_lowputc('N');
  arm_lowputc('6');
  arm_lowputc('\r');
  arm_lowputc('\n');
  stm32n6_clock_bootlog();

  nx_start();

  for (; ; );
}
