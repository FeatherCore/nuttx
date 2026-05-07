/****************************************************************************
 * arch/arm/src/stm32u5/hardware/stm32_gfxmmu.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_GFXMMU_H
#define __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_GFXMMU_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_GFXMMU_VIRTUAL_BUFFERS_BASE 0x24000000
#define STM32_GFXMMU_VIRTUAL_BUFFER0_BASE STM32_GFXMMU_VIRTUAL_BUFFERS_BASE

#define STM32_GFXMMU_CR        (STM32_GFXMMU_BASE + 0x0000)
#define STM32_GFXMMU_SR        (STM32_GFXMMU_BASE + 0x0004)
#define STM32_GFXMMU_FCR       (STM32_GFXMMU_BASE + 0x0008)
#define STM32_GFXMMU_DVR       (STM32_GFXMMU_BASE + 0x0010)
#define STM32_GFXMMU_B0CR      (STM32_GFXMMU_BASE + 0x0020)
#define STM32_GFXMMU_LUT(n)    (STM32_GFXMMU_BASE + 0x1000 + ((n) * 4))

#define GFXMMU_CR_192BM        (1 << 6)

#define GFXMMU_LUTL_EN         (1 << 0)
#define GFXMMU_LUTL_FVB(n)     (((n) & 0xff) << 8)
#define GFXMMU_LUTL_LVB(n)     (((n) & 0xff) << 16)

#endif /* __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_GFXMMU_H */
