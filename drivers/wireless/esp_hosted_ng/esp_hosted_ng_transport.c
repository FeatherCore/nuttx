/****************************************************************************
 * drivers/wireless/esp_hosted_ng/esp_hosted_ng_transport.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>

#include <nuttx/clock.h>
#include <nuttx/compiler.h>
#include <nuttx/mutex.h>
#include <nuttx/spi/spi.h>

#include "esp_hosted_ng_transport.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static mutex_t g_esp_hosted_transport_lock = NXMUTEX_INITIALIZER;
static uint8_t g_esp_hosted_txbuf[ESP_HOSTED_NG_SPI_BUF_SIZE];
static uint8_t g_esp_hosted_rxbuf[ESP_HOSTED_NG_SPI_BUF_SIZE];

#define ESP_HOSTED_NG_HANDSHAKE_TIMEOUT 2000000u

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static unsigned long esp_hosted_ng_msec(void)
{
  return (unsigned long)TICK2MSEC(clock_systime_ticks());
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp_hosted_ng_spibus_initialize
 *
 * Description:
 *   Board hook used by the ESP-Hosted NG driver to obtain the SPI master
 *   connected to ESP32-C5.  The STM32H7RS tree currently has XSPI memory
 *   support but no regular SPI master implementation, so the weak default
 *   keeps the driver buildable and returns ENODEV through transport_init().
 *
 ****************************************************************************/

FAR struct spi_dev_s *weak_function esp_hosted_ng_spibus_initialize(int bus)
{
  (void)bus;
  return NULL;
}

int weak_function esp_hosted_ng_board_initialize(void)
{
  return OK;
}

void weak_function esp_hosted_ng_board_reset(bool reset)
{
  (void)reset;
}

bool weak_function esp_hosted_ng_board_handshake_ready(void)
{
  return true;
}

bool weak_function esp_hosted_ng_board_data_ready(void)
{
  return false;
}

void weak_function esp_hosted_ng_board_spi_ready(void)
{
}

