/****************************************************************************
 * drivers/wireless/esp_hosted_ng/esp_hosted_ng_netdev.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>

#include <net/if.h>

#include <nuttx/net/net.h>
#include <nuttx/net/netdev_lowerhalf.h>
#include <nuttx/wireless/esp_hosted_ng.h>
#include <nuttx/wireless/wireless.h>

#include "esp_hosted_ng_transport.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct esp_hosted_ng_dev_s
{
  struct netdev_lowerhalf_s lower;
  bool registered;
  bool ifup;
  bool connected;
  int mode;
  char ssid[IW_ESSID_MAX_SIZE + 1];
  char passphrase[ESP_HOSTED_NG_MAX_PASSPHRASE_LEN + 1];
  struct esp_hosted_ng_transport_s transport;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int esp_hosted_ifup(FAR struct netdev_lowerhalf_s *lower);
static int esp_hosted_ifdown(FAR struct netdev_lowerhalf_s *lower);
static int esp_hosted_transmit(FAR struct netdev_lowerhalf_s *lower,
                               FAR netpkt_t *pkt);
static FAR netpkt_t *
esp_hosted_receive(FAR struct netdev_lowerhalf_s *lower);
static int esp_hosted_connect(FAR struct netdev_lowerhalf_s *lower);
static int esp_hosted_disconnect(FAR struct netdev_lowerhalf_s *lower);
static int esp_hosted_essid(FAR struct netdev_lowerhalf_s *lower,
                            FAR struct iwreq *iwr, bool set);
static int esp_hosted_passwd(FAR struct netdev_lowerhalf_s *lower,
                             FAR struct iwreq *iwr, bool set);
static int esp_hosted_mode(FAR struct netdev_lowerhalf_s *lower,
                           FAR struct iwreq *iwr, bool set);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct netdev_ops_s g_esp_hosted_netdev_ops =
{
  .ifup     = esp_hosted_ifup,
  .ifdown   = esp_hosted_ifdown,
  .transmit = esp_hosted_transmit,
  .receive  = esp_hosted_receive,
};

#ifdef CONFIG_NETDEV_WIRELESS_HANDLER
static const struct wireless_ops_s g_esp_hosted_iw_ops =
{
  .connect    = esp_hosted_connect,
  .disconnect = esp_hosted_disconnect,
  .essid      = esp_hosted_essid,
  .passwd     = esp_hosted_passwd,
  .mode       = esp_hosted_mode,
};
#endif

static struct esp_hosted_ng_dev_s g_esp_hosted_dev;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR struct esp_hosted_ng_dev_s *
esp_hosted_from_lower(FAR struct netdev_lowerhalf_s *lower)
{
  return (FAR struct esp_hosted_ng_dev_s *)lower;
}

static int esp_hosted_ifup(FAR struct netdev_lowerhalf_s *lower)
{
  FAR struct esp_hosted_ng_dev_s *priv = esp_hosted_from_lower(lower);

  priv->ifup = true;
  syslog(LOG_INFO, "esp_hosted_ng: %s up, waiting for SPI transport\n",
         lower->netdev.d_ifname);
  return OK;
}

static int esp_hosted_ifdown(FAR struct netdev_lowerhalf_s *lower)
{
  FAR struct esp_hosted_ng_dev_s *priv = esp_hosted_from_lower(lower);

  priv->ifup = false;
  priv->connected = false;
  syslog(LOG_INFO, "esp_hosted_ng: %s down\n", lower->netdev.d_ifname);
  return OK;
}

static int esp_hosted_transmit(FAR struct netdev_lowerhalf_s *lower,
                               FAR netpkt_t *pkt)
{
  FAR struct esp_hosted_ng_dev_s *priv = esp_hosted_from_lower(lower);
  FAR uint8_t *data;
  unsigned int len;

  if (!priv->ifup)
    {
      return -ENETDOWN;
    }

  if (!esp_hosted_ng_transport_ready(&priv->transport))
    {
      return -ENODEV;
    }

  len = netpkt_getdatalen(lower, pkt);
  data = netpkt_getdata(lower, pkt);
  if (data == NULL || len == 0)
    {
      return -EINVAL;
    }

  return esp_hosted_ng_transport_send(&priv->transport,
                                      ESP_HOSTED_NG_STA_IF,
                                      ESP_HOSTED_NG_PACKET_TYPE_DATA,
                                      data, len);
}

static FAR netpkt_t *
esp_hosted_receive(FAR struct netdev_lowerhalf_s *lower)
{
  (void)lower;
  return NULL;
}

static int esp_hosted_connect(FAR struct netdev_lowerhalf_s *lower)
{
  FAR struct esp_hosted_ng_dev_s *priv = esp_hosted_from_lower(lower);
  struct esp_hosted_ng_sta_connect_cmd_s cmd;
  size_t passphrase_len;
  size_t ssid_len;
  int ret;

  if (priv->ssid[0] == '\0')
    {
      syslog(LOG_WARNING, "esp_hosted_ng: connect requested without SSID\n");
      return -EINVAL;
    }

  if (!priv->ifup)
    {
      return -ENETDOWN;
    }

  syslog(LOG_INFO,
         "esp_hosted_ng: connect ssid=%s mode=%d auth=%s\n",
         priv->ssid, priv->mode,
         priv->passphrase[0] == '\0' ? "open" : "secured");

  if (!esp_hosted_ng_transport_ready(&priv->transport))
    {
      return -ENODEV;
    }

  memset(&cmd, 0, sizeof(cmd));
  cmd.header.cmd_code = ESP_HOSTED_NG_CMD_STA_CONNECT;
  cmd.header.len = sizeof(cmd);
  cmd.channel = 0;
  cmd.is_auth_open = priv->passphrase[0] == '\0' ? 1 : 0;
  ssid_len = strnlen(priv->ssid, ESP_HOSTED_NG_MAX_SSID_LEN);
  memcpy(cmd.ssid, priv->ssid, ssid_len);
  passphrase_len = strnlen(priv->passphrase,
                           ESP_HOSTED_NG_MAX_PASSPHRASE_LEN);
  cmd.passphrase_len = passphrase_len;
  memcpy(cmd.passphrase, priv->passphrase, passphrase_len);

  ret = esp_hosted_ng_transport_send(&priv->transport,
                                     ESP_HOSTED_NG_STA_IF,
                                     ESP_HOSTED_NG_PACKET_TYPE_COMMAND_REQUEST,
                                     (FAR const uint8_t *)&cmd,
                                     sizeof(cmd));
  if (ret < 0)
    {
      return ret;
    }

  priv->connected = true;
  netdev_lower_carrier_on(lower);
  return OK;
}

static int esp_hosted_disconnect(FAR struct netdev_lowerhalf_s *lower)
{
  FAR struct esp_hosted_ng_dev_s *priv = esp_hosted_from_lower(lower);

  priv->connected = false;
  netdev_lower_carrier_off(lower);
  return OK;
}

static int esp_hosted_essid(FAR struct netdev_lowerhalf_s *lower,
                            FAR struct iwreq *iwr, bool set)
{
  FAR struct esp_hosted_ng_dev_s *priv = esp_hosted_from_lower(lower);
  size_t len;

  if (iwr == NULL)
    {
      return -EINVAL;
    }

  if (set)
    {
      if (iwr->u.essid.pointer == NULL ||
          iwr->u.essid.length > IW_ESSID_MAX_SIZE)
        {
          return -EINVAL;
        }

      len = iwr->u.essid.length;
      memcpy(priv->ssid, iwr->u.essid.pointer, len);
      priv->ssid[len] = '\0';
      return OK;
    }

  len = strnlen(priv->ssid, IW_ESSID_MAX_SIZE);
  if (iwr->u.essid.pointer != NULL && iwr->u.essid.length >= len)
    {
      memcpy(iwr->u.essid.pointer, priv->ssid, len);
    }

  iwr->u.essid.length = len;
  iwr->u.essid.flags = priv->ssid[0] != '\0' ? IW_ESSID_ON : IW_ESSID_OFF;
  return OK;
}

static int esp_hosted_passwd(FAR struct netdev_lowerhalf_s *lower,
                             FAR struct iwreq *iwr, bool set)
{
  FAR struct esp_hosted_ng_dev_s *priv = esp_hosted_from_lower(lower);
  size_t len;

  if (iwr == NULL)
    {
      return -EINVAL;
    }

  if (set)
    {
      if (iwr->u.encoding.pointer == NULL || iwr->u.encoding.length == 0)
        {
          priv->passphrase[0] = '\0';
          return OK;
        }

      if (iwr->u.encoding.length > ESP_HOSTED_NG_MAX_PASSPHRASE_LEN)
        {
          return -EINVAL;
        }

      len = iwr->u.encoding.length;
      memcpy(priv->passphrase, iwr->u.encoding.pointer, len);
      priv->passphrase[len] = '\0';
      return OK;
    }

  len = strnlen(priv->passphrase, ESP_HOSTED_NG_MAX_PASSPHRASE_LEN);
  if (iwr->u.encoding.pointer != NULL && iwr->u.encoding.length >= len)
    {
      memcpy(iwr->u.encoding.pointer, priv->passphrase, len);
    }

  iwr->u.encoding.length = len;
  return OK;
}

static int esp_hosted_mode(FAR struct netdev_lowerhalf_s *lower,
                           FAR struct iwreq *iwr, bool set)
{
  FAR struct esp_hosted_ng_dev_s *priv = esp_hosted_from_lower(lower);

  if (iwr == NULL)
    {
      return -EINVAL;
    }

  if (set)
    {
      priv->mode = iwr->u.mode;
    }
  else
    {
      iwr->u.mode = priv->mode;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: esp_hosted_ng_initialize
 ****************************************************************************/

