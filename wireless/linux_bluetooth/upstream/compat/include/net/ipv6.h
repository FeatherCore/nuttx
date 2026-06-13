/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/net/ipv6.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_IPV6_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_IPV6_H

#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/if_ether.h>

#include <netinet/in.h>
#include <netinet/udp.h>

#include <stdbool.h>
#include <string.h>

struct net_device;

struct neighbour
{
  unsigned char ha[6];
};

#ifndef IPV6_MIN_MTU
#  define IPV6_MIN_MTU 1280
#endif

#ifndef NEXTHDR_IPV6
#  define NEXTHDR_IPV6 IPPROTO_IPV6
#endif
#ifndef NEXTHDR_HOP
#  define NEXTHDR_HOP 0
#endif
#ifndef NEXTHDR_TCP
#  define NEXTHDR_TCP 6
#endif
#ifndef NEXTHDR_UDP
#  define NEXTHDR_UDP 17
#endif
#ifndef NEXTHDR_ROUTING
#  define NEXTHDR_ROUTING 43
#endif
#ifndef NEXTHDR_FRAGMENT
#  define NEXTHDR_FRAGMENT 44
#endif
#ifndef NEXTHDR_ESP
#  define NEXTHDR_ESP 50
#endif
#ifndef NEXTHDR_AUTH
#  define NEXTHDR_AUTH 51
#endif
#ifndef NEXTHDR_NONE
#  define NEXTHDR_NONE 59
#endif
#ifndef NEXTHDR_DEST
#  define NEXTHDR_DEST 60
#endif
#ifndef NEXTHDR_MAX
#  define NEXTHDR_MAX 255
#endif
#ifndef NEXTHDR_MOBILITY
#  define NEXTHDR_MOBILITY 135
#endif

#ifndef IPV6_ADDR_ANY
#  define IPV6_ADDR_ANY       0x0000U
#endif
#ifndef IPV6_ADDR_UNICAST
#  define IPV6_ADDR_UNICAST   0x0001U
#endif
#ifndef IPV6_ADDR_MULTICAST
#  define IPV6_ADDR_MULTICAST 0x0002U
#endif
#ifndef IPV6_ADDR_LINKLOCAL
#  define IPV6_ADDR_LINKLOCAL 0x0020U
#endif

struct ipv6hdr
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  __u8 priority:4;
  __u8 version:4;
#else
  __u8 version:4;
  __u8 priority:4;
#endif
  __u8 flow_lbl[3];
  __be16 payload_len;
  __u8 nexthdr;
  __u8 hop_limit;
  struct in6_addr saddr;
  struct in6_addr daddr;
};

static inline struct ipv6hdr *ipv6_hdr(const struct sk_buff *skb)
{
  return (struct ipv6hdr *)skb_network_header(skb);
}

static inline bool ipv6_addr_equal(const struct in6_addr *a1,
                                   const struct in6_addr *a2)
{
  return memcmp(a1, a2, sizeof(*a1)) == 0;
}

static inline int ipv6_addr_cmp(const struct in6_addr *a1,
                                const struct in6_addr *a2)
{
  return memcmp(a1, a2, sizeof(*a1));
}

static inline bool ipv6_addr_any(const struct in6_addr *a)
{
  return IN6_IS_ADDR_UNSPECIFIED(a);
}

static inline bool ipv6_addr_is_multicast(const struct in6_addr *a)
{
  return IN6_IS_ADDR_MULTICAST(a);
}

static inline int ipv6_addr_type(const struct in6_addr *a)
{
  if (IN6_IS_ADDR_UNSPECIFIED(a))
    {
      return IPV6_ADDR_ANY;
    }

  if (IN6_IS_ADDR_MULTICAST(a))
    {
      return IPV6_ADDR_MULTICAST;
    }

  if (IN6_IS_ADDR_LINKLOCAL(a))
    {
      return IPV6_ADDR_UNICAST | IPV6_ADDR_LINKLOCAL;
    }

  return IPV6_ADDR_UNICAST;
}

static inline void ipv6_addr_prefix_copy(struct in6_addr *pfx,
                                         const struct in6_addr *addr,
                                         int plen)
{
  int bytes = plen / 8;
  int bits = plen % 8;

  memset(pfx, 0, sizeof(*pfx));
  if (bytes > 0)
    {
      memcpy(pfx->s6_addr, addr->s6_addr, bytes);
    }

  if (bits != 0 && bytes < 16)
    {
      pfx->s6_addr[bytes] = addr->s6_addr[bytes] & (0xff << (8 - bits));
    }
}

static inline void ipv6_addr_prefix(struct in6_addr *pfx,
                                    const struct in6_addr *addr, int plen)
{
  ipv6_addr_prefix_copy(pfx, addr, plen);
}

static inline bool ipv6_prefix_equal(const struct in6_addr *a1,
                                     const struct in6_addr *a2,
                                     unsigned int prefixlen)
{
  unsigned int bytes = prefixlen / 8;
  unsigned int bits = prefixlen % 8;
  uint8_t mask;

  if (bytes > 16)
    {
      bytes = 16;
      bits = 0;
    }

  if (bytes > 0 && memcmp(a1->s6_addr, a2->s6_addr, bytes) != 0)
    {
      return false;
    }

  if (bits == 0 || bytes >= 16)
    {
      return true;
    }

  mask = (uint8_t)(0xff << (8 - bits));
  return (a1->s6_addr[bytes] & mask) == (a2->s6_addr[bytes] & mask);
}

static inline void __ipv6_addr_set_half(__be32 *addr,
                                        __be32 wh, __be32 wl)
{
  addr[0] = wh;
  addr[1] = wl;
}

static inline struct neighbour *__ipv6_neigh_lookup(struct net_device *dev,
                                                    const struct in6_addr *addr)
{
  (void)dev;
  (void)addr;
  return NULL;
}

static inline void neigh_release(struct neighbour *neigh)
{
  (void)neigh;
}

static inline void *neighbour_priv(struct neighbour *neigh)
{
  return neigh;
}

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_IPV6_H */
