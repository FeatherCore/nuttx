/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32h7rs_start.c
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
#include "stm32h7rs.h"

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

  stm32h7rs_clockconfig();
  arm_fpuconfig();
  stm32h7rs_lowsetup();
  arm_lowputc('H');
  arm_lowputc('7');
  arm_lowputc('R');
  arm_lowputc('S');
  arm_lowputc('\r');
  arm_lowputc('\n');

  nx_start();

  for (; ; );
}
