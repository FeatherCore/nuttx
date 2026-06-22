#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_NETDEVICE_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_NETDEVICE_H
#include <linux/cfg80211_compat.h>
#include <linux/device.h>
#include <linux/if.h>
#include <linux/inetdevice.h>
struct wireless_dev;
struct sk_buff;
struct net_device_stats
{
  unsigned long rx_packets;
  unsigned long tx_packets;
  unsigned long rx_bytes;
  unsigned long tx_bytes;
  unsigned long rx_errors;
  unsigned long tx_errors;
  unsigned long rx_dropped;
  unsigned long tx_dropped;
};

struct net_device
{
  char name[IFNAMSIZ];
  int ifindex;
  unsigned int flags;
  unsigned int priv_flags;
  bool netns_immutable;
  unsigned char dev_addr[6];
  unsigned char perm_addr[6];
  unsigned char broadcast[6];
  unsigned char addr_len;
  struct device dev;
  struct wireless_dev *ieee80211_ptr;
  struct netdev_hw_addr_list mc;
  const struct net_device_ops *netdev_ops;
  struct net_device_stats stats;
  unsigned short type;
  unsigned int mtu;
  unsigned int hard_header_len;
  unsigned int tx_queue_len;
  bool needs_free_netdev;
  bool carrier;
  bool queue_stopped;
  int pcpu_stat_type;
  unsigned int needed_headroom;
  unsigned int needed_tailroom;
  unsigned int watchdog_timeo;
  netdev_features_t features;
  netdev_features_t hw_features;
  unsigned int min_mtu;
  unsigned int max_mtu;
  void *ml_priv;
};
struct netdev_hw_addr
{
  unsigned char addr[6];
};
struct net_device_ops
{
  int (*ndo_open)(struct net_device *dev);
  int (*ndo_stop)(struct net_device *dev);
  void (*ndo_uninit)(struct net_device *dev);
  netdev_tx_t (*ndo_start_xmit)(struct sk_buff *skb,
                                struct net_device *dev);
  void (*ndo_set_rx_mode)(struct net_device *dev);
  int (*ndo_set_mac_address)(struct net_device *dev, void *addr);
  int (*ndo_validate_addr)(struct net_device *dev);
  struct net_device_stats *(*ndo_get_stats)(struct net_device *dev);
  u16 (*ndo_select_queue)(struct net_device *dev, struct sk_buff *skb,
                          struct net_device *sb_dev);
  int (*ndo_fill_forward_path)(struct net_device_path_ctx *ctx,
                               struct net_device_path *path);
  int (*ndo_setup_tc)(struct net_device *dev, enum tc_setup_type type,
                      void *type_data);
};
#define netdev_priv(dev) ((dev)->ml_priv)
#define NETDEV_ALIGN 32
#ifndef IFF_DONT_BRIDGE
#define IFF_DONT_BRIDGE 0x80000000U
#endif
#define IFF_TX_SKB_SHARING 0x40000000U
#define IFF_NO_QUEUE 0x20000000U
#define IFF_LIVE_ADDR_CHANGE 0x10000000U
#define NETDEV_PCPU_STAT_TSTATS 1
#define NETDEV_POST_INIT 1
#define NETDEV_REGISTER 2
#define NETDEV_UNREGISTER 3
#define NETDEV_GOING_DOWN 4
#define NETDEV_DOWN 5
#define NETDEV_UP 6
#define NETDEV_PRE_UP 7
#define NETDEV_CHANGENAME 8
#define netdev_for_each_mc_addr(ha, dev) \
  for ((ha) = NULL; false; )
#define netdev_err(dev, fmt, args...) do { (void)(dev); } while (0)
#define netdev_info(dev, fmt, args...) do { (void)(dev); } while (0)
#define net_info_ratelimited(fmt, args...) do { } while (0)
static inline bool netif_running(const struct net_device *dev)
{
  return dev && (dev->flags & IFF_UP);
}

static inline bool netif_carrier_ok(const struct net_device *dev)
{
  return dev != NULL && dev->carrier;
}

static inline void netif_carrier_on(struct net_device *dev)
{
  if (dev != NULL)
    {
      dev->carrier = true;
    }
}

static inline void netif_carrier_off(struct net_device *dev)
{
  if (dev != NULL)
    {
      dev->carrier = false;
    }
}

static inline bool netif_queue_stopped(const struct net_device *dev)
{
  return dev != NULL && dev->queue_stopped;
}

static inline void netif_stop_queue(struct net_device *dev)
{
  if (dev != NULL)
    {
      dev->queue_stopped = true;
    }
}

static inline void netif_wake_queue(struct net_device *dev)
{
  if (dev != NULL)
    {
      dev->queue_stopped = false;
    }
}

static inline void netif_trans_update(struct net_device *dev)
{
  (void)dev;
}

#ifndef __WIRELESS_LINUX_COMPAT_NETIF_RX_DEFINED
static inline int netif_rx(struct sk_buff *skb)
{
  (void)skb;
  return 0;
}
#endif

