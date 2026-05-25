/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32n6xxx_bsec.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6XXX_BSEC_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6XXX_BSEC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#include "hardware/stm32n6xxx_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_BSEC_FVR(n)          (STM32_BSEC_BASE + \
                                      ((uint32_t)(n) << 2))

#define STM32_BSEC_HSLV_FUSE       124u
#define STM32_BSEC_HSLV_VDDIO3     (1u << 15)
#define STM32_BSEC_HSLV_VDDIO2     (1u << 16)

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6XXX_BSEC_H */
