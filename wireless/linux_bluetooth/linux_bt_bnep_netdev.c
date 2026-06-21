/****************************************************************************
 * wireless/linux_bluetooth/linux_bt_bnep_netdev.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_NET_LINUX_BLUETOOTH

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <nuttx/clock.h>
#include <nuttx/net/ethernet.h>
#include <nuttx/net/netdev_lowerhalf.h>
#include <nuttx/net/pkt.h>
#include <nuttx/wireless/linux_bluetooth.h>
#include <nuttx/wqueue.h>

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP
#  include <linux/netdevice.h>
#  include <linux/skbuff.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LINUX_BT_BNEP_HWSIM_BUFSIZE 1514
#define LINUX_BT_BNEP_HWSIM_PERIOD  MSEC2TICK(20)
#define LINUX_BT_BNEP_HWSIM_MTU     1500
#define LINUX_BT_BNEP_NETPKT_QUOTA  8

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct linux_bt_bnep_hwsim_netdev_s
{
  struct netdev_lowerhalf_s lower;
  struct work_s work;
  uint8_t buf[LINUX_BT_BNEP_HWSIM_BUFSIZE];
  bool registered;
};

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP
struct linux_bt_bnep_linux_netdev_s
{
  struct netdev_lowerhalf_s lower;
  struct work_s work;
  FAR struct net_device *linux_dev;
  FAR struct sk_buff *rx_head;
  FAR struct sk_buff *rx_tail;
  bool registered;
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int linux_bt_bnep_hwsim_send(FAR struct netdev_lowerhalf_s *lower,
                                    FAR netpkt_t *pkt);
static FAR netpkt_t *linux_bt_bnep_hwsim_recv(
                                    FAR struct netdev_lowerhalf_s *lower);
static int linux_bt_bnep_hwsim_ifup(FAR struct netdev_lowerhalf_s *lower);
static int linux_bt_bnep_hwsim_ifdown(FAR struct netdev_lowerhalf_s *lower);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct linux_bt_bnep_hwsim_netdev_s g_bnep_hwsim;

static const struct netdev_ops_s g_bnep_hwsim_ops =
{
  linux_bt_bnep_hwsim_ifup,
  linux_bt_bnep_hwsim_ifdown,
  linux_bt_bnep_hwsim_send,
  linux_bt_bnep_hwsim_recv
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int linux_bt_bnep_hwsim_send(FAR struct netdev_lowerhalf_s *lower,
                                    FAR netpkt_t *pkt)
{
  unsigned int len = netpkt_getdatalen(lower, pkt);
  int ret;

  if (len > sizeof(g_bnep_hwsim.buf))
    {
      len = sizeof(g_bnep_hwsim.buf);
    }

  netpkt_copyout(lower, g_bnep_hwsim.buf, pkt, len, 0);
  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_BNEP,
                            LINUX_BT_HWSIM_DST_BROADCAST,
                            g_bnep_hwsim.buf, len);

  netpkt_free(lower, pkt, NETPKT_TX);
  netdev_lower_txdone(lower);
  return ret < 0 ? ret : OK;
}

static FAR netpkt_t *linux_bt_bnep_hwsim_recv(
                                    FAR struct netdev_lowerhalf_s *lower)
{
  FAR netpkt_t *pkt;
  uint16_t src;
  uint16_t dst;
  uint32_t len;
  int ret;

  ret = linux_bt_hwsim_read_raw(LINUX_BT_HWSIM_TYPE_BNEP, &src, &dst,
                                g_bnep_hwsim.buf,
                                sizeof(g_bnep_hwsim.buf), &len);
  if (ret <= 0 || len == 0)
    {
      return NULL;
    }

  pkt = netpkt_alloc(lower, NETPKT_RX);
  if (pkt == NULL)
    {
      return NULL;
    }

  netpkt_copyin(lower, pkt, g_bnep_hwsim.buf, len, 0);
  return pkt;
}

static int linux_bt_bnep_hwsim_ifup(FAR struct netdev_lowerhalf_s *lower)
{
  netdev_lower_carrier_on(lower);
  return OK;
}

static int linux_bt_bnep_hwsim_ifdown(FAR struct netdev_lowerhalf_s *lower)
{
  netdev_lower_carrier_off(lower);
  return OK;
}

static void linux_bt_bnep_hwsim_work(FAR void *arg)
{
  FAR struct linux_bt_bnep_hwsim_netdev_s *priv = arg;

  if (!priv->registered)
    {
      return;
    }

  netdev_lower_rxready(&priv->lower);
  work_queue(HPWORK, &priv->work, linux_bt_bnep_hwsim_work, priv,
             LINUX_BT_BNEP_HWSIM_PERIOD);
}

static void linux_bt_bnep_hwsim_setaddr(FAR struct netdev_lowerhalf_s *lower)
{
  FAR uint8_t *mac = lower->netdev.d_mac.ether.ether_addr_octet;
  uint8_t role;

#ifdef CONFIG_SIM_BTHWSIM_ROLE
  role = CONFIG_SIM_BTHWSIM_ROLE;
#else
  role = 0xfe;
#endif

  mac[0] = 0x02;
  mac[1] = 0xfe;
  mac[2] = 0x42;
  mac[3] = 0x00;
  mac[4] = 0x00;
  mac[5] = role;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int linux_bt_bnep_hwsim_netdev_register(FAR const char *name,
                                        FAR char *actual_name,
                                        size_t actual_name_len)
{
  int ret;

  if (g_bnep_hwsim.registered)
    {
      if (actual_name != NULL && actual_name_len > 0)
        {
          strlcpy(actual_name, g_bnep_hwsim.lower.netdev.d_ifname,
                  actual_name_len);
        }

      return OK;
    }

  memset(&g_bnep_hwsim, 0, sizeof(g_bnep_hwsim));
  g_bnep_hwsim.lower.ops = &g_bnep_hwsim_ops;
  g_bnep_hwsim.lower.quota[NETPKT_TX] = LINUX_BT_BNEP_NETPKT_QUOTA;
  g_bnep_hwsim.lower.quota[NETPKT_RX] = LINUX_BT_BNEP_NETPKT_QUOTA;
  g_bnep_hwsim.lower.rxtype = NETDEV_RX_WORK;
  g_bnep_hwsim.lower.netdev.d_llhdrlen = ETH_HDRLEN;
  g_bnep_hwsim.lower.netdev.d_pktsize =
    LINUX_BT_BNEP_HWSIM_MTU + ETH_HDRLEN;
  linux_bt_bnep_hwsim_setaddr(&g_bnep_hwsim.lower);

  if (name != NULL && name[0] != '\0')
    {
      strlcpy(g_bnep_hwsim.lower.netdev.d_ifname, name,
              sizeof(g_bnep_hwsim.lower.netdev.d_ifname));
    }
  else
    {
      strlcpy(g_bnep_hwsim.lower.netdev.d_ifname, "btn%d",
              sizeof(g_bnep_hwsim.lower.netdev.d_ifname));
    }

  ret = netdev_lower_register(&g_bnep_hwsim.lower, NET_LL_ETHERNET);
  if (ret < 0)
    {
      memset(&g_bnep_hwsim, 0, sizeof(g_bnep_hwsim));
      return ret;
    }

  g_bnep_hwsim.registered = true;
  if (actual_name != NULL && actual_name_len > 0)
    {
      strlcpy(actual_name, g_bnep_hwsim.lower.netdev.d_ifname,
              actual_name_len);
    }

  work_queue(HPWORK, &g_bnep_hwsim.work, linux_bt_bnep_hwsim_work,
             &g_bnep_hwsim, LINUX_BT_BNEP_HWSIM_PERIOD);
  return OK;
}

void linux_bt_bnep_hwsim_netdev_unregister(void)
{
  if (!g_bnep_hwsim.registered)
    {
      return;
    }

  g_bnep_hwsim.registered = false;
  work_cancel(HPWORK, &g_bnep_hwsim.work);
  netdev_lower_unregister(&g_bnep_hwsim.lower);
  memset(&g_bnep_hwsim, 0, sizeof(g_bnep_hwsim));
}

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP

/* Explicit Linux-BT side boundary for the future full upstream BNEP PAN
 * netdev bridge.  Keep BNEP protocol behavior in the imported upstream BNEP
 * files and use these wrappers only as named handoff points while wiring
 * BR/EDR PAN into the NuttX IP stack.
 */

