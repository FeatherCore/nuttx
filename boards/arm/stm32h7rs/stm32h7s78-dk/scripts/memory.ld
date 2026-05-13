/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/scripts/memory.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#include <nuttx/config.h>

/* Protected NXboot app layout:
 *
 *   0x70000000..0x700003ff  NXboot header
 *   0x70000400..0x700803ff  kernel blob, 512 KiB window
 *   0x70080400..            user blob, begins with struct userspace_s
 *
 * Internal AXI SRAM keeps kernel state at the bottom and reserves only a
 * small top window for user .data/.bss.  The runtime user heap is provided
 * by XSPI1 PSRAM from board-specific heap allocation code.
 */

#if CONFIG_NXBOOT_HEADER_SIZE != 0x400
#  error "CONFIG_NXBOOT_HEADER_SIZE must be 0x400"
#endif

#if CONFIG_NUTTX_USERSPACE != 0x70080400
#  error "CONFIG_NUTTX_USERSPACE must be 0x70080400 to match memory.ld"
#endif

#if CONFIG_STM32H7RS_PROTECTED_USRAM_BASE != 0x24060000
#  error "CONFIG_STM32H7RS_PROTECTED_USRAM_BASE must be 0x24060000"
#endif

#if CONFIG_STM32H7RS_PROTECTED_USRAM_SIZE != 0x12000
#  error "CONFIG_STM32H7RS_PROTECTED_USRAM_SIZE must be 0x12000"
#endif

MEMORY
{
  kflash (rx)  : ORIGIN = 0x70000400, LENGTH = 512K
  uflash (rx)  : ORIGIN = 0x70080400, LENGTH = 32255K
  ksram  (rwx) : ORIGIN = 0x24000000, LENGTH = 384K
  usram  (rwx) : ORIGIN = 0x24060000, LENGTH = 72K
  itcm   (rwx) : ORIGIN = 0x00000000, LENGTH =    4K
  dtcm1  (rwx) : ORIGIN = 0x20000000, LENGTH =    4K
  sram1  (rwx) : ORIGIN = 0x30000000, LENGTH =   16K
  extram (rwx) : ORIGIN = 0x90000000, LENGTH =   32M
  bbram  (rwx) : ORIGIN = 0x38800000, LENGTH =    4K
}
