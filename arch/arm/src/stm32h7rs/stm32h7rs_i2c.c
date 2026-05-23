/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32h7rs_i2c.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <nuttx/arch.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/mutex.h>

#include "arm_internal.h"

#include "hardware/stm32h7rs_gpio.h"
#include "hardware/stm32h7rs_i2c.h"
#include "hardware/stm32h7rs_memorymap.h"
#include "hardware/stm32h7rs_rcc.h"

#include "stm32h7rs_i2c.h"

#ifdef CONFIG_STM32H7RS_I2C

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_STM32H7RS_I2CTIMEO
#  define CONFIG_STM32H7RS_I2CTIMEO 2000000
#endif

#ifndef CONFIG_STM32H7RS_I2C1_TIMING
/* CubeH7RS STM32H7S78-DK I2C1 100 kHz timing with PCLK1 at 150 MHz. */

#  define CONFIG_STM32H7RS_I2C1_TIMING 0x00e063ff
#endif

#define STM32H7RS_I2C_CR2_CLEAR_MASK \
  (I2C_CR2_SADD7_MASK | I2C_CR2_RD_WRN | I2C_CR2_ADD10 | \
   I2C_CR2_START | I2C_CR2_STOP | I2C_CR2_NBYTES_MASK | \
   I2C_CR2_RELOAD | I2C_CR2_AUTOEND)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32h7rs_i2c_priv_s
{
  struct i2c_master_s dev;
  mutex_t lock;
  uintptr_t base;
  uint32_t timing;
  uint8_t refs;
  bool initialized;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32h7rs_i2c_transfer(FAR struct i2c_master_s *dev,
                                  FAR struct i2c_msg_s *msgs, int count);
static int stm32h7rs_i2c_setup(FAR struct i2c_master_s *dev);
static int stm32h7rs_i2c_shutdown(FAR struct i2c_master_s *dev);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct i2c_ops_s g_i2c_ops =
{
  .transfer = stm32h7rs_i2c_transfer,
  .setup    = stm32h7rs_i2c_setup,
  .shutdown = stm32h7rs_i2c_shutdown,
};

#ifdef CONFIG_STM32H7RS_I2C1
static struct stm32h7rs_i2c_priv_s g_i2c1 =
{
  .dev =
    {
      .ops = &g_i2c_ops,
    },
  .lock   = NXMUTEX_INITIALIZER,
  .base   = STM32H7RS_I2C1_BASE,
  .timing = CONFIG_STM32H7RS_I2C1_TIMING,
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32h7rs_i2c_config_gpio(void)
{
  const uint32_t pins = (1u << 6) | (1u << 9);
  uint32_t regval;
  int pin;

  modifyreg32(STM32H7RS_RCC_AHB4ENR, 0, RCC_AHB4ENR_GPIOBEN);
  (void)getreg32(STM32H7RS_RCC_AHB4ENR);

  regval = getreg32(STM32H7RS_GPIOB_MODER);
  for (pin = 0; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_MODE_MASK(pin);
          regval |= GPIO_MODE_ALT << GPIO_MODE_SHIFT(pin);
        }
    }

  putreg32(regval, STM32H7RS_GPIOB_MODER);

  modifyreg32(STM32H7RS_GPIOB_OTYPER, 0, pins);

  regval = getreg32(STM32H7RS_GPIOB_OSPEEDR);
  for (pin = 0; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_SPEED_MASK(pin);
          regval |= GPIO_SPEED_VERYHIGH << GPIO_SPEED_SHIFT(pin);
        }
    }

  putreg32(regval, STM32H7RS_GPIOB_OSPEEDR);

  regval = getreg32(STM32H7RS_GPIOB_PUPDR);
  for (pin = 0; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_PUPD_MASK(pin);
          regval |= GPIO_PULLUP << GPIO_PUPD_SHIFT(pin);
        }
    }

  putreg32(regval, STM32H7RS_GPIOB_PUPDR);

  regval = getreg32(STM32H7RS_GPIOB_AFRL);
  regval &= ~GPIO_AFR_MASK(6);
  regval |= GPIO_AF_I2C1 << GPIO_AFR_SHIFT(6);
  putreg32(regval, STM32H7RS_GPIOB_AFRL);

  regval = getreg32(STM32H7RS_GPIOB_AFRH);
  regval &= ~GPIO_AFR_MASK(9);
  regval |= GPIO_AF_I2C1 << GPIO_AFR_SHIFT(9);
  putreg32(regval, STM32H7RS_GPIOB_AFRH);
}

static void stm32h7rs_i2c_clear(FAR struct stm32h7rs_i2c_priv_s *priv)
{
  putreg32(I2C_ICR_CLEARMASK, STM32H7RS_I2C_ICR(priv->base));
}

