/****************************************************************************
 * arch/arm/include/ra8p/ra8p1_irq.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_INCLUDE_RA8P_RA8P1_IRQ_H
#define __ARCH_ARM_INCLUDE_RA8P_RA8P1_IRQ_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* RA8P1 uses ICU event links for many peripheral interrupts.  The exact event
 * map will be filled in as the FSP-generated configuration is translated.
 * Reserve a conservative NVIC external vector range now so the Cortex-M85
 * vector table and generic interrupt plumbing can be built.
 */

#define RA8P_IRQ_NEXTINT        128

#endif /* __ARCH_ARM_INCLUDE_RA8P_RA8P1_IRQ_H */
