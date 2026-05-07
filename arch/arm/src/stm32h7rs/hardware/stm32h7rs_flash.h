/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32h7rs_flash.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_FLASH_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_FLASH_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32h7rs_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32H7RS_FLASH_ACR              (STM32H7RS_FLASH_R_BASE + 0x0000u)

#define FLASH_ACR_LATENCY_SHIFT          (0)
#define FLASH_ACR_LATENCY_MASK           (0x0fu << FLASH_ACR_LATENCY_SHIFT)
#define FLASH_ACR_LATENCY(n)             ((uint32_t)(n) << FLASH_ACR_LATENCY_SHIFT)
#define FLASH_ACR_LATENCY_7              FLASH_ACR_LATENCY(7)

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_FLASH_H */
