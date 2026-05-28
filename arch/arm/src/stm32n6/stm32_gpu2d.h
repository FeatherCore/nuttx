/****************************************************************************
 * arch/arm/src/stm32n6/stm32_gpu2d.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32_GPU2D_H
#define __ARCH_ARM_SRC_STM32N6_STM32_GPU2D_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifdef CONFIG_STM32N6_GPU2D
struct stm32_gpu2d_cmdlist_s
{
  FAR uint32_t *buffer;
  uint32_t size_words;
  uint32_t offset_words;
};

struct stm32_gpu2d_ring_s
{
  FAR uint32_t *buffer;
  uint32_t size_words;
  uint32_t usable_words;
  uint32_t offset_words;
  uintptr_t gpuaddr;
  uint32_t last_submit;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int stm32_gpu2dinitialize(void);
void stm32_gpu2duninitialize(void);

uint32_t stm32_gpu2dregread(uint32_t offset);
void stm32_gpu2dregwrite(uint32_t offset, uint32_t value);

uint32_t stm32_gpu2dreadid(void);
uint32_t stm32_gpu2dreadipversion(void);
uint32_t stm32_gpu2dreadconfig(void);
uint32_t stm32_gpu2dreadconfigh(void);
uint32_t stm32_gpu2dcorecount(uint32_t config);
uint32_t stm32_gpu2ddebuglevel(uint32_t config);

uint32_t stm32_gpu2dreadsyserror(void);
void stm32_gpu2dclearsyserror(uint32_t status);
uint32_t stm32_gpu2dlastclid(void);
uint32_t stm32_gpu2dlastsyserror(void);
uint32_t stm32_gpu2dlastcmdstatus(void);
uint32_t stm32_gpu2dlaststatus(void);
uint32_t stm32_gpu2dlastinterrupt(void);

void stm32_gpu2d_clinit(FAR struct stm32_gpu2d_cmdlist_s *cmdlist,
                        FAR uint32_t *buffer, uint32_t size_words);
int stm32_gpu2d_clemit(FAR struct stm32_gpu2d_cmdlist_s *cmdlist,
                       uint32_t regword, uint32_t data);
int stm32_gpu2d_clemitreg(FAR struct stm32_gpu2d_cmdlist_s *cmdlist,
                          uint32_t reg, uint32_t data);
int stm32_gpu2d_clemitreturn(FAR struct stm32_gpu2d_cmdlist_s *cmdlist);

int stm32_gpu2d_ringinit(FAR struct stm32_gpu2d_ring_s *ring,
                         FAR uint32_t *buffer, uint32_t size_words,
                         uintptr_t gpuaddr);
int stm32_gpu2d_submit(FAR struct stm32_gpu2d_ring_s *ring,
                       uintptr_t cmdlist_gpuaddr, uint32_t cmdlist_words,
                       FAR uint32_t *submit_id);
int stm32_gpu2d_wait(uint32_t submit_id, uint32_t timeout_ms);

#endif /* CONFIG_STM32N6_GPU2D */

#endif /* __ARCH_ARM_SRC_STM32N6_STM32_GPU2D_H */
