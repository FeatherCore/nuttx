/****************************************************************************
 * arch/arm/src/stm32n6/stm32_i2c.c
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

#include "hardware/stm32_i2c.h"
#include "hardware/stm32n6xxx_memorymap.h"
#include "hardware/stm32n6xxx_rcc.h"

#include "stm32_i2c.h"

#ifdef CONFIG_STM32N6_I2C

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_STM32N6_I2CTIMEO
#  define CONFIG_STM32N6_I2CTIMEO 2000000
#endif

#ifndef CONFIG_STM32N6_I2C2_TIMING
/* CubeN6 I2C_GetTiming(PCLK1=200 MHz, 100 kHz), analog filter on, DNF=0. */

#  define CONFIG_STM32N6_I2C2_TIMING 0xe0b03c3d
#endif

#define STM32_I2C_CR2_CLEAR_MASK \
  (I2C_CR2_SADD7_MASK | I2C_CR2_RD_WRN | I2C_CR2_ADD10 | \
   I2C_CR2_START | I2C_CR2_STOP | I2C_CR2_NBYTES_MASK | \
   I2C_CR2_RELOAD | I2C_CR2_AUTOEND)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32_i2c_priv_s
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

static int stm32_i2c_transfer(FAR struct i2c_master_s *dev,
                              FAR struct i2c_msg_s *msgs, int count);
static int stm32_i2c_setup(FAR struct i2c_master_s *dev);
static int stm32_i2c_shutdown(FAR struct i2c_master_s *dev);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct i2c_ops_s g_i2c_ops =
{
  .transfer = stm32_i2c_transfer,
  .setup    = stm32_i2c_setup,
  .shutdown = stm32_i2c_shutdown,
};

