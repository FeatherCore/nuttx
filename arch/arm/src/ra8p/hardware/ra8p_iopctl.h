/****************************************************************************
 * arch/arm/src/ra8p/hardware/ra8p_iopctl.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_IOPCTL_H
#define __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_IOPCTL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ra8p_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RA8P_PFS_PMN(port, pin)        (RA8P_PFS_BASE + ((port) * 0x40) + \
                                        ((pin) * 4))

#define RA8P_PMISC_PWPR                (RA8P_PMISC_BASE + 0x000c)
#define RA8P_PMISC_PWPRS               (RA8P_PMISC_BASE + 0x0014)

#define PMISC_PWPR_PFSWE               0x40
#define PMISC_PWPR_B0WI                0x80

#define PFS_PMR                        (1u << 16)
#define PFS_DSCR_SHIFT                 10
#define PFS_DSCR_HIGH                  (3u << PFS_DSCR_SHIFT)
#define PFS_PSEL_SHIFT                 24
#define PFS_PERIPHERAL_BUS             (0x0bu << PFS_PSEL_SHIFT)
#define PFS_PERIPHERAL_OSPI            (0x1cu << PFS_PSEL_SHIFT)

#endif /* __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_IOPCTL_H */