static int stm32h7rs_i2c_wait(FAR struct stm32h7rs_i2c_priv_s *priv,
                              uint32_t mask, bool stop_ok)
{
  uint32_t timeout = CONFIG_STM32H7RS_I2CTIMEO;
  uint32_t isr;

  do
    {
      isr = getreg32(STM32H7RS_I2C_ISR(priv->base));

      if ((isr & I2C_ISR_NACKF) != 0)
        {
          stm32h7rs_i2c_clear(priv);
          return -ENXIO;
        }

      if (!stop_ok && (isr & I2C_ISR_STOPF) != 0)
        {
          stm32h7rs_i2c_clear(priv);
          return -EIO;
        }

      if ((isr & I2C_ISR_ERRORMASK) != 0)
        {
          stm32h7rs_i2c_clear(priv);
          return -EIO;
        }

      if ((isr & mask) != 0)
        {
          return OK;
        }
    }
  while (timeout-- > 0);

  return -ETIMEDOUT;
}

static int stm32h7rs_i2c_wait_bus_free(FAR struct stm32h7rs_i2c_priv_s *priv)
{
  uint32_t timeout = CONFIG_STM32H7RS_I2CTIMEO;

  while ((getreg32(STM32H7RS_I2C_ISR(priv->base)) & I2C_ISR_BUSY) != 0)
    {
      if (timeout-- == 0)
        {
          return -ETIMEDOUT;
        }
    }

  return OK;
}

static void stm32h7rs_i2c_stop(FAR struct stm32h7rs_i2c_priv_s *priv)
{
  modifyreg32(STM32H7RS_I2C_CR2(priv->base), 0, I2C_CR2_STOP);
}

static int stm32h7rs_i2c_wait_stop(FAR struct stm32h7rs_i2c_priv_s *priv)
{
  int ret;

  ret = stm32h7rs_i2c_wait(priv, I2C_ISR_STOPF, true);
  stm32h7rs_i2c_clear(priv);
  return ret;
}

static int stm32h7rs_i2c_clock_enable(FAR struct stm32h7rs_i2c_priv_s *priv)
{
#ifdef CONFIG_STM32H7RS_I2C1
  if (priv->base == STM32H7RS_I2C1_BASE)
    {
      modifyreg32(STM32H7RS_RCC_CCIPR2, 0x3000u, 0);
      modifyreg32(STM32H7RS_RCC_APB1ENR1, 0, RCC_APB1ENR1_I2C1EN);
      (void)getreg32(STM32H7RS_RCC_APB1ENR1);

      modifyreg32(STM32H7RS_RCC_APB1RSTR1, 0, RCC_APB1RSTR1_I2C1RST);
      up_udelay(1);
      modifyreg32(STM32H7RS_RCC_APB1RSTR1, RCC_APB1RSTR1_I2C1RST, 0);
      return OK;
    }
#endif

  return -ENODEV;
}

static void stm32h7rs_i2c_clock_disable(FAR struct stm32h7rs_i2c_priv_s *priv)
{
#ifdef CONFIG_STM32H7RS_I2C1
  if (priv->base == STM32H7RS_I2C1_BASE)
    {
      modifyreg32(STM32H7RS_RCC_APB1ENR1, RCC_APB1ENR1_I2C1EN, 0);
    }
#endif
}

static int stm32h7rs_i2c_hw_init(FAR struct stm32h7rs_i2c_priv_s *priv)
{
  int ret;

  if (priv->initialized)
    {
      return OK;
    }

  stm32h7rs_i2c_config_gpio();

  ret = stm32h7rs_i2c_clock_enable(priv);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32H7RS_I2C_CR1(priv->base), I2C_CR1_PE, 0);
  putreg32(priv->timing, STM32H7RS_I2C_TIMINGR(priv->base));
  putreg32(0, STM32H7RS_I2C_TIMEOUTR(priv->base));
  putreg32(0, STM32H7RS_I2C_OAR1(priv->base));
  putreg32(0, STM32H7RS_I2C_OAR2(priv->base));
  stm32h7rs_i2c_clear(priv);

  putreg32(I2C_CR1_PE, STM32H7RS_I2C_CR1(priv->base));

  priv->initialized = true;
  return OK;
}

static int stm32h7rs_i2c_hw_deinit(FAR struct stm32h7rs_i2c_priv_s *priv)
{
  if (priv->initialized)
    {
      modifyreg32(STM32H7RS_I2C_CR1(priv->base), I2C_CR1_PE, 0);
      stm32h7rs_i2c_clock_disable(priv);
      priv->initialized = false;
    }

  return OK;
}

static int stm32h7rs_i2c_start_msg(FAR struct stm32h7rs_i2c_priv_s *priv,
                                   FAR struct i2c_msg_s *msg)
{
  uint32_t cr2;

  if ((msg->flags & (I2C_M_TEN | I2C_M_NOSTART)) != 0 ||
      msg->addr > 0x7f || msg->length < 0 || msg->length > 255)
    {
      return -EINVAL;
    }

  cr2 = ((uint32_t)msg->addr << I2C_CR2_SADD7_SHIFT) |
        I2C_CR2_NBYTES(msg->length) | I2C_CR2_START;

  if ((msg->flags & I2C_M_READ) != 0)
    {
      cr2 |= I2C_CR2_RD_WRN;
    }

  modifyreg32(STM32H7RS_I2C_CR2(priv->base),
              STM32H7RS_I2C_CR2_CLEAR_MASK, cr2);
  return OK;
}

