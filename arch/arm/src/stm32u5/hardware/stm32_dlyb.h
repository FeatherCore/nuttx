/****************************************************************************
 * arch/arm/src/stm32u5/hardware/stm32_dlyb.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DLYB_H
#define __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DLYB_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register Offsets *********************************************************/

#define STM32_DLYB_CR_OFFSET                0x0000
#define STM32_DLYB_CFGR_OFFSET              0x0004

/* Register Addresses *******************************************************/

#define STM32_DLYB_CR(b)                    ((b) + STM32_DLYB_CR_OFFSET)
#define STM32_DLYB_CFGR(b)                  ((b) + STM32_DLYB_CFGR_OFFSET)

/* Register Bitfield Definitions ********************************************/

#define DLYB_CR_DEN                         (1 << 0)
#define DLYB_CR_SEN                         (1 << 1)

#define DLYB_CFGR_SEL_MASK                  0x0000000f
#define DLYB_CFGR_UNIT_SHIFT                8
#define DLYB_CFGR_UNIT_MASK                 (0x7f << DLYB_CFGR_UNIT_SHIFT)
#define DLYB_CFGR_UNIT(n)                   ((uint32_t)(n) << \
                                             DLYB_CFGR_UNIT_SHIFT)
#define DLYB_CFGR_LNG_SHIFT                 16
#define DLYB_CFGR_LNG_MASK                  (0x0fff << DLYB_CFGR_LNG_SHIFT)
#define DLYB_CFGR_LNGF                      (1 << 31)

#endif /* __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DLYB_H */
