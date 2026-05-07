/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32h7rs_serial.c
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
#include <errno.h>

#include <debug.h>

#include <nuttx/irq.h>
#include <nuttx/serial/serial.h>

#include "arm_internal.h"
#include "chip.h"
#include "stm32h7rs.h"
#include "hardware/stm32h7rs_usart.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32H7RS_UART4_BASEADDR STM32H7RS_UART4_BASE
#define STM32H7RS_UART4_IRQ      STM32_IRQ_UART4
#define STM32H7RS_UART4_IE_MASK  (USART_CR1_RXNEIE_RXFNEIE | \
                                  USART_CR1_TXEIE_TXFNFIE)

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_STM32H7RS_UART4_SERIALDRIVER
struct stm32h7rs_serial_s
{
  struct uart_dev_s dev;
  uint32_t          ie;
  bool              initialized;
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_STM32H7RS_UART4_SERIALDRIVER
static int  stm32h7rs_uart4_setup(FAR struct uart_dev_s *dev);
static void stm32h7rs_uart4_shutdown(FAR struct uart_dev_s *dev);
static int  stm32h7rs_uart4_attach(FAR struct uart_dev_s *dev);
static void stm32h7rs_uart4_detach(FAR struct uart_dev_s *dev);
static int  stm32h7rs_uart4_ioctl(FAR struct file *filep, int cmd,
                                  unsigned long arg);
static int  stm32h7rs_uart4_receive(FAR struct uart_dev_s *dev,
                                    FAR unsigned int *status);
static void stm32h7rs_uart4_rxint(FAR struct uart_dev_s *dev, bool enable);
static bool stm32h7rs_uart4_rxavailable(FAR struct uart_dev_s *dev);
static void stm32h7rs_uart4_send(FAR struct uart_dev_s *dev, int ch);
static void stm32h7rs_uart4_txint(FAR struct uart_dev_s *dev, bool enable);
static bool stm32h7rs_uart4_txready(FAR struct uart_dev_s *dev);
static bool stm32h7rs_uart4_txempty(FAR struct uart_dev_s *dev);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_STM32H7RS_UART4_SERIALDRIVER
static char g_uart4rxbuffer[CONFIG_UART4_RXBUFSIZE];
static char g_uart4txbuffer[CONFIG_UART4_TXBUFSIZE];

static const struct uart_ops_s g_uart4_ops =
{
  .setup       = stm32h7rs_uart4_setup,
  .shutdown    = stm32h7rs_uart4_shutdown,
  .attach      = stm32h7rs_uart4_attach,
  .detach      = stm32h7rs_uart4_detach,
  .ioctl       = stm32h7rs_uart4_ioctl,
  .receive     = stm32h7rs_uart4_receive,
  .rxint       = stm32h7rs_uart4_rxint,
  .rxavailable = stm32h7rs_uart4_rxavailable,
  .send        = stm32h7rs_uart4_send,
  .txint       = stm32h7rs_uart4_txint,
  .txready     = stm32h7rs_uart4_txready,
  .txempty     = stm32h7rs_uart4_txempty,
};

static struct stm32h7rs_serial_s g_uart4priv =
{
  .dev =
    {
      .isconsole = true,
      .recv =
        {
          .size   = CONFIG_UART4_RXBUFSIZE,
          .buffer = g_uart4rxbuffer,
        },
      .xmit =
        {
          .size   = CONFIG_UART4_TXBUFSIZE,
          .buffer = g_uart4txbuffer,
        },
      .ops     = &g_uart4_ops,
      .priv    = &g_uart4priv,
    },
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_STM32H7RS_UART4_SERIALDRIVER
static void stm32h7rs_uart4_restoreints(FAR struct stm32h7rs_serial_s *priv)
{
  modifyreg32(STM32H7RS_UART4_CR1, STM32H7RS_UART4_IE_MASK, priv->ie);
}

static void stm32h7rs_uart4_disableints(FAR struct stm32h7rs_serial_s *priv)
{
  modifyreg32(STM32H7RS_UART4_CR1, STM32H7RS_UART4_IE_MASK, 0);
  priv->ie = 0;
}

static int stm32h7rs_uart4_interrupt(int irq, FAR void *context,
                                     FAR void *arg)
{
  FAR struct stm32h7rs_serial_s *priv =
    (FAR struct stm32h7rs_serial_s *)arg;
  uint32_t isr;
  uint32_t cr1;

  isr = getreg32(STM32H7RS_UART4_ISR);
  cr1 = getreg32(STM32H7RS_UART4_CR1);

  if ((isr & USART_ISR_ERROR_MASK) != 0)
    {
      putreg32(USART_ICR_ERROR_MASK, STM32H7RS_UART4_ICR);
    }

  if ((isr & USART_ISR_RXNE_RXFNE) != 0 &&
      (cr1 & USART_CR1_RXNEIE_RXFNEIE) != 0)
    {
      uart_recvchars(&priv->dev);
    }

  if ((isr & USART_ISR_TXE_TXFNF) != 0 &&
      (cr1 & USART_CR1_TXEIE_TXFNFIE) != 0)
    {
      uart_xmitchars(&priv->dev);
    }

  return OK;
}

static int stm32h7rs_uart4_setup(FAR struct uart_dev_s *dev)
{
  FAR struct stm32h7rs_serial_s *priv =
    (FAR struct stm32h7rs_serial_s *)dev->priv;

  stm32h7rs_lowsetup();
  putreg32(USART_ICR_ERROR_MASK | USART_ICR_TCCF, STM32H7RS_UART4_ICR);
  stm32h7rs_uart4_restoreints(priv);
  priv->initialized = true;

  return OK;
}

static void stm32h7rs_uart4_shutdown(FAR struct uart_dev_s *dev)
{
  FAR struct stm32h7rs_serial_s *priv =
    (FAR struct stm32h7rs_serial_s *)dev->priv;

  stm32h7rs_uart4_disableints(priv);
  priv->initialized = false;
}

static int stm32h7rs_uart4_attach(FAR struct uart_dev_s *dev)
{
  FAR struct stm32h7rs_serial_s *priv =
    (FAR struct stm32h7rs_serial_s *)dev->priv;
  int ret;

  ret = irq_attach(STM32H7RS_UART4_IRQ, stm32h7rs_uart4_interrupt, priv);
  if (ret < 0)
    {
      return ret;
    }

  up_enable_irq(STM32H7RS_UART4_IRQ);
  return OK;
}

static void stm32h7rs_uart4_detach(FAR struct uart_dev_s *dev)
{
  up_disable_irq(STM32H7RS_UART4_IRQ);
  irq_detach(STM32H7RS_UART4_IRQ);
}

static int stm32h7rs_uart4_ioctl(FAR struct file *filep, int cmd,
                                 unsigned long arg)
{
  return -ENOTTY;
}

static int stm32h7rs_uart4_receive(FAR struct uart_dev_s *dev,
                                   FAR unsigned int *status)
{
  uint32_t isr = getreg32(STM32H7RS_UART4_ISR);
  int ch;

  if (status != NULL)
    {
      *status = isr;
    }

  ch = getreg32(STM32H7RS_UART4_RDR) & 0xff;

  if ((isr & USART_ISR_ERROR_MASK) != 0)
    {
      putreg32(USART_ICR_ERROR_MASK, STM32H7RS_UART4_ICR);
    }

  return ch;
}

static void stm32h7rs_uart4_rxint(FAR struct uart_dev_s *dev, bool enable)
{
  FAR struct stm32h7rs_serial_s *priv =
    (FAR struct stm32h7rs_serial_s *)dev->priv;

  if (enable)
    {
      priv->ie |= USART_CR1_RXNEIE_RXFNEIE;
    }
  else
    {
      priv->ie &= ~USART_CR1_RXNEIE_RXFNEIE;
    }

  stm32h7rs_uart4_restoreints(priv);
}

static bool stm32h7rs_uart4_rxavailable(FAR struct uart_dev_s *dev)
{
  return (getreg32(STM32H7RS_UART4_ISR) & USART_ISR_RXNE_RXFNE) != 0;
}

static void stm32h7rs_uart4_send(FAR struct uart_dev_s *dev, int ch)
{
  putreg32((uint32_t)ch & 0xff, STM32H7RS_UART4_TDR);
}

static void stm32h7rs_uart4_txint(FAR struct uart_dev_s *dev, bool enable)
{
  FAR struct stm32h7rs_serial_s *priv =
    (FAR struct stm32h7rs_serial_s *)dev->priv;

  if (enable)
    {
      priv->ie |= USART_CR1_TXEIE_TXFNFIE;
      stm32h7rs_uart4_restoreints(priv);
      uart_xmitchars(dev);
    }
  else
    {
      priv->ie &= ~USART_CR1_TXEIE_TXFNFIE;
      stm32h7rs_uart4_restoreints(priv);
    }
}

static bool stm32h7rs_uart4_txready(FAR struct uart_dev_s *dev)
{
  return (getreg32(STM32H7RS_UART4_ISR) & USART_ISR_TXE_TXFNF) != 0;
}

static bool stm32h7rs_uart4_txempty(FAR struct uart_dev_s *dev)
{
  return (getreg32(STM32H7RS_UART4_ISR) & USART_ISR_TC) != 0;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void arm_earlyserialinit(void)
{
  stm32h7rs_lowsetup();
}

void arm_serialinit(void)
{
  stm32h7rs_lowsetup();

#ifdef CONFIG_STM32H7RS_UART4_SERIALDRIVER
  uart_register("/dev/console", &g_uart4priv.dev);
  uart_register("/dev/ttyS0", &g_uart4priv.dev);
#endif
}
