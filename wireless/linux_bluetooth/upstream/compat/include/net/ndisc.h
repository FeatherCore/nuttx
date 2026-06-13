/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/net/ndisc.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_NDISC_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_NDISC_H

#include <linux/types.h>
#include <net/ipv6.h>

#include <stdbool.h>

struct inet6_dev;
struct nd_opt_hdr;
struct ndisc_options;
struct net;
struct net_device;
struct prefix_info;
struct sk_buff;

#define ND_OPT_SOURCE_LL_ADDR          1
#define ND_OPT_TARGET_LL_ADDR          2
#define NDISC_ROUTER_SOLICITATION      133
#define NDISC_ROUTER_ADVERTISEMENT     134
#define NDISC_NEIGHBOUR_SOLICITATION   135
#define NDISC_NEIGHBOUR_ADVERTISEMENT  136
#define NDISC_REDIRECT                 137
#define NEIGH_UPDATE_F_OVERRIDE        0x00000001

struct ndisc_ops
{
  int (*parse_options)(const struct net_device *dev,
                       struct nd_opt_hdr *nd_opt,
                       struct ndisc_options *ndopts);
  void (*update)(const struct net_device *dev, struct neighbour *n,
                 u32 flags, u8 icmp6_type,
                 const struct ndisc_options *ndopts);
  int (*opt_addr_space)(const struct net_device *dev, u8 icmp6_type,
                        struct neighbour *neigh, u8 *ha_buf, u8 **ha);
  void (*fill_addr_option)(const struct net_device *dev, struct sk_buff *skb,
                           u8 icmp6_type, const u8 *ha);
  void (*prefix_rcv_add_addr)(struct net *net, struct net_device *dev,
                              const struct prefix_info *pinfo,
                              struct inet6_dev *in6_dev,
                              struct in6_addr *addr, int addr_type,
                              u32 addr_flags, bool sllao, bool tokenized,
                              __u32 valid_lft, u32 prefered_lft,
                              bool dev_addr_generated);
};

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_NDISC_H */
