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
 * top window for user .data/.bss.  The runtime user heap is provided by
 * XSPI1 PSRAM from board-specific heap allocation code.
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

#if CONFIG_RAM_START != 0x34000000
#  error "CONFIG_RAM_START must be 0x34000000"
#endif

MEMORY
{
  kflash (rx)  : ORIGIN = 0x70100400, LENGTH =   512K
  uflash (rx)  : ORIGIN = 0x70180400, LENGTH = 32255K
  ksram  (rwx) : ORIGIN = CONFIG_RAM_START, LENGTH = CONFIG_RAM_SIZE
  usram  (rwx) : ORIGIN = CONFIG_STM32N6_PROTECTED_USRAM_BASE, LENGTH = CONFIG_STM32N6_PROTECTED_USRAM_SIZE
  psram  (rwx) : ORIGIN = 0x90000000, LENGTH =    32M
}
