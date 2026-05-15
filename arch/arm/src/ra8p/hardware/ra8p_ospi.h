/****************************************************************************
 * arch/arm/src/ra8p/hardware/ra8p_ospi.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_OSPI_H
#define __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_OSPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ra8p_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RA8P_XSPI_WRAPCFG             (RA8P_XSPI0_BASE + 0x0000)
#define RA8P_XSPI_COMCFG              (RA8P_XSPI0_BASE + 0x0004)
#define RA8P_XSPI_BMCFGCH(n)          (RA8P_XSPI0_BASE + 0x0008 + ((n) * 4))
#define RA8P_XSPI_CMCFGCS(n)          (RA8P_XSPI0_BASE + 0x0010 + ((n) * 16))
#define RA8P_XSPI_CMCFG0(n)           (RA8P_XSPI_CMCFGCS(n) + 0x0000)
#define RA8P_XSPI_CMCFG1(n)           (RA8P_XSPI_CMCFGCS(n) + 0x0004)
#define RA8P_XSPI_CMCFG2(n)           (RA8P_XSPI_CMCFGCS(n) + 0x0008)
#define RA8P_XSPI_LIOCFGCS(n)         (RA8P_XSPI0_BASE + 0x0050 + ((n) * 4))
#define RA8P_XSPI_BMCTL0              (RA8P_XSPI0_BASE + 0x0060)
#define RA8P_XSPI_BMCTL1              (RA8P_XSPI0_BASE + 0x0064)
#define RA8P_XSPI_CDCTL0              (RA8P_XSPI0_BASE + 0x0070)
#define RA8P_XSPI_CDBUF(n)            (RA8P_XSPI0_BASE + 0x0080 + ((n) * 16))
#define RA8P_XSPI_CDBUF_CDT(n)        (RA8P_XSPI_CDBUF(n) + 0x0000)
#define RA8P_XSPI_CDBUF_CDA(n)        (RA8P_XSPI_CDBUF(n) + 0x0004)
#define RA8P_XSPI_CDBUF_CDD0(n)       (RA8P_XSPI_CDBUF(n) + 0x0008)
#define RA8P_XSPI_CDBUF_CDD1(n)       (RA8P_XSPI_CDBUF(n) + 0x000c)
#define RA8P_XSPI_LIOCTL              (RA8P_XSPI0_BASE + 0x0108)
#define RA8P_XSPI_COMSTT              (RA8P_XSPI0_BASE + 0x0184)
#define RA8P_XSPI_INTS                (RA8P_XSPI0_BASE + 0x0190)
#define RA8P_XSPI_INTC                (RA8P_XSPI0_BASE + 0x0194)

#define XSPI_WRAPCFG_DSSFTCS1_SHIFT   24
#define XSPI_WRAPCFG_DSSFTCS1_MASK    (0x1fu << XSPI_WRAPCFG_DSSFTCS1_SHIFT)

#define XSPI_BMCFGCH_MWRCOMB          (1u << 7)
#define XSPI_BMCFGCH_MWRSIZE_SHIFT    8
#define XSPI_BMCFGCH_PREEN            (1u << 16)

#define XSPI_CMCFG0_FFMT_SHIFT        0
#define XSPI_CMCFG0_ADDSIZE_SHIFT     2
#define XSPI_CMCFG0_ADDRPEN_SHIFT     16
#define XSPI_CMCFG0_ADDRPEN_MASK      (0xffu << XSPI_CMCFG0_ADDRPEN_SHIFT)

#define XSPI_CMCFG1_RDLATE_SHIFT      16
#define XSPI_CMCFG2_WRLATE_SHIFT      16

#define XSPI_LIOCFGCS_PRTMD_MASK      0x000003ffu
#define XSPI_LIOCFGCS_LATEMD          (1u << 10)
#define XSPI_LIOCFGCS_CSMIN_SHIFT     16
#define XSPI_LIOCFGCS_CSASTEX         (1u << 20)
#define XSPI_LIOCFGCS_CSNEGEX         (1u << 21)
#define XSPI_LIOCFGCS_SDRDRV          (1u << 22)
#define XSPI_LIOCFGCS_SDRSMPMD        (1u << 23)
#define XSPI_LIOCFGCS_SDRSMPSFT_SHIFT 24
#define XSPI_LIOCFGCS_DDRSMPEX_SHIFT  28

#define XSPI_BMCTL0_CH0CS1ACC_MASK    0x0000000cu
#define XSPI_BMCTL0_CH1CS1ACC_MASK    0x000000c0u
#define XSPI_BMCTL0_CS1ACC_MASK       (XSPI_BMCTL0_CH0CS1ACC_MASK | \
                                       XSPI_BMCTL0_CH1CS1ACC_MASK)

#define XSPI_CDCTL0_TRREQ             (1u << 0)
#define XSPI_CDCTL0_CSSEL             (1u << 3)

#define XSPI_CDBUF_CDT_CMDSIZE_SHIFT  0
#define XSPI_CDBUF_CDT_ADDSIZE_SHIFT  2
#define XSPI_CDBUF_CDT_DATASIZE_SHIFT 5
#define XSPI_CDBUF_CDT_LATE_SHIFT     9
#define XSPI_CDBUF_CDT_TRTYPE         (1u << 15)
#define XSPI_CDBUF_CDT_CMD_SHIFT      16
#define XSPI_CDBUF_CDT_CMD_1B_SHIFT   24

#define XSPI_LIOCTL_RSTCS0            (1u << 16)

#define XSPI_INTS_CMDCMP              (1u << 0)

#endif /* __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_OSPI_H */
