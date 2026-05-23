/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32h7rs_rcc.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_RCC_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_RCC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32h7rs_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32H7RS_RCC_CR                 (STM32H7RS_RCC_BASE + 0x0000u)
#define STM32H7RS_RCC_HSICFGR            (STM32H7RS_RCC_BASE + 0x0004u)
#define STM32H7RS_RCC_CSICFGR            (STM32H7RS_RCC_BASE + 0x000cu)
#define STM32H7RS_RCC_CFGR               (STM32H7RS_RCC_BASE + 0x0010u)
#define STM32H7RS_RCC_CDCFGR             (STM32H7RS_RCC_BASE + 0x0018u)
#define STM32H7RS_RCC_BMCFGR             (STM32H7RS_RCC_BASE + 0x001cu)
#define STM32H7RS_RCC_APBCFGR            (STM32H7RS_RCC_BASE + 0x0020u)
#define STM32H7RS_RCC_PLLCKSELR          (STM32H7RS_RCC_BASE + 0x0028u)
#define STM32H7RS_RCC_PLLCFGR            (STM32H7RS_RCC_BASE + 0x002cu)
#define STM32H7RS_RCC_PLL1DIVR1          (STM32H7RS_RCC_BASE + 0x0030u)
#define STM32H7RS_RCC_PLL1FRACR          (STM32H7RS_RCC_BASE + 0x0034u)
#define STM32H7RS_RCC_PLL2DIVR1          (STM32H7RS_RCC_BASE + 0x0038u)
#define STM32H7RS_RCC_PLL2FRACR          (STM32H7RS_RCC_BASE + 0x003cu)
#define STM32H7RS_RCC_PLL3DIVR1          (STM32H7RS_RCC_BASE + 0x0040u)
#define STM32H7RS_RCC_PLL3FRACR          (STM32H7RS_RCC_BASE + 0x0044u)
#define STM32H7RS_RCC_CCIPR1             (STM32H7RS_RCC_BASE + 0x004cu)
#define STM32H7RS_RCC_CCIPR2             (STM32H7RS_RCC_BASE + 0x0050u)
#define STM32H7RS_RCC_CCIPR4             (STM32H7RS_RCC_BASE + 0x0058u)
#define STM32H7RS_RCC_PLL1DIVR2          (STM32H7RS_RCC_BASE + 0x00c0u)
#define STM32H7RS_RCC_PLL2DIVR2          (STM32H7RS_RCC_BASE + 0x00c4u)
#define STM32H7RS_RCC_PLL3DIVR2          (STM32H7RS_RCC_BASE + 0x00c8u)
#define STM32H7RS_RCC_AHB5RSTR           (STM32H7RS_RCC_BASE + 0x007cu)
#define STM32H7RS_RCC_APB5RSTR           (STM32H7RS_RCC_BASE + 0x008cu)
#define STM32H7RS_RCC_APB1RSTR1          (STM32H7RS_RCC_BASE + 0x0090u)
#define STM32H7RS_RCC_AHB4ENR            (STM32H7RS_RCC_BASE + 0x0140u)
#define STM32H7RS_RCC_APB5ENR            (STM32H7RS_RCC_BASE + 0x0144u)
#define STM32H7RS_RCC_APB1ENR1           (STM32H7RS_RCC_BASE + 0x0148u)
#define STM32H7RS_RCC_AHB5ENR            (STM32H7RS_RCC_BASE + 0x0134u)
#define STM32H7RS_RCC_APB4ENR            (STM32H7RS_RCC_BASE + 0x0154u)

#define RCC_CR_HSION                     (1u << 0)
#define RCC_CR_HSIRDY                    (1u << 2)
#define RCC_CR_HSIDIV_SHIFT              3
#define RCC_CR_HSIDIV_MASK               (3u << RCC_CR_HSIDIV_SHIFT)
#define RCC_CR_CSION                     (1u << 7)
#define RCC_CR_CSIRDY                    (1u << 8)
#define RCC_CR_PLL1ON                    (1u << 24)
#define RCC_CR_PLL1RDY                   (1u << 25)
#define RCC_CR_PLL2ON                    (1u << 26)
#define RCC_CR_PLL2RDY                   (1u << 27)
#define RCC_CR_PLL3ON                    (1u << 28)
#define RCC_CR_PLL3RDY                   (1u << 29)

#define RCC_HSICFGR_HSITRIM_SHIFT        24
#define RCC_HSICFGR_HSITRIM_MASK         (0x7fu << RCC_HSICFGR_HSITRIM_SHIFT)
#define RCC_HSICFGR_HSITRIM(n)           ((uint32_t)(n) << \
                                           RCC_HSICFGR_HSITRIM_SHIFT)

