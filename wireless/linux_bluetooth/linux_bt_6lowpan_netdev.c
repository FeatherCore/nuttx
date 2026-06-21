/****************************************************************************
 * wireless/linux_bluetooth/linux_bt_6lowpan_netdev.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/net/netdev_lowerhalf.h>
#include <nuttx/wqueue.h>
#include <nuttx/wireless/linux_bluetooth.h>

#ifdef CONFIG_NET_LINUX_BLUETOOTH_6LOWPAN_BRIDGE

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
#  include <linux/if_ether.h>
#  include <linux/netdevice.h>
#  include <linux/skbuff.h>
#  include <net/ipv6.h>

extern int linux_bt_compat_module_init_lowpan_module_init(void);
extern int linux_bt_compat_module_init_bt_6lowpan_init(void);
extern void bt_6lowpan_sim_attach_ipsp(void);
extern void bt_6lowpan_sim_detach_ipsp(bool had_chan);
extern void bt_6lowpan_sim_transmit_packet(void);
extern void bt_6lowpan_sim_receive_packet(void);
extern const char *bt_6lowpan_sim_owner(void);
extern const char *bt_6lowpan_sim_contract(void);
extern void bt_6lowpan_sim_get_stats(unsigned long *netdev_registers,
                                     unsigned long *netdev_unregisters,
                                     unsigned long *tx_packets,
                                     unsigned long *rx_packets);
extern void bt_6lowpan_sim_get_lifecycle_stats(unsigned long *peer_adds,
                                               unsigned long *peer_dels,
                                               unsigned long *coc_opens,
                                               unsigned long *coc_closes,
                                               unsigned long *xmit_packets,
                                               unsigned long *rx_delivers,
                                               unsigned long *setup_netdevs,
                                               unsigned long *delete_netdevs,
                                               unsigned long *chan_ready_cbs,
                                               unsigned long *chan_close_cbs,
                                               unsigned long *bt_xmits,
                                               unsigned long *recv_cbs);
extern void bt_6lowpan_sim_get_owner_state(bool *netdev_active,
                                           bool *coc_active,
                                           bool *peer_active,
                                           unsigned long *netdev_refs,
                                           unsigned long *chan_refs,
                                           unsigned long *peer_refs,
                                           unsigned long *tx_active,
                                           unsigned long *rx_active);
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LINUX_BT_6LOWPAN_IFNAME      "bt%d"
#define LINUX_BT_6LOWPAN_MTU         1280
#define LINUX_BT_6LOWPAN_DISPATCH_IP 0x41
#define LINUX_BT_6LOWPAN_DISPATCH_IPHC 0x60
#define LINUX_BT_6LOWPAN_DISPATCH_IPHC_MASK 0xe0
#define LINUX_BT_6LOWPAN_PSM_IPSP    0x0023
#define LINUX_BT_6LOWPAN_CID         0x0040
#define LINUX_BT_6LOWPAN_HANDLE      0x0040
#define LINUX_BT_6LOWPAN_PERIOD      MSEC2TICK(20)
#define LINUX_BT_6LOWPAN_QUEUE_DEPTH 4
#define LINUX_BT_6LOWPAN_FRAME_MAX   (LINUX_BT_6LOWPAN_MTU + 1)
#define LINUX_BT_6LOWPAN_ACL_MAX     (LINUX_BT_6LOWPAN_FRAME_MAX + 8)
#define LINUX_BT_6LOWPAN_AIR_MTU     512
#define LINUX_BT_6LOWPAN_FRAG1       0xc0
#define LINUX_BT_6LOWPAN_FRAGN       0xe0
#define LINUX_BT_6LOWPAN_FRAG_MASK   0xf8
#define LINUX_BT_6LOWPAN_FRAG_SIZE_MASK 0x07

#ifndef ARPHRD_6LOWPAN
#  define ARPHRD_6LOWPAN 825
#endif

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
#  define LINUX_BT_6LOWPAN_IPHC_CTX_TABLE_SIZE 16
#  define LINUX_BT_6LOWPAN_IPHC_MAX_HC_BUF_LEN \
     (sizeof(struct ipv6hdr) + 4 + sizeof(struct udphdr))
#  define LINUX_BT_6LOWPAN_LLTYPE_BTLE 0

struct linux_bt_6lowpan_iphc_ctx
{
  uint8_t id;
  struct in6_addr pfx;
  uint8_t plen;
  unsigned long flags;
};

struct linux_bt_6lowpan_iphc_ctx_table
{
  spinlock_t lock;
  const void *ops;
  struct linux_bt_6lowpan_iphc_ctx
    table[LINUX_BT_6LOWPAN_IPHC_CTX_TABLE_SIZE];
};

struct linux_bt_6lowpan_dev
{
  int lltype;
  void *iface_debugfs;
  struct linux_bt_6lowpan_iphc_ctx_table ctx;
  uint8_t priv[] __aligned(sizeof(void *));
};

extern int lowpan_header_compress(struct sk_buff *skb,
                                  const struct net_device *dev,
                                  const void *daddr,
                                  const void *saddr);
extern int lowpan_header_decompress(struct sk_buff *skb,
                                    const struct net_device *dev,
                                    const void *daddr,
                                    const void *saddr);
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct linux_bt_6lowpan_frame_s
{
  uint16_t len;
  uint8_t data[LINUX_BT_6LOWPAN_MTU];
};

struct linux_bt_6lowpan_netdev_s
{
  struct netdev_lowerhalf_s lower;
  struct work_s work;
  struct linux_bt_6lowpan_frame_s rx[LINUX_BT_6LOWPAN_QUEUE_DEPTH];
  uint8_t txbuf[LINUX_BT_6LOWPAN_ACL_MAX];
  uint8_t encoded[LINUX_BT_6LOWPAN_FRAME_MAX];
  uint8_t reasm[LINUX_BT_6LOWPAN_FRAME_MAX];
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
  struct net_device linux_dev;
  uint8_t linux_lowpan_priv[sizeof(struct linux_bt_6lowpan_dev)];
  uint8_t linux_lladdr[ETH_ALEN];
  uint8_t peer_lladdr[ETH_ALEN];
#endif
  uint8_t last_tx_src[16];
  uint8_t last_tx_dst[16];
  uint8_t last_rx_src[16];
  uint8_t last_rx_dst[16];
  uint8_t last_tx_nexthdr;
  uint8_t last_rx_nexthdr;
  uint8_t last_tx_dispatch;
  uint8_t last_rx_dispatch;
  uint8_t rx_head;
  uint8_t rx_tail;
  uint8_t rx_count;
  uint16_t next_frag_tag;
  uint16_t reasm_tag;
  uint16_t reasm_size;
  uint16_t reasm_len;
  uint16_t ipsp_peer;
  uint16_t ipsp_handle;
  int ipsp_open_ret;
  bool reasm_active;
  bool registered;
  unsigned long tx_packets;
  unsigned long rx_packets;
  unsigned long rx_dropped;
  unsigned long tx_frag_datagrams;
  unsigned long tx_frag_frames;
  unsigned long rx_frag_datagrams;
  unsigned long rx_frag_frames;
  unsigned long rx_frag_dropped;
  unsigned long ipsp_open_count;
  unsigned long ipsp_close_count;
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
  unsigned long tx_iphc_packets;
  unsigned long rx_iphc_packets;
  unsigned long tx_uncompressed_packets;
  unsigned long rx_uncompressed_packets;
  unsigned long tx_iphc_errors;
#endif
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int linux_bt_6lowpan_ifup(FAR struct netdev_lowerhalf_s *lower);
static int linux_bt_6lowpan_ifdown(FAR struct netdev_lowerhalf_s *lower);
static int linux_bt_6lowpan_send(FAR struct netdev_lowerhalf_s *lower,
                                 FAR netpkt_t *pkt);
static FAR netpkt_t *linux_bt_6lowpan_recv(
                                 FAR struct netdev_lowerhalf_s *lower);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct linux_bt_6lowpan_netdev_s g_6lowpan;
static unsigned long g_6lowpan_total_registers;
static unsigned long g_6lowpan_total_unregisters;
static unsigned long g_6lowpan_total_ifups;
static unsigned long g_6lowpan_total_ifdowns;
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
static bool g_6lowpan_upstream_init_done;
static int g_6lowpan_upstream_core_init_ret;
static int g_6lowpan_upstream_bt_init_ret;
static unsigned long g_6lowpan_upstream_core_init_count;
static unsigned long g_6lowpan_upstream_bt_init_count;
#endif

static const struct netdev_ops_s g_6lowpan_ops =
{
  .ifup    = linux_bt_6lowpan_ifup,
  .ifdown  = linux_bt_6lowpan_ifdown,
  .transmit = linux_bt_6lowpan_send,
  .receive = linux_bt_6lowpan_recv,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void linux_bt_6lowpan_put_le16(FAR uint8_t *dst, uint16_t value)
{
  dst[0] = (uint8_t)(value & 0xff);
  dst[1] = (uint8_t)((value >> 8) & 0xff);
}

static uint16_t linux_bt_6lowpan_get_frag_size(FAR const uint8_t *data)
{
  return (uint16_t)(((data[0] & LINUX_BT_6LOWPAN_FRAG_SIZE_MASK) << 8) |
                    data[1]);
}

static uint16_t linux_bt_6lowpan_get_be16(FAR const uint8_t *data)
{
  return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static void linux_bt_6lowpan_put_frag_size(FAR uint8_t *data,
                                           uint8_t dispatch,
                                           uint16_t size)
{
  data[0] = dispatch | (uint8_t)((size >> 8) &
                                  LINUX_BT_6LOWPAN_FRAG_SIZE_MASK);
  data[1] = (uint8_t)(size & 0xff);
}

static void linux_bt_6lowpan_put_be16(FAR uint8_t *data, uint16_t value)
{
  data[0] = (uint8_t)((value >> 8) & 0xff);
  data[1] = (uint8_t)(value & 0xff);
}

static bool linux_bt_6lowpan_is_iphc(uint8_t dispatch)
{
  return (dispatch & LINUX_BT_6LOWPAN_DISPATCH_IPHC_MASK) ==
         LINUX_BT_6LOWPAN_DISPATCH_IPHC;
}

static bool linux_bt_6lowpan_is_frag1(uint8_t dispatch)
{
  return (dispatch & LINUX_BT_6LOWPAN_FRAG_MASK) ==
         LINUX_BT_6LOWPAN_FRAG1;
}

static bool linux_bt_6lowpan_is_fragn(uint8_t dispatch)
{
  return (dispatch & LINUX_BT_6LOWPAN_FRAG_MASK) ==
         LINUX_BT_6LOWPAN_FRAGN;
}

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
static void linux_bt_6lowpan_upstream_ensure_init(void)
{
  if (g_6lowpan_upstream_init_done)
    {
      return;
    }

  g_6lowpan_upstream_core_init_ret =
    linux_bt_compat_module_init_lowpan_module_init();
  g_6lowpan_upstream_core_init_count++;

  g_6lowpan_upstream_bt_init_ret =
    linux_bt_compat_module_init_bt_6lowpan_init();
  g_6lowpan_upstream_bt_init_count++;

  g_6lowpan_upstream_init_done = true;
}

static FAR struct linux_bt_6lowpan_dev *linux_bt_6lowpan_lowpan_dev(
                                FAR struct linux_bt_6lowpan_netdev_s *priv)
{
  return (FAR struct linux_bt_6lowpan_dev *)priv->linux_lowpan_priv;
}

static void linux_bt_6lowpan_init_linux_dev(
                                FAR struct linux_bt_6lowpan_netdev_s *priv)
{
  FAR struct linux_bt_6lowpan_dev *ldev = linux_bt_6lowpan_lowpan_dev(priv);

  memset(&priv->linux_dev, 0, sizeof(priv->linux_dev));
  memset(ldev, 0, sizeof(*ldev));

  priv->linux_lladdr[0] = 0x02;
  priv->linux_lladdr[1] = 0xaa;
  priv->linux_lladdr[2] = 0x00;
  priv->linux_lladdr[3] = 0x00;
  priv->linux_lladdr[4] = 0x00;
  priv->linux_lladdr[5] = 0x01;

  priv->peer_lladdr[0] = 0x02;
  priv->peer_lladdr[1] = 0xaa;
  priv->peer_lladdr[2] = 0x00;
  priv->peer_lladdr[3] = 0x00;
  priv->peer_lladdr[4] = 0x00;
  priv->peer_lladdr[5] = 0x02;

  ldev->lltype = LINUX_BT_6LOWPAN_LLTYPE_BTLE;
  spin_lock_init(&ldev->ctx.lock);

  strlcpy(priv->linux_dev.name, priv->lower.netdev.d_ifname,
          sizeof(priv->linux_dev.name));
  priv->linux_dev.type = ARPHRD_6LOWPAN;
  priv->linux_dev.addr_len = ETH_ALEN;
  priv->linux_dev.mtu = LINUX_BT_6LOWPAN_MTU;
  priv->linux_dev.flags = IFF_UP | IFF_RUNNING | IFF_MULTICAST;
  priv->linux_dev.carrier = true;
  memcpy(priv->linux_dev.dev_addr, priv->linux_lladdr, ETH_ALEN);
  priv->linux_dev.ml_priv = ldev;
}

static size_t linux_bt_6lowpan_encode_payload(
                                FAR struct linux_bt_6lowpan_netdev_s *priv,
                                FAR const uint8_t *ipv6,
                                size_t ipv6_len,
                                FAR uint8_t *out,
                                size_t out_len)
{
  FAR struct sk_buff *skb;
  size_t encoded_len = 0;
  int ret;

  if (ipv6 == NULL || out == NULL || ipv6_len == 0)
    {
      return 0;
    }

  skb = alloc_skb(LINUX_BT_6LOWPAN_IPHC_MAX_HC_BUF_LEN + ipv6_len,
                  GFP_KERNEL);
  if (skb == NULL)
    {
      return 0;
    }

  skb_reserve(skb, LINUX_BT_6LOWPAN_IPHC_MAX_HC_BUF_LEN);
  skb_put_data(skb, ipv6, ipv6_len);
  skb->dev = &priv->linux_dev;
  skb->protocol = htons(ETH_P_IPV6);
  skb_reset_network_header(skb);
  skb_set_transport_header(skb, sizeof(struct ipv6hdr));

  ret = lowpan_header_compress(skb, &priv->linux_dev,
                               priv->peer_lladdr, priv->linux_lladdr);
  if (ret >= 0 && skb->len <= out_len)
    {
      memcpy(out, skb->data, skb->len);
      encoded_len = skb->len;
    }

  kfree_skb(skb);
  return encoded_len;
}

static size_t linux_bt_6lowpan_decode_iphc(
                                FAR struct linux_bt_6lowpan_netdev_s *priv,
                                FAR const uint8_t *payload,
                                size_t payload_len,
                                FAR uint8_t *ipv6,
                                size_t ipv6_max)
{
  FAR struct sk_buff *skb;
  size_t decoded_len = 0;
  int ret;

  if (payload == NULL || ipv6 == NULL || payload_len == 0)
    {
      return 0;
    }

  skb = alloc_skb(NET_SKB_PAD + sizeof(struct ipv6hdr) + payload_len,
                  GFP_KERNEL);
  if (skb == NULL)
    {
      return 0;
    }

  skb_reserve(skb, NET_SKB_PAD + sizeof(struct ipv6hdr));
  skb_put_data(skb, payload, payload_len);
  skb->dev = &priv->linux_dev;
  skb_reset_network_header(skb);

  ret = lowpan_header_decompress(skb, &priv->linux_dev,
                                 priv->linux_lladdr, priv->peer_lladdr);
  if (ret >= 0 && skb->len <= ipv6_max)
    {
      memcpy(ipv6, skb->data, skb->len);
      decoded_len = skb->len;
    }

  kfree_skb(skb);
  return decoded_len;
}
#endif

static int linux_bt_6lowpan_send_l2cap(
                                FAR struct linux_bt_6lowpan_netdev_s *priv,
                                FAR const uint8_t *payload,
                                size_t payload_len)
{
  uint16_t l2cap_len;
  uint16_t acl_len;

  if (payload == NULL || payload_len == 0 ||
      payload_len > LINUX_BT_6LOWPAN_FRAME_MAX)
    {
      return -EINVAL;
    }

  l2cap_len = (uint16_t)payload_len;
  acl_len = (uint16_t)(l2cap_len + 4);

  linux_bt_6lowpan_put_le16(&priv->txbuf[0], LINUX_BT_6LOWPAN_HANDLE);
  linux_bt_6lowpan_put_le16(&priv->txbuf[2], acl_len);
  linux_bt_6lowpan_put_le16(&priv->txbuf[4], l2cap_len);
  linux_bt_6lowpan_put_le16(&priv->txbuf[6], LINUX_BT_6LOWPAN_CID);
  memcpy(&priv->txbuf[8], payload, payload_len);

  return linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL,
                             LINUX_BT_HWSIM_DST_BROADCAST,
                             priv->txbuf, payload_len + 8);
}

static int linux_bt_6lowpan_send_air(
                                FAR struct linux_bt_6lowpan_netdev_s *priv,
                                FAR const uint8_t *payload,
                                size_t payload_len)
{
  uint8_t frag[LINUX_BT_6LOWPAN_AIR_MTU];
  uint16_t tag;
  size_t offset = 0;
  int ret;

  if (payload_len <= LINUX_BT_6LOWPAN_AIR_MTU)
    {
      return linux_bt_6lowpan_send_l2cap(priv, payload, payload_len);
    }

  tag = ++priv->next_frag_tag;
  if (tag == 0)
    {
      tag = ++priv->next_frag_tag;
    }

  while (offset < payload_len)
    {
      size_t hdr_len = offset == 0 ? 4 : 5;
      size_t max_chunk = LINUX_BT_6LOWPAN_AIR_MTU - hdr_len;
      size_t remain = payload_len - offset;
      size_t chunk;

      if (remain > max_chunk)
        {
          chunk = max_chunk & ~(size_t)0x07;
        }
      else
        {
          chunk = remain;
        }

      if (chunk == 0 || offset > UINT8_MAX * 8)
        {
          priv->rx_frag_dropped++;
          return -EMSGSIZE;
        }

      if (offset == 0)
        {
          linux_bt_6lowpan_put_frag_size(frag, LINUX_BT_6LOWPAN_FRAG1,
                                         (uint16_t)payload_len);
          linux_bt_6lowpan_put_be16(&frag[2], tag);
        }
      else
        {
          linux_bt_6lowpan_put_frag_size(frag, LINUX_BT_6LOWPAN_FRAGN,
                                         (uint16_t)payload_len);
          linux_bt_6lowpan_put_be16(&frag[2], tag);
          frag[4] = (uint8_t)(offset >> 3);
        }

      memcpy(&frag[hdr_len], &payload[offset], chunk);
      ret = linux_bt_6lowpan_send_l2cap(priv, frag, hdr_len + chunk);
      if (ret < 0)
        {
          return ret;
        }

      priv->tx_frag_frames++;
      offset += chunk;
    }

  priv->tx_frag_datagrams++;
  return OK;
}

static int linux_bt_6lowpan_ifup(FAR struct netdev_lowerhalf_s *lower)
{
  g_6lowpan_total_ifups++;
  netdev_lower_carrier_on(lower);
  return OK;
}

static int linux_bt_6lowpan_ifdown(FAR struct netdev_lowerhalf_s *lower)
{
  g_6lowpan_total_ifdowns++;
  netdev_lower_carrier_off(lower);
  return OK;
}

static void linux_bt_6lowpan_record_ipv6(FAR const uint8_t *ipv6,
                                         size_t ipv6_len,
                                         FAR uint8_t *src,
                                         FAR uint8_t *dst,
                                         FAR uint8_t *nexthdr)
{
  if (ipv6 == NULL || ipv6_len < 40 || (ipv6[0] >> 4) != 6)
    {
      memset(src, 0, 16);
      memset(dst, 0, 16);
      *nexthdr = 0;
      return;
    }

  *nexthdr = ipv6[6];
  memcpy(src, &ipv6[8], 16);
  memcpy(dst, &ipv6[24], 16);
}

static void linux_bt_6lowpan_format_ipv6(FAR char *out, size_t out_len,
                                         FAR const uint8_t *addr)
{
  snprintf(out, out_len,
           "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
           "%02x%02x:%02x%02x:%02x%02x:%02x%02x",
           addr[0], addr[1], addr[2], addr[3],
           addr[4], addr[5], addr[6], addr[7],
           addr[8], addr[9], addr[10], addr[11],
           addr[12], addr[13], addr[14], addr[15]);
}

static int linux_bt_6lowpan_send(FAR struct netdev_lowerhalf_s *lower,
                                 FAR netpkt_t *pkt)
{
  FAR struct linux_bt_6lowpan_netdev_s *priv =
    (FAR struct linux_bt_6lowpan_netdev_s *)lower;
  unsigned int ipv6_len = netpkt_getdatalen(lower, pkt);
  size_t lowpan_len = 0;
  int ret;

  if (ipv6_len > LINUX_BT_6LOWPAN_MTU)
    {
      ipv6_len = LINUX_BT_6LOWPAN_MTU;
    }

  netpkt_copyout(lower, &priv->encoded[1], pkt, ipv6_len, 0);
  linux_bt_6lowpan_record_ipv6(&priv->encoded[1], ipv6_len,
                               priv->last_tx_src, priv->last_tx_dst,
                               &priv->last_tx_nexthdr);

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
  lowpan_len = linux_bt_6lowpan_encode_payload(priv, &priv->encoded[1],
                                               ipv6_len, priv->encoded,
                                               LINUX_BT_6LOWPAN_FRAME_MAX);
  if (lowpan_len > 0)
    {
      priv->last_tx_dispatch = priv->encoded[0];
      priv->tx_iphc_packets++;
    }
  else
    {
      priv->tx_iphc_errors++;
      ret = -EIO;
      goto done;
    }
#else
  priv->encoded[0] = LINUX_BT_6LOWPAN_DISPATCH_IP;
  lowpan_len = ipv6_len + 1;
  priv->last_tx_dispatch = LINUX_BT_6LOWPAN_DISPATCH_IP;
#endif

  ret = linux_bt_6lowpan_send_air(priv, priv->encoded, lowpan_len);

done:
  netpkt_free(lower, pkt, NETPKT_TX);
  netdev_lower_txdone(lower);

  if (ret >= 0)
    {
      priv->tx_packets++;
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
      bt_6lowpan_sim_transmit_packet();
#endif
      return OK;
    }

  return ret;
}

static int linux_bt_6lowpan_queue_payload(FAR const uint8_t *data,
                                          size_t payload_len)
{
  FAR struct linux_bt_6lowpan_frame_s *frame;
  size_t ipv6_len;
  uint8_t dispatch;

  if (data == NULL || payload_len < 1)
    {
      return 0;
    }

  dispatch = data[0];
  if (dispatch == LINUX_BT_6LOWPAN_DISPATCH_IP)
    {
      ipv6_len = payload_len - 1;
      if (ipv6_len > LINUX_BT_6LOWPAN_MTU)
        {
          ipv6_len = LINUX_BT_6LOWPAN_MTU;
        }
    }
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
  else if (linux_bt_6lowpan_is_iphc(dispatch))
    {
      ipv6_len = LINUX_BT_6LOWPAN_MTU;
    }
#endif
  else
    {
      return 0;
    }

  if (g_6lowpan.rx_count >= LINUX_BT_6LOWPAN_QUEUE_DEPTH)
    {
      g_6lowpan.rx_dropped++;
      return 0;
    }

  frame = &g_6lowpan.rx[g_6lowpan.rx_head];
  if (dispatch == LINUX_BT_6LOWPAN_DISPATCH_IP)
    {
      frame->len = (uint16_t)ipv6_len;
      if (ipv6_len > 0)
        {
          memcpy(frame->data, &data[1], ipv6_len);
        }

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
      g_6lowpan.rx_uncompressed_packets++;
#endif
    }
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
  else
    {
      ipv6_len = linux_bt_6lowpan_decode_iphc(&g_6lowpan, data,
                                              payload_len, frame->data,
                                              LINUX_BT_6LOWPAN_MTU);
      if (ipv6_len == 0)
        {
          g_6lowpan.rx_dropped++;
          return 0;
        }

      frame->len = (uint16_t)ipv6_len;
      g_6lowpan.rx_iphc_packets++;
    }
#endif

  linux_bt_6lowpan_record_ipv6(frame->data, ipv6_len,
                               g_6lowpan.last_rx_src,
                               g_6lowpan.last_rx_dst,
                               &g_6lowpan.last_rx_nexthdr);
  g_6lowpan.last_rx_dispatch = dispatch;
  g_6lowpan.rx_head = (g_6lowpan.rx_head + 1) %
                      LINUX_BT_6LOWPAN_QUEUE_DEPTH;
  g_6lowpan.rx_count++;
  netdev_lower_rxready(&g_6lowpan.lower);
  return 1;
}

static int linux_bt_6lowpan_recv_fragment(FAR const uint8_t *data,
                                          size_t payload_len)
{
  uint16_t size;
  uint16_t tag;
  size_t hdr_len;
  size_t offset;
  size_t chunk;

  if (payload_len < 4)
    {
      g_6lowpan.rx_frag_dropped++;
      return 0;
    }

  size = linux_bt_6lowpan_get_frag_size(data);
  tag = linux_bt_6lowpan_get_be16(&data[2]);
  if (size == 0 || size > LINUX_BT_6LOWPAN_FRAME_MAX)
    {
      g_6lowpan.rx_frag_dropped++;
      g_6lowpan.reasm_active = false;
      return 0;
    }

  if (linux_bt_6lowpan_is_frag1(data[0]))
    {
      hdr_len = 4;
      offset = 0;
      g_6lowpan.reasm_active = true;
      g_6lowpan.reasm_tag = tag;
      g_6lowpan.reasm_size = size;
      g_6lowpan.reasm_len = 0;
    }
  else
    {
      if (payload_len < 5 || !g_6lowpan.reasm_active ||
          g_6lowpan.reasm_tag != tag || g_6lowpan.reasm_size != size)
        {
          g_6lowpan.rx_frag_dropped++;
          return 0;
        }

      hdr_len = 5;
      offset = (size_t)data[4] << 3;
      if (offset != g_6lowpan.reasm_len)
        {
          g_6lowpan.rx_frag_dropped++;
          g_6lowpan.reasm_active = false;
          return 0;
        }
    }

  if (payload_len <= hdr_len)
    {
      g_6lowpan.rx_frag_dropped++;
      g_6lowpan.reasm_active = false;
      return 0;
    }

  chunk = payload_len - hdr_len;
  if (offset + chunk > size)
    {
      g_6lowpan.rx_frag_dropped++;
      g_6lowpan.reasm_active = false;
      return 0;
    }

  memcpy(&g_6lowpan.reasm[offset], &data[hdr_len], chunk);
  g_6lowpan.reasm_len = (uint16_t)(offset + chunk);
  g_6lowpan.rx_frag_frames++;

  if (g_6lowpan.reasm_len < g_6lowpan.reasm_size)
    {
      return 1;
    }

  g_6lowpan.reasm_active = false;
  g_6lowpan.rx_frag_datagrams++;
  return linux_bt_6lowpan_queue_payload(g_6lowpan.reasm,
                                        g_6lowpan.reasm_size);
}

static FAR netpkt_t *linux_bt_6lowpan_recv(
                                 FAR struct netdev_lowerhalf_s *lower)
{
  FAR struct linux_bt_6lowpan_netdev_s *priv =
    (FAR struct linux_bt_6lowpan_netdev_s *)lower;
  FAR struct linux_bt_6lowpan_frame_s *frame;
  FAR netpkt_t *pkt;

  if (priv->rx_count == 0)
    {
      return NULL;
    }

  frame = &priv->rx[priv->rx_tail];
  pkt = netpkt_alloc(lower, NETPKT_RX);
  if (pkt == NULL)
    {
      priv->rx_dropped++;
      return NULL;
    }

  netpkt_copyin(lower, pkt, frame->data, frame->len, 0);
  priv->rx_tail = (priv->rx_tail + 1) % LINUX_BT_6LOWPAN_QUEUE_DEPTH;
  priv->rx_count--;
  priv->rx_packets++;
  return pkt;
}

static void linux_bt_6lowpan_work(FAR void *arg)
{
  FAR struct linux_bt_6lowpan_netdev_s *priv = arg;

  if (!priv->registered)
    {
      return;
    }

  if (priv->rx_count > 0)
    {
      netdev_lower_rxready(&priv->lower);
    }

  work_queue(HPWORK, &priv->work, linux_bt_6lowpan_work, priv,
             LINUX_BT_6LOWPAN_PERIOD);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int linux_bt_6lowpan_netdev_register(FAR const char *name,
                                     FAR char *actual_name,
                                     size_t actual_name_len)
{
  int ret;

  if (g_6lowpan.registered)
    {
      if (actual_name != NULL && actual_name_len > 0)
        {
          strlcpy(actual_name, g_6lowpan.lower.netdev.d_ifname,
                  actual_name_len);
        }

      return OK;
    }

  memset(&g_6lowpan, 0, sizeof(g_6lowpan));
  g_6lowpan.lower.ops = &g_6lowpan_ops;
  g_6lowpan.lower.quota[NETPKT_TX] = 1;
  g_6lowpan.lower.quota[NETPKT_RX] = LINUX_BT_6LOWPAN_QUEUE_DEPTH;
  g_6lowpan.lower.rxtype = NETDEV_RX_WORK;
  g_6lowpan.lower.netdev.d_llhdrlen = 0;
  g_6lowpan.lower.netdev.d_pktsize = LINUX_BT_6LOWPAN_MTU;

  if (name != NULL && name[0] != '\0')
    {
      strlcpy(g_6lowpan.lower.netdev.d_ifname, name,
              sizeof(g_6lowpan.lower.netdev.d_ifname));
    }
  else
    {
      strlcpy(g_6lowpan.lower.netdev.d_ifname, LINUX_BT_6LOWPAN_IFNAME,
              sizeof(g_6lowpan.lower.netdev.d_ifname));
    }

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
  linux_bt_6lowpan_upstream_ensure_init();
  linux_bt_6lowpan_init_linux_dev(&g_6lowpan);
#endif

  /* NuttX generic lower-half RX currently dispatches raw L3 packets through
   * NET_LL_MBIM's ip_input() path. NET_LL_TUN is handled by the dedicated
   * tun character-device driver and is not decoded by netdev_upperhalf RX.
   */

  ret = netdev_lower_register(&g_6lowpan.lower, NET_LL_MBIM);
  if (ret < 0)
    {
      memset(&g_6lowpan, 0, sizeof(g_6lowpan));
      return ret;
    }

  g_6lowpan.registered = true;
  g_6lowpan_total_registers++;
  g_6lowpan.ipsp_open_ret =
    linux_bt_l2cap_ipsp_open(LINUX_BT_6LOWPAN_CID,
                             &g_6lowpan.ipsp_peer,
                             &g_6lowpan.ipsp_handle);
  if (g_6lowpan.ipsp_open_ret == 0)
    {
      g_6lowpan.ipsp_open_count++;
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
      bt_6lowpan_sim_attach_ipsp();
#endif
    }

  if (actual_name != NULL && actual_name_len > 0)
    {
      strlcpy(actual_name, g_6lowpan.lower.netdev.d_ifname,
              actual_name_len);
    }

  work_queue(HPWORK, &g_6lowpan.work, linux_bt_6lowpan_work,
             &g_6lowpan, LINUX_BT_6LOWPAN_PERIOD);
  return OK;
}

