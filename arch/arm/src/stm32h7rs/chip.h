/****************************************************************************
 * arch/arm/src/stm32h7rs/chip.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_CHIP_H
#define __ARCH_ARM_SRC_STM32H7RS_CHIP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <arch/irq.h>
#include <arch/stm32h7rs/chip.h>
#include "hardware/stm32h7rs_memorymap.h"

#define ARMV7M_PERIPHERAL_INTERRUPTS STM32_IRQ_NEXTINTS

#define ARMV7M_DCACHE_LINESIZE       32
#define ARMV7M_ICACHE_LINESIZE       32

#endif /* __ARCH_ARM_SRC_STM32H7RS_CHIP_H */
