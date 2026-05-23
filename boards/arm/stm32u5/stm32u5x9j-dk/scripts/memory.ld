/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/scripts/memory.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#include <nuttx/config.h>

/* Protected internal-Flash layout without NXboot:
 *
 *   0x08000000..0x0807ffff  kernel blob, 512 KiB window
 *   0x08080000..            user blob, begins with struct userspace_s
 *
 * SRAM1/2/3/5 are contiguous on STM32U5x9J and are reserved for the kernel.
 * User .data/.bss and the user heap live in HSPI1 PSRAM.  The first 2 MiB
 * are reserved for the framebuffer used by the DK LCD path.
 */

#if CONFIG_NUTTX_USERSPACE != 0x08080000
#  error "CONFIG_NUTTX_USERSPACE must be 0x08080000 to match memory.ld"
#endif

MEMORY
{
  kflash (rx)  : ORIGIN = 0x08000000, LENGTH =   512K
  uflash (rx)  : ORIGIN = 0x08080000, LENGTH =  3584K
  ksram  (rwx) : ORIGIN = 0x20000000, LENGTH =  2496K
  usram  (rwx) : ORIGIN = 0xa0200000, LENGTH =    62M
}
