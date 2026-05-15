/****************************************************************************
 * boards/arm/ra8p/ek-ra8p1/src/ra8p1_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <debug.h>

#include "ek-ra8p1.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int ra8p_bringup(void)
{
#if defined(CONFIG_RA8P_OSPI_B) || defined(CONFIG_RA8P_SDRAM)
  int ret;

  ret = ra8p1_extmem_initialize();
  if (ret < 0)
    {
      berr("ra8p1_extmem_initialize failed: %d\n", ret);
      return ret;
    }
#endif

  return 0;
}

#ifdef CONFIG_BOARD_LATE_INITIALIZE
void board_late_initialize(void)
{
  ra8p_bringup();
}
#endif
