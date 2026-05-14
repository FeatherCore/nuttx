/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32n6_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <sys/mount.h>

#include <debug.h>

#include "stm32n6570-dk.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_NSH_PROC_MOUNTPOINT)
#  define CONFIG_NSH_PROC_MOUNTPOINT "/proc"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BOARD_LATE_INITIALIZE
void board_late_initialize(void)
{
  int ret;

  syslog(LOG_INFO, "STM32N6570-DK NXboot bring-up\n");

#ifdef CONFIG_STM32N6_XSPI
  ret = stm32n6_extmem_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32n6_extmem_initialize failed: %d\n", ret);
    }
#else
  UNUSED(ret);
#endif

#ifdef CONFIG_FS_PROCFS
  ret = mount(NULL, CONFIG_NSH_PROC_MOUNTPOINT, "procfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "procfs mount failed: %d\n", errno);
    }
#endif
}
#endif
