/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32h7rs_ltdc.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_STM32H7RS_LTDC_H
#define __ARCH_ARM_SRC_STM32H7RS_STM32H7RS_LTDC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <stdbool.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

struct fb_vtable_s;

int stm32h7rs_ltdcinitialize(void);
FAR struct fb_vtable_s *stm32h7rs_ltdcgetvplane(int vplane);
void stm32h7rs_ltdcuninitialize(void);

/* Boards may override this hook to control panel power/backlight GPIOs from
 * the framebuffer setpower callback.
 */

void stm32h7rs_ltdcpower(bool on);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_ARM_SRC_STM32H7RS_STM32H7RS_LTDC_H */
