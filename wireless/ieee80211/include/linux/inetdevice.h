#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_INETDEVICE_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_INETDEVICE_H
#include <linux/cfg80211_compat.h>

struct net_device;

struct in_device
{
  struct net_device *dev;
};

struct in_ifaddr
{
  struct in_device *ifa_dev;
  __be32 ifa_local;
  __be32 ifa_address;
  __be32 ifa_mask;
};
#endif
