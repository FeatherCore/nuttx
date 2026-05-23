/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_ltdc.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_LTDC_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_LTDC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>

#include <nuttx/video/fb.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int stm32n6_ltdcinitialize(void);
void stm32n6_ltdcuninitialize(void);
FAR struct fb_vtable_s *stm32n6_ltdcgetvplane(int vplane);

/* Boards may override this hook to control panel power/backlight GPIOs from
 * the framebuffer setpower callback.
 */

void stm32n6_ltdcpower(bool on);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_LTDC_H */
