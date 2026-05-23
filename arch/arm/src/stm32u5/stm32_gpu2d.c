/****************************************************************************
 * arch/arm/src/stm32u5/stm32_gpu2d.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <nuttx/clock.h>
#include <nuttx/irq.h>
#include <nuttx/semaphore.h>

#include "arm_internal.h"
#include "stm32_gpu2d.h"

#include "hardware/stm32_gpu2d.h"
#include "hardware/stm32u5xx_rcc.h"

#ifdef CONFIG_STM32U5_GPU2D

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GPU2D_RING_RESERVED_WORDS 4
#define GPU2D_PAIR_WORDS          2

/****************************************************************************
 * Private Data
 ****************************************************************************/

static sem_t g_gpu2d_sem;
static volatile uint32_t g_gpu2d_last_clid;
static volatile uint32_t g_gpu2d_last_syserror;
static bool g_gpu2d_initialized;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_gpu2d_irq(int irq, FAR void *context, FAR void *arg)
{
  uint32_t interrupt;

  interrupt = getreg32(STM32_GPU2D_INTERRUPT);
  if ((interrupt & GPU2D_INTERRUPT_CLC) != 0)
    {
      putreg32(interrupt & ~GPU2D_INTERRUPT_CLC, STM32_GPU2D_INTERRUPT);
      g_gpu2d_last_clid = getreg32(STM32_GPU2D_CLID) & GPU2D_CLID_MASK;
      nxsem_post(&g_gpu2d_sem);
    }

  return OK;
}

static int stm32_gpu2d_er_irq(int irq, FAR void *context, FAR void *arg)
{
  g_gpu2d_last_syserror = getreg32(STM32_GPU2D_SYS_INTERRUPT);
  putreg32(g_gpu2d_last_syserror, STM32_GPU2D_SYS_INTERRUPT);
  nxsem_post(&g_gpu2d_sem);

  return OK;
}

static int stm32_gpu2d_ringemit(FAR struct stm32_gpu2d_ring_s *ring,
                                uint32_t regword, uint32_t data)
{
  if (ring == NULL || ring->buffer == NULL || ring->usable_words < 2)
    {
      return -EINVAL;
    }

  if ((ring->offset_words + GPU2D_PAIR_WORDS) > ring->usable_words)
    {
      ring->offset_words = 0;
    }

  ring->buffer[ring->offset_words] = regword;
  ring->buffer[ring->offset_words + 1] = data;
  ring->offset_words += GPU2D_PAIR_WORDS;

  return OK;
}