#define RCC_CFGR_SW_SHIFT                0
#define RCC_CFGR_SW_MASK                 (7u << RCC_CFGR_SW_SHIFT)
#define RCC_CFGR_SW_PLL1                 (3u << RCC_CFGR_SW_SHIFT)
#define RCC_CFGR_SWS_SHIFT               3
#define RCC_CFGR_SWS_MASK                (7u << RCC_CFGR_SWS_SHIFT)
#define RCC_CFGR_SWS_PLL1                (3u << RCC_CFGR_SWS_SHIFT)

#define RCC_CDCFGR_CPRE_MASK             0x0fu
#define RCC_CDCFGR_CPRE_DIV1             0u

#define RCC_BMCFGR_BMPRE_MASK            0x0fu
#define RCC_BMCFGR_BMPRE_DIV2            (1u << 3)

#define RCC_APBCFGR_PPRE1_SHIFT          0
#define RCC_APBCFGR_PPRE1_MASK           (7u << RCC_APBCFGR_PPRE1_SHIFT)
#define RCC_APBCFGR_PPRE1_DIV2           (4u << RCC_APBCFGR_PPRE1_SHIFT)
#define RCC_APBCFGR_PPRE2_SHIFT          4
#define RCC_APBCFGR_PPRE2_MASK           (7u << RCC_APBCFGR_PPRE2_SHIFT)
#define RCC_APBCFGR_PPRE2_DIV2           (4u << RCC_APBCFGR_PPRE2_SHIFT)
#define RCC_APBCFGR_PPRE4_SHIFT          8
#define RCC_APBCFGR_PPRE4_MASK           (7u << RCC_APBCFGR_PPRE4_SHIFT)
#define RCC_APBCFGR_PPRE4_DIV2           (4u << RCC_APBCFGR_PPRE4_SHIFT)
#define RCC_APBCFGR_PPRE5_SHIFT          12
#define RCC_APBCFGR_PPRE5_MASK           (7u << RCC_APBCFGR_PPRE5_SHIFT)
#define RCC_APBCFGR_PPRE5_DIV2           (4u << RCC_APBCFGR_PPRE5_SHIFT)

#define RCC_PLLCKSELR_PLLSRC_MASK        3u
#define RCC_PLLCKSELR_PLLSRC_HSI         0u
#define RCC_PLLCKSELR_DIVM1_SHIFT        4
#define RCC_PLLCKSELR_DIVM1_MASK         (0x3fu << RCC_PLLCKSELR_DIVM1_SHIFT)
#define RCC_PLLCKSELR_DIVM1(n)           ((uint32_t)(n) << \
                                           RCC_PLLCKSELR_DIVM1_SHIFT)
#define RCC_PLLCKSELR_DIVM2_SHIFT        12
#define RCC_PLLCKSELR_DIVM2_MASK         (0x3fu << RCC_PLLCKSELR_DIVM2_SHIFT)
#define RCC_PLLCKSELR_DIVM2(n)           ((uint32_t)(n) << \
                                           RCC_PLLCKSELR_DIVM2_SHIFT)
#define RCC_PLLCKSELR_DIVM3_SHIFT        20
#define RCC_PLLCKSELR_DIVM3_MASK         (0x3fu << RCC_PLLCKSELR_DIVM3_SHIFT)
#define RCC_PLLCKSELR_DIVM3(n)           ((uint32_t)(n) << \
                                           RCC_PLLCKSELR_DIVM3_SHIFT)

#define RCC_PLLCFGR_PLL1VCOSEL           (1u << 1)
#define RCC_PLLCFGR_PLL1RGE_SHIFT        3
#define RCC_PLLCFGR_PLL1RGE_MASK         (3u << RCC_PLLCFGR_PLL1RGE_SHIFT)
#define RCC_PLLCFGR_PLL1RGE_2_4          (1u << RCC_PLLCFGR_PLL1RGE_SHIFT)
#define RCC_PLLCFGR_PLL1PEN              (1u << 5)
#define RCC_PLLCFGR_PLL2VCOSEL           (1u << 12)
#define RCC_PLLCFGR_PLL2RGE_SHIFT        14
#define RCC_PLLCFGR_PLL2RGE_MASK         (3u << RCC_PLLCFGR_PLL2RGE_SHIFT)
#define RCC_PLLCFGR_PLL2RGE_8_16         (3u << RCC_PLLCFGR_PLL2RGE_SHIFT)
#define RCC_PLLCFGR_PLL2SEN              (1u << 19)
#define RCC_PLLCFGR_PLL3VCOSEL           (1u << 23)
#define RCC_PLLCFGR_PLL3RGE_SHIFT        25
#define RCC_PLLCFGR_PLL3RGE_MASK         (3u << RCC_PLLCFGR_PLL3RGE_SHIFT)
#define RCC_PLLCFGR_PLL3RGE_8_16         (3u << RCC_PLLCFGR_PLL3RGE_SHIFT)
#define RCC_PLLCFGR_PLL3PEN              (1u << 27)
#define RCC_PLLCFGR_PLL3QEN              (1u << 28)
#define RCC_PLLCFGR_PLL3REN              (1u << 29)
#define RCC_PLLCFGR_PLL3SEN              (1u << 30)

