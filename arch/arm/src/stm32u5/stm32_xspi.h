/****************************************************************************
 * arch/arm/src/stm32u5/stm32_xspi.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_STM32_XSPI_H
#define __ARCH_ARM_SRC_STM32U5_STM32_XSPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hardware/stm32_xspi.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum stm32_xspi_port_e
{
  STM32_XSPI_PORT_OCTOSPI1 = 0,
  STM32_XSPI_PORT_OCTOSPI2,
  STM32_XSPI_PORT_HSPI1
};

struct stm32_xspi_config_s
{
  enum stm32_xspi_port_e port;
  uint32_t memory_type;
  uint32_t device_size;
  uint32_t csht;
  uint32_t prescaler;
  uint32_t fifo_threshold;
  uint32_t maxtran;
  uint32_t csbound;
  uint32_t refresh;
};

#ifdef CONFIG_STM32U5_OCTOSPIM
struct stm32_xspim_config_s
{
  enum stm32_xspi_port_e port;
  uint8_t clk_port;
  uint8_t dqs_port;
  uint8_t ncs_port;
  uint8_t iolow_port;
  uint8_t iohigh_port;
  uint32_t req2ack_time;
};
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int stm32_xspi_common_setup(void);
#ifdef CONFIG_STM32U5_OCTOSPIM
int stm32_xspim_configure(FAR const struct stm32_xspim_config_s *config);
#endif
int stm32_xspi_configure(FAR const struct stm32_xspi_config_s *config);
bool stm32_xspi_is_mapped(enum stm32_xspi_port_e port);
int stm32_xspi_set_prescaler(enum stm32_xspi_port_e port,
                             uint32_t prescaler);
int stm32_xspi_set_memory_type(enum stm32_xspi_port_e port,
                               uint32_t memory_type);
int stm32_xspi_dlyb_set(enum stm32_xspi_port_e port, uint32_t phase,
                        uint32_t units);
int stm32_xspi_dlyb_configure(enum stm32_xspi_port_e port,
                              FAR uint32_t *phase, FAR uint32_t *units);
int stm32_xspi_abort(enum stm32_xspi_port_e port);
int stm32_xspi_command_tcr(enum stm32_xspi_port_e port,
                           uint32_t instruction, uint32_t ccr,
                           uint32_t tcr);
int stm32_xspi_command(enum stm32_xspi_port_e port, uint32_t instruction,
                       uint32_t ccr);
int stm32_xspi_command_addr(enum stm32_xspi_port_e port,
                            uint32_t instruction, uint32_t address,
                            uint32_t ccr, uint32_t tcr);
int stm32_xspi_read_data(enum stm32_xspi_port_e port, uint32_t instruction,
                         uint32_t address, uint32_t ccr, uint32_t tcr,
                         size_t nbytes, FAR uint8_t *buffer);
int stm32_xspi_write_data(enum stm32_xspi_port_e port, uint32_t instruction,
                          uint32_t address, uint32_t ccr, uint32_t tcr,
                          uint8_t value, size_t nbytes, bool repeat);
int stm32_xspi_write_buffer(enum stm32_xspi_port_e port,
                            uint32_t instruction, uint32_t address,
                            uint32_t ccr, uint32_t tcr, size_t nbytes,
                            FAR const uint8_t *buffer);
int stm32_xspi_enter_memory_mapped(enum stm32_xspi_port_e port,
                                   uint32_t readccr, uint32_t readtcr,
                                   uint32_t readinst);
int stm32_xspi_enter_memory_mapped_rw(enum stm32_xspi_port_e port,
                                      uint32_t readccr, uint32_t readtcr,
                                      uint32_t readinst,
                                      uint32_t writeccr,
                                      uint32_t writetcr,
                                      uint32_t writeinst);

#endif /* __ARCH_ARM_SRC_STM32U5_STM32_XSPI_H */
