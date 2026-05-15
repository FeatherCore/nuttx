/****************************************************************************
 * arch/arm/include/ra8p/irq.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_INCLUDE_RA8P_IRQ_H
#define __ARCH_ARM_INCLUDE_RA8P_IRQ_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <arch/ra8p/chip.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RA8P_IRQ_RESERVED       (0)
#define RA8P_IRQ_NMI            (2)
#define RA8P_IRQ_HARDFAULT      (3)
#define RA8P_IRQ_MEMFAULT       (4)
#define RA8P_IRQ_BUSFAULT       (5)
#define RA8P_IRQ_USAGEFAULT     (6)
#define RA8P_IRQ_SVCALL        (11)
#define RA8P_IRQ_DBGMONITOR    (12)
#define RA8P_IRQ_PENDSV        (14)
#define RA8P_IRQ_SYSTICK       (15)

#define RA8P_IRQ_FIRST         (16)

#if defined(CONFIG_RA8P1_FAMILY)
#  include <arch/ra8p/ra8p1_irq.h>
#else
#  error Unrecognized RA8P architecture
#endif

#define NR_IRQS                (RA8P_IRQ_FIRST + RA8P_IRQ_NEXTINT)

#endif /* __ARCH_ARM_INCLUDE_RA8P_IRQ_H */