#define RCC_PLL1DIVR1_DIVN_SHIFT         0
#define RCC_PLL1DIVR1_DIVN_MASK          (0x1ffu << RCC_PLL1DIVR1_DIVN_SHIFT)
#define RCC_PLL1DIVR1_DIVN(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL1DIVR1_DIVN_SHIFT)
#define RCC_PLL1DIVR1_DIVP_SHIFT         9
#define RCC_PLL1DIVR1_DIVP_MASK          (0x7fu << RCC_PLL1DIVR1_DIVP_SHIFT)
#define RCC_PLL1DIVR1_DIVP(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL1DIVR1_DIVP_SHIFT)
#define RCC_PLL1DIVR1_DIVQ_SHIFT         16
#define RCC_PLL1DIVR1_DIVQ_MASK          (0x7fu << RCC_PLL1DIVR1_DIVQ_SHIFT)
#define RCC_PLL1DIVR1_DIVQ(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL1DIVR1_DIVQ_SHIFT)
#define RCC_PLL1DIVR1_DIVR_SHIFT         24
#define RCC_PLL1DIVR1_DIVR_MASK          (0x7fu << RCC_PLL1DIVR1_DIVR_SHIFT)
#define RCC_PLL1DIVR1_DIVR(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL1DIVR1_DIVR_SHIFT)

#define RCC_PLL1DIVR2_DIVS_SHIFT         0
#define RCC_PLL1DIVR2_DIVS_MASK          (7u << RCC_PLL1DIVR2_DIVS_SHIFT)
#define RCC_PLL1DIVR2_DIVS(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL1DIVR2_DIVS_SHIFT)

#define RCC_PLL2DIVR1_DIVN_SHIFT         0
#define RCC_PLL2DIVR1_DIVN_MASK          (0x1ffu << RCC_PLL2DIVR1_DIVN_SHIFT)
#define RCC_PLL2DIVR1_DIVN(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL2DIVR1_DIVN_SHIFT)
#define RCC_PLL2DIVR1_DIVP_SHIFT         9
#define RCC_PLL2DIVR1_DIVP_MASK          (0x7fu << RCC_PLL2DIVR1_DIVP_SHIFT)
#define RCC_PLL2DIVR1_DIVP(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL2DIVR1_DIVP_SHIFT)
#define RCC_PLL2DIVR1_DIVQ_SHIFT         16
#define RCC_PLL2DIVR1_DIVQ_MASK          (0x7fu << RCC_PLL2DIVR1_DIVQ_SHIFT)
#define RCC_PLL2DIVR1_DIVQ(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL2DIVR1_DIVQ_SHIFT)
#define RCC_PLL2DIVR1_DIVR_SHIFT         24
#define RCC_PLL2DIVR1_DIVR_MASK          (0x7fu << RCC_PLL2DIVR1_DIVR_SHIFT)
#define RCC_PLL2DIVR1_DIVR(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL2DIVR1_DIVR_SHIFT)

#define RCC_PLL2DIVR2_DIVS_SHIFT         0
#define RCC_PLL2DIVR2_DIVS_MASK          (7u << RCC_PLL2DIVR2_DIVS_SHIFT)
#define RCC_PLL2DIVR2_DIVS(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL2DIVR2_DIVS_SHIFT)
#define RCC_PLL2DIVR2_DIVT_SHIFT         8
#define RCC_PLL2DIVR2_DIVT_MASK          (7u << RCC_PLL2DIVR2_DIVT_SHIFT)
#define RCC_PLL2DIVR2_DIVT(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL2DIVR2_DIVT_SHIFT)

