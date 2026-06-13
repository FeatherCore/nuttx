/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/netdevice.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_NETDEVICE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_NETDEVICE_H

#include <linux/if.h>
#include <linux/device.h>
#include <linux/if_packet.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/slab.h>

#include <stdbool.h>
#include <string.h>

#ifndef ETH_ALEN
#  define ETH_ALEN 6
#endif
#ifndef ETH_HLEN
#  define ETH_HLEN 14
#endif
#ifndef ETH_DATA_LEN
#  define ETH_DATA_LEN 1500
#endif
#ifndef ETH_MAX_MTU
#  define ETH_MAX_MTU 65535
#endif

#ifndef NET_NAME_UNKNOWN
#  define NET_NAME_UNKNOWN 0
#endif

typedef enum
{
  NETDEV_TX_OK = 0,
  NETDEV_TX_BUSY = 1,
} netdev_tx_t;

typedef uint64_t netdev_features_t;

struct net_device;
struct net_device_path_ctx;
struct net_device_path;
struct notifier_block;
struct net;
struct ndisc_ops;

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

struct netdev_hw_addr
{
  unsigned char addr[ETH_ALEN];
};

struct netdev_hw_addr_list
{
  int count;
};

struct net_device_ops
{
  int (*ndo_init)(struct net_device *dev);
  int (*ndo_open)(struct net_device *dev);
  int (*ndo_stop)(struct net_device *dev);
  void (*ndo_uninit)(struct net_device *dev);
  netdev_tx_t (*ndo_start_xmit)(struct sk_buff *skb,
                                struct net_device *dev);
  void (*ndo_tx_timeout)(struct net_device *dev, unsigned int txqueue);
  void (*ndo_set_rx_mode)(struct net_device *dev);
  int (*ndo_set_mac_address)(struct net_device *dev, void *addr);
  int (*ndo_validate_addr)(struct net_device *dev);
  struct net_device_stats *(*ndo_get_stats)(struct net_device *dev);
};

struct header_ops
{
  int (*create)(struct sk_buff *skb, struct net_device *dev,
                unsigned short type, const void *daddr,
                const void *saddr, unsigned int len);
};

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NOTIFIER_BLOCK_DEFINED
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NOTIFIER_BLOCK_DEFINED
struct notifier_block
{
  int (*notifier_call)(struct notifier_block *nb, unsigned long action,
                       void *data);
};
#endif

struct net_device
{
  char name[IFNAMSIZ];
  int ifindex;
  unsigned int flags;
  unsigned int priv_flags;
  unsigned char dev_addr[ETH_ALEN];
  unsigned char perm_addr[ETH_ALEN];
  unsigned char broadcast[ETH_ALEN];
  unsigned char addr_len;
  const struct net_device_ops *netdev_ops;
  struct net_device_stats stats;
  unsigned short type;
  unsigned int mtu;
  unsigned int hard_header_len;
  unsigned int tx_queue_len;
  bool carrier;
  bool queue_stopped;
  netdev_features_t features;
  netdev_features_t hw_features;
  unsigned int min_mtu;
  unsigned int max_mtu;
  unsigned int needed_headroom;
  unsigned int needed_tailroom;
  unsigned int watchdog_timeo;
  unsigned char addr_assign_type;
  unsigned long state;
  bool needs_free_netdev;
  const struct ndisc_ops *ndisc_ops;
  const struct header_ops *header_ops;
  struct device dev;
  void *ieee802154_ptr;
  void *ml_priv;
};

#define netdev_priv(dev) ((dev)->ml_priv)
#define netdev_for_each_mc_addr(ha, dev) for ((ha) = NULL; false; )
#define netdev_err(dev, fmt, args...) do { (void)(dev); } while (0)
#define netdev_info(dev, fmt, args...) do { (void)(dev); } while (0)
#define netdev_warn(dev, fmt, args...) do { (void)(dev); } while (0)
#define net_info_ratelimited(fmt, args...) do { } while (0)

#ifndef IFF_TX_SKB_SHARING
#  define IFF_TX_SKB_SHARING 0x10000
#endif

#define NETDEV_UP 6
#define NETDEV_DOWN 7
#define NETDEV_CHANGE 8
#define NETDEV_UNREGISTER 9

#define NOTIFY_DONE 0
#define NOTIFY_OK 1

#define NET_RX_SUCCESS 0
#define NET_RX_DROP 1
#define NET_XMIT_SUCCESS 0
#define NET_XMIT_DROP 1

#define NET_ADDR_PERM 0
#define DEFAULT_TX_QUEUE_LEN 1000
#define __LINK_STATE_PRESENT 0

static inline bool netif_running(const struct net_device *dev)
{
  return dev != NULL && (dev->flags & IFF_UP);
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

static inline void netif_start_queue(struct net_device *dev)
{
  if (dev != NULL)
    {
      dev->queue_stopped = false;
    }
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

static inline void ether_setup(struct net_device *dev)
{
  if (dev != NULL)
    {
      dev->addr_len = ETH_ALEN;
      dev->hard_header_len = ETH_HLEN;
      dev->mtu = ETH_DATA_LEN;
      dev->min_mtu = 68;
      dev->max_mtu = ETH_MAX_MTU;
      dev->tx_queue_len = 1000;
      dev->type = 1;
      memset(dev->broadcast, 0xff, ETH_ALEN);
    }
}

#define SET_NETDEV_DEV(dev, parent) do { (void)(dev); (void)(parent); } while (0)
#define SET_NETDEV_DEVTYPE(dev, typep) do { (void)(dev); (void)(typep); } while (0)

static inline void netif_trans_update(struct net_device *dev)
{
  (void)dev;
}

static inline int netif_rx(struct sk_buff *skb)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP
  extern int linux_bt_bnep_netif_rx(struct sk_buff *skb);

  if (skb != NULL && skb->dev != NULL)
    {
      return linux_bt_bnep_netif_rx(skb);
    }
#endif

  kfree_skb(skb);
  return 0;
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

static inline void netdev_notify_peers(struct net_device *dev)
{
  (void)dev;
}

static inline void eth_hw_addr_set(struct net_device *dev,
                                   const unsigned char *addr)
{
  if (dev != NULL && addr != NULL)
    {
      memcpy(dev->dev_addr, addr, ETH_ALEN);
      dev->addr_len = ETH_ALEN;
    }
}

static inline struct net_device *alloc_netdev_mqs(
  int sizeof_priv, const char *name, unsigned char name_assign_type,
  void (*setup)(struct net_device *), unsigned int txqs, unsigned int rxqs)
{
  struct net_device *dev;

  (void)name_assign_type;
  (void)txqs;
  (void)rxqs;

  dev = kzalloc(sizeof(*dev), GFP_KERNEL);
  if (dev == NULL)
    {
      return NULL;
    }

  if (sizeof_priv > 0)
    {
      dev->ml_priv = kzalloc(sizeof_priv, GFP_KERNEL);
      if (dev->ml_priv == NULL)
        {
          kfree(dev);
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
      kfree(dev->ml_priv);
      kfree(dev);
    }
}

int register_netdevice(struct net_device *dev);
void unregister_netdevice(struct net_device *dev);

static inline int register_netdev(struct net_device *dev)
{
  return register_netdevice(dev);
}

static inline void unregister_netdev(struct net_device *dev)
{
  unregister_netdevice(dev);
}

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_NETDEVICE_H */
