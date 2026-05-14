/****************************************************************************
 * arch/arm/include/stm32n6/chip.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_INCLUDE_STM32N6_CHIP_H
#define __ARCH_ARM_INCLUDE_STM32N6_CHIP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if !defined(CONFIG_ARCH_CHIP_STM32N657X0)
#  error STM32N6 chip not identified
#endif

/* STM32N6570-DK resources used by the first NXboot port.  The SRAM and
 * XIP address ranges match STM32CubeN6 Template_FSBL_XIP_Custom.
 */

#define STM32N6_AXISRAM_SIZE        (0x003ca000u)
#define STM32N6_AHBSRAM_SIZE        (32 * 1024u)
#define STM32N6_FSBL_RAM_SIZE       (511 * 1024u)
#define STM32N6_APP_RAM_SIZE        (2 * 1024 * 1024u)
#define STM32N6_XSPI2_NOR_SIZE      (128 * 1024 * 1024u)
#define STM32N6_XSPI1_PSRAM_SIZE    (32 * 1024 * 1024u)

#define STM32N6_NGPIO               17
#define STM32N6_NUSART              10
#define STM32N6_NXSPI               3

#define NVIC_SYSH_PRIORITY_MIN      0xf0
#define NVIC_SYSH_PRIORITY_DEFAULT  0x80
#define NVIC_SYSH_PRIORITY_MAX      0x00
#define NVIC_SYSH_PRIORITY_STEP     0x10

#endif /* __ARCH_ARM_INCLUDE_STM32N6_CHIP_H */
