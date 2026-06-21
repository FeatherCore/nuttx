#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_BLUETOOTH_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_BLUETOOTH_H

#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <net/sock.h>

#include <string.h>

struct seq_file;

#define BT_ERR(fmt, ...) pr_err("Bluetooth: " fmt, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) pr_warn("Bluetooth: " fmt, ##__VA_ARGS__)
#define BT_INFO(fmt, ...) pr_info("Bluetooth: " fmt, ##__VA_ARGS__)
#define BT_DBG(fmt, ...) pr_debug("Bluetooth: " fmt, ##__VA_ARGS__)
#define bt_dev_err(hdev, fmt, ...) BT_ERR(fmt, ##__VA_ARGS__)
#define bt_dev_warn(hdev, fmt, ...) BT_WARN(fmt, ##__VA_ARGS__)
#define bt_dev_warn_ratelimited(hdev, fmt, ...) BT_WARN(fmt, ##__VA_ARGS__)
#define bt_dev_err_ratelimited(hdev, fmt, ...) BT_ERR(fmt, ##__VA_ARGS__)
#define bt_dev_info(hdev, fmt, ...) BT_INFO(fmt, ##__VA_ARGS__)
#define bt_dev_dbg(hdev, fmt, ...) BT_DBG(fmt, ##__VA_ARGS__)

#define BLUETOOTH_VER_1_1 1
#define BLUETOOTH_VER_1_2 2
#define BLUETOOTH_VER_2_0 3
#define BLUETOOTH_VER_2_1 4
#define BLUETOOTH_VER_4_0 6

#define BTPROTO_L2CAP  0
#define BTPROTO_HCI    1
#define BTPROTO_SCO    2
#define BTPROTO_RFCOMM 3
#define BTPROTO_BNEP   4
#define BTPROTO_CMTP   5
#define BTPROTO_HIDP   6
#define BTPROTO_AVDTP  7
#define BTPROTO_ISO    8
#define BTPROTO_LAST   BTPROTO_ISO

#define BT_SUBSYS_VERSION 2
#define BT_SUBSYS_REVISION 22

#define BT_SK_SUSPEND     0
#define BT_SK_DEFER_SETUP 1
#define BT_SK_PKT_STATUS  2
#define BT_SK_PKT_SEQNUM  3

#define BT_SECURITY       4
struct bt_security
{
  __u8 level;
  __u8 key_size;
};
#define BT_SECURITY_SDP   0
#define BT_SECURITY_LOW   1
#define BT_SECURITY_MEDIUM 2
#define BT_SECURITY_HIGH  3
#define BT_SECURITY_FIPS  4

#define BT_DEFER_SETUP    7
#define BT_FLUSHABLE      8
#define BT_FLUSHABLE_OFF  0
#define BT_FLUSHABLE_ON   1
#define BT_POWER          9
struct bt_power
{
  __u8 force_active;
};
#define BT_POWER_FORCE_ACTIVE_OFF 0
#define BT_POWER_FORCE_ACTIVE_ON  1
#define BT_CHANNEL_POLICY 10
#define BT_CHANNEL_POLICY_BREDR_ONLY      0
#define BT_CHANNEL_POLICY_BREDR_PREFERRED 1
#define BT_CHANNEL_POLICY_AMP_PREFERRED   2

#define BT_SCM_PKT_STATUS 1
#define BT_SCM_PKT_SEQNUM 2
#define BT_SCM_ERROR      4

#define BT_PHY                 14
#define BT_PHY_BR_1M_1SLOT     0x00000001
#define BT_PHY_BR_1M_3SLOT     0x00000002
#define BT_PHY_BR_1M_5SLOT     0x00000004
#define BT_PHY_EDR_2M_1SLOT    0x00000008
#define BT_PHY_EDR_2M_3SLOT    0x00000010
#define BT_PHY_EDR_2M_5SLOT    0x00000020
#define BT_PHY_EDR_3M_1SLOT    0x00000040
#define BT_PHY_EDR_3M_3SLOT    0x00000080
#define BT_PHY_EDR_3M_5SLOT    0x00000100
#define BT_PHY_LE_1M_TX        0x00000200
#define BT_PHY_LE_1M_RX        0x00000400
#define BT_PHY_LE_2M_TX        0x00000800
#define BT_PHY_LE_2M_RX        0x00001000
#define BT_PHY_LE_CODED_TX     0x00002000
#define BT_PHY_LE_CODED_RX     0x00004000
#define BT_PHY_BREDR_MASK      (BT_PHY_BR_1M_1SLOT | \
                                BT_PHY_BR_1M_3SLOT | \
                                BT_PHY_BR_1M_5SLOT | \
                                BT_PHY_EDR_2M_1SLOT | \
                                BT_PHY_EDR_2M_3SLOT | \
                                BT_PHY_EDR_2M_5SLOT | \
                                BT_PHY_EDR_3M_1SLOT | \
                                BT_PHY_EDR_3M_3SLOT | \
                                BT_PHY_EDR_3M_5SLOT)
