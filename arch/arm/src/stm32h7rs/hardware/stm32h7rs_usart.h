/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32h7rs_usart.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_USART_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_USART_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "stm32h7rs_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32H7RS_USART_CR1_OFFSET       0x0000u
#define STM32H7RS_USART_CR2_OFFSET       0x0004u
#define STM32H7RS_USART_CR3_OFFSET       0x0008u
#define STM32H7RS_USART_BRR_OFFSET       0x000cu
#define STM32H7RS_USART_ISR_OFFSET       0x001cu
#define STM32H7RS_USART_ICR_OFFSET       0x0020u
#define STM32H7RS_USART_RDR_OFFSET       0x0024u
#define STM32H7RS_USART_TDR_OFFSET       0x0028u
#define STM32H7RS_USART_PRESC_OFFSET     0x002cu

#define STM32H7RS_UART4_CR1              (STM32H7RS_UART4_BASE + \
                                           STM32H7RS_USART_CR1_OFFSET)
#define STM32H7RS_UART4_CR2              (STM32H7RS_UART4_BASE + \
                                           STM32H7RS_USART_CR2_OFFSET)
#define STM32H7RS_UART4_CR3              (STM32H7RS_UART4_BASE + \
                                           STM32H7RS_USART_CR3_OFFSET)
#define STM32H7RS_UART4_BRR              (STM32H7RS_UART4_BASE + \
                                           STM32H7RS_USART_BRR_OFFSET)
#define STM32H7RS_UART4_ISR              (STM32H7RS_UART4_BASE + \
                                           STM32H7RS_USART_ISR_OFFSET)
#define STM32H7RS_UART4_ICR              (STM32H7RS_UART4_BASE + \
                                           STM32H7RS_USART_ICR_OFFSET)
#define STM32H7RS_UART4_RDR              (STM32H7RS_UART4_BASE + \
                                           STM32H7RS_USART_RDR_OFFSET)
#define STM32H7RS_UART4_TDR              (STM32H7RS_UART4_BASE + \
                                           STM32H7RS_USART_TDR_OFFSET)
#define STM32H7RS_UART4_PRESC            (STM32H7RS_UART4_BASE + \
                                           STM32H7RS_USART_PRESC_OFFSET)

#define USART_CR1_UE                     (1u << 0)
#define USART_CR1_RE                     (1u << 2)
#define USART_CR1_TE                     (1u << 3)
#define USART_CR1_RXNEIE_RXFNEIE         (1u << 5)
#define USART_CR1_TCIE                   (1u << 6)
#define USART_CR1_TXEIE_TXFNFIE          (1u << 7)

#define USART_ISR_PE                     (1u << 0)
#define USART_ISR_FE                     (1u << 1)
#define USART_ISR_NE                     (1u << 2)
#define USART_ISR_ORE                    (1u << 3)
#define USART_ISR_RXNE_RXFNE             (1u << 5)
#define USART_ISR_TC                     (1u << 6)
#define USART_ISR_TXE_TXFNF              (1u << 7)
#define USART_ISR_ERROR_MASK             (USART_ISR_PE | USART_ISR_FE | \
                                           USART_ISR_NE | USART_ISR_ORE)

#define USART_ICR_PECF                   (1u << 0)
#define USART_ICR_FECF                   (1u << 1)
#define USART_ICR_NECF                   (1u << 2)
#define USART_ICR_ORECF                  (1u << 3)
#define USART_ICR_TCCF                   (1u << 6)
#define USART_ICR_ERROR_MASK             (USART_ICR_PECF | USART_ICR_FECF | \
                                           USART_ICR_NECF | USART_ICR_ORECF)

#define USART_BRR_MASK                   0x0000ffffu

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32H7RS_USART_H */
