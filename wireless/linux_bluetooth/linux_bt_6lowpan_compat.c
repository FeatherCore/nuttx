/****************************************************************************
 * wireless/linux_bluetooth/linux_bt_6lowpan_compat.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN

#include <errno.h>
#include <string.h>

#include <linux/debugfs.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <net/addrconf.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LINUX_BT_WEAK __attribute__((weak))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_linux_bt_6lowpan_ifindex = 1;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

LINUX_BT_WEAK struct sk_buff *skb_share_check(struct sk_buff *skb,
                                              gfp_t priority)
{
  (void)priority;
  return skb;
}

LINUX_BT_WEAK struct sk_buff *skb_unshare(struct sk_buff *skb,
                                          gfp_t priority)
{
  (void)priority;
  return skb;
}

LINUX_BT_WEAK struct sk_buff *skb_copy_expand(const struct sk_buff *skb,
                                              int newheadroom,
                                              int newtailroom,
                                              gfp_t priority)
{
  struct sk_buff *copy;

  (void)priority;

  if (skb == NULL || newheadroom < 0 || newtailroom < 0)
    {
      return NULL;
    }

  copy = alloc_skb((unsigned int)newheadroom + skb->len +
                   (unsigned int)newtailroom, GFP_KERNEL);
  if (copy == NULL)
    {
      return NULL;
    }

  skb_reserve(copy, (unsigned int)newheadroom);
  skb_put_data(copy, skb->data, skb->len);
  copy->protocol = skb->protocol;
  copy->dev = skb->dev;
  copy->pkt_type = skb->pkt_type;
  copy->priority = skb->priority;
  return copy;
}

LINUX_BT_WEAK void dev_kfree_skb(struct sk_buff *skb)
{
  kfree_skb(skb);
}

LINUX_BT_WEAK void consume_skb(struct sk_buff *skb)
{
  kfree_skb(skb);
}

LINUX_BT_WEAK int dev_hard_header(struct sk_buff *skb,
                                  struct net_device *dev,
                                  unsigned short type,
                                  const void *daddr,
                                  const void *saddr,
                                  unsigned int len)
{
  if (dev != NULL && dev->header_ops != NULL &&
      dev->header_ops->create != NULL)
    {
      return dev->header_ops->create(skb, dev, type, daddr, saddr, len);
    }

  (void)skb;
  (void)type;
  (void)daddr;
  (void)saddr;
  (void)len;
  return 0;
}

LINUX_BT_WEAK void netdev_lockdep_set_classes(struct net_device *dev)
{
  (void)dev;
}

LINUX_BT_WEAK void rtnl_lock(void)
{
}

LINUX_BT_WEAK void rtnl_unlock(void)
{
}

LINUX_BT_WEAK int dev_open(struct net_device *dev, void *extack)
{
  int ret = 0;

  (void)extack;

  if (dev == NULL)
    {
      return -EINVAL;
    }

  if (dev->netdev_ops != NULL && dev->netdev_ops->ndo_open != NULL)
    {
      ret = dev->netdev_ops->ndo_open(dev);
      if (ret < 0)
        {
          return ret;
        }
    }

  dev->flags |= IFF_UP | IFF_RUNNING;
  netif_carrier_on(dev);
  return 0;
}

LINUX_BT_WEAK int dev_close(struct net_device *dev)
{
  if (dev == NULL)
    {
      return -EINVAL;
    }

  if (dev->netdev_ops != NULL && dev->netdev_ops->ndo_stop != NULL)
    {
      dev->netdev_ops->ndo_stop(dev);
    }

  dev->flags &= ~IFF_UP;
  netif_carrier_off(dev);
  return 0;
}

LINUX_BT_WEAK void __dev_addr_set(struct net_device *dev,
                                  const void *addr, size_t len)
{
  if (dev == NULL || addr == NULL)
    {
      return;
    }

  if (len > ETH_ALEN)
    {
      len = ETH_ALEN;
    }

  memcpy(dev->dev_addr, addr, len);
  dev->addr_len = (unsigned char)len;
}

LINUX_BT_WEAK struct net_device *netdev_notifier_info_to_dev(void *info)
{
  return (struct net_device *)info;
}

LINUX_BT_WEAK struct dentry *debugfs_create_file_unsafe(
                                  const char *name, unsigned int mode,
                                  struct dentry *parent, void *data,
                                  const void *fops)
{
  return debugfs_create_file(name, mode, parent, data, fops);
}

LINUX_BT_WEAK int register_netdevice(struct net_device *dev)
{
  char name[IFNAMSIZ];
  char *fmt;

  if (dev == NULL)
    {
      return -EINVAL;
    }

  fmt = strchr(dev->name, '%');
  if (fmt != NULL && fmt[1] == 'd')
    {
      snprintf(name, sizeof(name), "%.*s%d%s", (int)(fmt - dev->name),
               dev->name, g_linux_bt_6lowpan_ifindex - 1, fmt + 2);
      strlcpy(dev->name, name, IFNAMSIZ);
    }

  dev->ifindex = g_linux_bt_6lowpan_ifindex++;
  dev->flags |= IFF_BROADCAST | IFF_MULTICAST;
  netif_carrier_on(dev);

  if (dev->netdev_ops != NULL && dev->netdev_ops->ndo_init != NULL)
    {
      return dev->netdev_ops->ndo_init(dev);
    }

  return 0;
}

LINUX_BT_WEAK void unregister_netdevice(struct net_device *dev)
{
  if (dev != NULL)
    {
      dev->flags &= ~IFF_UP;
      netif_carrier_off(dev);
    }
}

LINUX_BT_WEAK struct inet6_dev *__in6_dev_get(struct net_device *dev)
{
  (void)dev;
  return NULL;
}

LINUX_BT_WEAK int request_module_nowait(const char *name)
{
  (void)name;
  return 0;
}

#endif /* CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN */
