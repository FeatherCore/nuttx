/****************************************************************************
 * drivers/wireless/virtual/virtual_hwsim.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>

#include <nuttx/net/netdev_lowerhalf.h>
#include <nuttx/wireless/ieee80211_linux.h>
#include <nuttx/wireless/wireless.h>
#include <nuttx/wireless/virtual_hwsim.h>

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int virtual_hwsim_ifup(FAR struct netdev_lowerhalf_s *dev);
static int virtual_hwsim_ifdown(FAR struct netdev_lowerhalf_s *dev);
static int virtual_hwsim_transmit(FAR struct netdev_lowerhalf_s *dev,
                                  FAR netpkt_t *pkt);
static FAR netpkt_t *virtual_hwsim_receive(FAR struct netdev_lowerhalf_s *dev);
static int virtual_hwsim_connect(FAR struct netdev_lowerhalf_s *dev);
static int virtual_hwsim_disconnect(FAR struct netdev_lowerhalf_s *dev);
static int virtual_hwsim_essid(FAR struct netdev_lowerhalf_s *dev,
                               FAR struct iwreq *iwr, bool set);
static int virtual_hwsim_bssid(FAR struct netdev_lowerhalf_s *dev,
                               FAR struct iwreq *iwr, bool set);
static int virtual_hwsim_passwd(FAR struct netdev_lowerhalf_s *dev,
                                FAR struct iwreq *iwr, bool set);
static int virtual_hwsim_mode(FAR struct netdev_lowerhalf_s *dev,
                              FAR struct iwreq *iwr, bool set);
static int virtual_hwsim_auth(FAR struct netdev_lowerhalf_s *dev,
                              FAR struct iwreq *iwr, bool set);
static int virtual_hwsim_freq(FAR struct netdev_lowerhalf_s *dev,
                              FAR struct iwreq *iwr, bool set);
static int virtual_hwsim_bitrate(FAR struct netdev_lowerhalf_s *dev,
                                 FAR struct iwreq *iwr, bool set);
static int virtual_hwsim_txpower(FAR struct netdev_lowerhalf_s *dev,
                                 FAR struct iwreq *iwr, bool set);
static int virtual_hwsim_country(FAR struct netdev_lowerhalf_s *dev,
                                 FAR struct iwreq *iwr, bool set);
static int virtual_hwsim_sensitivity(FAR struct netdev_lowerhalf_s *dev,
                                     FAR struct iwreq *iwr, bool set);
static int virtual_hwsim_scan(FAR struct netdev_lowerhalf_s *dev,
                              FAR struct iwreq *iwr, bool set);

/****************************************************************************
 * Private Data
 ****************************************************************************/

int mac80211_hwsim_linux_initialize(void);
void mac80211_hwsim_linux_uninitialize(void);

static const struct netdev_ops_s g_virtual_hwsim_ops =
{
  virtual_hwsim_ifup,
  virtual_hwsim_ifdown,
  virtual_hwsim_transmit,
  virtual_hwsim_receive
};

