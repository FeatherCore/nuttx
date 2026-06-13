#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_ETHTOOL_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_ETHTOOL_H
#include <linux/cfg80211_compat.h>
struct ethtool_ringparam
{
  u32 rx_max_pending;
  u32 rx_mini_max_pending;
  u32 rx_jumbo_max_pending;
  u32 tx_max_pending;
  u32 rx_pending;
  u32 rx_mini_pending;
  u32 rx_jumbo_pending;
  u32 tx_pending;
};
struct kernel_ethtool_ringparam
{
  int dummy;
};
struct ethtool_stats
{
  int dummy;
};
struct ethtool_regs
{
  u32 version;
  u32 len;
};
struct ethtool_ops
{
  void (*get_drvinfo)(struct net_device *dev, struct ethtool_drvinfo *info);
  int (*get_regs_len)(struct net_device *dev);
  void (*get_regs)(struct net_device *dev, struct ethtool_regs *regs,
                   void *data);
  u32 (*get_link)(struct net_device *dev);
  void (*get_ringparam)(struct net_device *dev,
                        struct ethtool_ringparam *ring,
                        struct kernel_ethtool_ringparam *kernel_ring,
                        struct netlink_ext_ack *extack);
  int (*set_ringparam)(struct net_device *dev,
                       struct ethtool_ringparam *ring,
                       struct kernel_ethtool_ringparam *kernel_ring,
                       struct netlink_ext_ack *extack);
  void (*get_strings)(struct net_device *dev, u32 stringset, u8 *data);
  void (*get_ethtool_stats)(struct net_device *dev,
                            struct ethtool_stats *stats, u64 *data);
  int (*get_sset_count)(struct net_device *dev, int sset);
};
#define ETHTOOL_FWVERS_LEN 32
#define ETH_GSTRING_LEN 32
#define ETH_SS_STATS 1
static inline u32 ethtool_op_get_link(struct net_device *dev)
{
  (void)dev;
  return 0;
}
#endif