static int stm32h7rs_i2c_transfer_msg(FAR struct stm32h7rs_i2c_priv_s *priv,
                                      FAR struct i2c_msg_s *msg)
{
  ssize_t i;
  int ret;

  ret = stm32h7rs_i2c_start_msg(priv, msg);
  if (ret < 0)
    {
      return ret;
    }

  if ((msg->flags & I2C_M_READ) != 0)
    {
      for (i = 0; i < msg->length; i++)
        {
          ret = stm32h7rs_i2c_wait(priv, I2C_ISR_RXNE, false);
          if (ret < 0)
            {
              return ret;
            }

          msg->buffer[i] =
            getreg32(STM32H7RS_I2C_RXDR(priv->base)) & I2C_RXDR_MASK;
        }
    }
  else
    {
      for (i = 0; i < msg->length; i++)
        {
          ret = stm32h7rs_i2c_wait(priv, I2C_ISR_TXIS, false);
          if (ret < 0)
            {
              return ret;
            }

          putreg32(msg->buffer[i] & I2C_TXDR_MASK,
                   STM32H7RS_I2C_TXDR(priv->base));
        }
    }

  return stm32h7rs_i2c_wait(priv, I2C_ISR_TC, false);
}

static int stm32h7rs_i2c_transfer_locked(FAR struct stm32h7rs_i2c_priv_s *priv,
                                         FAR struct i2c_msg_s *msgs,
                                         int count)
{
  int ret;
  int i;

  ret = stm32h7rs_i2c_hw_init(priv);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_i2c_wait_bus_free(priv);
  if (ret < 0)
    {
      return ret;
    }

  for (i = 0; i < count; i++)
    {
      ret = stm32h7rs_i2c_transfer_msg(priv, &msgs[i]);
      if (ret < 0)
        {
          stm32h7rs_i2c_stop(priv);
          (void)stm32h7rs_i2c_wait_stop(priv);
          return ret;
        }

      if ((msgs[i].flags & I2C_M_NOSTOP) == 0 &&
          (i + 1 >= count || (msgs[i + 1].flags & I2C_M_NOSTART) == 0))
        {
          stm32h7rs_i2c_stop(priv);
          ret = stm32h7rs_i2c_wait_stop(priv);
          if (ret < 0)
            {
              return ret;
            }
        }
    }

  return OK;
}

static int stm32h7rs_i2c_transfer(FAR struct i2c_master_s *dev,
                                  FAR struct i2c_msg_s *msgs, int count)
{
  FAR struct stm32h7rs_i2c_priv_s *priv =
    (FAR struct stm32h7rs_i2c_priv_s *)dev;
  int ret;

  DEBUGASSERT(priv != NULL && msgs != NULL && count > 0);

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_i2c_transfer_locked(priv, msgs, count);
  nxmutex_unlock(&priv->lock);
  return ret;
}

static int stm32h7rs_i2c_setup(FAR struct i2c_master_s *dev)
{
  FAR struct stm32h7rs_i2c_priv_s *priv =
    (FAR struct stm32h7rs_i2c_priv_s *)dev;
  int ret;

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_i2c_hw_init(priv);
  nxmutex_unlock(&priv->lock);
  return ret;
}

static int stm32h7rs_i2c_shutdown(FAR struct i2c_master_s *dev)
{
  FAR struct stm32h7rs_i2c_priv_s *priv =
    (FAR struct stm32h7rs_i2c_priv_s *)dev;
  int ret;

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_i2c_hw_deinit(priv);
  nxmutex_unlock(&priv->lock);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

FAR struct i2c_master_s *stm32h7rs_i2cbus_initialize(int port)
{
  FAR struct stm32h7rs_i2c_priv_s *priv = NULL;
  int ret;

  switch (port)
    {
#ifdef CONFIG_STM32H7RS_I2C1
      case 1:
        priv = &g_i2c1;
        break;
#endif
      default:
        return NULL;
    }

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return NULL;
    }

  if (priv->refs == 0)
    {
      ret = stm32h7rs_i2c_hw_init(priv);
      if (ret < 0)
        {
          nxmutex_unlock(&priv->lock);
          return NULL;
        }
    }

  priv->refs++;
  nxmutex_unlock(&priv->lock);

  return &priv->dev;
}

int stm32h7rs_i2cbus_uninitialize(FAR struct i2c_master_s *dev)
{
  FAR struct stm32h7rs_i2c_priv_s *priv =
    (FAR struct stm32h7rs_i2c_priv_s *)dev;
  int ret = OK;

  if (priv == NULL)
    {
      return -EINVAL;
    }

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  if (priv->refs > 0)
    {
      priv->refs--;
    }

  if (priv->refs == 0)
    {
      ret = stm32h7rs_i2c_hw_deinit(priv);
    }

  nxmutex_unlock(&priv->lock);
  return ret;
}

#endif /* CONFIG_STM32H7RS_I2C */
