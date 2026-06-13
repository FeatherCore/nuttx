/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32_exti_gpio.c
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

#include <arch/irq.h>
#include <nuttx/arch.h>
#include <nuttx/irq.h>

#include "arm_internal.h"
#include "hardware/stm32_exti.h"
#include "hardware/stm32_memorymap.h"
#include "hardware/stm32_rcc.h"
#include "hardware/stm32_sbs.h"
#include "stm32.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct gpio_callback_s
{
  xcpt_t callback;
  FAR void *arg;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct gpio_callback_s g_gpio_callbacks[16];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_exti_port(uintptr_t portbase)
{
  switch (portbase)
    {
      case STM32_GPIOA_BASE:
        return SBS_EXTICR_PORTA;

      case STM32_GPIOB_BASE:
        return SBS_EXTICR_PORTB;

      case STM32_GPIOC_BASE:
        return SBS_EXTICR_PORTC;

      case STM32_GPIOD_BASE:
        return SBS_EXTICR_PORTD;

      case STM32_GPIOE_BASE:
        return SBS_EXTICR_PORTE;

      case STM32_GPIOF_BASE:
        return SBS_EXTICR_PORTF;

      case STM32_GPIOG_BASE:
        return SBS_EXTICR_PORTG;

      case STM32_GPIOH_BASE:
        return SBS_EXTICR_PORTH;

      case STM32_GPIOM_BASE:
        return SBS_EXTICR_PORTM;

      case STM32_GPION_BASE:
        return SBS_EXTICR_PORTN;

      case STM32_GPIOO_BASE:
        return SBS_EXTICR_PORTO;

      case STM32_GPIOP_BASE:
        return SBS_EXTICR_PORTP;

      default:
        return -EINVAL;
    }
}

static int stm32_exti_irq(unsigned int pin)
{
  return STM32_IRQ_EXTI0 + pin;
}

static int stm32_exti_dispatch(int irq, FAR void *context, FAR void *arg)
{
  uintptr_t pin = (uintptr_t)arg;
  uint32_t mask = STM32_EXTI_MASK(pin);
  int ret = OK;

  putreg32(mask, STM32_EXTI_CPUPR1);

  if (g_gpio_callbacks[pin].callback != NULL)
    {
      ret = g_gpio_callbacks[pin].callback(irq, context,
                                           g_gpio_callbacks[pin].arg);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_gpiosetevent(uintptr_t portbase, uint8_t pin, bool risingedge,
                       bool fallingedge, bool event, xcpt_t func,
                       FAR void *arg)
{
  uint32_t exti;
  uint32_t regaddr;
  int port;
  int irq;
  int ret;

  if (pin > 15)
    {
      return -EINVAL;
    }

  port = stm32_exti_port(portbase);
  if (port < 0)
    {
      return port;
    }

  exti = STM32_EXTI_MASK(pin);
  irq  = stm32_exti_irq(pin);

  modifyreg32(STM32_RCC_APB4ENR, 0, RCC_APB4ENR_SBSEN);
  (void)getreg32(STM32_RCC_APB4ENR);

  regaddr = STM32_SBS_EXTICR(pin);
  modifyreg32(regaddr, SBS_EXTICR_MASK(pin),
              SBS_EXTICR_PORT(pin, (uint32_t)port));

  g_gpio_callbacks[pin].callback = func;
  g_gpio_callbacks[pin].arg      = arg;

  modifyreg32(STM32_EXTI_RTSR1, risingedge ? 0 : exti,
              risingedge ? exti : 0);
  modifyreg32(STM32_EXTI_FTSR1, fallingedge ? 0 : exti,
              fallingedge ? exti : 0);
  modifyreg32(STM32_EXTI_CPUEMR1, event ? 0 : exti, event ? exti : 0);
  modifyreg32(STM32_EXTI_CPUIMR1, func ? 0 : exti, func ? exti : 0);
  putreg32(exti, STM32_EXTI_CPUPR1);

  if (func != NULL)
    {
      ret = irq_attach(irq, stm32_exti_dispatch, (FAR void *)(uintptr_t)pin);
      if (ret < 0)
        {
          modifyreg32(STM32_EXTI_CPUIMR1, exti, 0);
          g_gpio_callbacks[pin].callback = NULL;
          g_gpio_callbacks[pin].arg      = NULL;
          return ret;
        }

      up_enable_irq(irq);
    }
  else
    {
      up_disable_irq(irq);
      irq_detach(irq);
    }

  return OK;
}