#define BT_PHY_LE_MASK         (BT_PHY_LE_1M_TX | BT_PHY_LE_1M_RX | \
                                BT_PHY_LE_2M_TX | BT_PHY_LE_2M_RX | \
                                BT_PHY_LE_CODED_TX | BT_PHY_LE_CODED_RX)

#define BT_MODE                15
#define BT_MODE_BASIC          0x00
#define BT_MODE_ERTM           0x01
#define BT_MODE_STREAMING      0x02
#define BT_MODE_LE_FLOWCTL     0x03
#define BT_MODE_EXT_FLOWCTL    0x04

#define BT_PKT_STATUS          16
#define BT_ISO_QOS             17
#define BT_ISO_SYNC_TIMEOUT    0x07d0
#define BT_CODEC               19
#define BT_ISO_BASE            20
#define BT_PKT_SEQNUM          22

#define BT_CODEC_CVSD          0x02
#define BT_CODEC_MSBC          0x05
#define BT_CODEC_TRANSPARENT   0x03

#define BT_ISO_PHY_1M          0x01
#define BT_ISO_PHY_2M          0x02
#define BT_ISO_PHY_CODED       0x04
#define BT_ISO_PHY_ANY         (BT_ISO_PHY_1M | BT_ISO_PHY_2M | \
                                BT_ISO_PHY_CODED)
#define BT_ISO_QOS_CIG_UNSET   0xff
#define BT_ISO_QOS_CIS_UNSET   0xff
#define BT_ISO_QOS_BIG_UNSET   0xff
#define BT_ISO_QOS_BIS_UNSET   0xff

typedef struct
{
  uint8_t b[6];
} bdaddr_t;

static const bdaddr_t linux_bt_bdaddr_any = {{0, 0, 0, 0, 0, 0}};
static const bdaddr_t linux_bt_bdaddr_none = {{0, 0, 0, 0, 0, 0}};
#define BDADDR_ANY (&linux_bt_bdaddr_any)
#define BDADDR_NONE (&linux_bt_bdaddr_none)

#define BDADDR_BREDR      0x00
#define BDADDR_LE_PUBLIC  0x01
#define BDADDR_LE_RANDOM  0x02

static inline bool bdaddr_type_is_le(uint8_t type)
{
  return type == BDADDR_LE_PUBLIC || type == BDADDR_LE_RANDOM;
}

static inline bool bdaddr_type_is_valid(uint8_t type)
{
  return type == BDADDR_BREDR || bdaddr_type_is_le(type);
}

static inline void bacpy(bdaddr_t *dst, const bdaddr_t *src)
{
  memcpy(dst, src, sizeof(*dst));
}

static inline int bacmp(const bdaddr_t *a, const bdaddr_t *b)
{
  return memcmp(a, b, sizeof(*a));
}

enum bt_sock_state
{
  BT_CONNECTED = 1,
  BT_OPEN,
  BT_BOUND,
  BT_LISTEN,
  BT_CONNECT,
  BT_CONNECT2,
  BT_CONFIG,
  BT_DISCONN,
  BT_CLOSED,
};

static inline const char *state_to_string(int state)
{
  switch (state)
    {
      case BT_CONNECTED:
        return "BT_CONNECTED";
      case BT_OPEN:
        return "BT_OPEN";
      case BT_BOUND:
        return "BT_BOUND";
      case BT_LISTEN:
        return "BT_LISTEN";
      case BT_CONNECT:
        return "BT_CONNECT";
      case BT_CONNECT2:
        return "BT_CONNECT2";
      case BT_CONFIG:
        return "BT_CONFIG";
      case BT_DISCONN:
        return "BT_DISCONN";
      case BT_CLOSED:
        return "BT_CLOSED";
      default:
        return "invalid state";
    }
}

struct bt_codec
{
  __u8 id;
  __u16 cid;
  __u16 vid;
  __u8 data_path;
  __u8 data_path_id;
  __u8 num_caps;
};

struct bt_iso_io_qos
{
  __u32 interval;
  __u16 latency;
  __u16 sdu;
  __u8 phy;
  __u8 rtn;
};

struct bt_iso_ucast_qos
{
  __u8 cig;
  __u8 cis;
  __u8 sca;
  __u8 packing;
  __u8 framing;
  struct bt_iso_io_qos in;
  struct bt_iso_io_qos out;
};

struct bt_iso_bcast_qos
{
  __u8 big;
  __u8 bis;
  __u8 sync_factor;
  __u8 packing;
  __u8 framing;
  struct bt_iso_io_qos in;
  struct bt_iso_io_qos out;
  __u8 encryption;
  __u8 bcode[16];
  __u8 options;
  __u16 skip;
  __u16 sync_timeout;
  __u8 sync_cte_type;
  __u8 mse;
  __u16 timeout;
};

