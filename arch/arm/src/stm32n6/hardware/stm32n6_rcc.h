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
#define STM32N6_RCC_HSICFGR          (STM32N6_RCC_BASE + 0x0048u)
#define STM32N6_RCC_PLL1CFGR1        (STM32N6_RCC_BASE + 0x0080u)
#define STM32N6_RCC_PLL1CFGR2        (STM32N6_RCC_BASE + 0x0084u)
#define STM32N6_RCC_PLL1CFGR3        (STM32N6_RCC_BASE + 0x0088u)
#define STM32N6_RCC_IC1CFGR          (STM32N6_RCC_BASE + 0x00c4u)
#define STM32N6_RCC_IC2CFGR          (STM32N6_RCC_BASE + 0x00c8u)
#define STM32N6_RCC_IC3CFGR          (STM32N6_RCC_BASE + 0x00ccu)
#define STM32N6_RCC_IC6CFGR          (STM32N6_RCC_BASE + 0x00d8u)
#define STM32N6_RCC_IC11CFGR         (STM32N6_RCC_BASE + 0x00ecu)
#define STM32N6_RCC_IC16CFGR         (STM32N6_RCC_BASE + 0x0100u)
#define STM32N6_RCC_CCIPR4           (STM32N6_RCC_BASE + 0x0150u)
#define STM32N6_RCC_CCIPR6           (STM32N6_RCC_BASE + 0x0158u)
#define STM32N6_RCC_CCIPR13          (STM32N6_RCC_BASE + 0x0174u)
#define STM32N6_RCC_AHB5RSTR         (STM32N6_RCC_BASE + 0x0220u)
#define STM32N6_RCC_APB1RSTR1        (STM32N6_RCC_BASE + 0x0224u)
#define STM32N6_RCC_APB2RSTR         (STM32N6_RCC_BASE + 0x022cu)
#define STM32N6_RCC_APB5RSTR         (STM32N6_RCC_BASE + 0x023cu)
#define STM32N6_RCC_AHB5RSTSR        (STM32N6_RCC_BASE + 0x0a20u)
#define STM32N6_RCC_APB1RSTSR1       (STM32N6_RCC_BASE + 0x0a24u)
#define STM32N6_RCC_APB2RSTSR        (STM32N6_RCC_BASE + 0x0a2cu)
#define STM32N6_RCC_APB5RSTSR        (STM32N6_RCC_BASE + 0x0a3cu)
#define STM32N6_RCC_AHB5RSTCR        (STM32N6_RCC_BASE + 0x1220u)
#define STM32N6_RCC_APB1RSTCR1       (STM32N6_RCC_BASE + 0x1224u)
#define STM32N6_RCC_APB2RSTCR        (STM32N6_RCC_BASE + 0x122cu)
#define STM32N6_RCC_APB5RSTCR        (STM32N6_RCC_BASE + 0x123cu)
#define STM32N6_RCC_DIVENR           (STM32N6_RCC_BASE + 0x0240u)
#define STM32N6_RCC_BUSENR           (STM32N6_RCC_BASE + 0x0244u)
#define STM32N6_RCC_MISCENR          (STM32N6_RCC_BASE + 0x0248u)
#define STM32N6_RCC_MEMENR           (STM32N6_RCC_BASE + 0x024cu)
#define STM32N6_RCC_AHB3ENR          (STM32N6_RCC_BASE + 0x0258u)
#define STM32N6_RCC_AHB4ENR          (STM32N6_RCC_BASE + 0x025cu)
#define STM32N6_RCC_AHB5ENR          (STM32N6_RCC_BASE + 0x0260u)
#define STM32N6_RCC_APB1ENR1         (STM32N6_RCC_BASE + 0x0264u)
#define STM32N6_RCC_APB2ENR          (STM32N6_RCC_BASE + 0x026cu)
#define STM32N6_RCC_APB4ENR2         (STM32N6_RCC_BASE + 0x0278u)
#define STM32N6_RCC_APB5ENR          (STM32N6_RCC_BASE + 0x027cu)
#define STM32N6_RCC_CSR              (STM32N6_RCC_BASE + 0x0800u)
#define STM32N6_RCC_DIVENSR          (STM32N6_RCC_BASE + 0x0a40u)
#define STM32N6_RCC_BUSENSR          (STM32N6_RCC_BASE + 0x0a44u)
#define STM32N6_RCC_MISCENSR         (STM32N6_RCC_BASE + 0x0a48u)
#define STM32N6_RCC_MEMENSR          (STM32N6_RCC_BASE + 0x0a4cu)
#define STM32N6_RCC_AHB3ENSR         (STM32N6_RCC_BASE + 0x0a58u)
#define STM32N6_RCC_AHB4ENSR         (STM32N6_RCC_BASE + 0x0a5cu)
#define STM32N6_RCC_AHB5ENSR         (STM32N6_RCC_BASE + 0x0a60u)
#define STM32N6_RCC_APB1ENSR1        (STM32N6_RCC_BASE + 0x0a64u)
#define STM32N6_RCC_APB2ENSR         (STM32N6_RCC_BASE + 0x0a6cu)
#define STM32N6_RCC_APB4ENSR2        (STM32N6_RCC_BASE + 0x0a78u)
#define STM32N6_RCC_APB5ENSR         (STM32N6_RCC_BASE + 0x0a7cu)
#define STM32N6_RCC_CCR              (STM32N6_RCC_BASE + 0x1000u)
#define STM32N6_RCC_APB1ENCR1        (STM32N6_RCC_BASE + 0x1264u)

