/****************************************************************************
 * arch/arm/src/stm32u5/hardware/stm32_xspi.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_XSPI_H
#define __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_XSPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register Offsets *********************************************************/

#define STM32_XSPI_CR_OFFSET                0x0000
#define STM32_XSPI_DCR1_OFFSET              0x0008
#define STM32_XSPI_DCR2_OFFSET              0x000c
#define STM32_XSPI_DCR3_OFFSET              0x0010
#define STM32_XSPI_DCR4_OFFSET              0x0014
#define STM32_XSPI_SR_OFFSET                0x0020
#define STM32_XSPI_FCR_OFFSET               0x0024
#define STM32_XSPI_DLR_OFFSET               0x0040
#define STM32_XSPI_AR_OFFSET                0x0048
#define STM32_XSPI_DR_OFFSET                0x0050
#define STM32_XSPI_CCR_OFFSET               0x0100
#define STM32_XSPI_TCR_OFFSET               0x0108
#define STM32_XSPI_IR_OFFSET                0x0110
#define STM32_XSPI_WCCR_OFFSET              0x0180
#define STM32_XSPI_WTCR_OFFSET              0x0188
#define STM32_XSPI_WIR_OFFSET               0x0190

#define STM32_XSPIM_CR_OFFSET               0x0000

/* Register Addresses *******************************************************/

#define STM32_XSPI_CR(b)                    ((b) + STM32_XSPI_CR_OFFSET)
#define STM32_XSPI_DCR1(b)                  ((b) + STM32_XSPI_DCR1_OFFSET)
#define STM32_XSPI_DCR2(b)                  ((b) + STM32_XSPI_DCR2_OFFSET)
#define STM32_XSPI_DCR3(b)                  ((b) + STM32_XSPI_DCR3_OFFSET)
#define STM32_XSPI_DCR4(b)                  ((b) + STM32_XSPI_DCR4_OFFSET)
#define STM32_XSPI_SR(b)                    ((b) + STM32_XSPI_SR_OFFSET)
#define STM32_XSPI_FCR(b)                   ((b) + STM32_XSPI_FCR_OFFSET)
#define STM32_XSPI_DLR(b)                   ((b) + STM32_XSPI_DLR_OFFSET)
#define STM32_XSPI_AR(b)                    ((b) + STM32_XSPI_AR_OFFSET)
#define STM32_XSPI_DR(b)                    ((b) + STM32_XSPI_DR_OFFSET)
#define STM32_XSPI_CCR(b)                   ((b) + STM32_XSPI_CCR_OFFSET)
#define STM32_XSPI_TCR(b)                   ((b) + STM32_XSPI_TCR_OFFSET)
#define STM32_XSPI_IR(b)                    ((b) + STM32_XSPI_IR_OFFSET)
#define STM32_XSPI_WCCR(b)                  ((b) + STM32_XSPI_WCCR_OFFSET)
#define STM32_XSPI_WTCR(b)                  ((b) + STM32_XSPI_WTCR_OFFSET)
#define STM32_XSPI_WIR(b)                   ((b) + STM32_XSPI_WIR_OFFSET)

#define STM32_XSPIM_CR                      (STM32_OCTOSPIM_BASE + \
                                             STM32_XSPIM_CR_OFFSET)

/* Register Bitfield Definitions ********************************************/

#define XSPI_CR_EN                          (1 << 0)
#define XSPI_CR_ABORT                       (1 << 1)
#define XSPI_CR_FTHRES_SHIFT                8
#define XSPI_CR_FMODE_SHIFT                 28
#define XSPI_CR_FMODE_MASK                  (3 << XSPI_CR_FMODE_SHIFT)
#define XSPI_CR_FMODE_INDIRECT_READ         (1 << XSPI_CR_FMODE_SHIFT)
#define XSPI_CR_FMODE_MEMORY_MAPPED         (3 << XSPI_CR_FMODE_SHIFT)

#define XSPI_DCR1_CSHT_SHIFT                8
#define XSPI_DCR1_CSHT(n)                   ((uint32_t)((n) - 1) << \
                                             XSPI_DCR1_CSHT_SHIFT)
#define XSPI_DCR1_DEVSIZE_SHIFT             16
#define XSPI_DCR1_DEVSIZE(n)                ((uint32_t)(n) << \
                                             XSPI_DCR1_DEVSIZE_SHIFT)
#define XSPI_DCR1_MTYP_MICRON              0
#define XSPI_DCR1_MTYP_MACRONIX             (1 << 24)
#define XSPI_DCR1_MTYP_APMEM_16BIT          ((4 | 2) << 24)

#define XSPI_DCR2_PRESCALER_MASK            0xff
#define XSPI_DCR2_PRESCALER(n)              ((uint32_t)(n))

#define XSPI_DCR3_MAXTRAN_SHIFT             0
#define XSPI_DCR3_MAXTRAN(n)                ((uint32_t)(n) << \
                                             XSPI_DCR3_MAXTRAN_SHIFT)
#define XSPI_DCR3_CSBOUND_SHIFT             16
#define XSPI_DCR3_CSBOUND(n)                ((uint32_t)(n) << \
                                             XSPI_DCR3_CSBOUND_SHIFT)

#define XSPI_DCR4_REFRESH_SHIFT             0
#define XSPI_DCR4_REFRESH(n)                ((uint32_t)(n) << \
                                             XSPI_DCR4_REFRESH_SHIFT)

#define XSPI_SR_TEF                         (1 << 0)
#define XSPI_SR_TCF                         (1 << 1)
#define XSPI_SR_FTF                         (1 << 2)
#define XSPI_SR_BUSY                        (1 << 5)

#define XSPI_FCR_CTEF                       (1 << 0)
#define XSPI_FCR_CTCF                       (1 << 1)

#define XSPI_CCR_IMODE_1_LINE               (1 << 0)
#define XSPI_CCR_IMODE_8_LINES              (4 << 0)
#define XSPI_CCR_IDTR                       (1 << 3)
#define XSPI_CCR_ISIZE_16                   (1 << 4)
#define XSPI_CCR_ADMODE_1_LINE              (1 << 8)
#define XSPI_CCR_ADMODE_8_LINES             (4 << 8)
#define XSPI_CCR_ADDTR                      (1 << 11)
#define XSPI_CCR_ADSIZE_24                  (2 << 12)
#define XSPI_CCR_ADSIZE_32                  (3 << 12)
#define XSPI_CCR_DMODE_1_LINE               (1 << 24)
#define XSPI_CCR_DMODE_8_LINES              (4 << 24)
#define XSPI_CCR_DMODE_16_LINES             ((1 | 4) << 24)
#define XSPI_CCR_DDTR                       (1 << 27)
#define XSPI_CCR_DQSE                       (1 << 29)

#define XSPI_TCR_DCYC(n)                    ((uint32_t)(n) & 0x1f)

#define XSPIM_CR_REQ2ACK_TIME_SHIFT         16
#define XSPIM_CR_REQ2ACK_TIME(n)            ((uint32_t)((n) - 1) << \
                                             XSPIM_CR_REQ2ACK_TIME_SHIFT)

#endif /* __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_XSPI_H */