int esp_hosted_ng_initialize(void)
{
  FAR struct esp_hosted_ng_dev_s *priv = &g_esp_hosted_dev;
  FAR struct netdev_lowerhalf_s *lower = &priv->lower;
  static const uint8_t mac[IFHWADDRLEN] =
  {
    0x02, 0xfc, 0x32, 0xc5, 0x00, 0x01
  };
  int ret;

  if (priv->registered)
    {
      return OK;
    }

  memset(priv, 0, sizeof(*priv));

  lower->ops = &g_esp_hosted_netdev_ops;
#ifdef CONFIG_NETDEV_WIRELESS_HANDLER
  lower->iw_ops = &g_esp_hosted_iw_ops;
#endif
  lower->rxtype = NETDEV_RX_WORK;
  lower->quota[NETPKT_TX] = 4;
  lower->quota[NETPKT_RX] = 4;
  lower->netdev.d_llhdrlen = ETH_HDRLEN;
  lower->netdev.d_pktsize = CONFIG_NET_ETH_PKTSIZE;
  memcpy(&lower->netdev.d_mac.ether, mac, sizeof(mac));
  strlcpy(lower->netdev.d_ifname, CONFIG_WL_ESP_HOSTED_NG_IFNAME,
          sizeof(lower->netdev.d_ifname));

  priv->mode = IW_MODE_INFRA;
  ret = esp_hosted_ng_transport_init(&priv->transport);
  if (ret < 0)
    {
      syslog(LOG_WARNING,
             "esp_hosted_ng: SPI transport not ready yet: %d\n", ret);
    }

  ret = netdev_lower_register(lower, NET_LL_IEEE80211);
  if (ret < 0)
    {
      syslog(LOG_ERR, "esp_hosted_ng: failed to register %s: %d\n",
             CONFIG_WL_ESP_HOSTED_NG_IFNAME, ret);
      return ret;
    }

  priv->registered = true;
  netdev_lower_carrier_off(lower);

  syslog(LOG_INFO,
         "esp_hosted_ng: registered %s bus=%d freq=%d, SPI transport pending\n",
         lower->netdev.d_ifname, CONFIG_WL_ESP_HOSTED_NG_SPI_DEV,
         CONFIG_WL_ESP_HOSTED_NG_SPI_FREQUENCY);

  return OK;
}
