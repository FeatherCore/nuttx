/****************************************************************************
 * arch/arm/src/ra8p/ra8p_irq.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <arch/irq.h>

#include "arm_internal.h"
#include "nvic.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFPRIORITY32 \
  (NVIC_SYSH_PRIORITY_DEFAULT << 24 | NVIC_SYSH_PRIORITY_DEFAULT << 16 | \
   NVIC_SYSH_PRIORITY_DEFAULT << 8  | NVIC_SYSH_PRIORITY_DEFAULT)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int ra8p_irqinfo(int irq, uintptr_t *regaddr, uint32_t *bit,
                        uintptr_t offset)
{
  int n;

  if (irq >= RA8P_IRQ_FIRST && irq < NR_IRQS)
    {
      n        = irq - RA8P_IRQ_FIRST;
      *regaddr = NVIC_IRQ_ENABLE(n) + offset;
      *bit     = (uint32_t)1 << (n & 0x1f);
      return OK;
    }

  *regaddr = NVIC_SYSHCON;

  if (irq == RA8P_IRQ_MEMFAULT)
    {
      *bit = NVIC_SYSHCON_MEMFAULTENA;
    }
  else if (irq == RA8P_IRQ_BUSFAULT)
    {
      *bit = NVIC_SYSHCON_BUSFAULTENA;
    }
  else if (irq == RA8P_IRQ_USAGEFAULT)
    {
      *bit = NVIC_SYSHCON_USGFAULTENA;
    }
  else if (irq == RA8P_IRQ_SYSTICK)
    {
      *regaddr = NVIC_SYSTICK_CTRL;
      *bit     = NVIC_SYSTICK_CTRL_ENABLE;
    }
  else
    {
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void up_irqinitialize(void)
{
  uint32_t regaddr;
  int i;

  for (i = 0; i < NR_IRQS - RA8P_IRQ_FIRST; i += 32)
    {
      putreg32(0xffffffff, NVIC_IRQ_CLEAR(i));
    }

  putreg32((uint32_t)_vectors, NVIC_VECTAB);

  putreg32(DEFPRIORITY32, NVIC_SYSH4_7_PRIORITY);
  putreg32(DEFPRIORITY32, NVIC_SYSH8_11_PRIORITY);
  putreg32(DEFPRIORITY32, NVIC_SYSH12_15_PRIORITY);

  for (regaddr = NVIC_IRQ0_3_PRIORITY, i = 0;
       i < NR_IRQS - RA8P_IRQ_FIRST;
       regaddr += 4, i += 4)
    {
      putreg32(DEFPRIORITY32, regaddr);
    }

  irq_attach(RA8P_IRQ_HARDFAULT, arm_hardfault, NULL);
  irq_attach(RA8P_IRQ_MEMFAULT, arm_memfault, NULL);
  irq_attach(RA8P_IRQ_BUSFAULT, arm_busfault, NULL);
  irq_attach(RA8P_IRQ_USAGEFAULT, arm_usagefault, NULL);

  up_enable_irq(RA8P_IRQ_MEMFAULT);
  up_enable_irq(RA8P_IRQ_BUSFAULT);
  up_enable_irq(RA8P_IRQ_USAGEFAULT);

#ifndef CONFIG_SUPPRESS_INTERRUPTS
  up_irq_enable();
#endif
}

void up_disable_irq(int irq)
{
  uintptr_t regaddr;
  uint32_t bit;

  if (ra8p_irqinfo(irq, &regaddr, &bit, NVIC_IRQ0_31_CLEAR -
                   NVIC_IRQ0_31_ENABLE) == OK)
    {
      if (irq >= RA8P_IRQ_FIRST)
        {
          putreg32(bit, regaddr);
        }
      else
        {
          modifyreg32(regaddr, bit, 0);
        }
    }
}

void up_enable_irq(int irq)
{
  uintptr_t regaddr;
  uint32_t bit;

  if (ra8p_irqinfo(irq, &regaddr, &bit, 0) == OK)
    {
      if (irq >= RA8P_IRQ_FIRST)
        {
          putreg32(bit, regaddr);
        }
      else
        {
          modifyreg32(regaddr, 0, bit);
        }
    }
}

void arm_ack_irq(int irq)
{
}

#ifdef CONFIG_ARCH_IRQPRIO
int up_prioritize_irq(int irq, int priority)
{
  uintptr_t regaddr;
  uint32_t regval;
  uint32_t shift;

  if (irq < RA8P_IRQ_FIRST || irq >= NR_IRQS)
    {
      return ERROR;
    }

  irq -= RA8P_IRQ_FIRST;
  regaddr = NVIC_IRQ_PRIORITY(irq);
  shift = (irq & 3) << 3;

  regval = getreg32(regaddr);
  regval &= ~(0xff << shift);
  regval |= ((uint32_t)priority << shift);
  putreg32(regval, regaddr);

  return OK;
}
#endif
