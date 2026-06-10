/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32_spi.c
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

#include <arch/board/board.h>
#include <nuttx/mutex.h>
#include <nuttx/spi/spi.h>

#include "arm_internal.h"

#include "hardware/stm32_gpio.h"
#include "hardware/stm32_memorymap.h"
#include "hardware/stm32_rcc.h"
#include "hardware/stm32_spi.h"

#include "stm32_spi.h"

#ifdef CONFIG_STM32H7RS_SPI

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_STM32H7RS_SPITIMEO
#  define CONFIG_STM32H7RS_SPITIMEO 2000000
#endif

#define STM32_SPI_MAX_TSIZE          0xffffu

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32_spidev_s
{
  struct spi_dev_s spidev;
  mutex_t lock;
  uintptr_t base;
  uint32_t frequency;
  uint32_t actual;
  enum spi_mode_e mode;
  uint8_t nbits;
  bool initialized;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32_spi_lock(FAR struct spi_dev_s *dev, bool lock);
static uint32_t stm32_spi_setfrequency(FAR struct spi_dev_s *dev,
                                       uint32_t frequency);
static void stm32_spi_setmode(FAR struct spi_dev_s *dev,
                              enum spi_mode_e mode);
static void stm32_spi_setbits(FAR struct spi_dev_s *dev, int nbits);
static uint32_t stm32_spi_send(FAR struct spi_dev_s *dev, uint32_t wd);
static void stm32_spi_exchange(FAR struct spi_dev_s *dev,
                               FAR const void *txbuffer,
                               FAR void *rxbuffer, size_t nwords);
#ifndef CONFIG_SPI_EXCHANGE
static void stm32_spi_sndblock(FAR struct spi_dev_s *dev,
                               FAR const void *buffer, size_t nwords);
static void stm32_spi_recvblock(FAR struct spi_dev_s *dev,
                                FAR void *buffer, size_t nwords);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_STM32H7RS_SPI4
static const struct spi_ops_s g_spi4ops =
{
  .lock             = stm32_spi_lock,
  .select           = stm32_spi4select,
  .setfrequency     = stm32_spi_setfrequency,
  .setmode          = stm32_spi_setmode,
  .setbits          = stm32_spi_setbits,
  .status           = stm32_spi4status,
  .send             = stm32_spi_send,
#ifdef CONFIG_SPI_EXCHANGE
  .exchange         = stm32_spi_exchange,
#else
  .sndblock         = stm32_spi_sndblock,
  .recvblock        = stm32_spi_recvblock,
#endif
};

static struct stm32_spidev_s g_spi4 =
{
  .spidev =
    {
      .ops = &g_spi4ops,
    },
  .lock      = NXMUTEX_INITIALIZER,
  .base      = STM32_SPI4_BASE,
  .frequency = 1000000,
  .actual    = 0,
  .mode      = SPIDEV_MODE0,
  .nbits     = 8,
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32_spi_config_gpio4(void)
{
  const uint32_t pins = (1u << 12) | (1u << 13) | (1u << 14);
  uint32_t regval;
  int pin;

  modifyreg32(STM32_RCC_AHB4ENR, 0, RCC_AHB4ENR_GPIOEEN);
  (void)getreg32(STM32_RCC_AHB4ENR);

  regval = getreg32(STM32_GPIOE_MODER);
  for (pin = 12; pin <= 14; pin++)
    {
      regval &= ~GPIO_MODE_MASK(pin);
      regval |= GPIO_MODE_ALT << GPIO_MODE_SHIFT(pin);
    }

  putreg32(regval, STM32_GPIOE_MODER);
  modifyreg32(STM32_GPIOE_OTYPER, pins, 0);

  regval = getreg32(STM32_GPIOE_OSPEEDR);
  for (pin = 12; pin <= 14; pin++)
    {
      regval &= ~GPIO_SPEED_MASK(pin);
      regval |= GPIO_SPEED_VERYHIGH << GPIO_SPEED_SHIFT(pin);
    }

  putreg32(regval, STM32_GPIOE_OSPEEDR);

  regval = getreg32(STM32_GPIOE_PUPDR);
  for (pin = 12; pin <= 14; pin++)
    {
      regval &= ~GPIO_PUPD_MASK(pin);
      regval |= GPIO_FLOAT << GPIO_PUPD_SHIFT(pin);
    }

  putreg32(regval, STM32_GPIOE_PUPDR);

  regval = getreg32(STM32_GPIOE_AFRH);
  for (pin = 12; pin <= 14; pin++)
    {
      regval &= ~GPIO_AFR_MASK(pin);
      regval |= GPIO_AF_SPI4 << GPIO_AFR_SHIFT(pin);
    }

  putreg32(regval, STM32_GPIOE_AFRH);
}

static void stm32_spi_clock4_enable(void)
{
  modifyreg32(STM32_RCC_CCIPR3, RCC_CCIPR3_SPI45SEL_MASK,
              RCC_CCIPR3_SPI45SEL_PCLK);
  modifyreg32(STM32_RCC_APB2ENR, 0, RCC_APB2ENR_SPI4EN);
  (void)getreg32(STM32_RCC_APB2ENR);
  modifyreg32(STM32_RCC_APB2RSTR, 0, RCC_APB2RSTR_SPI4RST);
  modifyreg32(STM32_RCC_APB2RSTR, RCC_APB2RSTR_SPI4RST, 0);
}

static int stm32_spi_wait(FAR struct stm32_spidev_s *priv, uint32_t mask)
{
  uint32_t timeout = CONFIG_STM32H7RS_SPITIMEO;
  uint32_t sr;

  do
    {
      sr = getreg32(STM32_SPI_SR(priv->base));
      if ((sr & SPI_SR_ERRORMASK) != 0)
        {
          putreg32(SPI_IFCR_CLEARMASK, STM32_SPI_IFCR(priv->base));
          return -EIO;
        }

      if ((sr & mask) != 0)
        {
          return OK;
        }
    }
  while (timeout-- > 0);

  return -ETIMEDOUT;
}

static uint32_t stm32_spi_configure(FAR struct stm32_spidev_s *priv)
{
  uint32_t requested = priv->frequency;
  uint32_t actual;
  uint32_t div;
  uint32_t mbr;
  uint32_t cfg1;
  uint32_t cfg2;

  if (requested == 0)
    {
      requested = 1;
    }

  for (mbr = 0, div = 2; mbr < 7 && (STM32_PCLK2_FREQUENCY / div) > requested;
       mbr++, div <<= 1)
    {
    }

  actual = STM32_PCLK2_FREQUENCY / div;

  modifyreg32(STM32_SPI_CR1(priv->base), SPI_CR1_SPE, 0);
  putreg32(0, STM32_SPI_IER(priv->base));
  putreg32(SPI_IFCR_CLEARMASK, STM32_SPI_IFCR(priv->base));

  cfg1 = SPI_CFG1_DSIZE(priv->nbits) | SPI_CFG1_FTHLV(1) |
         SPI_CFG1_MBR(mbr);
  putreg32(cfg1, STM32_SPI_CFG1(priv->base));

  cfg2 = SPI_CFG2_MASTER | SPI_CFG2_SSM | SPI_CFG2_AFCNTR |
         SPI_CFG2_FULLDUPLEX;
  if (priv->mode == SPIDEV_MODE1 || priv->mode == SPIDEV_MODE3)
    {
      cfg2 |= SPI_CFG2_CPHA;
    }

  if (priv->mode == SPIDEV_MODE2 || priv->mode == SPIDEV_MODE3)
    {
      cfg2 |= SPI_CFG2_CPOL;
    }

  putreg32(cfg2, STM32_SPI_CFG2(priv->base));
  putreg32(SPI_CR1_SSI | SPI_CR1_SPE, STM32_SPI_CR1(priv->base));

  priv->actual = actual;
  return actual;
}

static int stm32_spi_exchange_chunk(FAR struct stm32_spidev_s *priv,
                                    FAR const uint8_t *tx,
                                    FAR uint8_t *rx, size_t nwords)
{
  size_t ntx = 0;
  size_t nrx = 0;
  int ret;

  DEBUGASSERT(nwords <= STM32_SPI_MAX_TSIZE);

  putreg32(SPI_IFCR_CLEARMASK, STM32_SPI_IFCR(priv->base));
  putreg32(SPI_CR2_TSIZE(nwords), STM32_SPI_CR2(priv->base));
  modifyreg32(STM32_SPI_CR1(priv->base), 0, SPI_CR1_CSTART);

  while (nrx < nwords)
    {
      if (ntx < nwords)
        {
          ret = stm32_spi_wait(priv, SPI_SR_TXP);
          if (ret < 0)
            {
              return ret;
            }

          putreg8(tx == NULL ? 0xff : tx[ntx], STM32_SPI_TXDR(priv->base));
          ntx++;
        }

      ret = stm32_spi_wait(priv, SPI_SR_RXP);
      if (ret < 0)
        {
          return ret;
        }

      if (rx != NULL)
        {
          rx[nrx] = getreg8(STM32_SPI_RXDR(priv->base));
        }
      else
        {
          (void)getreg8(STM32_SPI_RXDR(priv->base));
        }

      nrx++;
    }

  ret = stm32_spi_wait(priv, SPI_SR_EOT);
  putreg32(SPI_IFCR_CLEARMASK, STM32_SPI_IFCR(priv->base));
  return ret;
}

static int stm32_spi_lock(FAR struct spi_dev_s *dev, bool lock)
{
  FAR struct stm32_spidev_s *priv = (FAR struct stm32_spidev_s *)dev;

  if (lock)
    {
      return nxmutex_lock(&priv->lock);
    }

  return nxmutex_unlock(&priv->lock);
}

static uint32_t stm32_spi_setfrequency(FAR struct spi_dev_s *dev,
                                       uint32_t frequency)
{
  FAR struct stm32_spidev_s *priv = (FAR struct stm32_spidev_s *)dev;

  if (priv->frequency != frequency || priv->actual == 0)
    {
      priv->frequency = frequency;
      stm32_spi_configure(priv);
    }

  return priv->actual;
}

static void stm32_spi_setmode(FAR struct spi_dev_s *dev,
                              enum spi_mode_e mode)
{
  FAR struct stm32_spidev_s *priv = (FAR struct stm32_spidev_s *)dev;

  if (priv->mode != mode)
    {
      priv->mode = mode;
      stm32_spi_configure(priv);
    }
}

static void stm32_spi_setbits(FAR struct spi_dev_s *dev, int nbits)
{
  FAR struct stm32_spidev_s *priv = (FAR struct stm32_spidev_s *)dev;

  if (nbits < 4)
    {
      nbits = 4;
    }
  else if (nbits > 16)
    {
      nbits = 16;
    }

  if (priv->nbits != nbits)
    {
      priv->nbits = nbits;
      stm32_spi_configure(priv);
    }
}

static uint32_t stm32_spi_send(FAR struct spi_dev_s *dev, uint32_t wd)
{
  uint8_t tx = wd;
  uint8_t rx = 0;

  stm32_spi_exchange(dev, &tx, &rx, 1);
  return rx;
}

static void stm32_spi_exchange(FAR struct spi_dev_s *dev,
                               FAR const void *txbuffer,
                               FAR void *rxbuffer, size_t nwords)
{
  FAR struct stm32_spidev_s *priv = (FAR struct stm32_spidev_s *)dev;
  FAR const uint8_t *tx = (FAR const uint8_t *)txbuffer;
  FAR uint8_t *rx = (FAR uint8_t *)rxbuffer;
  size_t chunk;

  while (nwords > 0)
    {
      chunk = nwords > STM32_SPI_MAX_TSIZE ? STM32_SPI_MAX_TSIZE : nwords;
      if (stm32_spi_exchange_chunk(priv, tx, rx, chunk) < 0)
        {
          return;
        }

      if (tx != NULL)
        {
          tx += chunk;
        }

      if (rx != NULL)
        {
          rx += chunk;
        }

      nwords -= chunk;
    }
}

#ifndef CONFIG_SPI_EXCHANGE
static void stm32_spi_sndblock(FAR struct spi_dev_s *dev,
                               FAR const void *buffer, size_t nwords)
{
  stm32_spi_exchange(dev, buffer, NULL, nwords);
}

static void stm32_spi_recvblock(FAR struct spi_dev_s *dev,
                                FAR void *buffer, size_t nwords)
{
  stm32_spi_exchange(dev, NULL, buffer, nwords);
}
#endif /* CONFIG_SPI_EXCHANGE */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

struct spi_dev_s *stm32_spibus_initialize(int bus)
{
#ifdef CONFIG_STM32H7RS_SPI4
  if (bus == 4)
    {
      if (!g_spi4.initialized)
        {
          stm32_spi_config_gpio4();
          stm32_spi_clock4_enable();
          g_spi4.initialized = true;
          stm32_spi_configure(&g_spi4);
        }

      return &g_spi4.spidev;
    }
#endif

  return NULL;
}

#endif /* CONFIG_STM32H7RS_SPI */
