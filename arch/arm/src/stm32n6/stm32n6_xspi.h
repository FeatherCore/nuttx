/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_xspi.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_XSPI_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_XSPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct stm32n6_xspi_config_s
{
  uintptr_t base;
  uint32_t  reset;
  uint32_t  memory_type;
  uint32_t  device_size;
  uint32_t  csht;
  uint32_t  prescaler;
  uint32_t  fifo_threshold;
  uint32_t  csbound;
  uint32_t  refresh;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

uint32_t stm32n6_xspi_prescaler(uint32_t source_hz, uint32_t target_hz);
uint32_t stm32n6_xspi_effective_hz(uint32_t source_hz,
                                   uint32_t prescaler);
bool stm32n6_xspi_common_ready(void);
uint32_t stm32n6_xspi_get_source_hz(uintptr_t base);
bool stm32n6_xspi_hslv_enabled(uintptr_t base);
int stm32n6_xspi_common_setup(void);
bool stm32n6_xspi_is_mapped(uintptr_t base);
int stm32n6_xspi_wait_idle(uintptr_t base);
void stm32n6_xspi_config_gpio(uintptr_t portbase, uint32_t pins,
                              uint32_t af);
int stm32n6_xspi_command(uintptr_t base, uint32_t instruction,
                         uint32_t ccr);
int stm32n6_xspi_command_addr(uintptr_t base, uint32_t instruction,
                              uint32_t address, uint32_t ccr,
                              uint32_t tcr);
int stm32n6_xspi_read_data(uintptr_t base, uint32_t instruction,
                           uint32_t address, uint32_t ccr,
                           uint32_t tcr, size_t nbytes,
                           FAR uint8_t *buffer);
int stm32n6_xspi_write_data(uintptr_t base, uint32_t instruction,
                            uint32_t address, uint32_t ccr,
                            uint32_t tcr, uint8_t value,
                            size_t nbytes, bool repeat);
int stm32n6_xspi_write_buffer(uintptr_t base, uint32_t instruction,
                              uint32_t address, uint32_t ccr,
                              uint32_t tcr, FAR const uint8_t *buffer,
                              size_t nbytes);
void stm32n6_xspi_controller_config(
  FAR const struct stm32n6_xspi_config_s *config);
int stm32n6_xspi_set_prescaler(uintptr_t base, uint32_t prescaler);
int stm32n6_xspi_abort_memory_mapped(uintptr_t base);
int stm32n6_xspi_enter_memory_mapped(uintptr_t base, uint32_t read_ccr,
                                     uint32_t read_tcr,
                                     uint32_t read_instruction,
                                     uint32_t write_ccr,
                                     uint32_t write_tcr,
                                     uint32_t write_instruction);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_XSPI_H */
