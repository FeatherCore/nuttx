/****************************************************************************
 * arch/arm/src/stm32u5/stm32_gfxmmu.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_STM32_GFXMMU_H
#define __ARCH_ARM_SRC_STM32U5_STM32_GFXMMU_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct stm32_gfxmmu_config_s
{
  uintptr_t physical;
  uint16_t width;
  uint16_t height;
  uint16_t stride;
  uint8_t bpp;
  bool circular;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_STM32U5_GFXMMU
int stm32_gfxmmuinitialize(const struct stm32_gfxmmu_config_s *config);
uintptr_t stm32_gfxmmu_buffer0(void);
#endif

#endif /* __ARCH_ARM_SRC_STM32U5_STM32_GFXMMU_H */