#ifdef CONFIG_STM32N6_I2C2
static struct stm32_i2c_priv_s g_i2c2 =
{
  .dev =
    {
      .ops = &g_i2c_ops,
    },
  .lock   = NXMUTEX_INITIALIZER,
  .base   = STM32_I2C2_BASE,
  .timing = CONFIG_STM32N6_I2C2_TIMING,
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32_i2c_clear(FAR struct stm32_i2c_priv_s *priv)
{
  putreg32(I2C_ICR_CLEARMASK, STM32_I2C_ICR(priv->base));
}

static int stm32_i2c_wait(FAR struct stm32_i2c_priv_s *priv,
                          uint32_t mask, bool stop_ok)
{
  uint32_t timeout = CONFIG_STM32N6_I2CTIMEO;
  uint32_t isr;

  do
    {
      isr = getreg32(STM32_I2C_ISR(priv->base));

      if ((isr & I2C_ISR_NACKF) != 0)
        {
          stm32_i2c_clear(priv);
          return -ENXIO;
        }

      if (!stop_ok && (isr & I2C_ISR_STOPF) != 0)
        {
          stm32_i2c_clear(priv);
          return -EIO;
        }

      if ((isr & I2C_ISR_ERRORMASK) != 0)
        {
          stm32_i2c_clear(priv);
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

static int stm32_i2c_wait_bus_free(FAR struct stm32_i2c_priv_s *priv)
{
  uint32_t timeout = CONFIG_STM32N6_I2CTIMEO;

  while ((getreg32(STM32_I2C_ISR(priv->base)) & I2C_ISR_BUSY) != 0)
    {
      if (timeout-- == 0)
        {
          return -ETIMEDOUT;
        }
    }

  return OK;
}

static void stm32_i2c_stop(FAR struct stm32_i2c_priv_s *priv)
{
  modifyreg32(STM32_I2C_CR2(priv->base), 0, I2C_CR2_STOP);
}

static int stm32_i2c_wait_stop(FAR struct stm32_i2c_priv_s *priv)
{
  int ret;

  ret = stm32_i2c_wait(priv, I2C_ISR_STOPF, true);
  stm32_i2c_clear(priv);
  return ret;
}

static int stm32_i2c_clock_enable(FAR struct stm32_i2c_priv_s *priv)
{
#ifdef CONFIG_STM32N6_I2C2
  if (priv->base == STM32_I2C2_BASE)
    {
      modifyreg32(STM32_RCC_CCIPR4, RCC_CCIPR4_I2C2SEL_MASK,
                  RCC_CCIPR4_I2C2SEL_PCLK1);

      putreg32(RCC_APB1ENSR1_I2C2ENS, STM32_RCC_APB1ENSR1);
      (void)getreg32(STM32_RCC_APB1ENR1);

      putreg32(RCC_APB1RSTR1_I2C2RST, STM32_RCC_APB1RSTR1);
      up_udelay(1);
      putreg32(RCC_APB1RSTCR1_I2C2RSTC, STM32_RCC_APB1RSTCR1);
      return OK;
    }
#endif

  return -ENODEV;
}

static void stm32_i2c_clock_disable(FAR struct stm32_i2c_priv_s *priv)
{
#ifdef CONFIG_STM32N6_I2C2
  if (priv->base == STM32_I2C2_BASE)
    {
      putreg32(RCC_APB1ENCR1_I2C2ENC, STM32_RCC_APB1ENCR1);
    }
#endif
}

static int stm32_i2c_hw_init(FAR struct stm32_i2c_priv_s *priv)
{
  int ret;

  if (priv->initialized)
    {
      return OK;
    }

  ret = stm32_i2c_clock_enable(priv);
  if (ret < 0)
    {
      return ret;
    }

  modifyreg32(STM32_I2C_CR1(priv->base), I2C_CR1_PE, 0);
  putreg32(priv->timing, STM32_I2C_TIMINGR(priv->base));
  putreg32(0, STM32_I2C_TIMEOUTR(priv->base));
  putreg32(0, STM32_I2C_OAR1(priv->base));
  putreg32(0, STM32_I2C_OAR2(priv->base));
  stm32_i2c_clear(priv);

  /* Match CubeN6: analog filter enabled, digital filter coefficient 0. */

  putreg32(I2C_CR1_PE, STM32_I2C_CR1(priv->base));

  priv->initialized = true;
  return OK;
}

static int stm32_i2c_hw_deinit(FAR struct stm32_i2c_priv_s *priv)
{
  if (priv->initialized)
    {
      modifyreg32(STM32_I2C_CR1(priv->base), I2C_CR1_PE, 0);
      stm32_i2c_clock_disable(priv);
      priv->initialized = false;
    }

  return OK;
}

static int stm32_i2c_start_msg(FAR struct stm32_i2c_priv_s *priv,
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

  modifyreg32(STM32_I2C_CR2(priv->base), STM32_I2C_CR2_CLEAR_MASK, cr2);
  return OK;
}

static int stm32_i2c_transfer_msg(FAR struct stm32_i2c_priv_s *priv,
                                  FAR struct i2c_msg_s *msg)
{
  ssize_t i;
  int ret;

  ret = stm32_i2c_start_msg(priv, msg);
  if (ret < 0)
    {
      return ret;
    }

  if ((msg->flags & I2C_M_READ) != 0)
    {
      for (i = 0; i < msg->length; i++)
        {
          ret = stm32_i2c_wait(priv, I2C_ISR_RXNE, false);
          if (ret < 0)
            {
              return ret;
            }

          msg->buffer[i] =
            getreg32(STM32_I2C_RXDR(priv->base)) & I2C_RXDR_MASK;
        }
    }
  else
    {
      for (i = 0; i < msg->length; i++)
        {
          ret = stm32_i2c_wait(priv, I2C_ISR_TXIS, false);
          if (ret < 0)
            {
              return ret;
            }

          putreg32(msg->buffer[i] & I2C_TXDR_MASK,
                   STM32_I2C_TXDR(priv->base));
        }
    }

  return stm32_i2c_wait(priv, I2C_ISR_TC, false);
}

static int stm32_i2c_transfer_locked(FAR struct stm32_i2c_priv_s *priv,
                                     FAR struct i2c_msg_s *msgs,
                                     int count)
{
  int ret;
  int i;

  ret = stm32_i2c_hw_init(priv);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_i2c_wait_bus_free(priv);
  if (ret < 0)
    {
      return ret;
    }

  for (i = 0; i < count; i++)
    {
      ret = stm32_i2c_transfer_msg(priv, &msgs[i]);
      if (ret < 0)
        {
          stm32_i2c_stop(priv);
          (void)stm32_i2c_wait_stop(priv);
          return ret;
        }

      if ((msgs[i].flags & I2C_M_NOSTOP) == 0 &&
          (i + 1 >= count || (msgs[i + 1].flags & I2C_M_NOSTART) == 0))
        {
          stm32_i2c_stop(priv);
          ret = stm32_i2c_wait_stop(priv);
          if (ret < 0)
            {
              return ret;
            }
        }
    }

  return OK;
}

static int stm32_i2c_transfer(FAR struct i2c_master_s *dev,
                              FAR struct i2c_msg_s *msgs, int count)
{
  FAR struct stm32_i2c_priv_s *priv =
    (FAR struct stm32_i2c_priv_s *)dev;
  int ret;

  DEBUGASSERT(priv != NULL && msgs != NULL && count > 0);

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_i2c_transfer_locked(priv, msgs, count);
  nxmutex_unlock(&priv->lock);
  return ret;
}

static int stm32_i2c_setup(FAR struct i2c_master_s *dev)
{
  FAR struct stm32_i2c_priv_s *priv =
    (FAR struct stm32_i2c_priv_s *)dev;
  int ret;

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_i2c_hw_init(priv);
  nxmutex_unlock(&priv->lock);
  return ret;
}

static int stm32_i2c_shutdown(FAR struct i2c_master_s *dev)
{
  FAR struct stm32_i2c_priv_s *priv =
    (FAR struct stm32_i2c_priv_s *)dev;
  int ret;

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_i2c_hw_deinit(priv);
  nxmutex_unlock(&priv->lock);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

FAR struct i2c_master_s *stm32_i2cbus_initialize(int port)
{
  FAR struct stm32_i2c_priv_s *priv = NULL;
  int ret;

  switch (port)
    {
#ifdef CONFIG_STM32N6_I2C2
      case 2:
        priv = &g_i2c2;
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
      ret = stm32_i2c_hw_init(priv);
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

int stm32_i2cbus_uninitialize(FAR struct i2c_master_s *dev)
{
  FAR struct stm32_i2c_priv_s *priv =
    (FAR struct stm32_i2c_priv_s *)dev;
  int ret;

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
      ret = stm32_i2c_hw_deinit(priv);
    }

  nxmutex_unlock(&priv->lock);
  return ret;
}

#endif /* CONFIG_STM32N6_I2C */
