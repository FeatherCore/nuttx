#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_IF_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_IF_H
#include <linux/cfg80211_compat.h>
#define IFNAMSIZ 16
enum net_device_flags
{
  IFF_UP = 1 << 0,
  IFF_BROADCAST = 1 << 1,
  IFF_DEBUG = 1 << 2,
  IFF_LOOPBACK = 1 << 3,
  IFF_POINTOPOINT = 1 << 4,
  IFF_NOTRAILERS = 1 << 5,
  IFF_RUNNING = 1 << 6,
  IFF_NOARP = 1 << 7,
  IFF_PROMISC = 1 << 8,
  IFF_ALLMULTI = 1 << 9,
  IFF_MASTER = 1 << 10,
  IFF_SLAVE = 1 << 11,
  IFF_MULTICAST = 1 << 12,
};
#endif