static struct linux_bt_bnep_linux_netdev_s g_bnep_linux_netdev;
static int g_bnep_linux_next_ifindex = 1;

static void linux_bt_bnep_assign_name(FAR struct net_device *dev)
{
  FAR char *fmt;
  char name[IFNAMSIZ];

  fmt = strchr(dev->name, '%');
  if (fmt == NULL || fmt[1] != 'd')
    {
      return;
    }

  snprintf(name, sizeof(name), "%.*s0%s", (int)(fmt - dev->name),
           dev->name, fmt + 2);
  strlcpy(dev->name, name, IFNAMSIZ);
}

int register_netdevice(FAR struct net_device *dev)
{
  if (dev == NULL)
    {
      return -EINVAL;
    }

  linux_bt_bnep_assign_name(dev);
  dev->ifindex = g_bnep_linux_next_ifindex++;
  dev->flags |= IFF_BROADCAST | IFF_MULTICAST;
  netif_carrier_on(dev);
  return OK;
}

void unregister_netdevice(FAR struct net_device *dev)
{
  if (dev != NULL)
    {
      dev->flags &= ~IFF_UP;
      netif_carrier_off(dev);
    }
}

static FAR struct sk_buff *linux_bt_bnep_linux_pop_rx(void)
{
  FAR struct sk_buff *skb;

  skb = g_bnep_linux_netdev.rx_head;
  if (skb != NULL)
    {
      g_bnep_linux_netdev.rx_head = skb->next;
      if (g_bnep_linux_netdev.rx_head == NULL)
        {
          g_bnep_linux_netdev.rx_tail = NULL;
        }

      skb->next = NULL;
    }

  return skb;
}