static int esp_hosted_ng_wait_handshake(void)
{
  uint32_t timeout = ESP_HOSTED_NG_HANDSHAKE_TIMEOUT;

  do
    {
      if (esp_hosted_ng_board_handshake_ready())
        {
          return OK;
        }
    }
  while (timeout-- > 0);

  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: esp_hosted_ng_transport_init
 ****************************************************************************/

int esp_hosted_ng_transport_init(FAR struct esp_hosted_ng_transport_s *priv)
{
  FAR struct spi_dev_s *spi;
  int ret;

  if (priv == NULL)
    {
      return -EINVAL;
    }

  memset(priv, 0, sizeof(*priv));
  priv->frequency = CONFIG_WL_ESP_HOSTED_NG_SPI_FREQUENCY;
  priv->devid = SPIDEV_WIRELESS(0);
  priv->mode = CONFIG_WL_ESP_HOSTED_NG_SPI_MODE;

  ret = esp_hosted_ng_board_initialize();
  if (ret < 0)
    {
      syslog(LOG_WARNING,
             "esp_hosted_ng: board GPIO initialization failed: %d\n", ret);
      return ret;
    }

  spi = esp_hosted_ng_spibus_initialize(CONFIG_WL_ESP_HOSTED_NG_SPI_DEV);
  if (spi == NULL)
    {
      syslog(LOG_WARNING,
             "esp_hosted_ng: SPI bus %d unavailable; board SPI hook missing\n",
             CONFIG_WL_ESP_HOSTED_NG_SPI_DEV);
      return -ENODEV;
    }

  SPI_LOCK(spi, true);
  SPI_SETMODE(spi, (enum spi_mode_e)priv->mode);
  SPI_SETBITS(spi, 8);
  SPI_SETFREQUENCY(spi, priv->frequency);
  SPI_LOCK(spi, false);

  priv->spi = spi;
  priv->initialized = true;
  priv->datapath_open = true;

  syslog(LOG_INFO,
         "esp_hosted_ng: SPI transport ready bus=%d freq=%" PRIu32
         " mode=%u hs=%d dr=%d\n",
         CONFIG_WL_ESP_HOSTED_NG_SPI_DEV, priv->frequency, priv->mode,
         esp_hosted_ng_board_handshake_ready(),
         esp_hosted_ng_board_data_ready());
  return OK;
}

/****************************************************************************
 * Name: esp_hosted_ng_transport_deinit
 ****************************************************************************/

void esp_hosted_ng_transport_deinit(FAR struct esp_hosted_ng_transport_s *priv)
{
  if (priv != NULL)
    {
      priv->datapath_open = false;
      priv->initialized = false;
      priv->spi = NULL;
    }
}

/****************************************************************************
 * Name: esp_hosted_ng_transport_ready
 ****************************************************************************/

bool esp_hosted_ng_transport_ready(
  FAR const struct esp_hosted_ng_transport_s *priv)
{
  return priv != NULL && priv->initialized && priv->datapath_open &&
         priv->spi != NULL;
}

/****************************************************************************
 * Name: esp_hosted_ng_transport_exchange
 ****************************************************************************/

int esp_hosted_ng_transport_exchange(FAR struct esp_hosted_ng_transport_s *priv,
                                     FAR const uint8_t *txbuf,
                                     FAR uint8_t *rxbuf, size_t buflen)
{
  unsigned long start;
  unsigned long locked;
  unsigned long hs_done;
  unsigned long spi_done;
  int ret;

  if (!esp_hosted_ng_transport_ready(priv))
    {
      return -ENODEV;
    }

  if (txbuf == NULL || rxbuf == NULL || buflen == 0)
    {
      return -EINVAL;
    }

  start = esp_hosted_ng_msec();
  ret = nxmutex_lock(&g_esp_hosted_transport_lock);
  if (ret < 0)
    {
      return ret;
    }

  locked = esp_hosted_ng_msec();

  SPI_LOCK(priv->spi, true);
  SPI_SETMODE(priv->spi, (enum spi_mode_e)priv->mode);
  SPI_SETBITS(priv->spi, 8);
  SPI_SETFREQUENCY(priv->spi, priv->frequency);

  ret = esp_hosted_ng_wait_handshake();
  hs_done = esp_hosted_ng_msec();
  if (ret >= 0)
    {
      SPI_SELECT(priv->spi, priv->devid, true);
      SPI_EXCHANGE(priv->spi, txbuf, rxbuf, buflen);
      SPI_SELECT(priv->spi, priv->devid, false);
    }

  spi_done = esp_hosted_ng_msec();
  SPI_LOCK(priv->spi, false);
  nxmutex_unlock(&g_esp_hosted_transport_lock);
  if (spi_done - start >= 100)
    {
      syslog(LOG_WARNING,
             "esp_hosted_ng: transport exchange slow buflen=%zu "
             "lock_ms=%lu hs_ms=%lu spi_ms=%lu total_ms=%lu ret=%d "
             "hs=%d dr=%d\n",
             buflen, locked - start, hs_done - locked, spi_done - hs_done,
             spi_done - start, ret, esp_hosted_ng_board_handshake_ready(),
             esp_hosted_ng_board_data_ready());
    }

  return ret;
}

/****************************************************************************
 * Name: esp_hosted_ng_transport_send
 ****************************************************************************/

int esp_hosted_ng_transport_send(FAR struct esp_hosted_ng_transport_s *priv,
                                 uint8_t if_type, uint8_t packet_type,
                                 FAR const uint8_t *payload,
                                 size_t payload_len)
{
  FAR struct esp_hosted_ng_payload_header_s *header;
  size_t offset = sizeof(struct esp_hosted_ng_payload_header_s);
  int ret;

  if (!esp_hosted_ng_transport_ready(priv))
    {
      return -ENODEV;
    }

  if (payload == NULL && payload_len != 0)
    {
      return -EINVAL;
    }

  if (if_type >= ESP_HOSTED_NG_MAX_IF ||
      payload_len > ESP_HOSTED_NG_SPI_BUF_SIZE - offset)
    {
      return -EINVAL;
    }

  ret = nxmutex_lock(&g_esp_hosted_transport_lock);
  if (ret < 0)
    {
      return ret;
    }

  memset(g_esp_hosted_txbuf, 0, sizeof(g_esp_hosted_txbuf));
  memset(g_esp_hosted_rxbuf, 0, sizeof(g_esp_hosted_rxbuf));

  header = (FAR struct esp_hosted_ng_payload_header_s *)g_esp_hosted_txbuf;
  header->if_type = if_type;
  header->if_num = 0;
  header->packet_type = packet_type;
  header->len = payload_len;
  header->offset = offset;

  if (payload_len > 0)
    {
      memcpy(g_esp_hosted_txbuf + offset, payload, payload_len);
    }

  SPI_LOCK(priv->spi, true);
  SPI_SETMODE(priv->spi, (enum spi_mode_e)priv->mode);
  SPI_SETBITS(priv->spi, 8);
  SPI_SETFREQUENCY(priv->spi, priv->frequency);
  ret = esp_hosted_ng_wait_handshake();
  if (ret < 0)
    {
      SPI_LOCK(priv->spi, false);
      nxmutex_unlock(&g_esp_hosted_transport_lock);
      return ret;
    }

  SPI_SELECT(priv->spi, priv->devid, true);
  SPI_EXCHANGE(priv->spi, g_esp_hosted_txbuf, g_esp_hosted_rxbuf,
               ESP_HOSTED_NG_SPI_BUF_SIZE);
  SPI_SELECT(priv->spi, priv->devid, false);
  SPI_LOCK(priv->spi, false);

  nxmutex_unlock(&g_esp_hosted_transport_lock);
  return OK;
}
