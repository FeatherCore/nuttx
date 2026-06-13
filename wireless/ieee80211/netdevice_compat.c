/****************************************************************************
 * wireless/ieee80211/netdevice_compat.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include <nuttx/irq.h>
#include <linux/netdevice.h>
#include <net/cfg80211.h>
#include <net/net_namespace.h>
#include <net/wext.h>
#include <nuttx/wireless/ieee80211_linux.h>

#include "cfg80211/core.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void cfg80211_init_wdev(FAR struct wireless_dev *wdev);
struct net_driver_s;
FAR struct net_driver_s *netdev_findbyname(FAR const char *ifname);

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IEEE80211_COMPAT_MAX_NETDEVS 32
#define IEEE80211_COMPAT_MAX_BRIDGES 8
#define IEEE80211_COMPAT_MAX_AUTO_NETDEVS 8
#define IEEE80211_COMPAT_RX_QUEUE_LIMIT 32
#define IEEE80211_COMPAT_LINUX_IFF_UP 0x00000001
#define IEEE80211_COMPAT_ETH_ALEN 6
#define IEEE80211_COMPAT_ETH_HLEN 14
#define IEEE80211_COMPAT_IPV4_HLEN 20
#define IEEE80211_COMPAT_IPV6_HLEN 40
#define IEEE80211_COMPAT_ARP_HLEN 28
#define IEEE80211_COMPAT_IPPROTO_ICMPV6 58
#define IEEE80211_COMPAT_ICMPV6_NA 136
#define IEEE80211_COMPAT_ICMPV6_NA_SOLICITED 0x40

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ieee80211_compat_bridge
{
  FAR struct net_device *linux_dev;
  FAR struct netdev_lowerhalf_s *lower;
  FAR struct sk_buff *rx_head;
  FAR struct sk_buff *rx_tail;
  unsigned int rx_qlen;
  uint32_t data_frame_filters;
};

struct ieee80211_compat_auto_netdev
{
  struct netdev_lowerhalf_s lower;
  FAR struct net_device *linux_dev;
  bool registered;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct net_device *g_netdevices[IEEE80211_COMPAT_MAX_NETDEVS];
static struct ieee80211_compat_bridge
  g_bridges[IEEE80211_COMPAT_MAX_BRIDGES];
static struct ieee80211_compat_auto_netdev
  g_auto_netdevs[IEEE80211_COMPAT_MAX_AUTO_NETDEVS];
static int g_next_ifindex = 1;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int auto_netdev_ifup(FAR struct netdev_lowerhalf_s *lower);
static int auto_netdev_ifdown(FAR struct netdev_lowerhalf_s *lower);
static int auto_netdev_transmit(FAR struct netdev_lowerhalf_s *lower,
                                FAR netpkt_t *pkt);
static FAR netpkt_t *auto_netdev_receive(
  FAR struct netdev_lowerhalf_s *lower);
static int auto_netdev_connect(FAR struct netdev_lowerhalf_s *lower);
static int auto_netdev_disconnect(FAR struct netdev_lowerhalf_s *lower);
static int auto_netdev_iw_rw(FAR struct netdev_lowerhalf_s *lower,
                             FAR struct iwreq *iwr, bool set);
static void netdevice_sync_lowerhalf_mac(FAR struct net_device *linux_dev,
                                         FAR struct netdev_lowerhalf_s *lower);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct netdev_ops_s g_auto_netdev_ops =
{
  auto_netdev_ifup,
  auto_netdev_ifdown,
  auto_netdev_transmit,
  auto_netdev_receive
};

#ifdef CONFIG_NETDEV_WIRELESS_HANDLER
static const struct wireless_ops_s g_auto_netdev_iw_ops =
{
  auto_netdev_connect,
  auto_netdev_disconnect,
  auto_netdev_iw_rw, /* essid */
  auto_netdev_iw_rw, /* bssid */
  auto_netdev_iw_rw, /* passwd */
  auto_netdev_iw_rw, /* mode */
  auto_netdev_iw_rw, /* auth */
  auto_netdev_iw_rw, /* freq */
  auto_netdev_iw_rw, /* bitrate */
  auto_netdev_iw_rw, /* txpower */
  auto_netdev_iw_rw, /* country */
  auto_netdev_iw_rw, /* sensitivity */
  auto_netdev_iw_rw, /* scan */
  NULL               /* range */
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool netdevice_name_exists(FAR const char *name)
{
  int i;

  for (i = 0; i < IEEE80211_COMPAT_MAX_NETDEVS; i++)
    {
      if (g_netdevices[i] != NULL &&
          strncmp(g_netdevices[i]->name, name, IFNAMSIZ) == 0)
        {
          return true;
        }
    }

  return false;
}

static FAR struct net_device *netdevice_find_by_name(FAR const char *name)
{
  int i;

  if (name == NULL)
    {
      return NULL;
    }

  for (i = 0; i < IEEE80211_COMPAT_MAX_NETDEVS; i++)
    {
      if (g_netdevices[i] != NULL &&
          strncmp(g_netdevices[i]->name, name, IFNAMSIZ) == 0)
        {
          return g_netdevices[i];
        }
    }

  return NULL;
}

static FAR struct ieee80211_compat_bridge *
bridge_find_by_lower(FAR struct netdev_lowerhalf_s *lower)
{
  int i;

  if (lower == NULL)
    {
      return NULL;
    }

  for (i = 0; i < IEEE80211_COMPAT_MAX_BRIDGES; i++)
    {
      if (g_bridges[i].lower == lower)
        {
          return &g_bridges[i];
        }
    }

  return NULL;
}

static FAR struct ieee80211_compat_bridge *
bridge_find_by_linux_dev(FAR struct net_device *dev)
{
  int i;

  if (dev == NULL)
    {
      return NULL;
    }

  for (i = 0; i < IEEE80211_COMPAT_MAX_BRIDGES; i++)
    {
      if (g_bridges[i].linux_dev == dev)
        {
          return &g_bridges[i];
        }
    }

  return NULL;
}

static FAR struct sk_buff *
bridge_pop_rx(FAR struct ieee80211_compat_bridge *bridge)
{
  FAR struct sk_buff *skb;
  irqstate_t flags;

  flags = enter_critical_section();
  skb = bridge->rx_head;
  if (skb != NULL)
    {
      bridge->rx_head = skb->next;
      if (bridge->rx_head == NULL)
        {
          bridge->rx_tail = NULL;
        }

      bridge->rx_qlen--;
      skb->next = NULL;
    }

  leave_critical_section(flags);
  return skb;
}

static void bridge_purge_rx(FAR struct ieee80211_compat_bridge *bridge)
{
  FAR struct sk_buff *skb;

  while ((skb = bridge_pop_rx(bridge)) != NULL)
    {
      dev_kfree_skb(skb);
    }
}

static bool auto_netdev_should_register(FAR struct net_device *dev)
{
  if (dev == NULL)
    {
      return false;
    }

  if (netdev_findbyname(dev->name) != NULL ||
      bridge_find_by_linux_dev(dev) != NULL)
    {
      return false;
    }

  if (dev->ieee80211_ptr != NULL)
    {
      switch (dev->ieee80211_ptr->iftype)
        {
          case NL80211_IFTYPE_STATION:
          case NL80211_IFTYPE_AP:
            return true;

          default:
            break;
        }
    }

  return strncmp(dev->name, "p2p-", 4) == 0 ||
         strncmp(dev->name, "ap", 2) == 0;
}

static FAR struct ieee80211_compat_auto_netdev *
auto_netdev_find_by_linux_dev(FAR struct net_device *dev)
{
  int i;

  for (i = 0; i < IEEE80211_COMPAT_MAX_AUTO_NETDEVS; i++)
    {
      if (g_auto_netdevs[i].registered &&
          g_auto_netdevs[i].linux_dev == dev)
        {
          return &g_auto_netdevs[i];
        }
    }

  return NULL;
}

static FAR struct ieee80211_compat_auto_netdev *
auto_netdev_find_by_name(FAR const char *ifname)
{
  int i;

  if (ifname == NULL)
    {
      return NULL;
    }

  for (i = 0; i < IEEE80211_COMPAT_MAX_AUTO_NETDEVS; i++)
    {
      if (g_auto_netdevs[i].registered &&
          strncmp(g_auto_netdevs[i].lower.netdev.d_ifname, ifname,
                  IFNAMSIZ) == 0)
        {
          return &g_auto_netdevs[i];
        }
    }

  return NULL;
}

static FAR struct ieee80211_compat_auto_netdev *auto_netdev_alloc(void)
{
  int i;

  for (i = 0; i < IEEE80211_COMPAT_MAX_AUTO_NETDEVS; i++)
    {
      if (!g_auto_netdevs[i].registered)
        {
          return &g_auto_netdevs[i];
        }
    }

  return NULL;
}

static int auto_netdev_ifup(FAR struct netdev_lowerhalf_s *lower)
{
  int ret;

  ret = ieee80211_linux_bind_lowerhalf(lower->netdev.d_ifname, lower);
  if (ret < 0)
    {
      return ret;
    }

  ret = ieee80211_linux_set_netdev_flags(lower->netdev.d_ifname, true);
  if (ret < 0)
    {
      return ret;
    }

  netdev_lower_carrier_on(lower);
  return OK;
}

static int auto_netdev_ifdown(FAR struct netdev_lowerhalf_s *lower)
{
  netdev_lower_carrier_off(lower);
  return ieee80211_linux_set_netdev_flags(lower->netdev.d_ifname, false);
}

static int auto_netdev_transmit(FAR struct netdev_lowerhalf_s *lower,
                                FAR netpkt_t *pkt)
{
  return ieee80211_linux_transmit_netpkt(lower, pkt);
}

