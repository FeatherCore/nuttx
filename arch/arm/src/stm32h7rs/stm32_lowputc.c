/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32_lowputc.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "stm32.h"

#include "hardware/stm32_gpio.h"
#include "hardware/stm32_rcc.h"
#include "hardware/stm32_usart.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define STM32_UART4_TXDONE_TIMEOUT 1000000u

static bool g_uart4_initialized;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32_uart4_gpio_config(void)
{
  uint32_t regval;

  modifyreg32(STM32_RCC_AHB4ENR, 0, RCC_AHB4ENR_GPIODEN);
  (void)getreg32(STM32_RCC_AHB4ENR);

  regval = getreg32(STM32_GPIOD_MODER);
  regval &= ~(GPIO_MODE_MASK(0) | GPIO_MODE_MASK(1));
  regval |= (GPIO_MODE_ALT << GPIO_MODE_SHIFT(0)) |
            (GPIO_MODE_ALT << GPIO_MODE_SHIFT(1));
  putreg32(regval, STM32_GPIOD_MODER);

  regval = getreg32(STM32_GPIOD_OTYPER);
  regval &= ~((1u << 0) | (1u << 1));
  putreg32(regval, STM32_GPIOD_OTYPER);

  regval = getreg32(STM32_GPIOD_OSPEEDR);
  regval &= ~(GPIO_SPEED_MASK(0) | GPIO_SPEED_MASK(1));
  regval |= (GPIO_SPEED_HIGH << GPIO_SPEED_SHIFT(0)) |
            (GPIO_SPEED_HIGH << GPIO_SPEED_SHIFT(1));
  putreg32(regval, STM32_GPIOD_OSPEEDR);

  regval = getreg32(STM32_GPIOD_PUPDR);
  regval &= ~(GPIO_PUPD_MASK(0) | GPIO_PUPD_MASK(1));
  regval |= (GPIO_PULLUP << GPIO_PUPD_SHIFT(0)) |
            (GPIO_PULLUP << GPIO_PUPD_SHIFT(1));
  putreg32(regval, STM32_GPIOD_PUPDR);

  regval = getreg32(STM32_GPIOD_AFRL);
  regval &= ~(GPIO_AFR_MASK(0) | GPIO_AFR_MASK(1));
  regval |= (GPIO_AF_UART4 << GPIO_AFR_SHIFT(0)) |
            (GPIO_AF_UART4 << GPIO_AFR_SHIFT(1));
  putreg32(regval, STM32_GPIOD_AFRL);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void stm32_lowsetup(void)
{
  uint32_t brr;

  stm32_uart4_wait_txcomplete();
  stm32_uart4_gpio_config();

  modifyreg32(STM32_RCC_APB1ENR1, 0, RCC_APB1ENR1_UART4EN);
  (void)getreg32(STM32_RCC_APB1ENR1);

  putreg32(0, STM32_UART4_CR1);
  putreg32(0, STM32_UART4_CR2);
  putreg32(0, STM32_UART4_CR3);
  putreg32(0, STM32_UART4_PRESC);
  putreg32(USART_ICR_TCCF, STM32_UART4_ICR);

  brr = (STM32_PCLK1_FREQUENCY + (STM32_UART4_BAUD / 2)) /
        STM32_UART4_BAUD;
  putreg32(brr & USART_BRR_MASK, STM32_UART4_BRR);

  putreg32(USART_CR1_UE | USART_CR1_TE | USART_CR1_RE, STM32_UART4_CR1);
  g_uart4_initialized = true;
}

void stm32_uart4_wait_txcomplete(void)
{
  uint32_t timeout = STM32_UART4_TXDONE_TIMEOUT;

  if ((getreg32(STM32_RCC_APB1ENR1) & RCC_APB1ENR1_UART4EN) == 0)
    {
      return;
    }

  if ((getreg32(STM32_UART4_CR1) & (USART_CR1_UE | USART_CR1_TE)) !=
      (USART_CR1_UE | USART_CR1_TE))
    {
      return;
    }

  while ((getreg32(STM32_UART4_ISR) & USART_ISR_TC) == 0 &&
         timeout-- > 0)
    {
    }
}

void arm_lowputc(char ch)
{
  if (!g_uart4_initialized)
    {
      return;
    }

  while ((getreg32(STM32_UART4_ISR) & USART_ISR_TXE_TXFNF) == 0)
    {
    }

  putreg32((uint32_t)ch, STM32_UART4_TDR);
}

void up_putc(int ch)
{
  arm_lowputc((char)ch);
}
