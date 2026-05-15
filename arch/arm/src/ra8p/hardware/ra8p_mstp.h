/****************************************************************************
 * arch/arm/src/ra8p/hardware/ra8p_mstp.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_MSTP_H
#define __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_MSTP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ra8p_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RA8P_MSTP_MSTPCRB       (RA8P_MSTP_BASE + 0x0004)

#define MSTP_MSTPCRB_OSPI0      (1u << 16)

#endif /* __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_MSTP_H */