void linux_bt_6lowpan_netdev_unregister(void)
{
  if (!g_6lowpan.registered)
    {
      return;
    }

  g_6lowpan.registered = false;
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
  bt_6lowpan_sim_detach_ipsp(g_6lowpan.ipsp_open_ret == 0);
#endif
  linux_bt_l2cap_ipsp_close(LINUX_BT_6LOWPAN_CID);
  g_6lowpan.ipsp_close_count++;
  linux_bt_6lowpan_ifdown(&g_6lowpan.lower);
  work_cancel(HPWORK, &g_6lowpan.work);
  netdev_lower_unregister(&g_6lowpan.lower);
  g_6lowpan_total_unregisters++;
  memset(&g_6lowpan, 0, sizeof(g_6lowpan));
}

int linux_bt_6lowpan_recv_l2cap_payload(uint16_t cid, const void *payload,
                                        size_t payload_len)
{
  FAR const uint8_t *data = payload;
  uint8_t dispatch;

  if (!g_6lowpan.registered || cid != LINUX_BT_6LOWPAN_CID ||
      data == NULL || payload_len < 1)
    {
      return 0;
    }

  dispatch = data[0];
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
  bt_6lowpan_sim_receive_packet();
#endif
  if (linux_bt_6lowpan_is_frag1(dispatch) ||
      linux_bt_6lowpan_is_fragn(dispatch))
    {
      return linux_bt_6lowpan_recv_fragment(data, payload_len);
    }

  return linux_bt_6lowpan_queue_payload(data, payload_len);
}