static FAR netpkt_t *auto_netdev_receive(
  FAR struct netdev_lowerhalf_s *lower)
{
  return ieee80211_linux_receive_netpkt(lower);
}

static int auto_netdev_connect(FAR struct netdev_lowerhalf_s *lower)
{
  (void)lower;
  return OK;
}

static int auto_netdev_disconnect(FAR struct netdev_lowerhalf_s *lower)
{
  (void)lower;
  return OK;
}

static int auto_netdev_iw_rw(FAR struct netdev_lowerhalf_s *lower,
                             FAR struct iwreq *iwr, bool set)
{
  (void)lower;
  (void)iwr;
  (void)set;
  return OK;
}

static int auto_netdev_register_lower(FAR struct net_device *dev)
{
  FAR struct ieee80211_compat_auto_netdev *auto_dev;
  int ret;

  if (!auto_netdev_should_register(dev))
    {
      return OK;
    }

  auto_dev = auto_netdev_find_by_linux_dev(dev);
  if (auto_dev != NULL)
    {
      return OK;
    }

  auto_dev = auto_netdev_find_by_name(dev->name);
  if (auto_dev != NULL)
    {
      auto_dev->linux_dev = dev;
      return ieee80211_linux_bind_lowerhalf(dev->name, &auto_dev->lower);
    }

  auto_dev = auto_netdev_alloc();
  if (auto_dev == NULL)
    {
      return -ENOSPC;
    }

  memset(auto_dev, 0, sizeof(*auto_dev));
  auto_dev->lower.ops = &g_auto_netdev_ops;
#ifdef CONFIG_NETDEV_WIRELESS_HANDLER
  auto_dev->lower.iw_ops = &g_auto_netdev_iw_ops;
#endif
  auto_dev->lower.quota[NETPKT_TX] = 1;
  auto_dev->lower.quota[NETPKT_RX] = 1;
  strlcpy(auto_dev->lower.netdev.d_ifname, dev->name,
          sizeof(auto_dev->lower.netdev.d_ifname));
  netdevice_sync_lowerhalf_mac(dev, &auto_dev->lower);

  ret = netdev_lower_register(&auto_dev->lower, NET_LL_IEEE80211);
  if (ret < 0)
    {
      nuttx_hwsim_debugf("netdevice_compat: auto lower register %s failed: %d\n",
             dev->name, ret);
      memset(auto_dev, 0, sizeof(*auto_dev));
      return ret;
    }

  auto_dev->linux_dev = dev;
  auto_dev->registered = true;
  (void)ieee80211_linux_bind_lowerhalf(dev->name, &auto_dev->lower);

  nuttx_hwsim_debugf("netdevice_compat: auto lower %s native_ifindex=%d linux_ifindex=%d\n",
         auto_dev->lower.netdev.d_ifname,
         auto_dev->lower.netdev.d_ifindex, dev->ifindex);
  return OK;
}

static void auto_netdev_unregister_lower(FAR struct net_device *dev)
{
  FAR struct ieee80211_compat_auto_netdev *auto_dev;

  auto_dev = auto_netdev_find_by_linux_dev(dev);
  if (auto_dev == NULL)
    {
      return;
    }

  ieee80211_linux_unbind_lowerhalf(&auto_dev->lower);
  if (auto_dev->registered)
    {
      (void)netdev_lower_unregister(&auto_dev->lower);
    }

  nuttx_hwsim_debugf("netdevice_compat: auto lower %s removed\n",
         auto_dev->lower.netdev.d_ifname);
  memset(auto_dev, 0, sizeof(*auto_dev));
}

static void netdevice_assign_name(FAR struct net_device *dev)
{
  FAR char *fmt;
  char name[IFNAMSIZ];
  size_t prefix;
  int i;

  fmt = strchr(dev->name, '%');
  if (fmt == NULL || fmt[1] != 'd')
    {
      return;
    }

  prefix = fmt - dev->name;

  for (i = 0; i < IEEE80211_COMPAT_MAX_NETDEVS; i++)
    {
      snprintf(name, sizeof(name), "%.*s%d%s", (int)prefix, dev->name, i,
               fmt + 2);
      if (!netdevice_name_exists(name))
        {
          strlcpy(dev->name, name, IFNAMSIZ);
          return;
        }
    }
}

static bool netdevice_mac_is_zero(FAR const unsigned char *addr)
{
  int i;

  for (i = 0; i < ETH_ALEN; i++)
    {
      if (addr[i] != 0)
        {
          return false;
        }
    }

  return true;
}

static bool netdevice_skb_from_own_mac(FAR struct net_device *dev,
                                       FAR struct sk_buff *skb)
{
  FAR const uint8_t *data;

  if (dev == NULL || skb == NULL || skb->data == NULL ||
      skb->len < IEEE80211_COMPAT_ETH_HLEN)
    {
      return false;
    }

  data = skb->data;
  return memcmp(&data[IEEE80211_COMPAT_ETH_ALEN], dev->dev_addr,
                IEEE80211_COMPAT_ETH_ALEN) == 0;
}

