#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_IF_ETHER_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_IF_ETHER_H
#include <linux/cfg80211_compat.h>
#define ETH_ALEN 6
#define ETH_HLEN 14
#define ETH_P_IP 0x0800
#define ETH_P_ARP 0x0806
#define ETH_P_AARP 0x80f3
#define ETH_P_IPX 0x8137
#define ETH_P_IPV6 0x86dd
#define ETH_P_PAE 0x888e
#define ETH_P_PREAUTH 0x88c7
#define ETH_P_802_2 0x0004
#define ETH_P_802_3 0x0001
#define ETH_P_802_3_MIN 0x0600
#define ETH_P_8021Q 0x8100
#define ETH_P_TDLS 0x890d
#define ETH_P_MPLS_UC 0x8847
#define ETH_P_MPLS_MC 0x8848
#define ETH_P_80221 0x8917
struct ethhdr
{
  unsigned char h_dest[ETH_ALEN];
  unsigned char h_source[ETH_ALEN];
  __be16 h_proto;
} __packed;
#endif
