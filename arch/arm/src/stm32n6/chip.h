/****************************************************************************
 * arch/arm/src/stm32n6/chip.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_CHIP_H
#define __ARCH_ARM_SRC_STM32N6_CHIP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <arch/irq.h>
#include <arch/stm32n6/chip.h>

#include "hardware/stm32n6_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ARMV8M_PERIPHERAL_INTERRUPTS STM32_IRQ_NEXTINTS

#define ARMV8M_DCACHE_LINESIZE       32
#define ARMV8M_ICACHE_LINESIZE       32

#endif /* __ARCH_ARM_SRC_STM32N6_CHIP_H */
