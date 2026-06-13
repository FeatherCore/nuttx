/****************************************************************************
 * include/nuttx/wireless/ieee80211_linux.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_WIRELESS_IEEE80211_LINUX_H
#define __INCLUDE_NUTTX_WIRELESS_IEEE80211_LINUX_H

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include <nuttx/net/netdev_lowerhalf.h>

#define IEEE80211_LINUX_DATA_FRAME_FILTER_ARP (1u << 0)
#define IEEE80211_LINUX_DATA_FRAME_FILTER_NA  (1u << 1)
#define IEEE80211_LINUX_DATA_FRAME_FILTER_GTK (1u << 2)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

#if defined(CONFIG_WIRELESS_IEEE80211_CFG80211_LINUX) || \
    defined(CONFIG_WIRELESS_IEEE80211_MAC80211_LINUX)
int ieee80211_linux_initialize(void);
int ieee80211_linux_set_netdev_flags(const char *ifname, bool up);
int ieee80211_linux_if_nametoindex(const char *ifname);
int ieee80211_linux_if_indextonative(int ifindex);
int ieee80211_linux_get_netdev_mac(const char *ifname, uint8_t *addr);
int ieee80211_linux_set_netdev_mac(const char *ifname,
                                   const uint8_t *addr);
int ieee80211_linux_sync_lowerhalf_mac(const char *ifname,
                                       struct netdev_lowerhalf_s *lower);
int ieee80211_linux_bind_lowerhalf(const char *ifname,
                                   struct netdev_lowerhalf_s *lower);
void ieee80211_linux_unbind_lowerhalf(struct netdev_lowerhalf_s *lower);
bool ieee80211_linux_lowerhalf_connected(struct netdev_lowerhalf_s *lower);
int ieee80211_linux_transmit_netpkt(struct netdev_lowerhalf_s *lower,
                                    netpkt_t *pkt);
netpkt_t *ieee80211_linux_receive_netpkt(struct netdev_lowerhalf_s *lower);
int ieee80211_linux_wext_ioctl(const char *ifname, int cmd, void *arg);
int ieee80211_linux_set_data_frame_filters(const char *ifname,
                                           uint32_t filter_flags);
#else
static inline int ieee80211_linux_initialize(void)
{
  return 0;
}

static inline int ieee80211_linux_set_netdev_flags(const char *ifname,
                                                   bool up)
{
  return 0;
}

static inline int ieee80211_linux_if_nametoindex(const char *ifname)
{
  return 0;
}

static inline int ieee80211_linux_if_indextonative(int ifindex)
{
  return 0;
}

static inline int ieee80211_linux_get_netdev_mac(const char *ifname,
                                                 uint8_t *addr)
{
  return -ENODEV;
}

static inline int ieee80211_linux_set_netdev_mac(const char *ifname,
                                                 const uint8_t *addr)
{
  return -ENODEV;
}

static inline int ieee80211_linux_bind_lowerhalf(
  const char *ifname, struct netdev_lowerhalf_s *lower)
{
  return 0;
}

static inline int ieee80211_linux_sync_lowerhalf_mac(
  const char *ifname, struct netdev_lowerhalf_s *lower)
{
  return 0;
}

static inline void ieee80211_linux_unbind_lowerhalf(
  struct netdev_lowerhalf_s *lower)
{
}

static inline bool ieee80211_linux_lowerhalf_connected(
  struct netdev_lowerhalf_s *lower)
{
  return false;
}

static inline int ieee80211_linux_transmit_netpkt(
  struct netdev_lowerhalf_s *lower, netpkt_t *pkt)
{
  return 0;
}

static inline netpkt_t *ieee80211_linux_receive_netpkt(
  struct netdev_lowerhalf_s *lower)
{
  return NULL;
}

static inline int ieee80211_linux_wext_ioctl(const char *ifname, int cmd,
                                             void *arg)
{
  return -ENODEV;
}

static inline int ieee80211_linux_set_data_frame_filters(
  const char *ifname, uint32_t filter_flags)
{
  return -ENODEV;
}
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_NUTTX_WIRELESS_IEEE80211_LINUX_H */
