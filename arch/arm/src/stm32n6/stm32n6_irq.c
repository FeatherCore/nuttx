/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_irq.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdint.h>

#include <nuttx/irq.h>
#include <arch/armv8-m/nvicpri.h>

#include "arm_internal.h"
#include "chip.h"
#include "nvic.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NVIC_ENA_OFFSET    (0)
#define NVIC_CLRENA_OFFSET (NVIC_IRQ0_31_CLEAR - NVIC_IRQ0_31_ENABLE)

#define DEFPRIORITY32 \
  (NVIC_SYSH_PRIORITY_DEFAULT << 24 | \
   NVIC_SYSH_PRIORITY_DEFAULT << 16 | \
   NVIC_SYSH_PRIORITY_DEFAULT << 8  | \
   NVIC_SYSH_PRIORITY_DEFAULT)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void stm32n6_prioritize_syscall(int priority)
{
  uint32_t regval;

  regval  = getreg32(NVIC_SYSH8_11_PRIORITY);
  regval &= ~NVIC_SYSH_PRIORITY_PR11_MASK;
  regval |= (priority << NVIC_SYSH_PRIORITY_PR11_SHIFT);
  putreg32(regval, NVIC_SYSH8_11_PRIORITY);
}

static int stm32n6_irqinfo(int irq, uintptr_t *regaddr, uint32_t *bit,
                           uintptr_t offset)
{
  int n;

  if (irq >= STM32_IRQ_FIRST)
    {
      n        = irq - STM32_IRQ_FIRST;
      *regaddr = NVIC_IRQ_ENABLE(n) + offset;
      *bit     = (uint32_t)1 << (n & 31);
    }
  else
    {
      *regaddr = NVIC_SYSHCON;

      if (irq == STM32_IRQ_MEMFAULT)
        {
          *bit = NVIC_SYSHCON_MEMFAULTENA;
        }
      else if (irq == STM32_IRQ_BUSFAULT)
        {
          *bit = NVIC_SYSHCON_BUSFAULTENA;
        }
      else if (irq == STM32_IRQ_USAGEFAULT)
        {
          *bit = NVIC_SYSHCON_USGFAULTENA;
        }
      else if (irq == STM32_IRQ_SYSTICK)
        {
          *regaddr = NVIC_SYSTICK_CTRL;
          *bit     = NVIC_SYSTICK_CTRL_ENABLE;
        }
      else
        {
          return -EINVAL;
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int up_prioritize_irq(int irq, int priority)
{
  uintptr_t regaddr;
  uint32_t regval;
  int shift;

  DEBUGASSERT(irq >= 0 && irq < NR_IRQS &&
              (unsigned)priority <= NVIC_SYSH_PRIORITY_MIN);

  if (irq < STM32_IRQ_FIRST)
    {
      regaddr = NVIC_SYSH_PRIORITY(irq);
      irq    -= 4;
    }
  else
    {
      irq    -= STM32_IRQ_FIRST;
      regaddr = NVIC_IRQ_PRIORITY(irq);
    }

  regval  = getreg32(regaddr);
  shift   = ((irq & 3) << 3);
  regval &= ~(0xff << shift);
  regval |= (priority << shift);
  putreg32(regval, regaddr);

  return OK;
}

void up_irqinitialize(void)
{
  uintptr_t regaddr;
  int num_priority_registers;
  int i;

  for (i = 0; i < STM32_IRQ_NEXTINTS; i += 32)
    {
      putreg32(0xffffffff, NVIC_IRQ_CLEAR(i));
    }

  putreg32((uint32_t)_vectors, NVIC_VECTAB);

#ifdef CONFIG_ARCH_RAMVECTORS
  up_ramvec_initialize();
#endif

  putreg32(DEFPRIORITY32, NVIC_SYSH4_7_PRIORITY);
  putreg32(DEFPRIORITY32, NVIC_SYSH8_11_PRIORITY);
  putreg32(DEFPRIORITY32, NVIC_SYSH12_15_PRIORITY);

  num_priority_registers = (getreg32(NVIC_ICTR) + 1) * 8;
  regaddr = NVIC_IRQ0_3_PRIORITY;
  while (num_priority_registers-- > 0)
    {
      putreg32(DEFPRIORITY32, regaddr);
      regaddr += 4;
    }

  irq_attach(STM32_IRQ_SVCALL, arm_svcall, NULL);
  irq_attach(STM32_IRQ_HARDFAULT, arm_hardfault, NULL);
  irq_attach(STM32_IRQ_BUSFAULT, arm_busfault, NULL);
  irq_attach(STM32_IRQ_USAGEFAULT, arm_usagefault, NULL);

  up_prioritize_irq(STM32_IRQ_PENDSV, NVIC_SYSH_PRIORITY_MIN);
  stm32n6_prioritize_syscall(NVIC_SYSH_SVCALL_PRIORITY);

#ifdef CONFIG_ARM_MPU
  irq_attach(STM32_IRQ_MEMFAULT, arm_memfault, NULL);
  up_enable_irq(STM32_IRQ_MEMFAULT);
#endif

  putreg32(NVIC_INTCTRL_PENDSTCLR | NVIC_INTCTRL_PENDSVCLR,
           NVIC_INTCTRL);

#ifndef CONFIG_SUPPRESS_INTERRUPTS
  arm_color_intstack();
  up_irq_enable();
#endif
}

void up_disable_irq(int irq)
{
  uintptr_t regaddr;
  uint32_t regval;
  uint32_t bit;

  if (stm32n6_irqinfo(irq, &regaddr, &bit, NVIC_CLRENA_OFFSET) == OK)
    {
      if (irq >= STM32_IRQ_FIRST)
        {
          putreg32(bit, regaddr);
        }
      else
        {
          regval  = getreg32(regaddr);
          regval &= ~bit;
          putreg32(regval, regaddr);
        }
    }
}

void up_enable_irq(int irq)
{
  uintptr_t regaddr;
  uint32_t regval;
  uint32_t bit;

  if (stm32n6_irqinfo(irq, &regaddr, &bit, NVIC_ENA_OFFSET) == OK)
    {
      if (irq >= STM32_IRQ_FIRST)
        {
          putreg32(bit, regaddr);
        }
      else
        {
          regval  = getreg32(regaddr);
          regval |= bit;
          putreg32(regval, regaddr);
        }
    }
}

void arm_ack_irq(int irq)
{
}