int linux_bt_6lowpan_status(FAR char *out, size_t out_len)
{
  char tx_src[40];
  char tx_dst[40];
  char rx_src[40];
  char rx_dst[40];
  char ipsp[128];
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
  unsigned long owner_registers = 0;
  unsigned long owner_unregisters = 0;
  unsigned long owner_tx = 0;
  unsigned long owner_rx = 0;
  unsigned long owner_peer_adds = 0;
  unsigned long owner_peer_dels = 0;
  unsigned long owner_coc_opens = 0;
  unsigned long owner_coc_closes = 0;
  unsigned long owner_xmit = 0;
  unsigned long owner_rx_deliver = 0;
  unsigned long owner_setup_netdev = 0;
  unsigned long owner_delete_netdev = 0;
  unsigned long owner_chan_ready = 0;
  unsigned long owner_chan_close = 0;
  unsigned long owner_bt_xmit = 0;
  unsigned long owner_recv_cb = 0;
  bool owner_netdev_active = false;
  bool owner_coc_active = false;
  bool owner_peer_active = false;
  unsigned long owner_netdev_refs = 0;
  unsigned long owner_chan_refs = 0;
  unsigned long owner_peer_refs = 0;
  unsigned long owner_tx_active = 0;
  unsigned long owner_rx_active = 0;
#endif

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  linux_bt_6lowpan_format_ipv6(tx_src, sizeof(tx_src),
                               g_6lowpan.last_tx_src);
  linux_bt_6lowpan_format_ipv6(tx_dst, sizeof(tx_dst),
                               g_6lowpan.last_tx_dst);
  linux_bt_6lowpan_format_ipv6(rx_src, sizeof(rx_src),
                               g_6lowpan.last_rx_src);
  linux_bt_6lowpan_format_ipv6(rx_dst, sizeof(rx_dst),
                               g_6lowpan.last_rx_dst);
  linux_bt_l2cap_ipsp_status(LINUX_BT_6LOWPAN_CID, ipsp, sizeof(ipsp));
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
  bt_6lowpan_sim_get_stats(&owner_registers, &owner_unregisters,
                           &owner_tx, &owner_rx);
  bt_6lowpan_sim_get_lifecycle_stats(&owner_peer_adds, &owner_peer_dels,
                                     &owner_coc_opens, &owner_coc_closes,
                                     &owner_xmit, &owner_rx_deliver,
                                     &owner_setup_netdev,
                                     &owner_delete_netdev,
                                     &owner_chan_ready,
                                     &owner_chan_close,
                                     &owner_bt_xmit,
                                     &owner_recv_cb);
  bt_6lowpan_sim_get_owner_state(&owner_netdev_active,
                                 &owner_coc_active,
                                 &owner_peer_active,
                                 &owner_netdev_refs,
                                 &owner_chan_refs,
                                 &owner_peer_refs,
                                 &owner_tx_active,
                                 &owner_rx_active);
#endif

  snprintf(out, out_len,
           "linux-bt-6lowpan: registered=%u ifname=%s psm=0x%04x "
           "cid=0x%04x mtu=%u rx-queued=%u tx=%lu rx=%lu drop=%lu "
           "lifecycle-register=%lu lifecycle-unregister=%lu "
           "lifecycle-ifup=%lu lifecycle-ifdown=%lu "
           "ipsp-open=%lu ipsp-close=%lu ipsp-open-ret=%d "
           "last-tx-src=%s last-tx-dst=%s last-tx-nh=0x%02x "
           "last-rx-src=%s last-rx-dst=%s last-rx-nh=0x%02x "
           "last-tx-dispatch=0x%02x last-rx-dispatch=0x%02x "
           "tx-frag-dgrams=%lu tx-frag-frames=%lu "
           "rx-frag-dgrams=%lu rx-frag-frames=%lu rx-frag-drop=%lu"
           " %s"
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
           " upstream-core-init=%lu upstream-core-ret=%d "
           "upstream-bt6lowpan-init=%lu upstream-bt6lowpan-ret=%d "
           "upstream-link=%s "
           "upstream-object-contract-version=1 "
           "upstream-peer-ownership=peer_add,peer_lookup,peer_del,"
           "peer_ref,peer_unref "
           "upstream-coc-ownership=l2cap_le_connect,chan_ready_cb,"
           "recv_cb,chan_close_cb,credits,psm_0x0023,cid_0x0040 "
           "upstream-netdev-ownership=setup_netdev,register_netdev,"
           "ndo_start_xmit,netif_rx,delete_netdev,unregister_netdev "
           "upstream-state-ownership=netdev_active,coc_active,"
           "peer_active,tx_active,rx_active,registered_closed "
           "upstream-error-ownership=bad_psm,bad_cid,credit_exhausted,"
           "iphc_fail,fragment_drop,peer_missing,cleanup_after_error "
           "upstream-link-netdev-register=%lu "
           "upstream-link-netdev-unregister=%lu "
           "upstream-link-tx=%lu upstream-link-rx=%lu "
           "upstream-link-peer-add=%lu upstream-link-peer-del=%lu "
           "upstream-link-coc-open=%lu upstream-link-coc-close=%lu "
           "upstream-link-xmit=%lu upstream-link-rx-deliver=%lu "
           "upstream-link-setup-netdev=%lu "
           "upstream-link-delete-netdev=%lu "
           "upstream-link-chan-ready-cb=%lu "
           "upstream-link-chan-close-cb=%lu "
           "upstream-link-bt-xmit=%lu upstream-link-recv-cb=%lu "
           "upstream-link-state=netdev:%u,coc:%u,peer:%u "
           "upstream-link-refs=netdev:%lu,chan:%lu,peer:%lu "
           "upstream-link-active-tx=%lu upstream-link-active-rx=%lu "
           "%s "
           "upstream-datapath-ownership=nuttx-ip-tx,net_6lowpan-iphc,"
           "bt_6lowpan_xmit,l2cap-coc,hwsim-acl,hwsim-rx,l2cap-coc,"
           "bt_6lowpan_recv,netif-rx,nuttx-ip-rx "
           "upstream-link-ledger=netdev:%u,coc:%u,peer:%u,"
           "netdev-ref:%lu,chan-ref:%lu,peer-ref:%lu,tx-active:%lu,"
           "rx-active:%lu "
           "upstream-link-ledger=netdev:%u,coc:%u,peer:%u,"
           "netdev-ref:%lu,chan-ref:%lu,peer-ref:%lu,tx-active=%lu,"
           "rx-active=%lu "
           "upstream-iphc-link=net_6lowpan/iphc "
           "tx-iphc=%lu rx-iphc=%lu tx-uncompressed=%lu "
           "rx-uncompressed=%lu tx-iphc-error=%lu"
#endif
           "\n",
           g_6lowpan.registered ? 1 : 0,
           g_6lowpan.registered ? g_6lowpan.lower.netdev.d_ifname : "-",
           LINUX_BT_6LOWPAN_PSM_IPSP, LINUX_BT_6LOWPAN_CID,
           LINUX_BT_6LOWPAN_MTU, g_6lowpan.rx_count,
           g_6lowpan.tx_packets, g_6lowpan.rx_packets,
           g_6lowpan.rx_dropped,
           g_6lowpan_total_registers, g_6lowpan_total_unregisters,
           g_6lowpan_total_ifups, g_6lowpan_total_ifdowns,
           g_6lowpan.ipsp_open_count, g_6lowpan.ipsp_close_count,
           g_6lowpan.ipsp_open_ret,
           tx_src, tx_dst,
           g_6lowpan.last_tx_nexthdr, rx_src, rx_dst,
           g_6lowpan.last_rx_nexthdr,
           g_6lowpan.last_tx_dispatch, g_6lowpan.last_rx_dispatch,
           g_6lowpan.tx_frag_datagrams, g_6lowpan.tx_frag_frames,
           g_6lowpan.rx_frag_datagrams, g_6lowpan.rx_frag_frames,
           g_6lowpan.rx_frag_dropped, ipsp
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN
           , g_6lowpan_upstream_core_init_count,
           g_6lowpan_upstream_core_init_ret,
           g_6lowpan_upstream_bt_init_count,
           g_6lowpan_upstream_bt_init_ret,
           bt_6lowpan_sim_owner(),
           owner_registers, owner_unregisters,
           owner_tx, owner_rx,
           owner_peer_adds, owner_peer_dels,
           owner_coc_opens, owner_coc_closes,
           owner_xmit, owner_rx_deliver,
           owner_setup_netdev, owner_delete_netdev,
           owner_chan_ready, owner_chan_close,
           owner_bt_xmit, owner_recv_cb,
           owner_netdev_active ? 1 : 0,
           owner_coc_active ? 1 : 0,
           owner_peer_active ? 1 : 0,
           owner_netdev_refs, owner_chan_refs, owner_peer_refs,
           owner_tx_active, owner_rx_active,
           bt_6lowpan_sim_contract(),
           owner_netdev_active ? 1 : 0,
           owner_coc_active ? 1 : 0,
           owner_peer_active ? 1 : 0,
           owner_netdev_refs, owner_chan_refs, owner_peer_refs,
           owner_tx_active, owner_rx_active,
           owner_netdev_active ? 1 : 0,
           owner_coc_active ? 1 : 0,
           owner_peer_active ? 1 : 0,
           owner_netdev_refs, owner_chan_refs, owner_peer_refs,
           owner_tx_active, owner_rx_active,
           g_6lowpan.tx_iphc_packets, g_6lowpan.rx_iphc_packets,
           g_6lowpan.tx_uncompressed_packets,
           g_6lowpan.rx_uncompressed_packets,
           g_6lowpan.tx_iphc_errors
#endif
           );
  return OK;
}

#endif /* CONFIG_NET_LINUX_BLUETOOTH_6LOWPAN_BRIDGE */