static uint16_t netdevice_get_be16(FAR const uint8_t *data)
{
  return ((uint16_t)data[0] << 8) | data[1];
}

static bool netdevice_l2_addr_is_broadcast(FAR const uint8_t *addr)
{
  int i;

  for (i = 0; i < IEEE80211_COMPAT_ETH_ALEN; i++)
    {
      if (addr[i] != 0xff)
        {
          return false;
        }
    }

  return true;
}

static bool netdevice_l2_addr_is_multicast(FAR const uint8_t *addr)
{
  return (addr[0] & 0x01) != 0;
}

static bool netdevice_ipv4_addr_is_multicast(FAR const uint8_t *addr)
{
  return (addr[0] & 0xf0) == 0xe0;
}

static bool netdevice_skb_is_unicast_in_l2_multicast(
    FAR struct sk_buff *skb, uint16_t proto)
{
  FAR const uint8_t *data = skb->data;
  FAR const uint8_t *l3 = &data[IEEE80211_COMPAT_ETH_HLEN];

  if (!netdevice_l2_addr_is_multicast(data) ||
      netdevice_l2_addr_is_broadcast(data))
    {
      return false;
    }

  if (proto == ETH_P_IP)
    {
      if (skb->len < IEEE80211_COMPAT_ETH_HLEN + IEEE80211_COMPAT_IPV4_HLEN)
        {
          return false;
        }

      return !netdevice_ipv4_addr_is_multicast(&l3[16]);
    }

  if (proto == ETH_P_IPV6)
    {
      if (skb->len < IEEE80211_COMPAT_ETH_HLEN + IEEE80211_COMPAT_IPV6_HLEN)
        {
          return false;
        }

      return l3[24] != 0xff;
    }

  return false;
}

static bool netdevice_skb_is_gratuitous_arp(FAR struct sk_buff *skb)
{
  FAR const uint8_t *arp;

  if (skb->len < IEEE80211_COMPAT_ETH_HLEN + IEEE80211_COMPAT_ARP_HLEN)
    {
      return false;
    }

  arp = &skb->data[IEEE80211_COMPAT_ETH_HLEN];
  if (netdevice_get_be16(&arp[2]) != ETH_P_IP || arp[4] != ETH_ALEN ||
      arp[5] != 4)
    {
      return false;
    }

  return memcmp(&arp[14], &arp[24], 4) == 0;
}

static bool netdevice_skb_is_unsolicited_na(FAR struct sk_buff *skb)
{
  FAR const uint8_t *ipv6;
  FAR const uint8_t *icmpv6;

  if (skb->len < IEEE80211_COMPAT_ETH_HLEN + IEEE80211_COMPAT_IPV6_HLEN + 8)
    {
      return false;
    }

  ipv6 = &skb->data[IEEE80211_COMPAT_ETH_HLEN];
  if ((ipv6[0] >> 4) != 6 || ipv6[6] != IEEE80211_COMPAT_IPPROTO_ICMPV6)
    {
      return false;
    }

  icmpv6 = &ipv6[IEEE80211_COMPAT_IPV6_HLEN];
  return icmpv6[0] == IEEE80211_COMPAT_ICMPV6_NA &&
         (icmpv6[4] & IEEE80211_COMPAT_ICMPV6_NA_SOLICITED) == 0;
}

static bool netdevice_skb_filtered(
    FAR struct ieee80211_compat_bridge *bridge, FAR struct sk_buff *skb)
{
  uint16_t proto;

  if (bridge->data_frame_filters == 0 || skb == NULL || skb->data == NULL ||
      skb->len < IEEE80211_COMPAT_ETH_HLEN)
    {
      return false;
    }

  proto = netdevice_get_be16(&skb->data[12]);
  if ((bridge->data_frame_filters &
       IEEE80211_LINUX_DATA_FRAME_FILTER_GTK) != 0 &&
      netdevice_skb_is_unicast_in_l2_multicast(skb, proto))
    {
      return true;
    }

  if ((bridge->data_frame_filters &
       IEEE80211_LINUX_DATA_FRAME_FILTER_ARP) != 0 &&
      proto == ETH_P_ARP && netdevice_skb_is_gratuitous_arp(skb))
    {
      return true;
    }

  if ((bridge->data_frame_filters &
       IEEE80211_LINUX_DATA_FRAME_FILTER_NA) != 0 &&
      proto == ETH_P_IPV6 && netdevice_skb_is_unsolicited_na(skb))
    {
      return true;
    }

  return false;
}

