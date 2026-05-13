/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32h7rs_xspi.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_XSPI_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_XSPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32h7rs_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32H7RS_XSPI_CR(b)             ((b) + 0x0000u)
#define STM32H7RS_XSPI_DCR1(b)           ((b) + 0x0008u)
#define STM32H7RS_XSPI_DCR2(b)           ((b) + 0x000cu)
#define STM32H7RS_XSPI_DCR3(b)           ((b) + 0x0010u)
#define STM32H7RS_XSPI_DCR4(b)           ((b) + 0x0014u)
#define STM32H7RS_XSPI_SR(b)             ((b) + 0x0020u)
#define STM32H7RS_XSPI_FCR(b)            ((b) + 0x0024u)
#define STM32H7RS_XSPI_DLR(b)            ((b) + 0x0040u)
#define STM32H7RS_XSPI_AR(b)             ((b) + 0x0048u)
#define STM32H7RS_XSPI_DR(b)             ((b) + 0x0050u)
#define STM32H7RS_XSPI_CCR(b)            ((b) + 0x0100u)
#define STM32H7RS_XSPI_TCR(b)            ((b) + 0x0108u)
#define STM32H7RS_XSPI_IR(b)             ((b) + 0x0110u)
#define STM32H7RS_XSPI_WCCR(b)           ((b) + 0x0180u)
#define STM32H7RS_XSPI_WTCR(b)           ((b) + 0x0188u)
#define STM32H7RS_XSPI_WIR(b)            ((b) + 0x0190u)

#define STM32H7RS_XSPI2_CR               STM32H7RS_XSPI_CR(STM32H7RS_XSPI2_BASE)

#define STM32H7RS_XSPIM_CR               (STM32H7RS_XSPIM_BASE + 0x0000u)

#define XSPI_CR_EN                       (1u << 0)
#define XSPI_CR_ABORT                    (1u << 1)
#define XSPI_CR_FTHRES_SHIFT             8
#define XSPI_CR_FTHRES_MASK              (0x1fu << XSPI_CR_FTHRES_SHIFT)
#define XSPI_CR_CSSEL                    (1u << 24)
#define XSPI_CR_FMODE_SHIFT              28
#define XSPI_CR_FMODE_MASK               (3u << XSPI_CR_FMODE_SHIFT)
#define XSPI_CR_FMODE_INDIRECT_WRITE     (0u << XSPI_CR_FMODE_SHIFT)
#define XSPI_CR_FMODE_INDIRECT_READ      (1u << XSPI_CR_FMODE_SHIFT)
#define XSPI_CR_FMODE_MEMORY_MAPPED      (3u << XSPI_CR_FMODE_SHIFT)

#define XSPI_DCR1_CSHT_SHIFT             8
#define XSPI_DCR1_CSHT(n)                ((uint32_t)((n) - 1u) << \
                                           XSPI_DCR1_CSHT_SHIFT)
#define XSPI_DCR1_DEVSIZE_SHIFT          16
#define XSPI_DCR1_DEVSIZE(n)             ((uint32_t)(n) << \
                                           XSPI_DCR1_DEVSIZE_SHIFT)
#define XSPI_DCR1_MTYP_MACRONIX          (1u << 24)
#define XSPI_DCR1_MTYP_APMEM_16BIT       ((4u | 2u) << 24)

#define XSPI_DCR2_PRESCALER_MASK         0xffu
#define XSPI_DCR2_PRESCALER(n)           ((uint32_t)(n))

#define XSPI_DCR3_CSBOUND_SHIFT          16
#define XSPI_DCR3_CSBOUND(n)             ((uint32_t)(n) << \
                                           XSPI_DCR3_CSBOUND_SHIFT)
#define XSPI_DCR3_MAXTRAN_SHIFT          0
#define XSPI_DCR3_MAXTRAN(n)             ((uint32_t)(n) << \
                                           XSPI_DCR3_MAXTRAN_SHIFT)

#define XSPI_DCR4_REFRESH_SHIFT          0
#define XSPI_DCR4_REFRESH(n)             ((uint32_t)(n) << \
                                           XSPI_DCR4_REFRESH_SHIFT)

#define XSPI_SR_TEF                      (1u << 0)
#define XSPI_SR_TCF                      (1u << 1)
#define XSPI_SR_FTF                      (1u << 2)
#define XSPI_SR_BUSY                     (1u << 5)

#define XSPI_FCR_CTEF                    (1u << 0)
#define XSPI_FCR_CTCF                    (1u << 1)

#define XSPI_CCR_IMODE_1_LINE            (1u << 0)
#define XSPI_CCR_IMODE_8_LINES           (4u << 0)
#define XSPI_CCR_IDTR                    (1u << 3)
#define XSPI_CCR_ISIZE_16                (1u << 4)
#define XSPI_CCR_ADMODE_1_LINE           (1u << 8)
#define XSPI_CCR_ADMODE_8_LINES          (4u << 8)
#define XSPI_CCR_ADDTR                   (1u << 11)
#define XSPI_CCR_ADSIZE_24               (2u << 12)
#define XSPI_CCR_ADSIZE_32               (3u << 12)
#define XSPI_CCR_DMODE_1_LINE            (1u << 24)
#define XSPI_CCR_DMODE_8_LINES           (4u << 24)
#define XSPI_CCR_DMODE_16_LINES          ((1u | 4u) << 24)
#define XSPI_CCR_DDTR                    (1u << 27)
#define XSPI_CCR_DQSE                    (1u << 29)

#define XSPI_TCR_DCYC(n)                 ((uint32_t)(n) & 0x1fu)

#define XSPIM_CR_CSSEL_OVR_EN            (1u << 4)
#define XSPIM_CR_CSSEL_OVR_O1            (1u << 5)
#define XSPIM_CR_CSSEL_OVR_O2            (1u << 6)
#define XSPIM_CR_MODE                    (1u << 1)
#define XSPIM_CR_REQ2ACK_TIME_SHIFT      16
#define XSPIM_CR_REQ2ACK_TIME(n)         ((uint32_t)((n) - 1u) << \
                                           XSPIM_CR_REQ2ACK_TIME_SHIFT)

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_XSPI_H */
