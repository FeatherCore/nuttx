/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct stm32n6_xspi2_nor_info_s
{
  uint8_t  jedec[3];
  bool     jedec_valid;
  bool     mapped;
  bool     common_ready;
  bool     vddio2_hslv;
  bool     vddio3_hslv;
  uint32_t base;
  uint32_t size;
  uint32_t erase_size;
  uint32_t page_size;
  uint32_t source_hz;
  uint32_t effective_hz;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void stm32n6_clockconfig(void);
void stm32n6_clock_bootlog(void);
void stm32n6_power_config(void);
void stm32n6_lowsetup(void);
int stm32n6_xspi2_nor_initialize(void);
int stm32n6_xspi1_psram_initialize(void);
int stm32n6_xspi2_nor_getinfo(struct stm32n6_xspi2_nor_info_s *info);
int stm32n6_xspi2_nor_erase(uint32_t offset, size_t nbytes);
ssize_t stm32n6_xspi2_nor_write(uint32_t offset, const uint8_t *buffer,
                                size_t nbytes);
int stm32n6_xspi1_psram_selftest(uintptr_t address);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_H */
