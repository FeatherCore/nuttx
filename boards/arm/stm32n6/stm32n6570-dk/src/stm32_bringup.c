/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <inttypes.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <syslog.h>

#include <arch/board/board.h>
#include <debug.h>
#include <nuttx/video/fb.h>

#include "arm_internal.h"

#include "hardware/stm32n6_rcc.h"

#include "stm32n6570-dk.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_NSH_PROC_MOUNTPOINT)
#  define CONFIG_NSH_PROC_MOUNTPOINT "/proc"
#endif

#ifdef CONFIG_NXBOOT_BOOTLOADER
#  define STM32N6570_IMAGE_ROLE "nxboot"
#else
#  define STM32N6570_IMAGE_ROLE "app"
#endif

static void stm32_build_log(void)
{
  struct utsname name;

  if (uname(&name) < 0)
    {
      syslog(LOG_WARNING,
             "stm32n6570-dk: image=%s build unavailable errno=%d\n",
             STM32N6570_IMAGE_ROLE, errno);
      return;
    }

  syslog(LOG_INFO,
         "stm32n6570-dk: image=%s version=%s %s %s %s\n",
         STM32N6570_IMAGE_ROLE, name.sysname, name.release, name.version,
         name.machine);
}

static void stm32_clock_log(void)
{
  uint32_t cfgr1 = getreg32(STM32N6_RCC_CFGR1);
  uint32_t cfgr2 = getreg32(STM32N6_RCC_CFGR2);
  uint32_t sr = getreg32(STM32N6_RCC_SR);
  uint32_t pll1cfgr1 = getreg32(STM32N6_RCC_PLL1CFGR1);
  uint32_t pll1cfgr3 = getreg32(STM32N6_RCC_PLL1CFGR3);
  uint32_t ic1cfgr = getreg32(STM32N6_RCC_IC1CFGR);
  FAR const char *cpu_src;

  cpu_src = (cfgr1 & RCC_CFGR1_CPUSWS_MASK) == RCC_CFGR1_CPUSWS_IC1 ?
            "ic1" : "other";

  syslog(LOG_INFO,
         "stm32n6570-dk: clock cpu=%" PRIu32 "Hz hclk=%" PRIu32
         "Hz pclk1=%" PRIu32 "Hz pclk2=%" PRIu32
         "Hz src=%s sr=%08" PRIx32 " cfgr1=%08" PRIx32
         " cfgr2=%08" PRIx32 " pll1cfgr1=%08" PRIx32
         " pll1cfgr3=%08" PRIx32 " ic1cfgr=%08" PRIx32 "\n",
         (uint32_t)STM32N6_CPUCLK_FREQUENCY,
         (uint32_t)STM32N6_HCLK_FREQUENCY,
         (uint32_t)STM32N6_PCLK1_FREQUENCY,
         (uint32_t)STM32N6_PCLK2_FREQUENCY,
         cpu_src, sr, cfgr1, cfgr2, pll1cfgr1, pll1cfgr3, ic1cfgr);
}

static void stm32_board_initialize(void)
{
  int ret;

  syslog(LOG_INFO, "STM32N6570-DK bring-up skeleton\n");
  stm32_build_log();
  stm32_clock_log();

#ifdef CONFIG_STM32N6_XSPI
  ret = stm32_extmem_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32_extmem_initialize failed: %d\n", ret);
    }
#else
  UNUSED(ret);
#endif

#ifdef CONFIG_VIDEO_FB
  ret = fb_register(0, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "fb_register failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_STM32N6570_DK_GT911
  ret = stm32_touchscreen_setup();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32_touchscreen_setup failed: %d\n", ret);
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

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BOARD_EARLY_INITIALIZE
void board_early_initialize(void)
{
  stm32_board_initialize();
}
#endif

#ifdef CONFIG_BOARD_LATE_INITIALIZE
void board_late_initialize(void)
{
  stm32_board_initialize();
}
#endif