static void netdevice_sync_lowerhalf_mac(FAR struct net_device *linux_dev,
                                         FAR struct netdev_lowerhalf_s *lower)
{
  FAR uint8_t *lower_mac;

  if (linux_dev == NULL || lower == NULL ||
      netdevice_mac_is_zero(linux_dev->dev_addr))
    {
      return;
    }

  lower_mac = lower->netdev.d_mac.ether.ether_addr_octet;
  memcpy(lower_mac, linux_dev->dev_addr, ETH_ALEN);

  nuttx_hwsim_debugf("netdevice_compat: sync lower %s mac %02x:%02x:%02x:%02x:%02x:%02x\n",
         lower->netdev.d_ifname,
         lower_mac[0], lower_mac[1], lower_mac[2],
         lower_mac[3], lower_mac[4], lower_mac[5]);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

FAR struct net_device *dev_get_by_index(FAR struct net *net, int ifindex)
{
  int i;

  (void)net;

  for (i = 0; i < IEEE80211_COMPAT_MAX_NETDEVS; i++)
    {
      if (g_netdevices[i] != NULL && g_netdevices[i]->ifindex == ifindex)
        {
          nuttx_hwsim_debugf("netdevice_compat: dev_get_by_index(%d) -> %s\n",
                 ifindex, g_netdevices[i]->name);
          return g_netdevices[i];
        }
    }

  return NULL;
}

int ieee80211_linux_if_nametoindex(FAR const char *ifname)
{
  FAR struct net_device *dev;

  dev = netdevice_find_by_name(ifname);
  if (dev == NULL)
    {
      return 0;
    }

  nuttx_hwsim_debugf("netdevice_compat: if_nametoindex(%s) -> %d\n",
         ifname, dev->ifindex);
  return dev->ifindex;
}

int ieee80211_linux_if_indextonative(int ifindex)
{
  FAR struct ieee80211_compat_bridge *bridge;
  FAR struct net_device *dev;

  dev = dev_get_by_index(&init_net, ifindex);
  if (dev == NULL)
    {
      return 0;
    }

  bridge = bridge_find_by_linux_dev(dev);
  if (bridge == NULL || bridge->lower == NULL)
    {
      return 0;
    }

  return bridge->lower->netdev.d_ifindex;
}

int ieee80211_linux_get_netdev_mac(FAR const char *ifname,
                                   FAR uint8_t *addr)
{
  FAR struct net_device *dev;

  if (addr == NULL)
    {
      return -EINVAL;
    }

  dev = netdevice_find_by_name(ifname);
  if (dev == NULL)
    {
      return -ENODEV;
    }

  memcpy(addr, dev->dev_addr, ETH_ALEN);
  nuttx_hwsim_debugf("netdevice_compat: get mac %s -> %02x:%02x:%02x:%02x:%02x:%02x\n",
         ifname, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
  return 0;
}

int ieee80211_linux_set_netdev_mac(FAR const char *ifname,
                                   FAR const uint8_t *addr)
{
  FAR struct ieee80211_compat_bridge *bridge;
  FAR struct net_device *dev;
  struct sockaddr sa;
  int ret = 0;

  if (ifname == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  dev = netdevice_find_by_name(ifname);
  if (dev == NULL)
    {
      return -ENODEV;
    }

  if (dev->netdev_ops != NULL &&
      dev->netdev_ops->ndo_set_mac_address != NULL)
    {
      memset(&sa, 0, sizeof(sa));
      memcpy(sa.sa_data, addr, ETH_ALEN);
      ret = dev->netdev_ops->ndo_set_mac_address(dev, &sa);
      if (ret < 0)
        {
          return ret;
        }
    }
  else
    {
      memcpy(dev->dev_addr, addr, ETH_ALEN);
    }

  bridge = bridge_find_by_linux_dev(dev);
  if (bridge != NULL && bridge->lower != NULL)
    {
      netdevice_sync_lowerhalf_mac(dev, bridge->lower);
    }

  nuttx_hwsim_debugf("netdevice_compat: set mac %s -> %02x:%02x:%02x:%02x:%02x:%02x\n",
         ifname, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
  return ret;
}

int register_netdevice(FAR struct net_device *dev)
{
  bool register_wdev = false;
  int i;

  if (dev == NULL)
    {
      return -EINVAL;
    }

  netdevice_assign_name(dev);

  if (dev->ifindex <= 0)
    {
      dev->ifindex = g_next_ifindex++;
    }

  if (dev->ieee80211_ptr != NULL)
    {
      bool init_wdev = dev->ieee80211_ptr->netdev == NULL;

      dev->ieee80211_ptr->netdev = dev;
      if (init_wdev && dev->ieee80211_ptr->wiphy != NULL)
        {
          cfg80211_init_wdev(dev->ieee80211_ptr);
        }

      if (!dev->ieee80211_ptr->registered &&
          dev->ieee80211_ptr->wiphy != NULL)
        {
          register_wdev = true;
        }
    }

  for (i = 0; i < IEEE80211_COMPAT_MAX_NETDEVS; i++)
    {
      if (g_netdevices[i] == dev)
        {
          int ret;

          nuttx_hwsim_debugf("netdevice_compat: register existing %s ifindex=%d\n",
                 dev->name, dev->ifindex);
          ret = auto_netdev_register_lower(dev);
          if (ret == 0 && register_wdev)
            {
              cfg80211_register_wdev(
                wiphy_to_rdev(dev->ieee80211_ptr->wiphy),
                dev->ieee80211_ptr);
            }

          return ret;
        }
    }

  for (i = 0; i < IEEE80211_COMPAT_MAX_NETDEVS; i++)
    {
      if (g_netdevices[i] == NULL)
        {
          int ret;

          g_netdevices[i] = dev;
          nuttx_hwsim_debugf("netdevice_compat: register %s ifindex=%d\n",
                 dev->name, dev->ifindex);
          ret = auto_netdev_register_lower(dev);
          if (ret < 0)
            {
              g_netdevices[i] = NULL;
              return ret;
            }

          if (register_wdev)
            {
              cfg80211_register_wdev(
                wiphy_to_rdev(dev->ieee80211_ptr->wiphy),
                dev->ieee80211_ptr);
            }

          return 0;
        }
    }

  return -ENOSPC;
}

void unregister_netdevice(FAR struct net_device *dev)
{
  int i;

  if (dev == NULL)
    {
      return;
    }

  auto_netdev_unregister_lower(dev);
  if (dev->ieee80211_ptr != NULL && dev->ieee80211_ptr->registered)
    {
      cfg80211_unregister_wdev(dev->ieee80211_ptr);
    }

  for (i = 0; i < IEEE80211_COMPAT_MAX_NETDEVS; i++)
    {
      if (g_netdevices[i] == dev)
        {
          g_netdevices[i] = NULL;
          break;
        }
    }

  for (i = 0; i < IEEE80211_COMPAT_MAX_BRIDGES; i++)
    {
      if (g_bridges[i].linux_dev == dev)
        {
          bridge_purge_rx(&g_bridges[i]);
          memset(&g_bridges[i], 0, sizeof(g_bridges[i]));
        }
    }
}

int ieee80211_linux_set_netdev_flags(FAR const char *ifname, bool up)
{
  FAR struct net_device *dev;
  int ret = 0;

  dev = netdevice_find_by_name(ifname);
  if (dev == NULL)
    {
      return -ENODEV;
    }

  if (up)
    {
      if ((dev->flags & IEEE80211_COMPAT_LINUX_IFF_UP) != 0)
        {
          return 0;
        }

      if (dev->netdev_ops != NULL && dev->netdev_ops->ndo_open != NULL)
        {
          ret = dev->netdev_ops->ndo_open(dev);
          if (ret < 0)
            {
              nuttx_hwsim_debugf("netdevice_compat: ndo_open(%s) failed: %d\n",
                     dev->name, ret);
              return ret;
            }
        }

      nuttx_hwsim_debugf("netdevice_compat: %s up before=0x%x linux_up=0x%x nuttx_up=0x%x\n",
             dev->name, dev->flags, IEEE80211_COMPAT_LINUX_IFF_UP, IFF_UP);
      dev->flags |= IEEE80211_COMPAT_LINUX_IFF_UP | IFF_UP;
      nuttx_hwsim_debugf("netdevice_compat: %s up after=0x%x\n", dev->name,
             dev->flags);
      return 0;
    }

  if ((dev->flags & IEEE80211_COMPAT_LINUX_IFF_UP) == 0)
    {
      return 0;
    }

  dev->flags &= ~(IEEE80211_COMPAT_LINUX_IFF_UP | IFF_UP);
  if (dev->netdev_ops != NULL && dev->netdev_ops->ndo_stop != NULL)
    {
      ret = dev->netdev_ops->ndo_stop(dev);
      if (ret < 0)
        {
          nuttx_hwsim_debugf("netdevice_compat: ndo_stop(%s) failed: %d\n",
                 dev->name, ret);
          return ret;
        }
    }

  nuttx_hwsim_debugf("netdevice_compat: %s down flags=0x%x\n", dev->name,
         dev->flags);
  return 0;
}

int ieee80211_linux_sync_lowerhalf_mac(FAR const char *ifname,
                                       FAR struct netdev_lowerhalf_s *lower)
{
  FAR struct net_device *linux_dev;

  if (ifname == NULL || lower == NULL)
    {
      return -EINVAL;
    }

  linux_dev = netdevice_find_by_name(ifname);
  if (linux_dev == NULL)
    {
      return -ENODEV;
    }

  netdevice_sync_lowerhalf_mac(linux_dev, lower);
  return 0;
}

int ieee80211_linux_bind_lowerhalf(FAR const char *ifname,
                                   FAR struct netdev_lowerhalf_s *lower)
{
  FAR struct ieee80211_compat_bridge *empty = NULL;
  FAR struct ieee80211_compat_bridge *bridge;
  FAR struct net_device *linux_dev;
  int i;

  if (ifname == NULL || lower == NULL)
    {
      return -EINVAL;
    }

  linux_dev = netdevice_find_by_name(ifname);
  if (linux_dev == NULL)
    {
      nuttx_hwsim_debugf("netdevice_compat: no Linux netdev for lower %s\n", ifname);
      return -ENODEV;
    }

  bridge = bridge_find_by_lower(lower);
  if (bridge != NULL)
    {
      bridge->linux_dev = linux_dev;
      netdevice_sync_lowerhalf_mac(linux_dev, lower);
      nuttx_hwsim_debugf("netdevice_compat: rebind lower %s -> Linux %s ifindex=%d\n",
             ifname, linux_dev->name, linux_dev->ifindex);
      return 0;
    }

  for (i = 0; i < IEEE80211_COMPAT_MAX_BRIDGES; i++)
    {
      if (g_bridges[i].lower == NULL)
        {
          empty = &g_bridges[i];
          break;
        }
    }

  if (empty == NULL)
    {
      return -ENOSPC;
    }

  empty->linux_dev = linux_dev;
  empty->lower = lower;
  netdevice_sync_lowerhalf_mac(linux_dev, lower);
  nuttx_hwsim_debugf("netdevice_compat: bind lower %s -> Linux %s ifindex=%d\n",
         ifname, linux_dev->name, linux_dev->ifindex);
  return 0;
}

void ieee80211_linux_unbind_lowerhalf(FAR struct netdev_lowerhalf_s *lower)
{
  FAR struct ieee80211_compat_bridge *bridge;

  bridge = bridge_find_by_lower(lower);
  if (bridge == NULL)
    {
      return;
    }

  bridge_purge_rx(bridge);
  memset(bridge, 0, sizeof(*bridge));
}

bool ieee80211_linux_lowerhalf_connected(FAR struct netdev_lowerhalf_s *lower)
{
  FAR struct ieee80211_compat_bridge *bridge;

  bridge = bridge_find_by_lower(lower);
  if (bridge == NULL || bridge->linux_dev == NULL)
    {
      return false;
    }

  return (bridge->linux_dev->flags & IEEE80211_COMPAT_LINUX_IFF_UP) != 0 ||
         netif_carrier_ok(bridge->linux_dev);
}

int ieee80211_linux_transmit_netpkt(FAR struct netdev_lowerhalf_s *lower,
                                    FAR netpkt_t *pkt)
{
  FAR struct ieee80211_compat_bridge *bridge;
  FAR struct sk_buff *skb;
  FAR unsigned char *data;
  static int tx_logs;
  unsigned int headroom;
  unsigned int len;
  int ret;

  bridge = bridge_find_by_lower(lower);
  if (bridge == NULL || bridge->linux_dev == NULL)
    {
      return -ENODEV;
    }

  if (bridge->linux_dev->netdev_ops == NULL ||
      bridge->linux_dev->netdev_ops->ndo_start_xmit == NULL)
    {
      return -ENOTSUP;
    }

  len = netpkt_getdatalen(lower, pkt);
  headroom = bridge->linux_dev->needed_headroom;
  skb = dev_alloc_skb(headroom + len + bridge->linux_dev->needed_tailroom);
  if (skb == NULL)
    {
      return -ENOMEM;
    }

  skb_reserve(skb, headroom);
  data = skb_put(skb, len);
  ret = netpkt_copyout(lower, data, pkt, len, 0);
  if (ret < 0)
    {
      dev_kfree_skb(skb);
      return ret;
    }

  if (len >= 14)
    {
      skb->protocol = cpu_to_be16(((uint16_t)data[12] << 8) | data[13]);
    }

  if (tx_logs < 24)
    {
      unsigned int ethertype = 0;

      if (len >= 14)
        {
          ethertype = ((unsigned int)data[12] << 8) | data[13];
        }

      nuttx_hwsim_debugf("netdevice_compat: tx lower %s len=%u ethertype=0x%04x dst=%02x:%02x:%02x:%02x:%02x:%02x src=%02x:%02x:%02x:%02x:%02x:%02x\n",
             lower->netdev.d_ifname, len, ethertype,
             len >= 6 ? data[0] : 0, len >= 6 ? data[1] : 0,
             len >= 6 ? data[2] : 0, len >= 6 ? data[3] : 0,
             len >= 6 ? data[4] : 0, len >= 6 ? data[5] : 0,
             len >= 12 ? data[6] : 0, len >= 12 ? data[7] : 0,
             len >= 12 ? data[8] : 0, len >= 12 ? data[9] : 0,
             len >= 12 ? data[10] : 0, len >= 12 ? data[11] : 0);
      (void)ethertype;
      tx_logs++;
    }

  skb->dev = bridge->linux_dev;
  ret = bridge->linux_dev->netdev_ops->ndo_start_xmit(skb,
                                                       bridge->linux_dev);
  if (ret == NETDEV_TX_BUSY)
    {
      dev_kfree_skb(skb);
      netpkt_free(lower, pkt, NETPKT_TX);
      return -EAGAIN;
    }

  netpkt_free(lower, pkt, NETPKT_TX);
  return ret == NETDEV_TX_OK ? OK : -EIO;
}

FAR netpkt_t *ieee80211_linux_receive_netpkt(
  FAR struct netdev_lowerhalf_s *lower)
{
  FAR struct ieee80211_compat_bridge *bridge;
  FAR struct sk_buff *skb;
  FAR netpkt_t *pkt;
  int ret;

  bridge = bridge_find_by_lower(lower);
  if (bridge == NULL)
    {
      return NULL;
    }

  skb = bridge_pop_rx(bridge);
  if (skb == NULL)
    {
      return NULL;
    }

  pkt = netpkt_alloc(lower, NETPKT_RX);
  if (pkt == NULL)
    {
      dev_kfree_skb(skb);
      return NULL;
    }

  ret = netpkt_copyin(lower, pkt, skb->data, skb->len, 0);
  dev_kfree_skb(skb);
  if (ret < 0)
    {
      netpkt_free(lower, pkt, NETPKT_RX);
      return NULL;
    }

  return pkt;
}

int ieee80211_linux_wext_ioctl(FAR const char *ifname, int cmd, FAR void *arg)
{
  FAR struct iwreq *iwr = arg;

  if (ifname == NULL || iwr == NULL)
    {
      return -EINVAL;
    }

  strlcpy(iwr->ifr_name, ifname, IFNAMSIZ);
  return wext_handle_ioctl(&init_net, cmd, iwr);
}

int ieee80211_linux_set_data_frame_filters(FAR const char *ifname,
                                           uint32_t filter_flags)
{
  FAR struct ieee80211_compat_bridge *bridge;
  FAR struct net_device *dev;

  if (ifname == NULL)
    {
      return -EINVAL;
    }

  dev = netdevice_find_by_name(ifname);
  if (dev == NULL)
    {
      return -ENODEV;
    }

  bridge = bridge_find_by_linux_dev(dev);
  if (bridge == NULL)
    {
      return -ENODEV;
    }

  bridge->data_frame_filters =
    filter_flags & (IEEE80211_LINUX_DATA_FRAME_FILTER_ARP |
                    IEEE80211_LINUX_DATA_FRAME_FILTER_NA |
                    IEEE80211_LINUX_DATA_FRAME_FILTER_GTK);
  return 0;
}

int ieee80211_linux_netif_rx(FAR struct sk_buff *skb)
{
  FAR struct ieee80211_compat_bridge *bridge;
  irqstate_t flags;

  if (skb == NULL)
    {
      return -EINVAL;
    }

  bridge = bridge_find_by_linux_dev(skb->dev);
  if (bridge == NULL || bridge->lower == NULL)
    {
      dev_kfree_skb(skb);
      return 0;
    }

  /* Linux does not feed frames sourced by the local netdev MAC back into
   * the normal L3 receive path.  Packet sockets may observe outgoing frames
   * through the transmit path, but reflected TX frames from a bus-backed
   * device must not be delivered as inbound IP packets.  Without this guard,
   * raw ICMP sockets can receive their own echo requests as type 8 packets.
   */

  if (netdevice_skb_from_own_mac(skb->dev, skb))
    {
      dev_kfree_skb(skb);
      return 0;
    }

  if (netdevice_skb_filtered(bridge, skb))
    {
      dev_kfree_skb(skb);
      return 0;
    }

  flags = enter_critical_section();
  if (bridge->rx_qlen >= IEEE80211_COMPAT_RX_QUEUE_LIMIT)
    {
      leave_critical_section(flags);
      dev_kfree_skb(skb);
      return -ENOBUFS;
    }

  skb->next = NULL;
  if (bridge->rx_tail == NULL)
    {
      bridge->rx_head = skb;
      bridge->rx_tail = skb;
    }
  else
    {
      bridge->rx_tail->next = skb;
      bridge->rx_tail = skb;
    }

  bridge->rx_qlen++;
  leave_critical_section(flags);

  netdev_lower_rxready(bridge->lower);
  return 0;
}

int ieee80211_linux_dev_queue_xmit(FAR struct sk_buff *skb)
{
  FAR struct net_device *dev;
  int ret;

  if (skb == NULL)
    {
      return -EINVAL;
    }

  dev = skb->dev;
  if (dev == NULL || dev->netdev_ops == NULL ||
      dev->netdev_ops->ndo_start_xmit == NULL)
    {
      dev_kfree_skb(skb);
      return -ENODEV;
    }

  ret = dev->netdev_ops->ndo_start_xmit(skb, dev);
  if (ret == NETDEV_TX_BUSY)
    {
      dev_kfree_skb(skb);
      return -EAGAIN;
    }

  return ret == NETDEV_TX_OK ? OK : -EIO;
}