static inline void netif_device_detach(struct net_device *dev)
{
  if (dev != NULL)
    {
      dev->queue_stopped = true;
      dev->carrier = false;
    }
}

static inline void dev_hold(struct net_device *dev)
{
  (void)dev;
}
static inline void dev_put(struct net_device *dev)
{
  (void)dev;
}
struct net_device *dev_get_by_index(struct net *net, int ifindex);
static inline struct net_device *__dev_get_by_index(struct net *net,
                                                    int ifindex)
{
  return dev_get_by_index(net, ifindex);
}
static inline int dev_close(struct net_device *dev)
{
  if (dev)
    {
      dev->flags &= ~IFF_UP;
    }

  return 0;
}
void unregister_netdevice(struct net_device *dev);
static inline bool netif_is_bridge_port(const struct net_device *dev)
{
  (void)dev;
  return false;
}

static inline void ether_setup(struct net_device *dev)
{
  dev->addr_len = 6;
}

static inline void eth_hw_addr_set(struct net_device *dev, const u8 *addr)
{
  memcpy(dev->dev_addr, addr, dev->addr_len ? dev->addr_len : 6);
}

static inline struct net_device *alloc_netdev_mqs(int sizeof_priv,
                                                  const char *name,
                                                  unsigned char name_assign_type,
                                                  void (*setup)(struct net_device *),
                                                  unsigned int txqs,
                                                  unsigned int rxqs)
{
  struct net_device *dev;

  (void)name_assign_type;
  (void)txqs;
  (void)rxqs;

  dev = kmm_zalloc(sizeof(*dev));
  if (dev == NULL)
    {
      return NULL;
    }

  if (sizeof_priv > 0)
    {
      dev->ml_priv = kmm_zalloc(sizeof_priv);
      if (dev->ml_priv == NULL)
        {
          kmm_free(dev);
          return NULL;
        }
    }

  strlcpy(dev->name, name, IFNAMSIZ);
  if (setup != NULL)
    {
      setup(dev);
    }

  return dev;
}

#define alloc_netdev(sizeof_priv, name, name_assign_type, setup) \
  alloc_netdev_mqs(sizeof_priv, name, name_assign_type, setup, 1, 1)

static inline void free_netdev(struct net_device *dev)
{
  if (dev != NULL)
    {
      kmm_free(dev->ml_priv);
      kmm_free(dev);
    }
}

int register_netdevice(struct net_device *dev);

static inline int register_netdev(struct net_device *dev)
{
  return register_netdevice(dev);
}

static inline void unregister_netdev(struct net_device *dev)
{
  unregister_netdevice(dev);
}

static inline struct net_device *
netdev_notifier_info_to_dev(void *info)
{
  return (FAR struct net_device *)info;
}
static inline int register_netdevice_notifier(struct notifier_block *nb)
{
  (void)nb;
  return 0;
}
static inline int unregister_netdevice_notifier(struct notifier_block *nb)
{
  (void)nb;
  return 0;
}

static inline int register_inetaddr_notifier(struct notifier_block *nb)
{
  (void)nb;
  return 0;
}

static inline int unregister_inetaddr_notifier(struct notifier_block *nb)
{
  (void)nb;
  return 0;
}
#define SET_NETDEV_DEVTYPE(dev, devtype) do { \
  if (dev) \
    { \
      (dev)->dev.type = (devtype); \
    } \
} while (0)
#define SET_NETDEV_DEV(ndev_, parent_) do { \
  if (ndev_) \
    { \
      (ndev_)->dev.parent = (parent_); \
    } \
} while (0)
static inline int dev_alloc_name(struct net_device *dev, const char *name)
{
  strlcpy(dev->name, name, IFNAMSIZ);
  return 0;
}
static inline void netdev_set_default_ethtool_ops(struct net_device *dev,
                                                  const void *ops)
{
  (void)dev;
  (void)ops;
}
static inline void dev_net_set(struct net_device *dev, struct net *net)
{
  (void)dev;
  (void)net;
}
static inline int dev_change_net_namespace(struct net_device *dev,
                                           struct net *net,
                                           const char *pat)
{
  (void)dev;
  (void)net;
  (void)pat;
  return 0;
}
static inline void netif_addr_lock_bh(struct net_device *dev)
{
  (void)dev;
}
static inline void netif_addr_unlock_bh(struct net_device *dev)
{
  (void)dev;
}
static inline int __hw_addr_sync(struct netdev_hw_addr_list *to,
                                 struct netdev_hw_addr_list *from,
                                 int addr_len)
{
  (void)to;
  (void)from;
  (void)addr_len;
  return 0;
}
static inline void __hw_addr_unsync(struct netdev_hw_addr_list *to,
                                    struct netdev_hw_addr_list *from,
                                    int addr_len)
{
  (void)to;
  (void)from;
  (void)addr_len;
}
#endif
