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

#include <errno.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <syslog.h>

#include <debug.h>
#include <nuttx/video/fb.h>

#include "stm32h7s78-dk.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_NSH_PROC_MOUNTPOINT)
#  define CONFIG_NSH_PROC_MOUNTPOINT "/proc"
#endif

#ifdef CONFIG_NXBOOT_BOOTLOADER
#  define STM32H7S78_IMAGE_ROLE "nxboot"
#else
#  define STM32H7S78_IMAGE_ROLE "app"
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32h7s78_build_log(void)
{
  struct utsname name;

  if (uname(&name) < 0)
    {
      syslog(LOG_WARNING,
             "stm32h7s78-dk: image=%s build unavailable errno=%d\n",
             STM32H7S78_IMAGE_ROLE, errno);
      return;
    }

  syslog(LOG_INFO,
         "stm32h7s78-dk: image=%s version=%s %s %s %s\n",
         STM32H7S78_IMAGE_ROLE, name.sysname, name.release, name.version,
         name.machine);
}

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
  stm32h7s78_build_log();

#ifdef CONFIG_STM32H7RS_XSPI
  ret = stm32h7rs_extmem_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32h7rs_extmem_initialize failed: %d\n", ret);
    }
#else
  UNUSED(ret);
#endif

#ifdef CONFIG_STM32H7S78_DK_LCD
  ret = up_fbinitialize(0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "up_fbinitialize failed: %d\n", ret);
    }
  else
    {
      ret = fb_register(0, 0);
      if (ret < 0)
        {
          syslog(LOG_ERR, "fb_register failed: %d\n", ret);
        }
    }
#endif

#ifdef CONFIG_STM32H7S78_DK_GT911
  ret = stm32h7s78_touch_setup();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32h7s78_touch_setup failed: %d\n", ret);
    }
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