static int linux_bt_bnep_linux_ifup(FAR struct netdev_lowerhalf_s *lower)
{
  FAR struct net_device *dev = g_bnep_linux_netdev.linux_dev;
  int ret = OK;

  if (dev != NULL && dev->netdev_ops != NULL &&
      dev->netdev_ops->ndo_open != NULL)
    {
      ret = dev->netdev_ops->ndo_open(dev);
    }

  if (ret >= 0 && dev != NULL)
    {
      dev->flags |= IFF_UP;
      netdev_lower_carrier_on(lower);
    }

  return ret;
}

static int linux_bt_bnep_linux_ifdown(FAR struct netdev_lowerhalf_s *lower)
{
  FAR struct net_device *dev = g_bnep_linux_netdev.linux_dev;
  int ret = OK;

  netdev_lower_carrier_off(lower);
  if (dev != NULL)
    {
      dev->flags &= ~IFF_UP;
      if (dev->netdev_ops != NULL && dev->netdev_ops->ndo_stop != NULL)
        {
          ret = dev->netdev_ops->ndo_stop(dev);
        }
    }

  return ret;
}

static int linux_bt_bnep_linux_send(FAR struct netdev_lowerhalf_s *lower,
                                    FAR netpkt_t *pkt)
{
  FAR struct net_device *dev = g_bnep_linux_netdev.linux_dev;
  FAR struct sk_buff *skb;
  unsigned int len = netpkt_getdatalen(lower, pkt);
  int ret = OK;

  if (dev == NULL || dev->netdev_ops == NULL ||
      dev->netdev_ops->ndo_start_xmit == NULL)
    {
      netpkt_free(lower, pkt, NETPKT_TX);
      return -ENODEV;
    }

  skb = alloc_skb(len, GFP_ATOMIC);
  if (skb == NULL)
    {
      netpkt_free(lower, pkt, NETPKT_TX);
      return -ENOMEM;
    }

  if (skb_put(skb, len) == NULL)
    {
      kfree_skb(skb);
      netpkt_free(lower, pkt, NETPKT_TX);
      return -ENOMEM;
    }

  netpkt_copyout(lower, skb->data, pkt, len, 0);
  skb->dev = dev;
  linux_bt_upstream_bnep_note_native_netdev_xmit(len);
  if (dev->netdev_ops->ndo_start_xmit(skb, dev) != NETDEV_TX_OK)
    {
      ret = -EIO;
    }

  netpkt_free(lower, pkt, NETPKT_TX);
  netdev_lower_txdone(lower);
  return ret;
}

