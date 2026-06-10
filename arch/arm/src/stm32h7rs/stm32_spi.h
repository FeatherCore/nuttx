/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32_spi.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_STM32_SPI_H
#define __ARCH_ARM_SRC_STM32H7RS_STM32_SPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__

struct spi_dev_s;

struct spi_dev_s *stm32_spibus_initialize(int bus);

#ifdef CONFIG_STM32H7RS_SPI4
void stm32_spi4select(struct spi_dev_s *dev, uint32_t devid,
                      bool selected);
uint8_t stm32_spi4status(struct spi_dev_s *dev, uint32_t devid);
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_ARM_SRC_STM32H7RS_STM32_SPI_H */
