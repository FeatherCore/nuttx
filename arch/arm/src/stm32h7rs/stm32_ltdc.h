/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32_ltdc.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_STM32_LTDC_H
#define __ARCH_ARM_SRC_STM32H7RS_STM32_LTDC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>

#include <nuttx/video/fb.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int stm32_ltdcinitialize(void);
void stm32_ltdcuninitialize(void);
FAR struct fb_vtable_s *stm32_ltdcgetvplane(int vplane);

/* Boards may override this hook to control panel power/backlight GPIOs from
 * the framebuffer setpower callback.
 */

void stm32_ltdcpower(bool on);

#endif /* __ARCH_ARM_SRC_STM32H7RS_STM32_LTDC_H */
