/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/etherdevice.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_ETHERDEVICE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_ETHERDEVICE_H

#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>

static inline bool is_zero_ether_addr(const u8 *addr)
{
  return !addr[0] && !addr[1] && !addr[2] && !addr[3] && !addr[4] &&
         !addr[5];
}

static inline bool is_multicast_ether_addr(const u8 *addr)
{
  return (addr[0] & 0x01) != 0;
}

static inline bool is_valid_ether_addr(const u8 *addr)
{
  return !is_multicast_ether_addr(addr) && !is_zero_ether_addr(addr);
}

static inline bool ether_addr_equal(const u8 *addr1, const u8 *addr2)
{
  return memcmp(addr1, addr2, ETH_ALEN) == 0;
}

static inline void eth_broadcast_addr(u8 *addr)
{
  memset(addr, 0xff, ETH_ALEN);
}

static inline int eth_validate_addr(struct net_device *dev)
{
  return dev != NULL && is_valid_ether_addr(dev->dev_addr) ? 0 : -EINVAL;
}

static inline __be16 eth_type_trans(struct sk_buff *skb,
                                    struct net_device *dev)
{
  struct ethhdr *eth;
  __be16 proto;

  if (skb == NULL || skb->len < ETH_HLEN)
    {
      return 0;
    }

  skb->dev = dev;
  skb_reset_mac_header(skb);

  eth = (struct ethhdr *)skb->data;
  proto = eth->h_proto;
  skb_pull(skb, ETH_HLEN);
  skb->protocol = proto;
  return proto;
}

#endif
