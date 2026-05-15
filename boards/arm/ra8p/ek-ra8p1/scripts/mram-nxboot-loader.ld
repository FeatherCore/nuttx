/****************************************************************************
 * boards/arm/ra8p/ek-ra8p1/scripts/mram-nxboot-loader.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#include <nuttx/config.h>

/* EK-RA8P1 NXboot loader:
 *
 *   internal MRAM: 0x02000000, 1 MiB
 *   internal SRAM: 0x22000000, 1872 KiB
 *   OSPI0 CS1 NOR: 0x90000000, physical 64 MiB, split into two 32 MiB
 *                   NXboot slots.
 */

#if CONFIG_EK_RA8P1_OTA_PRIMARY_SLOT_OFFSET != 0x0
#  error "CONFIG_EK_RA8P1_OTA_PRIMARY_SLOT_OFFSET must be 0x0"
#endif

#if CONFIG_EK_RA8P1_OTA_SLOT_SIZE != 0x02000000
#  error "CONFIG_EK_RA8P1_OTA_SLOT_SIZE must be 0x02000000"
#endif

#if CONFIG_NXBOOT_HEADER_SIZE != 0x400
#  error "CONFIG_NXBOOT_HEADER_SIZE must be 0x400"
#endif

#include "mram.ld"
