/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32n6_i2c.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_I2C_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_I2C_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32n6_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_I2C_CR1_OFFSET       0x0000u
#define STM32N6_I2C_CR2_OFFSET       0x0004u
#define STM32N6_I2C_OAR1_OFFSET      0x0008u
#define STM32N6_I2C_OAR2_OFFSET      0x000cu
#define STM32N6_I2C_TIMINGR_OFFSET   0x0010u
#define STM32N6_I2C_TIMEOUTR_OFFSET  0x0014u
#define STM32N6_I2C_ISR_OFFSET       0x0018u
#define STM32N6_I2C_ICR_OFFSET       0x001cu
#define STM32N6_I2C_PECR_OFFSET      0x0020u
#define STM32N6_I2C_RXDR_OFFSET      0x0024u
#define STM32N6_I2C_TXDR_OFFSET      0x0028u

#define STM32N6_I2C_CR1(b)           ((b) + STM32N6_I2C_CR1_OFFSET)
#define STM32N6_I2C_CR2(b)           ((b) + STM32N6_I2C_CR2_OFFSET)
#define STM32N6_I2C_OAR1(b)          ((b) + STM32N6_I2C_OAR1_OFFSET)
#define STM32N6_I2C_OAR2(b)          ((b) + STM32N6_I2C_OAR2_OFFSET)
#define STM32N6_I2C_TIMINGR(b)       ((b) + STM32N6_I2C_TIMINGR_OFFSET)
#define STM32N6_I2C_TIMEOUTR(b)      ((b) + STM32N6_I2C_TIMEOUTR_OFFSET)
#define STM32N6_I2C_ISR(b)           ((b) + STM32N6_I2C_ISR_OFFSET)
#define STM32N6_I2C_ICR(b)           ((b) + STM32N6_I2C_ICR_OFFSET)
#define STM32N6_I2C_PECR(b)          ((b) + STM32N6_I2C_PECR_OFFSET)
#define STM32N6_I2C_RXDR(b)          ((b) + STM32N6_I2C_RXDR_OFFSET)
#define STM32N6_I2C_TXDR(b)          ((b) + STM32N6_I2C_TXDR_OFFSET)

#define I2C_CR1_PE                   (1u << 0)
#define I2C_CR1_TXIE                 (1u << 1)
#define I2C_CR1_RXIE                 (1u << 2)
#define I2C_CR1_NACKIE               (1u << 4)
#define I2C_CR1_STOPIE               (1u << 5)
#define I2C_CR1_TCIE                 (1u << 6)
#define I2C_CR1_ERRIE                (1u << 7)
#define I2C_CR1_DNF_SHIFT            8
#define I2C_CR1_DNF_MASK             (15u << I2C_CR1_DNF_SHIFT)
#define I2C_CR1_DNF(n)               ((uint32_t)(n) << I2C_CR1_DNF_SHIFT)
#define I2C_CR1_ANFOFF               (1u << 12)

#define I2C_CR2_SADD7_SHIFT          1
#define I2C_CR2_SADD7_MASK           (0x7fu << I2C_CR2_SADD7_SHIFT)
#define I2C_CR2_RD_WRN               (1u << 10)
#define I2C_CR2_ADD10                (1u << 11)
#define I2C_CR2_START                (1u << 13)
#define I2C_CR2_STOP                 (1u << 14)
#define I2C_CR2_NBYTES_SHIFT         16
#define I2C_CR2_NBYTES_MASK          (0xffu << I2C_CR2_NBYTES_SHIFT)
#define I2C_CR2_NBYTES(n)            ((uint32_t)(n) << I2C_CR2_NBYTES_SHIFT)
#define I2C_CR2_RELOAD               (1u << 24)
#define I2C_CR2_AUTOEND              (1u << 25)

#define I2C_ISR_TXE                  (1u << 0)
#define I2C_ISR_TXIS                 (1u << 1)
#define I2C_ISR_RXNE                 (1u << 2)
#define I2C_ISR_NACKF                (1u << 4)
#define I2C_ISR_STOPF                (1u << 5)
#define I2C_ISR_TC                   (1u << 6)
#define I2C_ISR_TCR                  (1u << 7)
#define I2C_ISR_BERR                 (1u << 8)
#define I2C_ISR_ARLO                 (1u << 9)
#define I2C_ISR_OVR                  (1u << 10)
#define I2C_ISR_TIMEOUT              (1u << 12)
#define I2C_ISR_ALERT                (1u << 13)
#define I2C_ISR_BUSY                 (1u << 15)

#define I2C_ISR_ERRORMASK            (I2C_ISR_BERR | I2C_ISR_ARLO | \
                                      I2C_ISR_OVR | I2C_ISR_TIMEOUT)

#define I2C_ICR_ADDRCF               (1u << 3)
#define I2C_ICR_NACKCF               (1u << 4)
#define I2C_ICR_STOPCF               (1u << 5)
#define I2C_ICR_BERRCF               (1u << 8)
#define I2C_ICR_ARLOCF               (1u << 9)
#define I2C_ICR_OVRCF                (1u << 10)
#define I2C_ICR_TIMOUTCF             (1u << 12)
#define I2C_ICR_ALERTCF              (1u << 13)

#define I2C_ICR_CLEARMASK            (I2C_ICR_ADDRCF | I2C_ICR_NACKCF | \
                                      I2C_ICR_STOPCF | I2C_ICR_BERRCF | \
                                      I2C_ICR_ARLOCF | I2C_ICR_OVRCF | \
                                      I2C_ICR_TIMOUTCF | I2C_ICR_ALERTCF)

#define I2C_RXDR_MASK                0xffu
#define I2C_TXDR_MASK                0xffu

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32N6_I2C_H */
