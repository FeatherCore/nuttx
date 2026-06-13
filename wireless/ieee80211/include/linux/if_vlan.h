#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_IF_VLAN_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_IF_VLAN_H
#include <linux/cfg80211_compat.h>
#include <linux/if_ether.h>
#define VLAN_PRIO_MASK 0xe000
#define VLAN_PRIO_SHIFT 13
#define VLAN_HLEN 4
#define VLAN_ETH_HLEN (ETH_HLEN + VLAN_HLEN)
struct vlan_ethhdr
{
  unsigned char h_dest[ETH_ALEN];
  unsigned char h_source[ETH_ALEN];
  __be16 h_vlan_proto;
  __be16 h_vlan_TCI;
  __be16 h_vlan_encapsulated_proto;
} __packed;
#endif
