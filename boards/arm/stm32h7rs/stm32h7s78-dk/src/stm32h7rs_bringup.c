/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/src/stm32h7rs_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <debug.h>

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_STM32H7RS_XSPI
int stm32h7rs_extmem_initialize(void);
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_late_initialize
 ****************************************************************************/

#ifdef CONFIG_BOARD_LATE_INITIALIZE
void board_late_initialize(void)
{
  int ret;

  syslog(LOG_INFO, "STM32H7S78-DK bring-up skeleton\n");

#ifdef CONFIG_STM32H7RS_XSPI
  ret = stm32h7rs_extmem_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32h7rs_extmem_initialize failed: %d\n", ret);
    }
#else
  UNUSED(ret);
#endif
}
#endif
