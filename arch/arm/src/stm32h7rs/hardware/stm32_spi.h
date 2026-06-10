/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32_spi.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_SPI_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_SPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_SPI_CR1_OFFSET          0x0000u
#define STM32_SPI_CR2_OFFSET          0x0004u
#define STM32_SPI_CFG1_OFFSET         0x0008u
#define STM32_SPI_CFG2_OFFSET         0x000cu
#define STM32_SPI_IER_OFFSET          0x0010u
#define STM32_SPI_SR_OFFSET           0x0014u
#define STM32_SPI_IFCR_OFFSET         0x0018u
#define STM32_SPI_TXDR_OFFSET         0x0020u
#define STM32_SPI_RXDR_OFFSET         0x0030u

#define STM32_SPI_CR1(b)              ((b) + STM32_SPI_CR1_OFFSET)
#define STM32_SPI_CR2(b)              ((b) + STM32_SPI_CR2_OFFSET)
#define STM32_SPI_CFG1(b)             ((b) + STM32_SPI_CFG1_OFFSET)
#define STM32_SPI_CFG2(b)             ((b) + STM32_SPI_CFG2_OFFSET)
#define STM32_SPI_IER(b)              ((b) + STM32_SPI_IER_OFFSET)
#define STM32_SPI_SR(b)               ((b) + STM32_SPI_SR_OFFSET)
#define STM32_SPI_IFCR(b)             ((b) + STM32_SPI_IFCR_OFFSET)
#define STM32_SPI_TXDR(b)             ((b) + STM32_SPI_TXDR_OFFSET)
#define STM32_SPI_RXDR(b)             ((b) + STM32_SPI_RXDR_OFFSET)

#define SPI_CR1_SPE                   (1u << 0)
#define SPI_CR1_CSTART                (1u << 9)
#define SPI_CR1_SSI                   (1u << 12)

#define SPI_CR2_TSIZE_SHIFT           0
#define SPI_CR2_TSIZE_MASK            (0xffffu << SPI_CR2_TSIZE_SHIFT)
#define SPI_CR2_TSIZE(n)              ((uint32_t)(n) << SPI_CR2_TSIZE_SHIFT)

#define SPI_CFG1_DSIZE_SHIFT          0
#define SPI_CFG1_DSIZE_MASK           (0x1fu << SPI_CFG1_DSIZE_SHIFT)
#define SPI_CFG1_DSIZE(n)             ((uint32_t)((n) - 1u) << \
                                       SPI_CFG1_DSIZE_SHIFT)
#define SPI_CFG1_FTHLV_SHIFT          5
#define SPI_CFG1_FTHLV_MASK           (0x0fu << SPI_CFG1_FTHLV_SHIFT)
#define SPI_CFG1_FTHLV(n)             ((uint32_t)((n) - 1u) << \
                                       SPI_CFG1_FTHLV_SHIFT)
#define SPI_CFG1_MBR_SHIFT            28
#define SPI_CFG1_MBR_MASK             (7u << SPI_CFG1_MBR_SHIFT)
#define SPI_CFG1_MBR(n)               ((uint32_t)(n) << SPI_CFG1_MBR_SHIFT)

#define SPI_CFG2_COMM_SHIFT           17
#define SPI_CFG2_COMM_MASK            (3u << SPI_CFG2_COMM_SHIFT)
#define SPI_CFG2_FULLDUPLEX           0u
#define SPI_CFG2_MASTER               (1u << 22)
#define SPI_CFG2_CPHA                 (1u << 24)
#define SPI_CFG2_CPOL                 (1u << 25)
#define SPI_CFG2_SSM                  (1u << 26)
#define SPI_CFG2_AFCNTR               (1u << 31)

#define SPI_SR_RXP                    (1u << 0)
#define SPI_SR_TXP                    (1u << 1)
#define SPI_SR_EOT                    (1u << 3)
#define SPI_SR_TXTF                   (1u << 4)
#define SPI_SR_UDR                    (1u << 5)
#define SPI_SR_OVR                    (1u << 6)
#define SPI_SR_CRCE                   (1u << 7)
#define SPI_SR_TIFRE                  (1u << 8)
#define SPI_SR_MODF                   (1u << 9)
#define SPI_SR_SUSP                   (1u << 11)
#define SPI_SR_ERRORMASK              (SPI_SR_UDR | SPI_SR_OVR | \
                                       SPI_SR_CRCE | SPI_SR_TIFRE | \
                                       SPI_SR_MODF)

#define SPI_IFCR_EOTC                 (1u << 3)
#define SPI_IFCR_TXTFC                (1u << 4)
#define SPI_IFCR_UDRC                 (1u << 5)
#define SPI_IFCR_OVRC                 (1u << 6)
#define SPI_IFCR_CRCEC                (1u << 7)
#define SPI_IFCR_TIFREC               (1u << 8)
#define SPI_IFCR_MODFC                (1u << 9)
#define SPI_IFCR_SUSPC                (1u << 11)
#define SPI_IFCR_CLEARMASK            (SPI_IFCR_EOTC | SPI_IFCR_TXTFC | \
                                       SPI_IFCR_UDRC | SPI_IFCR_OVRC | \
                                       SPI_IFCR_CRCEC | SPI_IFCR_TIFREC | \
                                       SPI_IFCR_MODFC | SPI_IFCR_SUSPC)

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_SPI_H */
