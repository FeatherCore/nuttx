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

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/serial/serial.h>

#include "arm_internal.h"
#include "stm32n6.h"
#include "hardware/stm32n6_memorymap.h"
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

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_STM32N6_USART1_SERIALDRIVER
struct stm32n6_usart_s
{
  uintptr_t base;
  int       irq;
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_STM32N6_USART1_SERIALDRIVER
static int  stm32n6_usart_setup(FAR struct uart_dev_s *dev);
static void stm32n6_usart_shutdown(FAR struct uart_dev_s *dev);
static int  stm32n6_usart_attach(FAR struct uart_dev_s *dev);
static void stm32n6_usart_detach(FAR struct uart_dev_s *dev);
static int  stm32n6_usart_interrupt(int irq, FAR void *context,
                                    FAR void *arg);
static int  stm32n6_usart_ioctl(FAR struct file *filep, int cmd,
                                unsigned long arg);
static int  stm32n6_usart_receive(FAR struct uart_dev_s *dev,
                                  FAR unsigned int *status);
static void stm32n6_usart_rxint(FAR struct uart_dev_s *dev, bool enable);
static bool stm32n6_usart_rxavailable(FAR struct uart_dev_s *dev);
static void stm32n6_usart_send(FAR struct uart_dev_s *dev, int ch);
static void stm32n6_usart_txint(FAR struct uart_dev_s *dev, bool enable);
static bool stm32n6_usart_txready(FAR struct uart_dev_s *dev);
static bool stm32n6_usart_txempty(FAR struct uart_dev_s *dev);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_STM32N6_USART1_SERIALDRIVER
static const struct uart_ops_s g_usart_ops =
{
  .setup       = stm32n6_usart_setup,
  .shutdown    = stm32n6_usart_shutdown,
  .attach      = stm32n6_usart_attach,
  .detach      = stm32n6_usart_detach,
  .ioctl       = stm32n6_usart_ioctl,
  .receive     = stm32n6_usart_receive,
  .rxint       = stm32n6_usart_rxint,
  .rxavailable = stm32n6_usart_rxavailable,
  .send        = stm32n6_usart_send,
  .txint       = stm32n6_usart_txint,
  .txready     = stm32n6_usart_txready,
  .txempty     = stm32n6_usart_txempty,
};

static char g_usart1_rxbuffer[CONFIG_USART1_RXBUFSIZE];
static char g_usart1_txbuffer[CONFIG_USART1_TXBUFSIZE];

static struct stm32n6_usart_s g_usart1_priv =
{
  .base = STM32N6_USART1_BASE,
  .irq  = STM32_IRQ_USART1,
};

static uart_dev_t g_usart1_dev =
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
  .ops  = &g_usart_ops,
  .priv = &g_usart1_priv,
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_STM32N6_USART1_SERIALDRIVER
static uintptr_t stm32n6_usart_reg(FAR struct stm32n6_usart_s *priv,
                                   uint32_t offset)
{
  return priv->base + offset;
}

static int stm32n6_usart_setup(FAR struct uart_dev_s *dev)
{
  stm32n6_lowsetup();
  return OK;
}

static void stm32n6_usart_shutdown(FAR struct uart_dev_s *dev)
{
  FAR struct stm32n6_usart_s *priv = dev->priv;

  modifyreg32(stm32n6_usart_reg(priv, STM32N6_USART_CR1_OFFSET),
              USART_CR1_RXNEIE_RXFNEIE | USART_CR1_TXEIE_TXFNFIE,
              0);
}

static int stm32n6_usart_attach(FAR struct uart_dev_s *dev)
{
  FAR struct stm32n6_usart_s *priv = dev->priv;
  int ret;

  ret = irq_attach(priv->irq, stm32n6_usart_interrupt, dev);
  if (ret >= 0)
    {
      up_enable_irq(priv->irq);
    }

  return ret;
}

static void stm32n6_usart_detach(FAR struct uart_dev_s *dev)
{
  FAR struct stm32n6_usart_s *priv = dev->priv;

  up_disable_irq(priv->irq);
  irq_detach(priv->irq);
}

static int stm32n6_usart_interrupt(int irq, FAR void *context,
                                   FAR void *arg)
{
  FAR struct uart_dev_s *dev = arg;
  FAR struct stm32n6_usart_s *priv = dev->priv;
  uintptr_t cr1 = stm32n6_usart_reg(priv, STM32N6_USART_CR1_OFFSET);
  uintptr_t isr = stm32n6_usart_reg(priv, STM32N6_USART_ISR_OFFSET);
  uintptr_t icr = stm32n6_usart_reg(priv, STM32N6_USART_ICR_OFFSET);
  uint32_t status;

  status = getreg32(isr);
  if ((status & USART_ISR_ERROR_MASK) != 0)
    {
      putreg32(USART_ICR_ERROR_MASK, icr);
    }

  if ((status & USART_ISR_RXNE_RXFNE) != 0)
    {
      uart_recvchars(dev);
    }

  if ((status & USART_ISR_TXE_TXFNF) != 0 &&
      (getreg32(cr1) & USART_CR1_TXEIE_TXFNFIE) != 0)
    {
      uart_xmitchars(dev);
    }

  return OK;
}

static int stm32n6_usart_ioctl(FAR struct file *filep, int cmd,
                               unsigned long arg)
{
  return -ENOTTY;
}

static int stm32n6_usart_receive(FAR struct uart_dev_s *dev,
                                 FAR unsigned int *status)
{
  FAR struct stm32n6_usart_s *priv = dev->priv;
  uint32_t regval;

  regval = getreg32(stm32n6_usart_reg(priv, STM32N6_USART_ISR_OFFSET));
  if (status != NULL)
    {
      *status = regval;
    }

  return getreg32(stm32n6_usart_reg(priv, STM32N6_USART_RDR_OFFSET)) & 0xff;
}

static void stm32n6_usart_rxint(FAR struct uart_dev_s *dev, bool enable)
{
  FAR struct stm32n6_usart_s *priv = dev->priv;

  modifyreg32(stm32n6_usart_reg(priv, STM32N6_USART_CR1_OFFSET),
              USART_CR1_RXNEIE_RXFNEIE,
              enable ? USART_CR1_RXNEIE_RXFNEIE : 0);

  if (enable && stm32n6_usart_rxavailable(dev))
    {
      uart_recvchars(dev);
    }
}

static bool stm32n6_usart_rxavailable(FAR struct uart_dev_s *dev)
{
  FAR struct stm32n6_usart_s *priv = dev->priv;

  return (getreg32(stm32n6_usart_reg(priv, STM32N6_USART_ISR_OFFSET)) &
          USART_ISR_RXNE_RXFNE) != 0;
}

static void stm32n6_usart_send(FAR struct uart_dev_s *dev, int ch)
{
  FAR struct stm32n6_usart_s *priv = dev->priv;

  putreg32((uint32_t)ch & 0xff,
           stm32n6_usart_reg(priv, STM32N6_USART_TDR_OFFSET));
}

static void stm32n6_usart_txint(FAR struct uart_dev_s *dev, bool enable)
{
  FAR struct stm32n6_usart_s *priv = dev->priv;

  modifyreg32(stm32n6_usart_reg(priv, STM32N6_USART_CR1_OFFSET),
              USART_CR1_TXEIE_TXFNFIE,
              enable ? USART_CR1_TXEIE_TXFNFIE : 0);

  if (enable)
    {
      uart_xmitchars(dev);
    }
}

static bool stm32n6_usart_txready(FAR struct uart_dev_s *dev)
{
  FAR struct stm32n6_usart_s *priv = dev->priv;

  return (getreg32(stm32n6_usart_reg(priv, STM32N6_USART_ISR_OFFSET)) &
          USART_ISR_TXE_TXFNF) != 0;
}

static bool stm32n6_usart_txempty(FAR struct uart_dev_s *dev)
{
  FAR struct stm32n6_usart_s *priv = dev->priv;

  return (getreg32(stm32n6_usart_reg(priv, STM32N6_USART_ISR_OFFSET)) &
          USART_ISR_TC) != 0;
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
#ifdef CONFIG_USART1_SERIAL_CONSOLE
  uart_register("/dev/console", &g_usart1_dev);
#endif
  uart_register("/dev/ttyS0", &g_usart1_dev);
#endif
}
