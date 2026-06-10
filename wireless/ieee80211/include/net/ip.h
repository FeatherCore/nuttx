#ifndef __WIRELESS_IEEE80211_INCLUDE_NET_IP_H
#define __WIRELESS_IEEE80211_INCLUDE_NET_IP_H
#include <linux/cfg80211_compat.h>
struct iphdr
{
  u8 ihl:4;
  u8 version:4;
  u8 tos;
  __be16 tot_len;
  __be16 id;
  __be16 frag_off;
  u8 ttl;
  u8 protocol;
  __be16 check;
  __be32 saddr;
  __be32 daddr;
};
#endif