static const struct wireless_ops_s g_virtual_hwsim_iw_ops =
{
  virtual_hwsim_connect,
  virtual_hwsim_disconnect,
  virtual_hwsim_essid,
  virtual_hwsim_bssid,
  virtual_hwsim_passwd,
  virtual_hwsim_mode,
  virtual_hwsim_auth,
  virtual_hwsim_freq,
  virtual_hwsim_bitrate,
  virtual_hwsim_txpower,
  virtual_hwsim_country,
  virtual_hwsim_sensitivity,
  virtual_hwsim_scan,
  NULL                 /* range */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int virtual_hwsim_ifup(FAR struct netdev_lowerhalf_s *dev)
{
  int ret;

  (void)ieee80211_linux_sync_lowerhalf_mac(dev->netdev.d_ifname, dev);

  ret = ieee80211_linux_bind_lowerhalf(dev->netdev.d_ifname, dev);
  if (ret < 0)
    {
      return ret;
    }

  ret = ieee80211_linux_set_netdev_flags(dev->netdev.d_ifname, true);
  if (ret < 0)
    {
      return ret;
    }

  netdev_lower_carrier_on(dev);
  return OK;
}

static int virtual_hwsim_ifdown(FAR struct netdev_lowerhalf_s *dev)
{
  netdev_lower_carrier_off(dev);
  return ieee80211_linux_set_netdev_flags(dev->netdev.d_ifname, false);
}

static int virtual_hwsim_transmit(FAR struct netdev_lowerhalf_s *dev,
                                  FAR netpkt_t *pkt)
{
  return ieee80211_linux_transmit_netpkt(dev, pkt);
}

static FAR netpkt_t *virtual_hwsim_receive(FAR struct netdev_lowerhalf_s *dev)
{
  return ieee80211_linux_receive_netpkt(dev);
}

static int virtual_hwsim_connect(FAR struct netdev_lowerhalf_s *dev)
{
  (void)dev;
  return OK;
}

static int virtual_hwsim_disconnect(FAR struct netdev_lowerhalf_s *dev)
{
  (void)dev;
  return OK;
}

static int virtual_hwsim_wext(FAR struct netdev_lowerhalf_s *dev,
                              FAR struct iwreq *iwr, int setcmd, int getcmd,
                              bool set)
{
  return ieee80211_linux_wext_ioctl(dev->netdev.d_ifname,
                                    set ? setcmd : getcmd, iwr);
}

static int virtual_hwsim_essid(FAR struct netdev_lowerhalf_s *dev,
                               FAR struct iwreq *iwr, bool set)
{
  return virtual_hwsim_wext(dev, iwr, SIOCSIWESSID, SIOCGIWESSID, set);
}

static int virtual_hwsim_bssid(FAR struct netdev_lowerhalf_s *dev,
                               FAR struct iwreq *iwr, bool set)
{
  return virtual_hwsim_wext(dev, iwr, SIOCSIWAP, SIOCGIWAP, set);
}

static int virtual_hwsim_passwd(FAR struct netdev_lowerhalf_s *dev,
                                FAR struct iwreq *iwr, bool set)
{
  return virtual_hwsim_wext(dev, iwr, SIOCSIWENCODEEXT,
                            SIOCGIWENCODEEXT, set);
}

static int virtual_hwsim_mode(FAR struct netdev_lowerhalf_s *dev,
                              FAR struct iwreq *iwr, bool set)
{
  return virtual_hwsim_wext(dev, iwr, SIOCSIWMODE, SIOCGIWMODE, set);
}

static int virtual_hwsim_auth(FAR struct netdev_lowerhalf_s *dev,
                              FAR struct iwreq *iwr, bool set)
{
  return virtual_hwsim_wext(dev, iwr, SIOCSIWAUTH, SIOCGIWAUTH, set);
}

static int virtual_hwsim_freq(FAR struct netdev_lowerhalf_s *dev,
                              FAR struct iwreq *iwr, bool set)
{
  return virtual_hwsim_wext(dev, iwr, SIOCSIWFREQ, SIOCGIWFREQ, set);
}

static int virtual_hwsim_bitrate(FAR struct netdev_lowerhalf_s *dev,
                                 FAR struct iwreq *iwr, bool set)
{
  return virtual_hwsim_wext(dev, iwr, SIOCSIWRATE, SIOCGIWRATE, set);
}

static int virtual_hwsim_txpower(FAR struct netdev_lowerhalf_s *dev,
                                 FAR struct iwreq *iwr, bool set)
{
  return virtual_hwsim_wext(dev, iwr, SIOCSIWTXPOW, SIOCGIWTXPOW, set);
}

static int virtual_hwsim_country(FAR struct netdev_lowerhalf_s *dev,
                                 FAR struct iwreq *iwr, bool set)
{
  return virtual_hwsim_wext(dev, iwr, SIOCSIWCOUNTRY, SIOCGIWCOUNTRY, set);
}

static int virtual_hwsim_sensitivity(FAR struct netdev_lowerhalf_s *dev,
                                     FAR struct iwreq *iwr, bool set)
{
  return virtual_hwsim_wext(dev, iwr, SIOCSIWSENS, SIOCGIWSENS, set);
}

static int virtual_hwsim_scan(FAR struct netdev_lowerhalf_s *dev,
                              FAR struct iwreq *iwr, bool set)
{
  return virtual_hwsim_wext(dev, iwr, SIOCSIWSCAN, SIOCGIWSCAN, set);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int virtual_hwsim_init(FAR struct wifi_sim_lowerhalf_s *netdev)
{
  int ret;

  ret = ieee80211_linux_initialize();
  if (ret < 0)
    {
      return ret;
    }

  ret = mac80211_hwsim_linux_initialize();
  if (ret < 0)
    {
      return ret;
    }

  netdev->lower.ops = &g_virtual_hwsim_ops;
  netdev->lower.iw_ops = &g_virtual_hwsim_iw_ops;
  netdev->wifi = NULL;

  (void)ieee80211_linux_sync_lowerhalf_mac(netdev->lower.netdev.d_ifname,
                                           &netdev->lower);

  return OK;
}

void virtual_hwsim_remove(FAR struct wifi_sim_lowerhalf_s *netdev)
{
  ieee80211_linux_unbind_lowerhalf(&netdev->lower);
  mac80211_hwsim_linux_uninitialize();
}

bool virtual_hwsim_connected(FAR struct wifi_sim_lowerhalf_s *netdev)
{
  return ieee80211_linux_lowerhalf_connected(&netdev->lower);
}
