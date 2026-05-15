/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32n6_memorymap.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_MEMORYMAP_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_MEMORYMAP_H

/* External memories visible through AXI */

#define STM32N6_XSPI2_MEM_BASE       0x70000000u
#define STM32N6_XSPI1_MEM_BASE       0x90000000u

/* Internal SRAM.  The STM32N6 BootROM loads the FSBL payload at
 * 0x34180400; the XIP application uses SRAM from 0x34000000.
 */

#define STM32N6_AXISRAM_BASE         0x34000000u
#define STM32N6_AXISRAM2_FSBL_BASE   0x34180400u
#define STM32N6_AHBSRAM_BASE         0x38000000u

/* Secure peripheral aliases used by the first-stage bring-up. */

#define STM32N6_PERIPH_BASE          0x50000000u
#define STM32N6_APB1_BASE            STM32N6_PERIPH_BASE
#define STM32N6_AHB1_BASE            (STM32N6_PERIPH_BASE + 0x00020000u)
#define STM32N6_APB2_BASE            (STM32N6_PERIPH_BASE + 0x02000000u)
#define STM32N6_AHB2_BASE            (STM32N6_PERIPH_BASE + 0x02020000u)
#define STM32N6_APB3_BASE            (STM32N6_PERIPH_BASE + 0x04000000u)
#define STM32N6_AHB3_BASE            (STM32N6_PERIPH_BASE + 0x04020000u)
#define STM32N6_APB4_BASE            (STM32N6_PERIPH_BASE + 0x06000000u)
#define STM32N6_AHB4_BASE            (STM32N6_PERIPH_BASE + 0x06020000u)
#define STM32N6_APB5_BASE            (STM32N6_PERIPH_BASE + 0x08000000u)
#define STM32N6_AHB5_BASE            (STM32N6_PERIPH_BASE + 0x08020000u)

#define STM32N6_USART1_BASE          (STM32N6_APB2_BASE + 0x1000u)

#define STM32N6_GPIOA_BASE           (STM32N6_AHB4_BASE + 0x0000u)
#define STM32N6_GPIOE_BASE           (STM32N6_AHB4_BASE + 0x1000u)
#define STM32N6_GPIOF_BASE           (STM32N6_AHB4_BASE + 0x1400u)
#define STM32N6_GPION_BASE           (STM32N6_AHB4_BASE + 0x3400u)
#define STM32N6_GPIOO_BASE           (STM32N6_AHB4_BASE + 0x3800u)
#define STM32N6_GPIOP_BASE           (STM32N6_AHB4_BASE + 0x3c00u)
#define STM32N6_GPIOQ_BASE           (STM32N6_AHB4_BASE + 0x4000u)
#define STM32N6_PWR_BASE             (STM32N6_AHB4_BASE + 0x4800u)
#define STM32N6_RCC_BASE             (STM32N6_AHB4_BASE + 0x8000u)
#define STM32N6_SYSCFG_BASE          (STM32N6_APB4_BASE + 0x8000u)
#define STM32N6_BSEC_BASE            (STM32N6_APB4_BASE + 0x9000u)

#define STM32N6_XSPI1_BASE           (STM32N6_AHB5_BASE + 0x5000u)
#define STM32N6_XSPI2_BASE           (STM32N6_AHB5_BASE + 0xa000u)
#define STM32N6_XSPIM_BASE           (STM32N6_AHB5_BASE + 0xb400u)

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_MEMORYMAP_H */