#define RCC_PLL3DIVR1_DIVN_SHIFT         0
#define RCC_PLL3DIVR1_DIVN_MASK          (0x1ffu << RCC_PLL3DIVR1_DIVN_SHIFT)
#define RCC_PLL3DIVR1_DIVN(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL3DIVR1_DIVN_SHIFT)
#define RCC_PLL3DIVR1_DIVP_SHIFT         9
#define RCC_PLL3DIVR1_DIVP_MASK          (0x7fu << RCC_PLL3DIVR1_DIVP_SHIFT)
#define RCC_PLL3DIVR1_DIVP(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL3DIVR1_DIVP_SHIFT)
#define RCC_PLL3DIVR1_DIVQ_SHIFT         16
#define RCC_PLL3DIVR1_DIVQ_MASK          (0x7fu << RCC_PLL3DIVR1_DIVQ_SHIFT)
#define RCC_PLL3DIVR1_DIVQ(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL3DIVR1_DIVQ_SHIFT)
#define RCC_PLL3DIVR1_DIVR_SHIFT         24
#define RCC_PLL3DIVR1_DIVR_MASK          (0x7fu << RCC_PLL3DIVR1_DIVR_SHIFT)
#define RCC_PLL3DIVR1_DIVR(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL3DIVR1_DIVR_SHIFT)

#define RCC_PLL3DIVR2_DIVS_SHIFT         0
#define RCC_PLL3DIVR2_DIVS_MASK          (7u << RCC_PLL3DIVR2_DIVS_SHIFT)
#define RCC_PLL3DIVR2_DIVS(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL3DIVR2_DIVS_SHIFT)
#define RCC_PLL3DIVR2_DIVT_SHIFT         8
#define RCC_PLL3DIVR2_DIVT_MASK          (7u << RCC_PLL3DIVR2_DIVT_SHIFT)
#define RCC_PLL3DIVR2_DIVT(n)            ((uint32_t)((n) - 1u) << \
                                           RCC_PLL3DIVR2_DIVT_SHIFT)

#define RCC_CCIPR1_XSPI1SEL_SHIFT        4
#define RCC_CCIPR1_XSPI1SEL_MASK         (3u << RCC_CCIPR1_XSPI1SEL_SHIFT)
#define RCC_CCIPR1_XSPI1SEL_HCLK         0u
#define RCC_CCIPR1_XSPI1SEL_PLL2S        (1u << RCC_CCIPR1_XSPI1SEL_SHIFT)
#define RCC_CCIPR1_XSPI2SEL_SHIFT        6
#define RCC_CCIPR1_XSPI2SEL_MASK         (3u << RCC_CCIPR1_XSPI2SEL_SHIFT)
#define RCC_CCIPR1_XSPI2SEL_HCLK         0u
#define RCC_CCIPR1_XSPI2SEL_PLL2S        (1u << RCC_CCIPR1_XSPI2SEL_SHIFT)

#define RCC_CCIPR2_LPTIM1SEL_SHIFT       16
#define RCC_CCIPR2_LPTIM1SEL_MASK        (7u << RCC_CCIPR2_LPTIM1SEL_SHIFT)
#define RCC_CCIPR2_LPTIM1SEL_PCLK        0u

#define RCC_AHB5RSTR_XSPI1RST            (1u << 5)
#define RCC_AHB5RSTR_XSPI2RST            (1u << 12)

#define RCC_APB1RSTR1_LPTIM1RST          (1u << 9)
#define RCC_APB1RSTR1_I2C1RST            (1u << 21)
#define RCC_APB5RSTR_LTDCRST             (1u << 1)

#define RCC_AHB4ENR_GPIOAEN              (1u << 0)
#define RCC_AHB4ENR_GPIOBEN              (1u << 1)
#define RCC_AHB4ENR_GPIOCEN              (1u << 2)
#define RCC_AHB4ENR_GPIODEN              (1u << 3)
#define RCC_AHB4ENR_GPIOEEN              (1u << 4)
#define RCC_AHB4ENR_GPIOFEN              (1u << 5)
#define RCC_AHB4ENR_GPIOGEN              (1u << 6)
#define RCC_AHB4ENR_GPIONEN              (1u << 13)
#define RCC_AHB4ENR_GPIOOEN              (1u << 14)
#define RCC_AHB4ENR_GPIOPEN              (1u << 15)

#define RCC_AHB5ENR_XSPI1EN              (1u << 5)
#define RCC_AHB5ENR_XSPI2EN              (1u << 12)
#define RCC_AHB5ENR_XSPIMEN              (1u << 14)

#define RCC_APB1ENR1_LPTIM1EN            (1u << 9)
#define RCC_APB1ENR1_UART4EN             (1u << 19)
#define RCC_APB1ENR1_I2C1EN              (1u << 21)
#define RCC_APB4ENR_SBSEN                (1u << 1)
#define RCC_APB5ENR_LTDCEN               (1u << 1)

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_RCC_H */
