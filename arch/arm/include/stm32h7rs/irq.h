/****************************************************************************
 * arch/arm/include/stm32h7rs/irq.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/* This file should never be included directly but, rather, only indirectly
 * through nuttx/irq.h.
 */

#ifndef __ARCH_ARM_INCLUDE_STM32H7RS_IRQ_H
#define __ARCH_ARM_INCLUDE_STM32H7RS_IRQ_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Processor Exceptions (vectors 0-15) */

#define STM32_IRQ_RESERVED         (0)
#define STM32_IRQ_NMI             (2)
#define STM32_IRQ_HARDFAULT       (3)
#define STM32_IRQ_MEMFAULT        (4)
#define STM32_IRQ_BUSFAULT        (5)
#define STM32_IRQ_USAGEFAULT      (6)
#define STM32_IRQ_SVCALL          (11)
#define STM32_IRQ_DBGMONITOR      (12)
#define STM32_IRQ_PENDSV          (14)
#define STM32_IRQ_SYSTICK         (15)

/* External interrupts */

#define STM32_IRQ_FIRST           (16)

#include <arch/stm32h7rs/stm32h7s7xx_irq.h>

#endif /* __ARCH_ARM_INCLUDE_STM32H7RS_IRQ_H */