#define RCC_CR_HSION                 (1u << 3)
#define RCC_CR_PLL1ON                (1u << 8)

#define RCC_SR_HSIRDY                (1u << 3)
#define RCC_SR_PLL1RDY               (1u << 8)

#define RCC_CSR_HSIONS               (1u << 3)
#define RCC_CSR_PLL1ONS              (1u << 8)

#define RCC_CCR_HSIONC               (1u << 3)
#define RCC_CCR_PLL1ONC              (1u << 8)

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

#define RCC_HSICFGR_HSIDIV_MASK      (3u << 7)
#define RCC_HSICFGR_HSIDIV_DIV1      (0u << 7)

#define RCC_PLL1CFGR3_PDIV2_SHIFT    24
#define RCC_PLL1CFGR3_PDIV1_SHIFT    27
#define RCC_PLL1CFGR3_MODSSRST       (1u << 0)
#define RCC_PLL1CFGR3_MODSSDIS       (1u << 2)
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
#define RCC_DIVENR_IC16EN            (1u << 15)

#define RCC_DIVENSR_IC1ENS           (1u << 0)
#define RCC_DIVENSR_IC2ENS           (1u << 1)
#define RCC_DIVENSR_IC3ENS           (1u << 2)
#define RCC_DIVENSR_IC6ENS           (1u << 5)
#define RCC_DIVENSR_IC11ENS          (1u << 10)
#define RCC_DIVENSR_IC16ENS          (1u << 15)

#define RCC_AHB5RST_XSPI1            (1u << 5)
#define RCC_AHB5RST_XSPI2            (1u << 12)
#define RCC_AHB5RST_XSPIM            (1u << 13)

#define RCC_BUSENR_ALL_BUS           ((1u << 0) | (1u << 1) | (1u << 2) | \
                                      (1u << 3) | (1u << 4) | (1u << 5) | \
                                      (1u << 6) | (1u << 7) | (1u << 8) | \
                                      (1u << 9) | (1u << 10) | \
                                      (1u << 11) | (1u << 12))

#define RCC_BUSENSR_ALL_BUS          RCC_BUSENR_ALL_BUS

#define RCC_MISCENR_XSPIPHYCOMPEN    (1u << 3)
#define RCC_MISCENSR_XSPIPHYCOMPENS  RCC_MISCENR_XSPIPHYCOMPEN

#define RCC_MEMENR_AXISRAM           ((1u << 0) | (1u << 1) | (1u << 2) | \
                                      (1u << 3) | (1u << 7) | (1u << 8))
#define RCC_MEMENR_AHBSRAM           ((1u << 4) | (1u << 5))

#define RCC_MEMENSR_AXISRAM          RCC_MEMENR_AXISRAM
#define RCC_MEMENSR_AHBSRAM          RCC_MEMENR_AHBSRAM

#define RCC_AHB3ENR_RIFSCEN          (1u << 9)
#define RCC_AHB3ENSR_RIFSCENS        RCC_AHB3ENR_RIFSCEN

#define RCC_AHB4ENR_GPIOAEN          (1u << 0)
#define RCC_AHB4ENR_GPIOBEN          (1u << 1)
#define RCC_AHB4ENR_GPIOCEN          (1u << 2)
#define RCC_AHB4ENR_GPIODEN          (1u << 3)
#define RCC_AHB4ENR_GPIOEEN          (1u << 4)
#define RCC_AHB4ENR_GPIOFEN          (1u << 5)
#define RCC_AHB4ENR_GPIOGEN          (1u << 6)
#define RCC_AHB4ENR_GPIOHEN          (1u << 7)
#define RCC_AHB4ENR_GPIONEN          (1u << 13)
#define RCC_AHB4ENR_GPIOOEN          (1u << 14)
#define RCC_AHB4ENR_GPIOPEN          (1u << 15)
#define RCC_AHB4ENR_GPIOQEN          (1u << 16)
#define RCC_AHB4ENR_PWREN            (1u << 18)

