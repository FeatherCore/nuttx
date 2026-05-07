/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32n6_rcc.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_RCC_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_RCC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32n6_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_RCC_CR               (STM32N6_RCC_BASE + 0x0000u)
#define STM32N6_RCC_SR               (STM32N6_RCC_BASE + 0x0004u)
#define STM32N6_RCC_CFGR1            (STM32N6_RCC_BASE + 0x0020u)
#define STM32N6_RCC_CFGR2            (STM32N6_RCC_BASE + 0x0024u)
#define STM32N6_RCC_PLL1CFGR1        (STM32N6_RCC_BASE + 0x0080u)
#define STM32N6_RCC_PLL1CFGR2        (STM32N6_RCC_BASE + 0x0084u)
#define STM32N6_RCC_PLL1CFGR3        (STM32N6_RCC_BASE + 0x0088u)
#define STM32N6_RCC_IC1CFGR          (STM32N6_RCC_BASE + 0x00c4u)
#define STM32N6_RCC_IC2CFGR          (STM32N6_RCC_BASE + 0x00c8u)
#define STM32N6_RCC_IC3CFGR          (STM32N6_RCC_BASE + 0x00ccu)
#define STM32N6_RCC_IC6CFGR          (STM32N6_RCC_BASE + 0x00d8u)
#define STM32N6_RCC_IC11CFGR         (STM32N6_RCC_BASE + 0x00ecu)
#define STM32N6_RCC_CCIPR6           (STM32N6_RCC_BASE + 0x0158u)
#define STM32N6_RCC_CCIPR13          (STM32N6_RCC_BASE + 0x0174u)
#define STM32N6_RCC_AHB5RSTR         (STM32N6_RCC_BASE + 0x0220u)
#define STM32N6_RCC_DIVENR           (STM32N6_RCC_BASE + 0x0240u)
#define STM32N6_RCC_BUSENR           (STM32N6_RCC_BASE + 0x0244u)
#define STM32N6_RCC_MISCENR          (STM32N6_RCC_BASE + 0x0248u)
#define STM32N6_RCC_MEMENR           (STM32N6_RCC_BASE + 0x024cu)
#define STM32N6_RCC_AHB4ENR          (STM32N6_RCC_BASE + 0x025cu)
#define STM32N6_RCC_AHB5ENR          (STM32N6_RCC_BASE + 0x0260u)
#define STM32N6_RCC_APB2ENR          (STM32N6_RCC_BASE + 0x026cu)
#define STM32N6_RCC_APB4ENR2         (STM32N6_RCC_BASE + 0x0278u)

#define RCC_CR_HSION                 (1u << 3)
#define RCC_CR_PLL1ON                (1u << 8)

#define RCC_SR_HSIRDY                (1u << 3)
#define RCC_SR_PLL1RDY               (1u << 8)

#define RCC_CFGR1_CPUSW_MASK         (3u << 16)
#define RCC_CFGR1_CPUSW_HSI          (0u << 16)
#define RCC_CFGR1_CPUSW_IC1          (3u << 16)
#define RCC_CFGR1_CPUSWS_MASK        (3u << 20)
#define RCC_CFGR1_CPUSWS_HSI         (0u << 20)
#define RCC_CFGR1_CPUSWS_IC1         (3u << 20)
#define RCC_CFGR1_SYSSW_MASK         (3u << 24)
#define RCC_CFGR1_SYSSW_HSI          (0u << 24)
#define RCC_CFGR1_SYSSW_IC2_IC6_IC11 (3u << 24)
#define RCC_CFGR1_SYSSWS_MASK        (3u << 28)
#define RCC_CFGR1_SYSSWS_HSI         (0u << 28)
#define RCC_CFGR1_SYSSWS_IC2_IC6_IC11 (3u << 28)

#define RCC_CFGR2_PPRE1_MASK         (7u << 0)
#define RCC_CFGR2_PPRE2_MASK         (7u << 4)
#define RCC_CFGR2_PPRE4_MASK         (7u << 12)
#define RCC_CFGR2_PPRE5_MASK         (7u << 16)
#define RCC_CFGR2_HPRE_MASK          (7u << 20)
#define RCC_CFGR2_HPRE_DIV2          (1u << 20)

