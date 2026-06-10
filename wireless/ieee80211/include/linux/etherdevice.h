#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_ETHERDEVICE_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_ETHERDEVICE_H
#include <linux/cfg80211_compat.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>
static inline bool is_zero_ether_addr(const u8 *addr)
{
  return !addr[0] && !addr[1] && !addr[2] && !addr[3] && !addr[4] && !addr[5];
}
static inline bool is_broadcast_ether_addr(const u8 *addr)
{
  return addr[0] == 0xff && addr[1] == 0xff && addr[2] == 0xff &&
         addr[3] == 0xff && addr[4] == 0xff && addr[5] == 0xff;
}
static inline bool is_multicast_ether_addr(const u8 *addr)
{
  return 0x01 & addr[0];
}
static inline bool is_unicast_ether_addr(const u8 *addr)
{
  return !is_multicast_ether_addr(addr);
}
static inline bool is_valid_ether_addr(const u8 *addr)
{
  return !is_multicast_ether_addr(addr) && !is_zero_ether_addr(addr);
}
static inline void ether_addr_copy(u8 *dst, const u8 *src)
{
  memcpy(dst, src, ETH_ALEN);
}
static inline void eth_zero_addr(u8 *addr)
{
  memset(addr, 0, ETH_ALEN);
}
static inline void eth_broadcast_addr(u8 *addr)
{
  memset(addr, 0xff, ETH_ALEN);
}

static inline void eth_random_addr(u8 *addr)
{
  static u8 counter = 1;

  addr[0] = 0x02;
  addr[1] = 0x00;
  addr[2] = 0x00;
  addr[3] = 0x00;
  addr[4] = 0x00;
  addr[5] = counter++;
}

static inline int eth_validate_addr(struct net_device *dev)
{
  return is_valid_ether_addr(dev->dev_addr) ? 0 : -EINVAL;
}

static inline int eth_mac_addr(struct net_device *dev, void *p)
{
  struct sockaddr *addr = p;

  if (addr == NULL || !is_valid_ether_addr((const u8 *)addr->sa_data))
    {
      return -EADDRNOTAVAIL;
    }

  ether_addr_copy(dev->dev_addr, (const u8 *)addr->sa_data);
  return 0;
}

static inline u64 ether_addr_to_u64(const u8 *addr)
{
  u64 value = 0;
  int i;

  for (i = 0; i < ETH_ALEN; i++)
    {
      value = (value << 8) | addr[i];
    }

  return value;
}
static inline void u64_to_ether_addr(u64 value, u8 *addr)
{
  int i;

  for (i = ETH_ALEN - 1; i >= 0; i--)
    {
      addr[i] = value & 0xff;
      value >>= 8;
    }
}
#endif
