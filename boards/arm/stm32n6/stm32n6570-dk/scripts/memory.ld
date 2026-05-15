/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/scripts/memory.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#include <nuttx/config.h>

/* Protected NXboot app layout:
 *
 *   0x70100000..0x701003ff  NXboot header
 *   0x70100400..0x701803ff  kernel blob, 512 KiB window
 *   0x70180400..            user blob, begins with struct userspace_s
 *
 * Internal AXI SRAM keeps kernel state at the bottom and reserves a small
 * top window for user .data/.bss plus the protected user heap bootstrap
 * control block.  The runtime user heap bulk storage is provided by the
 * whole XSPI1 PSRAM window from board-specific heap allocation code.
 */

#if CONFIG_STM32N6_OTA_PRIMARY_SLOT_OFFSET != 0x00100000
#  error "CONFIG_STM32N6_OTA_PRIMARY_SLOT_OFFSET must be 0x00100000"
#endif

#if CONFIG_STM32N6_OTA_SLOT_SIZE != 0x02000000
#  error "CONFIG_STM32N6_OTA_SLOT_SIZE must be 0x02000000"
#endif

#if CONFIG_NXBOOT_HEADER_SIZE != 0x400
#  error "CONFIG_NXBOOT_HEADER_SIZE must be 0x400"
#endif

#if CONFIG_NUTTX_USERSPACE != 0x70180400
#  error "CONFIG_NUTTX_USERSPACE must be 0x70180400 to match memory.ld"
#endif

#if CONFIG_STM32N6_PROTECTED_USRAM_BASE != 0x341e0000
#  error "CONFIG_STM32N6_PROTECTED_USRAM_BASE must be 0x341e0000"
#endif

#if CONFIG_STM32N6_PROTECTED_USRAM_SIZE != 0x20000
#  error "CONFIG_STM32N6_PROTECTED_USRAM_SIZE must be 0x20000"
#endif

MEMORY
{
  kflash (rx)  : ORIGIN = 0x70100400, LENGTH =   512K
  uflash (rx)  : ORIGIN = 0x70180400, LENGTH = 32255K
  ksram  (rwx) : ORIGIN = 0x34000000, LENGTH =  1920K
  usram  (rwx) : ORIGIN = 0x341e0000, LENGTH =   128K
  psram  (rwx) : ORIGIN = 0x90000000, LENGTH =    32M
}
