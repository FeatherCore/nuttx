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
#define STM32_DSI_CLTCR        (STM32_DSI_BASE + 0x0098)
#define STM32_DSI_DLTCR        (STM32_DSI_BASE + 0x009c)
#define STM32_DSI_PCTLR        (STM32_DSI_BASE + 0x00a0)
#define STM32_DSI_PCONFR       (STM32_DSI_BASE + 0x00a4)
#define STM32_DSI_WCFGR        (STM32_DSI_BASE + 0x0400)
#define STM32_DSI_WCR          (STM32_DSI_BASE + 0x0404)
#define STM32_DSI_WISR         (STM32_DSI_BASE + 0x040c)
#define STM32_DSI_WRPCR        (STM32_DSI_BASE + 0x0430)

#define DSI_CR_EN              (1 << 0)
#define DSI_CCR_TXECKDIV(n)    (((n) & 0xff) << 0)

#define DSI_LCOLCR_RGB888      5

#define DSI_MCR_CMDM           (1 << 0)
#define DSI_VMCR_BURST         0
#define DSI_VMCR_LPVSAE        (1 << 8)
#define DSI_VMCR_LPVBPE        (1 << 9)
#define DSI_VMCR_LPVFPE        (1 << 10)
#define DSI_VMCR_LPVAE         (1 << 11)
#define DSI_VMCR_LPHBPE        (1 << 12)
#define DSI_VMCR_LPHFPE        (1 << 13)
#define DSI_VMCR_LPCE          (1 << 15)
#define DSI_VMCR_FBTAAE        (1 << 16)

#define DSI_PCONFR_NL(n)       (((n) - 1) & 3)

#define DSI_GPSR_CMDFE         (1 << 0)
#define DSI_GPSR_CMDFF         (1 << 1)
#define DSI_GPSR_PWRFE         (1 << 2)
#define DSI_GPSR_PWRFF         (1 << 3)

#define DSI_GHCR_DT(n)         (((n) & 0x3f) << 0)
#define DSI_GHCR_VCID(n)       (((n) & 3) << 6)
#define DSI_GHCR_WC(n)         (((n) & 0xffff) << 8)

#define DSI_WCFGR_DSIM         (1 << 0)
#define DSI_WCFGR_COLMUX(n)    (((n) & 7) << 1)

#define DSI_WCR_LTDCEN         (1 << 2)
#define DSI_WCR_DSIEN          (1 << 3)

#define DSI_WISR_PLLLS         (1 << 8)

#define DSI_WRPCR_PLLEN        (1 << 0)
#define DSI_WRPCR_NDIV(n)      (((n) & 0x1ff) << 2)
#define DSI_WRPCR_IDF(n)       (((n) & 0x1ff) << 11)
#define DSI_WRPCR_ODF(n)       (((n) & 0x1ff) << 20)

#endif /* __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_DSI_H */
