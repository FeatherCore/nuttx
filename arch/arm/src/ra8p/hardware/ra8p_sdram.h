/****************************************************************************
 * arch/arm/src/ra8p/hardware/ra8p_sdram.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_SDRAM_H
#define __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_SDRAM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "hardware/ra8p_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RA8P_SDRAMC_BASE        (RA8P_BUS_BASE + 0x0c00)

#define RA8P_SDRAM_SDCCR        (RA8P_SDRAMC_BASE + 0x0000)
#define RA8P_SDRAM_SDCMOD       (RA8P_SDRAMC_BASE + 0x0001)
#define RA8P_SDRAM_SDAMOD       (RA8P_SDRAMC_BASE + 0x0002)
#define RA8P_SDRAM_SDSELF       (RA8P_SDRAMC_BASE + 0x0010)
#define RA8P_SDRAM_SDRFCR       (RA8P_SDRAMC_BASE + 0x0014)
#define RA8P_SDRAM_SDRFEN       (RA8P_SDRAMC_BASE + 0x0016)
#define RA8P_SDRAM_SDICR        (RA8P_SDRAMC_BASE + 0x0020)
#define RA8P_SDRAM_SDIR         (RA8P_SDRAMC_BASE + 0x0024)
#define RA8P_SDRAM_SDADR        (RA8P_SDRAMC_BASE + 0x0040)
#define RA8P_SDRAM_SDTR         (RA8P_SDRAMC_BASE + 0x0044)
#define RA8P_SDRAM_SDMOD        (RA8P_SDRAMC_BASE + 0x0048)
#define RA8P_SDRAM_SDSR         (RA8P_SDRAMC_BASE + 0x0050)

#define SDRAM_SDCCR_EXENB       (1 << 0)
#define SDRAM_SDCCR_BSIZE_SHIFT 4

#define SDRAM_SDRFCR_RFC_SHIFT  0
#define SDRAM_SDRFCR_REFW_SHIFT 12

#define SDRAM_SDRFEN_RFEN       (1 << 0)

#define SDRAM_SDICR_INIRQ       (1 << 0)

#define SDRAM_SDIR_ARFI_SHIFT   0
#define SDRAM_SDIR_ARFC_SHIFT   4
#define SDRAM_SDIR_PRC_SHIFT    8

#define SDRAM_SDTR_CL_SHIFT     0
#define SDRAM_SDTR_WR_SHIFT     8
#define SDRAM_SDTR_RP_SHIFT     9
#define SDRAM_SDTR_RCD_SHIFT    12
#define SDRAM_SDTR_RAS_SHIFT    16

#define SDRAM_SDSR_MRSST        (1 << 0)
#define SDRAM_SDSR_INIST        (1 << 3)
#define SDRAM_SDSR_SRFST        (1 << 4)

#endif /* __ARCH_ARM_SRC_RA8P_HARDWARE_RA8P_SDRAM_H */
