/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/if_ether.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_IF_ETHER_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_IF_ETHER_H

#include <linux/types.h>

#ifndef __packed
#  define __packed __attribute__((packed))
#endif

#define ETH_ALEN 6
#define ETH_HLEN 14
#define ETH_ZLEN 60
#define ETH_DATA_LEN 1500
#define ETH_FRAME_LEN 1514
#define ETH_MAX_MTU 65535
#define ETH_P_802_3_MIN 0x0600
#define ETH_P_802_3 0x0001
#define ETH_P_IP 0x0800
#define ETH_P_ARP 0x0806
#define ETH_P_8021Q 0x8100
#define ETH_P_IPV6 0x86dd

struct ethhdr
{
  unsigned char h_dest[ETH_ALEN];
  unsigned char h_source[ETH_ALEN];
  __be16 h_proto;
} __packed;

#endif
