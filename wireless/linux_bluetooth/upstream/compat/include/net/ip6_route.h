/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/net/ip6_route.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_IP6_ROUTE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_IP6_ROUTE_H

#include <net/ipv6.h>

struct dst_entry
{
  int unused;
};

struct rt6_info
{
  struct in6_addr rt6i_gateway;
};

static inline struct dst_entry *skb_dst(const struct sk_buff *skb)
{
  (void)skb;
  return NULL;
}

static inline struct rt6_info *dst_rt6_info(struct dst_entry *dst)
{
  (void)dst;
  return NULL;
}

static inline const struct in6_addr *rt6_nexthop(struct rt6_info *rt,
                                                 const struct in6_addr *daddr)
{
  if (rt != NULL && !ipv6_addr_any(&rt->rt6i_gateway))
    {
      return &rt->rt6i_gateway;
    }

  return daddr;
}

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_IP6_ROUTE_H */
