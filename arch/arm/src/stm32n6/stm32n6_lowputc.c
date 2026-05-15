/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_lowputc.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "stm32n6.h"

#include "hardware/stm32n6_gpio.h"
#include "hardware/stm32n6_rcc.h"
#include "hardware/stm32n6_usart.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_usart1_initialized;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32n6_usart1_gpio_config(void)
{
  uint32_t regval;

  putreg32(RCC_AHB4ENSR_GPIOEENS, STM32N6_RCC_AHB4ENSR);
  (void)getreg32(STM32N6_RCC_AHB4ENR);

  regval = getreg32(STM32N6_GPIO_MODER(STM32N6_GPIOE_BASE));
  regval &= ~(GPIO_MODE_MASK(5) | GPIO_MODE_MASK(6));
  regval |= (GPIO_MODE_ALT << GPIO_MODE_SHIFT(5)) |
            (GPIO_MODE_ALT << GPIO_MODE_SHIFT(6));
  putreg32(regval, STM32N6_GPIO_MODER(STM32N6_GPIOE_BASE));

  regval = getreg32(STM32N6_GPIO_OTYPER(STM32N6_GPIOE_BASE));
  regval &= ~((1u << 5) | (1u << 6));
  putreg32(regval, STM32N6_GPIO_OTYPER(STM32N6_GPIOE_BASE));

  regval = getreg32(STM32N6_GPIO_OSPEEDR(STM32N6_GPIOE_BASE));
  regval &= ~(GPIO_SPEED_MASK(5) | GPIO_SPEED_MASK(6));
  regval |= (GPIO_SPEED_LOW << GPIO_SPEED_SHIFT(5)) |
            (GPIO_SPEED_LOW << GPIO_SPEED_SHIFT(6));
  putreg32(regval, STM32N6_GPIO_OSPEEDR(STM32N6_GPIOE_BASE));

  regval = getreg32(STM32N6_GPIO_PUPDR(STM32N6_GPIOE_BASE));
  regval &= ~(GPIO_PUPD_MASK(5) | GPIO_PUPD_MASK(6));
  putreg32(regval, STM32N6_GPIO_PUPDR(STM32N6_GPIOE_BASE));

  regval = getreg32(STM32N6_GPIO_AFRL(STM32N6_GPIOE_BASE));
  regval &= ~(GPIO_AFR_MASK(5) | GPIO_AFR_MASK(6));
  regval |= (GPIO_AF_USART1 << GPIO_AFR_SHIFT(5)) |
            (GPIO_AF_USART1 << GPIO_AFR_SHIFT(6));
  putreg32(regval, STM32N6_GPIO_AFRL(STM32N6_GPIOE_BASE));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void stm32n6_lowsetup(void)
{
  uint32_t brr;
  uint32_t timeout;

  stm32n6_usart1_gpio_config();

  putreg32(RCC_APB2ENSR_USART1ENS, STM32N6_RCC_APB2ENSR);
  (void)getreg32(STM32N6_RCC_APB2ENR);

  putreg32(RCC_APB2RSTR_USART1RST, STM32N6_RCC_APB2RSTSR);
  putreg32(RCC_APB2RSTR_USART1RST, STM32N6_RCC_APB2RSTCR);

  putreg32(0, STM32N6_USART1_CR1);
  putreg32(0, STM32N6_USART1_CR2);
  putreg32(0, STM32N6_USART1_CR3);
  putreg32(0, STM32N6_USART1_PRESC);
  putreg32(USART_ICR_TCCF, STM32N6_USART1_ICR);

  brr = (STM32N6_PCLK2_FREQUENCY + (STM32N6_USART1_BAUD / 2)) /
        STM32N6_USART1_BAUD;
  putreg32(brr & USART_BRR_MASK, STM32N6_USART1_BRR);

  putreg32(USART_CR1_UE | USART_CR1_TE | USART_CR1_RE,
           STM32N6_USART1_CR1);

  timeout = 1000000u;
  while ((getreg32(STM32N6_USART1_ISR) &
          (USART_ISR_TEACK | USART_ISR_REACK)) !=
         (USART_ISR_TEACK | USART_ISR_REACK))
    {
      if (timeout-- == 0)
        {
          break;
        }
    }

  g_usart1_initialized = true;
}

void arm_lowputc(char ch)
{
  if (!g_usart1_initialized)
    {
      return;
    }

  while ((getreg32(STM32N6_USART1_ISR) & USART_ISR_TXE_TXFNF) == 0)
    {
    }

  putreg32((uint32_t)ch, STM32N6_USART1_TDR);
}

void up_putc(int ch)
{
  arm_lowputc((char)ch);
}
