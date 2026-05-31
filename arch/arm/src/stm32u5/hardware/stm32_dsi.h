/****************************************************************************
 * arch/arm/src/stm32u5/hardware/stm32_dsi.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DSI_H
#define __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DSI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_DSI_CR           (STM32_DSI_BASE + 0x0004)
#define STM32_DSI_CCR          (STM32_DSI_BASE + 0x0008)
#define STM32_DSI_LVCIDR       (STM32_DSI_BASE + 0x000c)
#define STM32_DSI_LCOLCR       (STM32_DSI_BASE + 0x0010)
#define STM32_DSI_LPCR         (STM32_DSI_BASE + 0x0014)
#define STM32_DSI_LPMCR        (STM32_DSI_BASE + 0x0018)
#define STM32_DSI_PCR          (STM32_DSI_BASE + 0x002c)
#define STM32_DSI_GVCIDR       (STM32_DSI_BASE + 0x0030)
#define STM32_DSI_MCR          (STM32_DSI_BASE + 0x0034)
#define STM32_DSI_VMCR         (STM32_DSI_BASE + 0x0038)
#define STM32_DSI_VPCR         (STM32_DSI_BASE + 0x003c)
#define STM32_DSI_VCCR         (STM32_DSI_BASE + 0x0040)
#define STM32_DSI_VNPCR        (STM32_DSI_BASE + 0x0044)
#define STM32_DSI_VHSACR       (STM32_DSI_BASE + 0x0048)
#define STM32_DSI_VHBPCR       (STM32_DSI_BASE + 0x004c)
#define STM32_DSI_VLCR         (STM32_DSI_BASE + 0x0050)
#define STM32_DSI_VVSACR       (STM32_DSI_BASE + 0x0054)
#define STM32_DSI_VVBPCR       (STM32_DSI_BASE + 0x0058)
#define STM32_DSI_VVFPCR       (STM32_DSI_BASE + 0x005c)
#define STM32_DSI_VVACR        (STM32_DSI_BASE + 0x0060)
#define STM32_DSI_CMCR         (STM32_DSI_BASE + 0x0068)
#define STM32_DSI_GHCR         (STM32_DSI_BASE + 0x006c)
#define STM32_DSI_GPDR         (STM32_DSI_BASE + 0x0070)
#define STM32_DSI_GPSR         (STM32_DSI_BASE + 0x0074)
#define STM32_DSI_TCCR(n)      (STM32_DSI_BASE + 0x0078 + ((n) * 4))
#define STM32_DSI_CLCR         (STM32_DSI_BASE + 0x0094)
#define STM32_DSI_CLTCR        (STM32_DSI_BASE + 0x0098)
#define STM32_DSI_DLTCR        (STM32_DSI_BASE + 0x009c)
#define STM32_DSI_PCTLR        (STM32_DSI_BASE + 0x00a0)
#define STM32_DSI_PCONFR       (STM32_DSI_BASE + 0x00a4)
#define STM32_DSI_PSR          (STM32_DSI_BASE + 0x00b0)
#define STM32_DSI_ISR(n)       (STM32_DSI_BASE + 0x00bc + ((n) * 4))
#define STM32_DSI_IER(n)       (STM32_DSI_BASE + 0x00c4 + ((n) * 4))
#define STM32_DSI_DLTRCR       (STM32_DSI_BASE + 0x00f4)
#define STM32_DSI_FBSR         (STM32_DSI_BASE + 0x0168)
#define STM32_DSI_WCFGR        (STM32_DSI_BASE + 0x0400)
#define STM32_DSI_WCR          (STM32_DSI_BASE + 0x0404)
#define STM32_DSI_WIER         (STM32_DSI_BASE + 0x0408)
#define STM32_DSI_WISR         (STM32_DSI_BASE + 0x040c)
#define STM32_DSI_WIFCR        (STM32_DSI_BASE + 0x0410)
#define STM32_DSI_WPCR0        (STM32_DSI_BASE + 0x0418)
#define STM32_DSI_WRPCR        (STM32_DSI_BASE + 0x0430)
#define STM32_DSI_WPTR         (STM32_DSI_BASE + 0x0434)
#define STM32_DSI_BCFGR        (STM32_DSI_BASE + 0x0808)
#define STM32_DSI_DPCBCR       (STM32_DSI_BASE + 0x0c04)
#define STM32_DSI_DPCSRCR      (STM32_DSI_BASE + 0x0c34)
#define STM32_DSI_DPDL0HSOCR   (STM32_DSI_BASE + 0x0c5c)
#define STM32_DSI_DPDL0LPXOCR  (STM32_DSI_BASE + 0x0c60)
#define STM32_DSI_DPDL0BCR     (STM32_DSI_BASE + 0x0c70)
#define STM32_DSI_DPDL0SRCR    (STM32_DSI_BASE + 0x0ca0)
#define STM32_DSI_DPDL1HSOCR   (STM32_DSI_BASE + 0x0cf4)
#define STM32_DSI_DPDL1LPXOCR  (STM32_DSI_BASE + 0x0cf8)
#define STM32_DSI_DPDL1BCR     (STM32_DSI_BASE + 0x0d08)
#define STM32_DSI_DPDL1SRCR    (STM32_DSI_BASE + 0x0d38)

#define DSI_CR_EN              (1 << 0)
#define DSI_CCR_TXECKDIV(n)    (((n) & 0xff) << 0)
#define DSI_CCR_TOCKDIV(n)     (((n) & 0xff) << 8)

#define DSI_LCOLCR_RGB565      0
#define DSI_LCOLCR_RGB666      3
#define DSI_LCOLCR_RGB888      5

#define DSI_PCR_BTAE           (1 << 2)

#define DSI_CMCR_DSW0TX        (1 << 16)
#define DSI_CMCR_DSW1TX        (1 << 17)
#define DSI_CMCR_DLWTX         (1 << 19)

#define DSI_MCR_CMDM           (1 << 0)
#define DSI_VMCR_VMT_MASK      3
#define DSI_VMCR_SYNC_PULSES   0
#define DSI_VMCR_SYNC_EVENTS   1
#define DSI_VMCR_BURST         2
#define DSI_VMCR_LPVSAE        (1 << 8)
#define DSI_VMCR_LPVBPE        (1 << 9)
#define DSI_VMCR_LPVFPE        (1 << 10)
#define DSI_VMCR_LPVAE         (1 << 11)
#define DSI_VMCR_LPHBPE        (1 << 12)
#define DSI_VMCR_LPHFPE        (1 << 13)
#define DSI_VMCR_LPCE          (1 << 15)
#define DSI_VMCR_FBTAAE        (1 << 14)

#define DSI_PCONFR_NL(n)       (((n) - 1) & 3)
#define DSI_PCONFR_SW_TIME(n)  (((n) & 0xff) << 8)

#define DSI_CLCR_DPCC          (1 << 0)

#define DSI_PCTLR_DEN          (1 << 1)
#define DSI_PCTLR_CKE          (1 << 2)

#define DSI_PSR_PSSC           (1 << 2)
#define DSI_PSR_PSS0           (1 << 4)
#define DSI_PSR_PSS1           (1 << 7)

#define DSI_GPSR_CMDFE         (1 << 0)
#define DSI_GPSR_CMDFF         (1 << 1)
#define DSI_GPSR_PWRFE         (1 << 2)
#define DSI_GPSR_PWRFF         (1 << 3)
#define DSI_GPSR_PRDFE         (1 << 4)
#define DSI_GPSR_RCB           (1 << 6)

#define DSI_GHCR_DT(n)         (((n) & 0x3f) << 0)
#define DSI_GHCR_VCID(n)       (((n) & 3) << 6)
#define DSI_GHCR_WC(n)         (((n) & 0xffff) << 8)

#define DSI_ISR1_PSE           (1 << 5)

#define DSI_WCFGR_DSIM         (1 << 0)
#define DSI_WCFGR_COLMUX(n)    (((n) & 7) << 1)

#define DSI_WCR_LTDCEN         (1 << 2)
#define DSI_WCR_DSIEN          (1 << 3)

#define DSI_WISR_TEIF          (1 << 0)
#define DSI_WISR_ERIF          (1 << 1)
#define DSI_WISR_BUSY          (1 << 2)
#define DSI_WISR_PLLLS         (1 << 8)
#define DSI_WISR_PLLLIF        (1 << 9)
#define DSI_WISR_PLLUIF        (1 << 10)

#define DSI_WIFCR_CTEIF        (1 << 0)
#define DSI_WIFCR_CERIF        (1 << 1)
#define DSI_WIFCR_CPLLLIF      (1 << 9)
#define DSI_WIFCR_CPLLUIF      (1 << 10)

#define DSI_WPCR0_SWCL         (1 << 6)
#define DSI_WPCR0_SWDL0        (1 << 7)
#define DSI_WPCR0_SWDL1        (1 << 8)
#define DSI_WPCR0_SWAP_MASK    (DSI_WPCR0_SWCL | DSI_WPCR0_SWDL0 | \
                                DSI_WPCR0_SWDL1)

#define DSI_BCFGR_PWRUP        (1 << 6)

#define DSI_DPHY_FRANGE_80MHZ_100MHZ  0
#define DSI_DPHY_FRANGE_100MHZ_120MHZ 1
#define DSI_DPHY_FRANGE_120MHZ_160MHZ 2
#define DSI_DPHY_FRANGE_160MHZ_200MHZ 3
#define DSI_DPHY_FRANGE_200MHZ_240MHZ 4
#define DSI_DPHY_FRANGE_240MHZ_320MHZ 5
#define DSI_DPHY_FRANGE_320MHZ_390MHZ 6
#define DSI_DPHY_FRANGE_390MHZ_450MHZ 7
#define DSI_DPHY_FRANGE_450MHZ_510MHZ 8
#define DSI_DPHY_SLEW_HS_TX_SPEED     0x0e
#define DSI_HS_PREPARE_OFFSET2        2

#define DSI_DPCBCR_FRANGE(n)   (((n) & 0x1f) << 3)
#define DSI_DPCSRCR_SLEW(n)    (((n) & 0xff) << 0)
#define DSI_DPDL0HSOCR_HS(n)   (((n) & 0x0f) << 4)
#define DSI_DPDL1HSOCR_HS(n)   (((n) & 0x0f) << 4)
#define DSI_DPDL_LPXO(n)       (((n) & 0x0f) << 0)
#define DSI_DPDL_BCR(n)        (((n) & 0x1f) << 0)
#define DSI_DPDL_SRCR(n)       (((n) & 0xff) << 0)

#define DSI_WRPCR_PLLEN        (1 << 0)
#define DSI_WRPCR_NDIV(n)      (((n) & 0x1ff) << 2)
#define DSI_WRPCR_IDF(n)       (((n) & 0x1ff) << 11)
#define DSI_WRPCR_ODF(n)       (((n) & 0x1ff) << 20)
#define DSI_WRPCR_BC           (1 << 29)
#define DSI_WRPCR_VCO_800_1GHZ DSI_WRPCR_BC

#endif /* __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DSI_H */
