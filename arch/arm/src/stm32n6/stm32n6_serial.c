/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_serial.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/serial/serial.h>

#include <arch/armv8-m/nvicpri.h>

#include "arm_internal.h"
#include "chip.h"
#include "stm32n6.h"
#include "hardware/stm32n6_usart.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_USART1_RXBUFSIZE
#  define CONFIG_USART1_RXBUFSIZE 256
#endif

#ifndef CONFIG_USART1_TXBUFSIZE
#  define CONFIG_USART1_TXBUFSIZE 256
#endif

#define STM32N6_USART1_IE_MASK  (USART_CR1_RXNEIE_RXFNEIE | \
                                 USART_CR1_TXEIE_TXFNFIE)

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_STM32N6_USART1_SERIALDRIVER
struct stm32n6_serial_s
{
  struct uart_dev_s dev;
  uint32_t          ie;
  bool              initialized;
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_STM32N6_USART1_SERIALDRIVER
static int  stm32n6_usart1_setup(FAR struct uart_dev_s *dev);
static void stm32n6_usart1_shutdown(FAR struct uart_dev_s *dev);
static int  stm32n6_usart1_attach(FAR struct uart_dev_s *dev);
static void stm32n6_usart1_detach(FAR struct uart_dev_s *dev);
static int  stm32n6_usart1_ioctl(FAR struct file *filep, int cmd,
                                 unsigned long arg);
static int  stm32n6_usart1_receive(FAR struct uart_dev_s *dev,
                                   FAR unsigned int *status);
static void stm32n6_usart1_rxint(FAR struct uart_dev_s *dev, bool enable);
static bool stm32n6_usart1_rxavailable(FAR struct uart_dev_s *dev);
static void stm32n6_usart1_send(FAR struct uart_dev_s *dev, int ch);
static void stm32n6_usart1_txint(FAR struct uart_dev_s *dev, bool enable);
static bool stm32n6_usart1_txready(FAR struct uart_dev_s *dev);
static bool stm32n6_usart1_txempty(FAR struct uart_dev_s *dev);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_STM32N6_USART1_SERIALDRIVER
static char g_usart1_rxbuffer[CONFIG_USART1_RXBUFSIZE];
static char g_usart1_txbuffer[CONFIG_USART1_TXBUFSIZE];

static const struct uart_ops_s g_usart1_ops =
{
  .setup       = stm32n6_usart1_setup,
  .shutdown    = stm32n6_usart1_shutdown,
  .attach      = stm32n6_usart1_attach,
  .detach      = stm32n6_usart1_detach,
  .ioctl       = stm32n6_usart1_ioctl,
  .receive     = stm32n6_usart1_receive,
  .rxint       = stm32n6_usart1_rxint,
  .rxavailable = stm32n6_usart1_rxavailable,
  .send        = stm32n6_usart1_send,
  .txint       = stm32n6_usart1_txint,
  .txready     = stm32n6_usart1_txready,
  .txempty     = stm32n6_usart1_txempty,
};

static struct stm32n6_serial_s g_usart1priv =
{
  .dev =
    {
#ifdef CONFIG_USART1_SERIAL_CONSOLE
      .isconsole = true,
#endif
      .recv =
        {
          .size   = CONFIG_USART1_RXBUFSIZE,
          .buffer = g_usart1_rxbuffer,
        },
      .xmit =
        {
          .size   = CONFIG_USART1_TXBUFSIZE,
          .buffer = g_usart1_txbuffer,
        },
      .ops     = &g_usart1_ops,
      .priv    = &g_usart1priv,
    },
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_STM32N6_USART1_SERIALDRIVER
static void stm32n6_usart1_restoreints(FAR struct stm32n6_serial_s *priv)
{
  modifyreg32(STM32N6_USART1_CR1, STM32N6_USART1_IE_MASK, priv->ie);
}

static void stm32n6_usart1_disableints(FAR struct stm32n6_serial_s *priv)
{
  modifyreg32(STM32N6_USART1_CR1, STM32N6_USART1_IE_MASK, 0);
  priv->ie = 0;
}

static int stm32n6_usart1_interrupt(int irq, FAR void *context,
                                    FAR void *arg)
{
  FAR struct stm32n6_serial_s *priv =
    (FAR struct stm32n6_serial_s *)arg;
  uint32_t isr;
  int passes;
  bool handled;

  DEBUGASSERT(priv != NULL);

  handled = true;
  for (passes = 0; passes < 256 && handled; passes++)
    {
      handled = false;
      isr = getreg32(STM32N6_USART1_ISR);

      if ((isr & USART_ISR_RXNE_RXFNE) != 0 &&
          (priv->ie & USART_CR1_RXNEIE_RXFNEIE) != 0)
        {
          uart_recvchars(&priv->dev);
          handled = true;
        }
      else if ((isr & USART_ISR_ERROR_MASK) != 0)
        {
          putreg32(USART_ICR_ERROR_MASK, STM32N6_USART1_ICR);
        }

      if ((isr & USART_ISR_TXE_TXFNF) != 0 &&
          (priv->ie & USART_CR1_TXEIE_TXFNFIE) != 0)
        {
          uart_xmitchars(&priv->dev);
          handled = true;
        }
    }

  return OK;
}

static int stm32n6_usart1_setup(FAR struct uart_dev_s *dev)
{
  FAR struct stm32n6_serial_s *priv =
    (FAR struct stm32n6_serial_s *)dev->priv;

  stm32n6_lowsetup();
  putreg32(USART_ICR_ERROR_MASK | USART_ICR_TCCF, STM32N6_USART1_ICR);
  stm32n6_usart1_restoreints(priv);
  priv->initialized = true;

  return OK;
}

static void stm32n6_usart1_shutdown(FAR struct uart_dev_s *dev)
{
  FAR struct stm32n6_serial_s *priv =
    (FAR struct stm32n6_serial_s *)dev->priv;

  stm32n6_usart1_disableints(priv);
  priv->initialized = false;
}

static int stm32n6_usart1_attach(FAR struct uart_dev_s *dev)
{
  FAR struct stm32n6_serial_s *priv =
    (FAR struct stm32n6_serial_s *)dev->priv;
  int ret;

  ret = irq_attach(STM32_IRQ_USART1, stm32n6_usart1_interrupt, priv);
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_STM32N6_USART1_HIGH_PRIORITY
  ret = up_prioritize_irq(STM32_IRQ_USART1, NVIC_SYSH_HIGH_PRIORITY);
  if (ret < 0)
    {
      return ret;
    }
#endif

  up_enable_irq(STM32_IRQ_USART1);
  return OK;
}

static void stm32n6_usart1_detach(FAR struct uart_dev_s *dev)
{
  up_disable_irq(STM32_IRQ_USART1);
  irq_detach(STM32_IRQ_USART1);
}

static int stm32n6_usart1_ioctl(FAR struct file *filep, int cmd,
                                unsigned long arg)
{
  return -ENOTTY;
}

static int stm32n6_usart1_receive(FAR struct uart_dev_s *dev,
                                  FAR unsigned int *status)
{
  uint32_t isr = getreg32(STM32N6_USART1_ISR);
  int ch;

  if (status != NULL)
    {
      *status = isr;
    }

  ch = getreg32(STM32N6_USART1_RDR) & 0xff;

  if ((isr & USART_ISR_ERROR_MASK) != 0)
    {
      putreg32(USART_ICR_ERROR_MASK, STM32N6_USART1_ICR);
    }

  return ch;
}

static void stm32n6_usart1_rxint(FAR struct uart_dev_s *dev, bool enable)
{
  FAR struct stm32n6_serial_s *priv =
    (FAR struct stm32n6_serial_s *)dev->priv;
  irqstate_t flags;

  flags = enter_critical_section();
  if (enable)
    {
      priv->ie |= USART_CR1_RXNEIE_RXFNEIE;
    }
  else
    {
      priv->ie &= ~USART_CR1_RXNEIE_RXFNEIE;
    }

  stm32n6_usart1_restoreints(priv);
  leave_critical_section(flags);
}

static bool stm32n6_usart1_rxavailable(FAR struct uart_dev_s *dev)
{
  return (getreg32(STM32N6_USART1_ISR) & USART_ISR_RXNE_RXFNE) != 0;
}

static void stm32n6_usart1_send(FAR struct uart_dev_s *dev, int ch)
{
  putreg32((uint32_t)ch & 0xff, STM32N6_USART1_TDR);
}

#ifdef CONFIG_STM32N6_USART1_TX_POLL_DRAIN
static void stm32n6_usart1_txpoll(FAR struct uart_dev_s *dev)
{
  while (dev->xmit.head != dev->xmit.tail)
    {
      while (!stm32n6_usart1_txready(dev))
        {
        }

      uart_xmitchars(dev);
    }
}
#endif

static void stm32n6_usart1_txint(FAR struct uart_dev_s *dev, bool enable)
{
  FAR struct stm32n6_serial_s *priv =
    (FAR struct stm32n6_serial_s *)dev->priv;
  irqstate_t flags;

  flags = enter_critical_section();
  if (enable)
    {
      priv->ie |= USART_CR1_TXEIE_TXFNFIE;
      stm32n6_usart1_restoreints(priv);
#ifdef CONFIG_STM32N6_USART1_TX_POLL_DRAIN
      stm32n6_usart1_txpoll(dev);
#else
      uart_xmitchars(dev);
#endif
    }
  else
    {
      priv->ie &= ~USART_CR1_TXEIE_TXFNFIE;
      stm32n6_usart1_restoreints(priv);
    }

  leave_critical_section(flags);
}

static bool stm32n6_usart1_txready(FAR struct uart_dev_s *dev)
{
  return (getreg32(STM32N6_USART1_ISR) & USART_ISR_TXE_TXFNF) != 0;
}

static bool stm32n6_usart1_txempty(FAR struct uart_dev_s *dev)
{
  return (getreg32(STM32N6_USART1_ISR) & USART_ISR_TC) != 0;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void arm_earlyserialinit(void)
{
  stm32n6_lowsetup();
}

void arm_serialinit(void)
{
  stm32n6_lowsetup();

#ifdef CONFIG_STM32N6_USART1_SERIALDRIVER
  uart_register("/dev/console", &g_usart1priv.dev);
  uart_register("/dev/ttyS0", &g_usart1priv.dev);
#endif
}
