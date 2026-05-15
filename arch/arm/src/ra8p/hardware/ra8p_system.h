/****************************************************************************
 * arch/arm/src/ra8p/hardware/ra8p_system.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_SYSTEM_H
#define __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_SYSTEM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ra8p_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RA8P_SYSTEM_SDCKOCR    (RA8P_SYSTEM_BASE + 0x0053)
#define RA8P_SYSTEM_PRCR       (RA8P_SYSTEM_BASE + 0x03fa)

#define SYSTEM_PRCR_KEY        0xa500
#define SYSTEM_PRCR_PRC1       (1 << 1)

#define SYSTEM_SDCKOCR_SDCKOEN (1 << 0)

#endif /* __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_SYSTEM_H */
