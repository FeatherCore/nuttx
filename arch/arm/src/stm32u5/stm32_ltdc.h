/****************************************************************************
 * arch/arm/src/stm32u5/stm32_ltdc.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_STM32_LTDC_H
#define __ARCH_ARM_SRC_STM32U5_STM32_LTDC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/video/fb.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct stm32_ltdc_config_s
{
  uintptr_t fb_base;
  uintptr_t layer_fb_base;
  uint16_t xres;
  uint16_t yres;
  uint16_t stride;
  uint16_t hsync;
  uint16_t hbp;
  uint16_t hfp;
  uint16_t vsync;
  uint16_t vbp;
  uint16_t vfp;
  uint8_t bpp;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_STM32U5_LTDC
int stm32_ltdcinitialize(const struct stm32_ltdc_config_s *config);
void stm32_ltdcuninitialize(void);
struct fb_vtable_s *stm32_ltdcgetvplane(int vplane);
#endif

#endif /* __ARCH_ARM_SRC_STM32U5_STM32_LTDC_H */