#define RCC_AHB4ENSR_GPIOAENS        RCC_AHB4ENR_GPIOAEN
#define RCC_AHB4ENSR_GPIOBENS        RCC_AHB4ENR_GPIOBEN
#define RCC_AHB4ENSR_GPIOCENS        RCC_AHB4ENR_GPIOCEN
#define RCC_AHB4ENSR_GPIODENS        RCC_AHB4ENR_GPIODEN
#define RCC_AHB4ENSR_GPIOEENS        RCC_AHB4ENR_GPIOEEN
#define RCC_AHB4ENSR_GPIOFENS        RCC_AHB4ENR_GPIOFEN
#define RCC_AHB4ENSR_GPIOGENS        RCC_AHB4ENR_GPIOGEN
#define RCC_AHB4ENSR_GPIOHENS        RCC_AHB4ENR_GPIOHEN
#define RCC_AHB4ENSR_GPIONENS        RCC_AHB4ENR_GPIONEN
#define RCC_AHB4ENSR_GPIOOENS        RCC_AHB4ENR_GPIOOEN
#define RCC_AHB4ENSR_GPIOPENS        RCC_AHB4ENR_GPIOPEN
#define RCC_AHB4ENSR_GPIOQENS        RCC_AHB4ENR_GPIOQEN
#define RCC_AHB4ENSR_PWRENS          RCC_AHB4ENR_PWREN

#define RCC_AHB5ENR_XSPI1EN          (1u << 5)
#define RCC_AHB5ENR_XSPI2EN          (1u << 12)
#define RCC_AHB5ENR_XSPIMEN          (1u << 13)

#define RCC_AHB5ENSR_XSPI1ENS        RCC_AHB5ENR_XSPI1EN
#define RCC_AHB5ENSR_XSPI2ENS        RCC_AHB5ENR_XSPI2EN
#define RCC_AHB5ENSR_XSPIMENS        RCC_AHB5ENR_XSPIMEN

#define RCC_AHB5RSTR_XSPI1RST        RCC_AHB5RST_XSPI1
#define RCC_AHB5RSTR_XSPI2RST        RCC_AHB5RST_XSPI2
#define RCC_AHB5RSTR_XSPIMRST        RCC_AHB5RST_XSPIM

#define RCC_APB2ENR_USART1EN         (1u << 4)
#define RCC_APB2ENSR_USART1ENS       RCC_APB2ENR_USART1EN

#define RCC_APB1ENR1_I2C1EN          (1u << 21)
#define RCC_APB1ENR1_I2C2EN          (1u << 22)
#define RCC_APB1ENR1_I2C3EN          (1u << 23)

#define RCC_APB1ENSR1_I2C1ENS        RCC_APB1ENR1_I2C1EN
#define RCC_APB1ENSR1_I2C2ENS        RCC_APB1ENR1_I2C2EN
#define RCC_APB1ENSR1_I2C3ENS        RCC_APB1ENR1_I2C3EN

#define RCC_APB1ENCR1_I2C1ENC        RCC_APB1ENR1_I2C1EN
#define RCC_APB1ENCR1_I2C2ENC        RCC_APB1ENR1_I2C2EN
#define RCC_APB1ENCR1_I2C3ENC        RCC_APB1ENR1_I2C3EN

#define RCC_APB1RSTR1_I2C1RST        (1u << 21)
#define RCC_APB1RSTR1_I2C2RST        (1u << 22)
#define RCC_APB1RSTR1_I2C3RST        (1u << 23)

#define RCC_APB1RSTCR1_I2C1RSTC      RCC_APB1RSTR1_I2C1RST
#define RCC_APB1RSTCR1_I2C2RSTC      RCC_APB1RSTR1_I2C2RST
#define RCC_APB1RSTCR1_I2C3RSTC      RCC_APB1RSTR1_I2C3RST

#define RCC_APB2RSTR_USART1RST       (1u << 4)

#define RCC_APB5ENR_LTDCEN           (1u << 1)
#define RCC_APB5ENSR_LTDCENS         RCC_APB5ENR_LTDCEN

#define RCC_APB5RSTR_LTDCRST         (1u << 1)

#define RCC_APB4ENR2_SYSCFGEN        (1u << 0)
#define RCC_APB4ENR2_BSECEN          (1u << 1)

#define RCC_APB4ENSR2_SYSCFGENS      RCC_APB4ENR2_SYSCFGEN
#define RCC_APB4ENSR2_BSECENS        RCC_APB4ENR2_BSECEN

#define RCC_CCIPR4_LTDCSEL_MASK      (3u << 24)
#define RCC_CCIPR4_LTDCSEL_IC16      (2u << 24)
#define RCC_CCIPR4_I2C2SEL_MASK      (7u << 4)
#define RCC_CCIPR4_I2C2SEL_PCLK1     (0u << 4)

#define RCC_CCIPR6_XSPI1SEL_MASK     (3u << 0)
#define RCC_CCIPR6_XSPI1SEL_HCLK     (0u << 0)
#define RCC_CCIPR6_XSPI2SEL_MASK     (3u << 4)
#define RCC_CCIPR6_XSPI2SEL_IC3      (2u << 4)

#define RCC_CCIPR13_USART1SEL_MASK   (7u << 0)
#define RCC_CCIPR13_USART1SEL_PCLK2  (0u << 0)

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_RCC_H */