static FAR netpkt_t *linux_bt_bnep_linux_recv(
                                    FAR struct netdev_lowerhalf_s *lower)
{
  FAR unsigned char *mac;
  FAR struct sk_buff *skb;
  FAR netpkt_t *pkt;
  unsigned int copy_len;
  bool restore_l2;

  skb = linux_bt_bnep_linux_pop_rx();
  if (skb == NULL)
    {
      return NULL;
    }

  mac = skb_mac_header(skb);
  restore_l2 = mac != NULL && skb->data >= mac + ETH_HDRLEN;
  copy_len = skb->len + (restore_l2 ? ETH_HDRLEN : 0);

  pkt = netpkt_alloc(lower, NETPKT_RX);
  if (pkt == NULL)
    {
      kfree_skb(skb);
      return NULL;
    }

  if (restore_l2)
    {
      netpkt_copyin(lower, pkt, mac, ETH_HDRLEN, 0);
      netpkt_copyin(lower, pkt, skb->data, skb->len, ETH_HDRLEN);
    }
  else
    {
      netpkt_copyin(lower, pkt, skb->data, skb->len, 0);
    }

  kfree_skb(skb);
  return pkt;
}

static const struct netdev_ops_s g_bnep_linux_ops =
{
  linux_bt_bnep_linux_ifup,
  linux_bt_bnep_linux_ifdown,
  linux_bt_bnep_linux_send,
  linux_bt_bnep_linux_recv
};

static void linux_bt_bnep_linux_work(FAR void *arg)
{
  FAR struct linux_bt_bnep_linux_netdev_s *priv = arg;

  if (!priv->registered)
    {
      return;
    }

  if (priv->rx_head != NULL)
    {
      netdev_lower_rxready(&priv->lower);
    }

  work_queue(HPWORK, &priv->work, linux_bt_bnep_linux_work, priv,
             LINUX_BT_BNEP_HWSIM_PERIOD);
}

