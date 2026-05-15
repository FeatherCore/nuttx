/****************************************************************************
 * arch/arm/src/ra8p/ra8p_userspace.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <stdint.h>

#include <nuttx/userspace.h>

#include "ra8p_userspace.h"

#ifdef CONFIG_BUILD_PROTECTED

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ra8p_userspace
 *
 * Description:
 *   Initialize the protected build user .bss and .data sections.
 *
 ****************************************************************************/

void ra8p_userspace(void)
{
  FAR const uint8_t *src;
  FAR uint8_t *dest;
  FAR uint8_t *end;

  DEBUGASSERT(USERSPACE->us_bssstart != 0 && USERSPACE->us_bssend != 0 &&
              USERSPACE->us_bssstart <= USERSPACE->us_bssend);

  dest = (FAR uint8_t *)USERSPACE->us_bssstart;
  end  = (FAR uint8_t *)USERSPACE->us_bssend;

  while (dest != end)
    {
      *dest++ = 0;
    }

  DEBUGASSERT(USERSPACE->us_datasource != 0 &&
              USERSPACE->us_datastart != 0 &&
              USERSPACE->us_dataend != 0 &&
              USERSPACE->us_datastart <= USERSPACE->us_dataend);

  src  = (FAR const uint8_t *)USERSPACE->us_datasource;
  dest = (FAR uint8_t *)USERSPACE->us_datastart;
  end  = (FAR uint8_t *)USERSPACE->us_dataend;

  while (dest != end)
    {
      *dest++ = *src++;
    }
}

#endif /* CONFIG_BUILD_PROTECTED */
