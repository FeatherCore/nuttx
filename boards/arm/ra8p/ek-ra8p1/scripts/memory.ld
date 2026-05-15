/****************************************************************************
 * boards/arm/ra8p/ek-ra8p1/scripts/memory.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#include <nuttx/config.h>

/* Protected NXboot app layout:
 *
 *   0x90000000..0x900003ff  NXboot header
 *   0x90000400..0x900803ff  kernel blob, 512 KiB window
 *   0x90080400..            user blob, begins with struct userspace_s
 */

#if CONFIG_NXBOOT_HEADER_SIZE != 0x400
#  error "CONFIG_NXBOOT_HEADER_SIZE must be 0x400"
#endif

#if CONFIG_NUTTX_USERSPACE != 0x90080400
#  error "CONFIG_NUTTX_USERSPACE must be 0x90080400 to match memory.ld"
#endif

#if CONFIG_EK_RA8P1_PROTECTED_USRAM_BASE != 0x221b4000
#  error "CONFIG_EK_RA8P1_PROTECTED_USRAM_BASE must be 0x221b4000"
#endif

#if CONFIG_EK_RA8P1_PROTECTED_USRAM_SIZE != 0x20000
#  error "CONFIG_EK_RA8P1_PROTECTED_USRAM_SIZE must be 0x20000"
#endif

MEMORY
{
  kflash (rx)  : ORIGIN = 0x90000400, LENGTH = 512K
  uflash (rx)  : ORIGIN = 0x90080400, LENGTH = 32255K
  ksram  (rwx) : ORIGIN = 0x22000000, LENGTH = 1744K
  usram  (rwx) : ORIGIN = 0x221b4000, LENGTH = 128K
  itcm   (rwx) : ORIGIN = 0x00000000, LENGTH = 128K
  dtcm   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
  sdram  (rwx) : ORIGIN = 0x68000000, LENGTH = 64M
}