static int linux_bt_bnep_linux_lower_register(FAR struct net_device *dev)
{
  FAR uint8_t *mac;
  bool zero_mac = true;
  unsigned int i;
  uint8_t role;
  int ret;

  if (g_bnep_linux_netdev.registered || dev == NULL)
    {
      return dev == NULL ? -EINVAL : OK;
    }

  memset(&g_bnep_linux_netdev, 0, sizeof(g_bnep_linux_netdev));
  g_bnep_linux_netdev.lower.ops = &g_bnep_linux_ops;
  g_bnep_linux_netdev.lower.quota[NETPKT_TX] = LINUX_BT_BNEP_NETPKT_QUOTA;
  g_bnep_linux_netdev.lower.quota[NETPKT_RX] = LINUX_BT_BNEP_NETPKT_QUOTA;
  g_bnep_linux_netdev.lower.rxtype = NETDEV_RX_WORK;
  g_bnep_linux_netdev.lower.netdev.d_llhdrlen = ETH_HDRLEN;
  g_bnep_linux_netdev.lower.netdev.d_pktsize =
    (dev->mtu != 0 ? dev->mtu : LINUX_BT_BNEP_HWSIM_MTU) + ETH_HDRLEN;
  strlcpy(g_bnep_linux_netdev.lower.netdev.d_ifname, dev->name,
          sizeof(g_bnep_linux_netdev.lower.netdev.d_ifname));
  memcpy(g_bnep_linux_netdev.lower.netdev.d_mac.ether.ether_addr_octet,
         dev->dev_addr, sizeof(g_bnep_linux_netdev.lower.netdev.d_mac.ether.
         ether_addr_octet));

  mac = g_bnep_linux_netdev.lower.netdev.d_mac.ether.ether_addr_octet;
  for (i = 0; i < sizeof(g_bnep_linux_netdev.lower.netdev.d_mac.ether.
                         ether_addr_octet); i++)
    {
      if (mac[i] != 0)
        {
          zero_mac = false;
          break;
        }
    }

  if (zero_mac)
    {
#ifdef CONFIG_SIM_BTHWSIM_ROLE
      role = CONFIG_SIM_BTHWSIM_ROLE;
#else
      role = 0xfe;
#endif
      mac[0] = 0x02;
      mac[1] = 0xfe;
      mac[2] = 0x42;
      mac[3] = 0x00;
      mac[4] = 0xb0;
      mac[5] = role;
      memcpy(dev->dev_addr, mac, ETH_ALEN);
    }

  ret = netdev_lower_register(&g_bnep_linux_netdev.lower, NET_LL_ETHERNET);
  if (ret < 0)
    {
      memset(&g_bnep_linux_netdev, 0, sizeof(g_bnep_linux_netdev));
      return ret;
    }

  g_bnep_linux_netdev.linux_dev = dev;
  g_bnep_linux_netdev.registered = true;
  work_queue(HPWORK, &g_bnep_linux_netdev.work, linux_bt_bnep_linux_work,
             &g_bnep_linux_netdev, LINUX_BT_BNEP_HWSIM_PERIOD);
  return OK;
}

static void linux_bt_bnep_linux_lower_unregister(FAR struct net_device *dev)
{
  FAR struct sk_buff *skb;

  if (!g_bnep_linux_netdev.registered ||
      g_bnep_linux_netdev.linux_dev != dev)
    {
      return;
    }

  g_bnep_linux_netdev.registered = false;
  work_cancel(HPWORK, &g_bnep_linux_netdev.work);
  while ((skb = linux_bt_bnep_linux_pop_rx()) != NULL)
    {
      kfree_skb(skb);
    }

  netdev_lower_unregister(&g_bnep_linux_netdev.lower);
  memset(&g_bnep_linux_netdev, 0, sizeof(g_bnep_linux_netdev));
}

int linux_bt_bnep_netdev_register(FAR struct net_device *dev)
{
  int ret;

  if (dev == NULL)
    {
      return -EINVAL;
    }

  ret = register_netdev(dev);
  if (ret < 0)
    {
      return ret;
    }

  ret = linux_bt_bnep_linux_lower_register(dev);
  if (ret < 0)
    {
      unregister_netdev(dev);
      return ret;
    }

  linux_bt_upstream_bnep_note_native_netdev_register();
  return ret;
}

void linux_bt_bnep_netdev_unregister(FAR struct net_device *dev)
{
  if (dev != NULL)
    {
      linux_bt_upstream_bnep_note_native_netdev_unregister();
      linux_bt_bnep_linux_lower_unregister(dev);
      unregister_netdev(dev);
    }
}

int linux_bt_bnep_netif_rx(FAR struct sk_buff *skb)
{
  if (skb == NULL)
    {
      return -EINVAL;
    }

  if (!g_bnep_linux_netdev.registered ||
      skb->dev != g_bnep_linux_netdev.linux_dev)
    {
      kfree_skb(skb);
      return -ENODEV;
    }

  skb->next = NULL;
  linux_bt_upstream_bnep_note_native_netif_rx(skb->len);
  if (g_bnep_linux_netdev.rx_tail != NULL)
    {
      g_bnep_linux_netdev.rx_tail->next = skb;
    }
  else
    {
      g_bnep_linux_netdev.rx_head = skb;
    }

  g_bnep_linux_netdev.rx_tail = skb;
  netdev_lower_rxready(&g_bnep_linux_netdev.lower);
  return OK;
}

#endif /* CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP */
#endif /* CONFIG_NET_LINUX_BLUETOOTH */