struct bt_iso_qos
{
  union
    {
      struct bt_iso_ucast_qos ucast;
      struct bt_iso_bcast_qos bcast;
    };
};

struct bt_sock_list
{
  struct hlist_head head;
  rwlock_t lock;
  int (*custom_seq_show)(struct seq_file *seq, void *v);
};

#ifndef BT_SNDMTU
#define BT_SNDMTU 12
#endif
#ifndef BT_RCVMTU
#define BT_RCVMTU 13
#endif

static inline struct bt_sock *bt_sk(const struct sock *sk)
{
  return (struct bt_sock *)sk;
}

struct sock *bt_sock_alloc(struct net *net, struct socket *sock,
                           struct proto *prot, int proto, gfp_t prio,
                           int kern);
int bt_sock_register(int proto, const struct net_proto_family *ops);
void bt_sock_unregister(int proto);
__poll_t bt_sock_poll(struct file *file, struct socket *sock,
                      poll_table *wait);
int bt_sock_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);

void bt_sock_link(struct bt_sock_list *list, struct sock *sk);
void bt_sock_unlink(struct bt_sock_list *list, struct sock *sk);
int bt_procfs_init(struct net *net, const char *name,
                   struct bt_sock_list *list,
                   int (*seq_show)(struct seq_file *seq, void *v));
void bt_procfs_cleanup(struct net *net, const char *name);

static inline struct sk_buff *bt_skb_sendmsg(struct sock *sk,
                                             struct msghdr *msg,
                                             size_t len,
                                             size_t mtu,
                                             size_t headroom,
                                             size_t tailroom)
{
  struct sk_buff *skb;
  size_t ncopy;

  (void)sk;

  if (msg == NULL || msg->msg_iter.data == NULL)
    {
      return NULL;
    }

  ncopy = len < mtu ? len : mtu;
  if (msg->msg_iter.count < ncopy)
    {
      return NULL;
    }

  skb = alloc_skb((unsigned int)(headroom + ncopy + tailroom), GFP_KERNEL);
  if (skb == NULL)
    {
      return NULL;
    }

  skb_reserve(skb, (unsigned int)headroom);
  memcpy(skb_put(skb, (unsigned int)ncopy), msg->msg_iter.data, ncopy);
  msg->msg_iter.data += ncopy;
  msg->msg_iter.count -= ncopy;
  return skb;
}

struct l2cap_chan;
struct hci_dev;

struct l2cap_ctrl
{
  __u8 sframe:1,
       poll:1,
       final:1,
       fcs:1,
       sar:2,
       super:2;

  __u16 reqseq;
  __u16 txseq;
  __u8 retries;
  __le16 psm;
  bdaddr_t bdaddr;
  struct l2cap_chan *chan;
};

struct hci_ctrl
{
  struct sock *sk;
  __u16 opcode;
  __u8 req_flags;
  __u8 req_event;
  union
    {
      void *req_complete;
      void *req_complete_skb;
    };
};

struct mgmt_ctrl
{
  struct hci_dev *hdev;
  __u16 opcode;
};

struct bt_skb_cb
{
  __u8 pkt_type;
  __u8 force_active;
  __u16 expect;
  __u16 pkt_seqnum;
  __u8 incoming:1;
  __u8 pkt_status:2;
  union
    {
      struct l2cap_ctrl l2cap;
      struct hci_ctrl hci;
      struct mgmt_ctrl mgmt;
      struct scm_creds creds;
    };
};

#define bt_cb(skb) ((struct bt_skb_cb *)((skb)->cb))
#define hci_skb_sk(skb) bt_cb((skb))->hci.sk
#define hci_skb_opcode(skb) bt_cb((skb))->hci.opcode
#define hci_skb_event(skb) bt_cb((skb))->hci.req_event
#define hci_skb_expect(skb) bt_cb((skb))->expect

#ifndef BT_SKB_RESERVE
#define BT_SKB_RESERVE 8
#endif

static inline struct sk_buff *bt_skb_alloc(unsigned int len, gfp_t how)
{
  struct sk_buff *skb;

  skb = alloc_skb(len + BT_SKB_RESERVE, how);
  if (skb != NULL)
    {
      skb_reserve(skb, BT_SKB_RESERVE);
    }

  return skb;
}

static inline struct sk_buff *bt_skb_send_alloc(struct sock *sk,
                                                unsigned long len, int nb,
                                                int *err)
{
  struct sk_buff *skb;

  (void)sk;
  (void)nb;
  skb = bt_skb_alloc((unsigned int)len, GFP_KERNEL);
  if (err != NULL)
    {
      *err = skb != NULL ? 0 : -ENOMEM;
    }

  return skb;
}

#endif
