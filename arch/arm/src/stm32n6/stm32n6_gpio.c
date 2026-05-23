/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_gpio.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <nuttx/irq.h>
#include <nuttx/spinlock.h>

#include "arm_internal.h"

#include "hardware/stm32n6_gpio.h"
#include "hardware/stm32n6_rcc.h"

#include "stm32n6_gpio.h"

#ifdef CONFIG_STM32N6_GPIO

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_gpio_port_s
{
  uintptr_t base;
  uint32_t enable;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static spinlock_t g_configgpio_lock = SP_UNLOCKED;

static const struct stm32n6_gpio_port_s g_gpioport[] =
{
  { STM32N6_GPIOA_BASE, RCC_AHB4ENSR_GPIOAENS },
  { STM32N6_GPIOB_BASE, RCC_AHB4ENSR_GPIOBENS },
  { STM32N6_GPIOC_BASE, RCC_AHB4ENSR_GPIOCENS },
  { STM32N6_GPIOD_BASE, RCC_AHB4ENSR_GPIODENS },
  { STM32N6_GPIOE_BASE, RCC_AHB4ENSR_GPIOEENS },
  { STM32N6_GPIOF_BASE, RCC_AHB4ENSR_GPIOFENS },
  { STM32N6_GPIOG_BASE, RCC_AHB4ENSR_GPIOGENS },
  { STM32N6_GPIOH_BASE, RCC_AHB4ENSR_GPIOHENS },
  { STM32N6_GPION_BASE, RCC_AHB4ENSR_GPIONENS },
  { STM32N6_GPIOO_BASE, RCC_AHB4ENSR_GPIOOENS },
  { STM32N6_GPIOP_BASE, RCC_AHB4ENSR_GPIOPENS },
  { STM32N6_GPIOQ_BASE, RCC_AHB4ENSR_GPIOQENS },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int
stm32n6_gpio_port(uint32_t cfgset,
                  FAR const struct stm32n6_gpio_port_s **port)
{
  unsigned int ndx;

  ndx = (cfgset & STM32N6_GPIO_PORT_MASK) >> STM32N6_GPIO_PORT_SHIFT;
  if (ndx >= sizeof(g_gpioport) / sizeof(g_gpioport[0]))
    {
      return -EINVAL;
    }

  *port = &g_gpioport[ndx];
  return OK;
}

static void stm32n6_gpio_clock_enable(uint32_t enable)
{
  putreg32(enable, STM32N6_RCC_AHB4ENSR);
  (void)getreg32(STM32N6_RCC_AHB4ENR);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6_configgpio(uint32_t cfgset)
{
  FAR const struct stm32n6_gpio_port_s *port;
  irqstate_t flags;
  uintptr_t afr;
  uint32_t mode;
  uint32_t regval;
  uint32_t setting;
  unsigned int pin;
  int ret;

  ret = stm32n6_gpio_port(cfgset, &port);
  if (ret < 0)
    {
      return ret;
    }

  pin = (cfgset & STM32N6_GPIO_PIN_MASK) >> STM32N6_GPIO_PIN_SHIFT;
  stm32n6_gpio_clock_enable(port->enable);

  switch (cfgset & STM32N6_GPIO_MODE_MASK)
    {
      default:
      case STM32N6_GPIO_INPUT:
        mode = GPIO_MODE_INPUT;
        break;

      case STM32N6_GPIO_OUTPUT:
        stm32n6_gpiowrite(cfgset,
                          (cfgset & STM32N6_GPIO_OUTPUT_SET) != 0);
        mode = GPIO_MODE_OUTPUT;
        break;

      case STM32N6_GPIO_ALT:
        mode = GPIO_MODE_ALT;
        break;

      case STM32N6_GPIO_ANALOG:
        mode = 3u;
        break;
    }

  flags = spin_lock_irqsave(&g_configgpio_lock);

  if (mode == GPIO_MODE_ALT)
    {
      afr = pin < 8 ? STM32N6_GPIO_AFRL(port->base) :
                      STM32N6_GPIO_AFRH(port->base);
      setting = (cfgset & STM32N6_GPIO_AF_MASK) >> STM32N6_GPIO_AF_SHIFT;

      regval = getreg32(afr);
      regval &= ~GPIO_AFR_MASK(pin);
      regval |= setting << GPIO_AFR_SHIFT(pin);
      putreg32(regval, afr);
    }

  regval = getreg32(STM32N6_GPIO_OTYPER(port->base));
  if ((cfgset & STM32N6_GPIO_OPENDRAIN) != 0)
    {
      regval |= 1u << pin;
    }
  else
    {
      regval &= ~(1u << pin);
    }

  putreg32(regval, STM32N6_GPIO_OTYPER(port->base));

  setting = (cfgset & STM32N6_GPIO_SPEED_MASK) >>
            STM32N6_GPIO_SPEED_SHIFT;
  regval = getreg32(STM32N6_GPIO_OSPEEDR(port->base));
  regval &= ~GPIO_SPEED_MASK(pin);
  regval |= setting << GPIO_SPEED_SHIFT(pin);
  putreg32(regval, STM32N6_GPIO_OSPEEDR(port->base));

  setting = (cfgset & STM32N6_GPIO_PUPD_MASK) >>
            STM32N6_GPIO_PUPD_SHIFT;
  regval = getreg32(STM32N6_GPIO_PUPDR(port->base));
  regval &= ~GPIO_PUPD_MASK(pin);
  regval |= setting << GPIO_PUPD_SHIFT(pin);
  putreg32(regval, STM32N6_GPIO_PUPDR(port->base));

  regval = getreg32(STM32N6_GPIO_MODER(port->base));
  regval &= ~GPIO_MODE_MASK(pin);
  regval |= mode << GPIO_MODE_SHIFT(pin);
  putreg32(regval, STM32N6_GPIO_MODER(port->base));

  spin_unlock_irqrestore(&g_configgpio_lock, flags);
  return OK;
}

int stm32n6_unconfiggpio(uint32_t cfgset)
{
  return stm32n6_configgpio((cfgset & (STM32N6_GPIO_PORT_MASK |
                                      STM32N6_GPIO_PIN_MASK)) |
                            STM32N6_GPIO_INPUT | STM32N6_GPIO_FLOAT);
}

void stm32n6_gpiowrite(uint32_t pinset, bool value)
{
  FAR const struct stm32n6_gpio_port_s *port;
  uint32_t bit;
  unsigned int pin;

  if (stm32n6_gpio_port(pinset, &port) < 0)
    {
      return;
    }

  pin = (pinset & STM32N6_GPIO_PIN_MASK) >> STM32N6_GPIO_PIN_SHIFT;
  bit = 1u << pin;
  putreg32(value ? bit : (bit << 16), STM32N6_GPIO_BSRR(port->base));
}

bool stm32n6_gpioread(uint32_t pinset)
{
  FAR const struct stm32n6_gpio_port_s *port;
  unsigned int pin;

  if (stm32n6_gpio_port(pinset, &port) < 0)
    {
      return false;
    }

  pin = (pinset & STM32N6_GPIO_PIN_MASK) >> STM32N6_GPIO_PIN_SHIFT;
  return (getreg32(STM32N6_GPIO_IDR(port->base)) & (1u << pin)) != 0;
}

#endif /* CONFIG_STM32N6_GPIO */
