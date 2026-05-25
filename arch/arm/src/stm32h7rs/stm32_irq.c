/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32_irq.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/irq.h>
#include <arch/armv7-m/nvicpri.h>

#include "arm_internal.h"
#include "chip.h"
#include "nvic.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFPRIORITY32 \
  (NVIC_SYSH_PRIORITY_DEFAULT << 24 | \
   NVIC_SYSH_PRIORITY_DEFAULT << 16 | \
   NVIC_SYSH_PRIORITY_DEFAULT << 8  | \
   NVIC_SYSH_PRIORITY_DEFAULT)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_prioritize_syscall
 *
 * Description:
 *   Keep SVC above the BASEPRI mask used by scheduler critical sections.
 *
 ****************************************************************************/

static inline void stm32_prioritize_syscall(int priority)
{
  uint32_t regval;

  regval  = getreg32(NVIC_SYSH8_11_PRIORITY);
  regval &= ~NVIC_SYSH_PRIORITY_PR11_MASK;
  regval |= (priority << NVIC_SYSH_PRIORITY_PR11_SHIFT);
  putreg32(regval, NVIC_SYSH8_11_PRIORITY);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void up_irqinitialize(void)
{
  uintptr_t regaddr;
  int i;

  for (i = 0; i < STM32_IRQ_NEXTINTS; i += 32)
    {
      putreg32(0xffffffff, NVIC_IRQ_CLEAR(i));
    }

  putreg32((uint32_t)_vectors, NVIC_VECTAB);

  putreg32(DEFPRIORITY32, NVIC_SYSH4_7_PRIORITY);
  putreg32(DEFPRIORITY32, NVIC_SYSH8_11_PRIORITY);
  putreg32(DEFPRIORITY32, NVIC_SYSH12_15_PRIORITY);

  for (i = 0, regaddr = NVIC_IRQ0_3_PRIORITY;
       i < STM32_IRQ_NEXTINTS;
       i += 4, regaddr += 4)
    {
      putreg32(DEFPRIORITY32, regaddr);
    }

  irq_attach(STM32_IRQ_SVCALL, arm_svcall, NULL);
  irq_attach(STM32_IRQ_HARDFAULT, arm_hardfault, NULL);

  stm32_prioritize_syscall(NVIC_SYSH_SVCALL_PRIORITY);

#ifndef CONFIG_SUPPRESS_INTERRUPTS
  arm_color_intstack();
  up_irq_enable();
#endif
}

void up_disable_irq(int irq)
{
  if (irq >= STM32_IRQ_FIRST)
    {
      irq -= STM32_IRQ_FIRST;
      putreg32(1u << (irq & 31), NVIC_IRQ_CLEAR(irq));
    }
  else if (irq == STM32_IRQ_SYSTICK)
    {
      modifyreg32(NVIC_SYSTICK_CTRL, NVIC_SYSTICK_CTRL_ENABLE, 0);
    }
#ifdef CONFIG_ARM_MPU
  else if (irq == STM32_IRQ_MEMFAULT)
    {
      modifyreg32(NVIC_SYSHCON, NVIC_SYSHCON_MEMFAULTENA, 0);
    }
#endif
  else if (irq == STM32_IRQ_BUSFAULT)
    {
      modifyreg32(NVIC_SYSHCON, NVIC_SYSHCON_BUSFAULTENA, 0);
    }
  else if (irq == STM32_IRQ_USAGEFAULT)
    {
      modifyreg32(NVIC_SYSHCON, NVIC_SYSHCON_USGFAULTENA, 0);
    }
}

void up_enable_irq(int irq)
{
  if (irq >= STM32_IRQ_FIRST)
    {
      irq -= STM32_IRQ_FIRST;
      putreg32(1u << (irq & 31), NVIC_IRQ_ENABLE(irq));
    }
#ifdef CONFIG_ARM_MPU
  else if (irq == STM32_IRQ_MEMFAULT)
    {
      modifyreg32(NVIC_SYSHCON, 0, NVIC_SYSHCON_MEMFAULTENA);
    }
#endif
  else if (irq == STM32_IRQ_BUSFAULT)
    {
      modifyreg32(NVIC_SYSHCON, 0, NVIC_SYSHCON_BUSFAULTENA);
    }
  else if (irq == STM32_IRQ_USAGEFAULT)
    {
      modifyreg32(NVIC_SYSHCON, 0, NVIC_SYSHCON_USGFAULTENA);
    }
}

void arm_ack_irq(int irq)
{
  (void)irq;
}