#define RCC_PLL1CFGR1_DIVN_SHIFT     8
#define RCC_PLL1CFGR1_DIVM_SHIFT     20
#define RCC_PLL1CFGR1_PLL1SEL_MASK   (7u << 28)
#define RCC_PLL1CFGR1_PLL1SEL_HSI    (0u << 28)
#define RCC_PLL1CFGR1_DIVN(n)        ((uint32_t)(n) << \
                                      RCC_PLL1CFGR1_DIVN_SHIFT)
#define RCC_PLL1CFGR1_DIVM(n)        ((uint32_t)(n) << \
                                      RCC_PLL1CFGR1_DIVM_SHIFT)

#define RCC_PLL1CFGR3_PDIV2_SHIFT    24
#define RCC_PLL1CFGR3_PDIV1_SHIFT    27
#define RCC_PLL1CFGR3_PDIVEN         (1u << 30)
#define RCC_PLL1CFGR3_PDIV2(n)       ((uint32_t)(n) << \
                                      RCC_PLL1CFGR3_PDIV2_SHIFT)
#define RCC_PLL1CFGR3_PDIV1(n)       ((uint32_t)(n) << \
                                      RCC_PLL1CFGR3_PDIV1_SHIFT)

#define RCC_ICCFGR_SEL_PLL1          (0u << 28)
#define RCC_ICCFGR_INT_SHIFT         16
#define RCC_ICCFGR_INT(n)            ((uint32_t)((n) - 1u) << \
                                      RCC_ICCFGR_INT_SHIFT)
#define RCC_ICCFGR(sel, div)         ((sel) | RCC_ICCFGR_INT(div))

#define RCC_DIVENR_IC1EN             (1u << 0)
#define RCC_DIVENR_IC2EN             (1u << 1)
#define RCC_DIVENR_IC3EN             (1u << 2)
#define RCC_DIVENR_IC6EN             (1u << 5)
#define RCC_DIVENR_IC11EN            (1u << 10)

#define RCC_BUSENR_ALL_BUS           ((1u << 0) | (1u << 1) | (1u << 2) | \
                                      (1u << 3) | (1u << 4) | (1u << 5) | \
                                      (1u << 6) | (1u << 7) | (1u << 8) | \
                                      (1u << 9) | (1u << 10) | \
                                      (1u << 11) | (1u << 12))

#define RCC_MISCENR_XSPIPHYCOMPEN    (1u << 3)

#define RCC_MEMENR_AXISRAM           ((1u << 0) | (1u << 1) | (1u << 2) | \
                                      (1u << 3) | (1u << 7) | (1u << 8))
#define RCC_MEMENR_AHBSRAM           ((1u << 4) | (1u << 5))

#define RCC_AHB4ENR_GPIOEEN          (1u << 4)
#define RCC_AHB4ENR_GPIONEN          (1u << 13)
#define RCC_AHB4ENR_GPIOOEN          (1u << 14)
#define RCC_AHB4ENR_GPIOPEN          (1u << 15)
#define RCC_AHB4ENR_GPIOQEN          (1u << 16)
#define RCC_AHB4ENR_PWREN            (1u << 18)

#define RCC_AHB5ENR_XSPI1EN          (1u << 5)
#define RCC_AHB5ENR_XSPI2EN          (1u << 12)
#define RCC_AHB5ENR_XSPIMEN          (1u << 13)

#define RCC_AHB5RSTR_XSPI1RST        (1u << 5)
#define RCC_AHB5RSTR_XSPI2RST        (1u << 12)
#define RCC_AHB5RSTR_XSPIMRST        (1u << 13)

#define RCC_APB2ENR_USART1EN         (1u << 4)

#define RCC_APB4ENR2_SYSCFGEN        (1u << 0)
#define RCC_APB4ENR2_BSECEN          (1u << 1)

#define RCC_CCIPR6_XSPI1SEL_MASK     (3u << 0)
#define RCC_CCIPR6_XSPI1SEL_HCLK     (0u << 0)
#define RCC_CCIPR6_XSPI2SEL_MASK     (3u << 4)
#define RCC_CCIPR6_XSPI2SEL_IC3      (2u << 4)

#define RCC_CCIPR13_USART1SEL_MASK   (7u << 0)
#define RCC_CCIPR13_USART1SEL_PCLK2  (0u << 0)

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_RCC_H */