static void stm32_gpu2d_ringflush(FAR const struct stm32_gpu2d_ring_s *ring)
{
  uintptr_t tail = ring->gpuaddr + (ring->offset_words * sizeof(uint32_t));

  UP_DMB();
  putreg32(tail | GPU2D_RINGSTOP_ENABLE, STM32_GPU2D_CMDRINGSTOP);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_gpu2dinitialize(void)
{
  int ret;

  if (g_gpu2d_initialized)
    {
      return OK;
    }

  modifyreg32(STM32_RCC_AHB1ENR, 0,
              RCC_AHB1ENR_GPU2DEN | RCC_AHB1ENR_DCACHE2EN);

#ifdef RCC_AHB1RSTR_GPU2DRST
  modifyreg32(STM32_RCC_AHB1RSTR, 0, RCC_AHB1RSTR_GPU2DRST);
  modifyreg32(STM32_RCC_AHB1RSTR, RCC_AHB1RSTR_GPU2DRST, 0);
#endif

  nxsem_init(&g_gpu2d_sem, 0, 0);
  nxsem_set_protocol(&g_gpu2d_sem, SEM_PRIO_NONE);

  putreg32(0, STM32_GPU2D_CMDSTATUS);
  putreg32(0, STM32_GPU2D_STATUS);
  putreg32(0, STM32_GPU2D_CLID);
  putreg32(0, STM32_GPU2D_INTERRUPT);
  putreg32(0x7e, STM32_GPU2D_SYS_ERROR_MASK);

  g_gpu2d_last_clid = 0;
  g_gpu2d_last_syserror = 0;

  ret = irq_attach(STM32_IRQ_GPU2D, stm32_gpu2d_irq, NULL);
  if (ret < 0)
    {
      nxsem_destroy(&g_gpu2d_sem);
      return ret;
    }

  ret = irq_attach(STM32_IRQ_GPU2D_ER, stm32_gpu2d_er_irq, NULL);
  if (ret < 0)
    {
      irq_detach(STM32_IRQ_GPU2D);
      nxsem_destroy(&g_gpu2d_sem);
      return ret;
    }

  up_enable_irq(STM32_IRQ_GPU2D);
  up_enable_irq(STM32_IRQ_GPU2D_ER);

  g_gpu2d_initialized = true;
  return OK;
}

void stm32_gpu2duninitialize(void)
{
  if (!g_gpu2d_initialized)
    {
      return;
    }

  up_disable_irq(STM32_IRQ_GPU2D);
  up_disable_irq(STM32_IRQ_GPU2D_ER);
  irq_detach(STM32_IRQ_GPU2D);
  irq_detach(STM32_IRQ_GPU2D_ER);
  nxsem_destroy(&g_gpu2d_sem);

  modifyreg32(STM32_RCC_AHB1ENR,
              RCC_AHB1ENR_GPU2DEN | RCC_AHB1ENR_DCACHE2EN, 0);
  g_gpu2d_initialized = false;
}

uint32_t stm32_gpu2dregread(uint32_t offset)
{
  return getreg32(STM32_GPU2D_BASE + offset);
}

void stm32_gpu2dregwrite(uint32_t offset, uint32_t value)
{
  putreg32(value, STM32_GPU2D_BASE + offset);
}

uint32_t stm32_gpu2dreadid(void)
{
  return getreg32(STM32_GPU2D_ID);
}

uint32_t stm32_gpu2dreadipversion(void)
{
  return getreg32(STM32_GPU2D_IP_VERSION);
}

uint32_t stm32_gpu2dreadconfig(void)
{
  return getreg32(STM32_GPU2D_CONFIG);
}

uint32_t stm32_gpu2dreadconfigh(void)
{
  return getreg32(STM32_GPU2D_CONFIGH);
}

uint32_t stm32_gpu2dcorecount(uint32_t config)
{
  return (config & GPU2D_CONFIG_CORE_COUNT_MASK) >>
         GPU2D_CONFIG_CORE_COUNT_SHIFT;
}

uint32_t stm32_gpu2ddebuglevel(uint32_t config)
{
  return (config & GPU2D_CONFIG_DEBUG_LEVEL_MASK) >>
         GPU2D_CONFIG_DEBUG_LEVEL_SHIFT;
}

uint32_t stm32_gpu2dreadsyserror(void)
{
  return getreg32(STM32_GPU2D_SYS_INTERRUPT);
}

void stm32_gpu2dclearsyserror(uint32_t status)
{
  putreg32(status, STM32_GPU2D_SYS_INTERRUPT);
  g_gpu2d_last_syserror = 0;
}

uint32_t stm32_gpu2dlastclid(void)
{
  return g_gpu2d_last_clid;
}

uint32_t stm32_gpu2dlastsyserror(void)
{
  return g_gpu2d_last_syserror;
}

void stm32_gpu2d_clinit(FAR struct stm32_gpu2d_cmdlist_s *cmdlist,
                        FAR uint32_t *buffer, uint32_t size_words)
{
  cmdlist->buffer = buffer;
  cmdlist->size_words = size_words;
  cmdlist->offset_words = 0;
}

int stm32_gpu2d_clemit(FAR struct stm32_gpu2d_cmdlist_s *cmdlist,
                       uint32_t regword, uint32_t data)
{
  if (cmdlist == NULL || cmdlist->buffer == NULL)
    {
      return -EINVAL;
    }

  if ((cmdlist->offset_words + GPU2D_PAIR_WORDS) > cmdlist->size_words)
    {
      return -ENOSPC;
    }

  cmdlist->buffer[cmdlist->offset_words] = regword;
  cmdlist->buffer[cmdlist->offset_words + 1] = data;
  cmdlist->offset_words += GPU2D_PAIR_WORDS;

  return OK;
}

int stm32_gpu2d_clemitreg(FAR struct stm32_gpu2d_cmdlist_s *cmdlist,
                          uint32_t reg, uint32_t data)
{
  return stm32_gpu2d_clemit(cmdlist, GPU2D_CL_REG(reg), data);
}

int stm32_gpu2d_clemitreturn(FAR struct stm32_gpu2d_cmdlist_s *cmdlist)
{
  return stm32_gpu2d_clemit(cmdlist, GPU2D_CL_RETURN | GPU2D_CL_NOP, 0);
}

int stm32_gpu2d_ringinit(FAR struct stm32_gpu2d_ring_s *ring,
                         FAR uint32_t *buffer, uint32_t size_words,
                         uintptr_t gpuaddr)
{
  uint32_t ring_size_bytes;

  if (ring == NULL || buffer == NULL || size_words <= GPU2D_RING_RESERVED_WORDS)
    {
      return -EINVAL;
    }

  if ((size_words & 1) != 0 || !GPU2D_IS_ALIGNED_8(gpuaddr))
    {
      return -EINVAL;
    }

  ring->buffer = buffer;
  ring->size_words = size_words;
  ring->usable_words = size_words - GPU2D_RING_RESERVED_WORDS;
  ring->offset_words = 0;
  ring->gpuaddr = gpuaddr;
  ring->last_submit = 0;

  ring->buffer[ring->usable_words] =
    GPU2D_CL_HOLD | GPU2D_CL_REG(STM32_GPU2D_CMDADDR - STM32_GPU2D_BASE);
  ring->buffer[ring->usable_words + 1] = gpuaddr;
  ring->buffer[ring->usable_words + 2] =
    GPU2D_CL_HOLD | GPU2D_CL_REG(STM32_GPU2D_CMDSIZE - STM32_GPU2D_BASE);
  ring->buffer[ring->usable_words + 3] = ring->usable_words * sizeof(uint32_t);

  ring_size_bytes = ring->usable_words * sizeof(uint32_t);

  UP_DMB();
  putreg32(0, STM32_GPU2D_CLID);
  putreg32(gpuaddr | GPU2D_RINGSTOP_NOTRIGGER | GPU2D_RINGSTOP_ENABLE,
           STM32_GPU2D_CMDRINGSTOP);
  putreg32(gpuaddr, STM32_GPU2D_CMDADDR);
  putreg32(ring_size_bytes, STM32_GPU2D_CMDSIZE);

  return OK;
}

int stm32_gpu2d_submit(FAR struct stm32_gpu2d_ring_s *ring,
                       uintptr_t cmdlist_gpuaddr, uint32_t cmdlist_words,
                       FAR uint32_t *submit_id)
{
  uint32_t id;
  int ret;

  if (ring == NULL || ring->buffer == NULL ||
      !GPU2D_IS_ALIGNED_8(cmdlist_gpuaddr) || (cmdlist_words & 1) != 0)
    {
      return -EINVAL;
    }

  ret = stm32_gpu2d_ringemit(ring,
          GPU2D_CL_REG(STM32_GPU2D_CMDADDR - STM32_GPU2D_BASE),
          cmdlist_gpuaddr);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_gpu2d_ringemit(ring,
          GPU2D_CL_PUSH | GPU2D_CL_REG(STM32_GPU2D_CMDSIZE - STM32_GPU2D_BASE),
          cmdlist_words);
  if (ret < 0)
    {
      return ret;
    }

  id = (ring->last_submit + 1) & GPU2D_CLID_MASK;
  if (id == 0)
    {
      id = 1;
    }

  ret = stm32_gpu2d_ringemit(ring,
          GPU2D_CL_REG(STM32_GPU2D_CLID - STM32_GPU2D_BASE), id);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_gpu2d_ringemit(ring,
          GPU2D_CL_REG(STM32_GPU2D_INTERRUPT - STM32_GPU2D_BASE),
          GPU2D_INTERRUPT_CLC);
  if (ret < 0)
    {
      return ret;
    }

  ring->last_submit = id;
  stm32_gpu2d_ringflush(ring);

  if (submit_id != NULL)
    {
      *submit_id = id;
    }

  return OK;
}

int stm32_gpu2d_wait(uint32_t submit_id, uint32_t timeout_ms)
{
  clock_t start;
  clock_t timeout;
  uint32_t target = submit_id & GPU2D_CLID_MASK;
  int ret;

  if (target == 0)
    {
      return -EINVAL;
    }

  if (timeout_ms != UINT32_MAX)
    {
      start = clock_systime_ticks();
      timeout = MSEC2TICK(timeout_ms);
    }

  while ((g_gpu2d_last_clid & GPU2D_CLID_MASK) < target)
    {
      clock_t elapsed;
      clock_t remaining;

      if (g_gpu2d_last_syserror != 0)
        {
          return -EIO;
        }

      if (timeout_ms == UINT32_MAX)
        {
          ret = nxsem_wait_uninterruptible(&g_gpu2d_sem);
        }
      else
        {
          elapsed = clock_systime_ticks() - start;
          if (elapsed >= timeout)
            {
              return -ETIMEDOUT;
            }

          remaining = timeout - elapsed;
          ret = nxsem_tickwait_uninterruptible(&g_gpu2d_sem, remaining);
        }

      if (ret < 0)
        {
          return ret;
        }
    }

  return OK;
}

#endif /* CONFIG_STM32U5_GPU2D */
