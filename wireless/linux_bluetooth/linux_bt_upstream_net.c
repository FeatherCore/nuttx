/****************************************************************************
 * wireless/linux_bluetooth/linux_bt_upstream_net.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <net/sock.h>

#include <endian.h>
#include <inttypes.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/kmalloc.h>
#include <nuttx/wireless/linux_bluetooth.h>

#include <linux/module.h>
#include <linux/err.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include <net/bluetooth/hci_mon.h>
#include <net/bluetooth/hci_sock.h>
#include <net/bluetooth/iso.h>
#include <net/bluetooth/l2cap.h>
#include <net/bluetooth/mgmt.h>
#include <linux/ioctl.h>

#ifndef cpu_to_le16
#  define cpu_to_le16(v) htole16(v)
#endif

#ifndef le16_to_cpu
#  define le16_to_cpu(v) le16toh(v)
#endif

#ifndef cpu_to_le32
#  define cpu_to_le32(v) htole32(v)
#endif

#ifndef le32_to_cpu
#  define le32_to_cpu(v) le32toh(v)
#endif

#define LINUX_BT_L2CAP_ACL_MTU 2048
#define LINUX_BT_HWSIM_ACL_RAW_MAX (LINUX_BT_L2CAP_ACL_MTU + 8)

#ifndef cpu_to_le64
#  define cpu_to_le64(v) htole64(v)
#endif

#ifndef le64_to_cpu
#  define le64_to_cpu(v) le64toh(v)
#endif

#ifndef ETH_ALEN
#  define ETH_ALEN 6
#endif

#ifndef SHUT_RD
#  define SHUT_RD 0
#endif

#ifndef SHUT_WR
#  define SHUT_WR 1
#endif

#ifndef SHUT_RDWR
#  define SHUT_RDWR 2
#endif

#ifdef TIOCINQ
#  undef TIOCINQ
#endif
#define TIOCINQ 0x541b

#ifdef TIOCOUTQ
#  undef TIOCOUTQ
#endif
#define TIOCOUTQ 0x5411

#ifndef BNEPCONNADD
#  define BNEPCONNADD      0x42c8
#  define BNEPCONNDEL      0x42c9
#  define BNEPGETCONNLIST  0x42d2
#  define BNEPGETCONNINFO  0x42d3
#  define BNEPGETSUPPFEAT  0x42d4
#endif

#ifndef BT_PSM_RFCOMM
#  define BT_PSM_RFCOMM 0x0003
#endif

#ifndef SO_RFCOMM_MTU
#  define SO_RFCOMM_MTU (__SO_PROTOCOL + 6)
#endif

#ifndef SO_RFCOMM_LM
#  define SO_RFCOMM_LM (__SO_PROTOCOL + 7)
#endif

#ifndef RFCOMM_CONNINFO
#  define RFCOMM_CONNINFO 0x02
#endif

#ifndef RFCOMM_LM
#  define RFCOMM_LM 0x03
#endif

#ifndef RFCOMM_LM_AUTH
#  define RFCOMM_LM_MASTER  0x0001
#  define RFCOMM_LM_AUTH    0x0002
#  define RFCOMM_LM_ENCRYPT 0x0004
#  define RFCOMM_LM_SECURE  0x0020
#  define RFCOMM_LM_FIPS    0x0040
#endif

#define LINUX_BT_BNEP_SETUP_RESPONSE 0
#define LINUX_BT_BNEP_SVC_PANU       0x1115
#define LINUX_BT_BNEP_CORE_STATE_CONNECTED 1
#define LINUX_BT_BNEP_CORE_MAX_SESSIONS    4
#define LINUX_BT_BNEP_CONNADD_KEPT_L2CAP      UINT32_MAX
#define LINUX_BT_BNEP_CORE_MTU             1500
#define LINUX_BT_RFCOMM_DEFAULT_MTU           127
#define LINUX_BT_RFCOMM_MAX_CHANNEL           30
#define LINUX_BT_RFCOMM_DEFAULT_CID           0x0060
#define LINUX_BT_RFCOMM_DEFAULT_HANDLE        0x0040
#define LINUX_BT_SCO_DEFAULT_MTU              60
#define LINUX_BT_SCO_DEFAULT_HANDLE           0x0060
#define LINUX_BT_ISO_BASE_MAX_LENGTH          248
#define LINUX_BT_NATIVE_BNEPCONNADD           _IOW('B', 200, int)
#define LINUX_BT_NATIVE_BNEPCONNDEL           _IOW('B', 201, int)
#define LINUX_BT_NATIVE_BNEPGETCONNLIST       _IOR('B', 210, int)
#define LINUX_BT_NATIVE_BNEPGETCONNINFO       _IOR('B', 211, int)
#define LINUX_BT_NATIVE_BNEPGETSUPPFEAT       _IOR('B', 212, int)
#ifndef LINUX_BT_NATIVE_CMTPCONNADD
#  define LINUX_BT_NATIVE_CMTPCONNADD         _IOW('C', 200, int)
#  define LINUX_BT_NATIVE_CMTPCONNDEL         _IOW('C', 201, int)
#  define LINUX_BT_NATIVE_CMTPGETCONNLIST     _IOR('C', 210, int)
#  define LINUX_BT_NATIVE_CMTPGETCONNINFO     _IOR('C', 211, int)
#endif
#define LINUX_BT_CMTP_CORE_MAX_SESSIONS    4
#define LINUX_BT_CMTP_CORE_STATE_CONNECTED 1
#define LINUX_BT_CMTP_LOOPBACK                0
#ifndef LINUX_BT_NATIVE_HIDPCONNADD
#  define LINUX_BT_NATIVE_HIDPCONNADD         _IOW('H', 200, int)
#  define LINUX_BT_NATIVE_HIDPCONNDEL         _IOW('H', 201, int)
#  define LINUX_BT_NATIVE_HIDPGETCONNLIST     _IOR('H', 210, int)
#  define LINUX_BT_NATIVE_HIDPGETCONNINFO     _IOR('H', 211, int)
#endif
#define LINUX_BT_HIDP_CORE_MAX_SESSIONS    4
#define LINUX_BT_HIDP_CORE_STATE_CONNECTED 1

#ifndef SO_SCO_MTU
#  define SO_SCO_MTU (__SO_PROTOCOL + 8)
#endif

#ifndef SO_SCO_HANDLE
#  define SO_SCO_HANDLE (__SO_PROTOCOL + 9)
#endif

#ifndef SCO_OPTIONS
#  define SCO_OPTIONS 0x01
#endif

#ifndef SCO_CONNINFO
#  define SCO_CONNINFO 0x02
#endif

#ifndef BT_VOICE
#  define BT_VOICE 11
#endif

#ifndef BT_VOICE_CVSD_16BIT
#  define BT_VOICE_CVSD_16BIT 0x0060
#endif

#ifndef HCI_OP_SET_EVENT_MASK
#  define HCI_OP_SET_EVENT_MASK               0x0c01
#endif

#ifndef HCI_OP_READ_LOCAL_NAME
#  define HCI_OP_READ_LOCAL_NAME              0x0c14
#endif

#ifndef HCI_OP_READ_LOCAL_EXT_FEATURES
#  define HCI_OP_READ_LOCAL_EXT_FEATURES      0x1004
#endif

#ifndef HCI_OP_LE_SET_EVENT_MASK
#  define HCI_OP_LE_SET_EVENT_MASK            0x2001
#endif

#ifndef HCI_OP_SET_EVENT_MASK_PAGE_2
#  define HCI_OP_SET_EVENT_MASK_PAGE_2        0x0c63
#endif

#ifndef HCI_OP_LE_SET_RANDOM_ADDR
#  define HCI_OP_LE_SET_RANDOM_ADDR           0x2005
#endif

#ifndef HCI_OP_LE_SET_ADV_PARAM
#  define HCI_OP_LE_SET_ADV_PARAM             0x2006
#endif

#ifndef HCI_OP_LE_READ_ADV_TX_POWER
#  define HCI_OP_LE_READ_ADV_TX_POWER         0x2007
#endif

#ifndef HCI_OP_LE_SET_ADV_DATA
#  define HCI_OP_LE_SET_ADV_DATA              0x2008
#endif

#ifndef HCI_OP_LE_SET_SCAN_RSP_DATA
#  define HCI_OP_LE_SET_SCAN_RSP_DATA         0x2009
#endif

#ifndef HCI_OP_LE_SET_ADV_ENABLE
#  define HCI_OP_LE_SET_ADV_ENABLE            0x200a
#endif

#ifndef HCI_OP_LE_SET_SCAN_PARAM
#  define HCI_OP_LE_SET_SCAN_PARAM            0x200b
#endif

#ifndef HCI_OP_LE_SET_SCAN_ENABLE
#  define HCI_OP_LE_SET_SCAN_ENABLE           0x200c
#endif

#ifndef HCI_OP_LE_CLEAR_ACCEPT_LIST
#  define HCI_OP_LE_CLEAR_ACCEPT_LIST         0x2010
#endif

#ifndef HCI_OP_LE_ADD_TO_ACCEPT_LIST
#  define HCI_OP_LE_ADD_TO_ACCEPT_LIST        0x2011
#endif

#ifndef HCI_OP_LE_DEL_FROM_ACCEPT_LIST
#  define HCI_OP_LE_DEL_FROM_ACCEPT_LIST      0x2012
#endif

#ifndef HCI_OP_LE_CLEAR_RESOLV_LIST
#  define HCI_OP_LE_CLEAR_RESOLV_LIST         0x2029
#endif

#ifndef HCI_OP_LE_READ_DEF_DATA_LEN
#  define HCI_OP_LE_READ_DEF_DATA_LEN         0x2023
#endif

#ifndef HCI_OP_LE_READ_MAX_DATA_LEN
#  define HCI_OP_LE_READ_MAX_DATA_LEN         0x202f
#endif

#ifndef HCI_OP_LE_SET_ADDR_RESOLV_ENABLE
#  define HCI_OP_LE_SET_ADDR_RESOLV_ENABLE    0x202d
#endif

#ifndef HCI_OP_LE_SET_RPA_TIMEOUT
#  define HCI_OP_LE_SET_RPA_TIMEOUT           0x202e
#endif

#ifndef HCI_OP_LE_READ_TRANSMIT_POWER
#  define HCI_OP_LE_READ_TRANSMIT_POWER       0x204b
#endif

#ifndef HCI_OP_WRITE_LE_HOST_SUPPORTED
#  define HCI_OP_WRITE_LE_HOST_SUPPORTED      0x0c6d
#endif

extern int linux_bt_mgmt_set_powered(uint8_t enabled);
extern int linux_bt_mgmt_set_connectable(uint8_t enabled);
extern int linux_bt_mgmt_set_discoverable(uint8_t enabled);
extern int linux_bt_mgmt_set_bondable(uint8_t enabled);
extern int linux_bt_mgmt_set_le(uint8_t enabled);
extern int linux_bt_mgmt_set_bredr(uint8_t enabled);
extern int linux_bt_mgmt_set_advertising(uint8_t enabled);

struct linux_bt_bnep_connadd_req
{
  int sock;
  uint32_t flags;
  uint16_t role;
  char device[16];
};

struct linux_bt_bnep_conndel_req
{
  uint32_t flags;
  uint8_t dst[ETH_ALEN];
};

struct linux_bt_bnep_conninfo
{
  uint32_t flags;
  uint16_t role;
  uint16_t state;
  uint8_t dst[ETH_ALEN];
  char device[16];
};

struct linux_bt_bnep_connlist_req
{
  uint32_t cnum;
  struct linux_bt_bnep_conninfo *ci;
};

struct linux_bt_cmtp_connadd_req
{
  int sock;
  uint32_t flags;
};

struct linux_bt_cmtp_conndel_req
{
  bdaddr_t bdaddr;
  uint32_t flags;
};

struct linux_bt_cmtp_conninfo
{
  bdaddr_t bdaddr;
  uint32_t flags;
  uint16_t state;
  int num;
};

struct linux_bt_cmtp_connlist_req
{
  uint32_t cnum;
  struct linux_bt_cmtp_conninfo *ci;
};

struct linux_bt_hidp_connadd_req
{
  int ctrl_sock;
  int intr_sock;
  uint16_t parser;
  uint16_t rd_size;
  uint8_t *rd_data;
  uint8_t country;
  uint8_t subclass;
  uint16_t vendor;
  uint16_t product;
  uint16_t version;
  uint32_t flags;
  uint32_t idle_to;
  char name[128];
};

struct linux_bt_hidp_conndel_req
{
  bdaddr_t bdaddr;
  uint32_t flags;
};

struct linux_bt_hidp_conninfo
{
  bdaddr_t bdaddr;
  uint32_t flags;
  uint16_t state;
  uint16_t vendor;
  uint16_t product;
  uint16_t version;
  char name[128];
};

struct linux_bt_hidp_connlist_req
{
  uint32_t cnum;
  struct linux_bt_hidp_conninfo *ci;
};

struct linux_bt_sockaddr_rc
{
  sa_family_t rc_family;
  bdaddr_t rc_bdaddr;
  uint8_t rc_channel;
};

struct linux_bt_rfcomm_conninfo
{
  uint16_t hci_handle;
  uint8_t dev_class[3];
};

struct linux_bt_sockaddr_sco
{
  sa_family_t sco_family;
  bdaddr_t sco_bdaddr;
};

struct linux_bt_sco_options
{
  uint16_t mtu;
};

struct linux_bt_sco_conninfo
{
  uint16_t hci_handle;
  uint8_t dev_class[3];
};

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP
extern int bnep_recv_l2cap_payload(uint16_t cid, const void *payload,
                                   size_t payload_len);
#endif
#ifdef CONFIG_NET_LINUX_BLUETOOTH_6LOWPAN_BRIDGE
extern int linux_bt_6lowpan_recv_l2cap_payload(uint16_t cid,
                                               const void *payload,
                                               size_t payload_len);
#endif

static __poll_t linux_bt_accept_poll(struct sock *parent)
{
  struct bt_sock *bt;

  if (parent == NULL)
    {
      return 0;
    }

  bt = bt_sk(parent);
  return !list_empty(&bt->accept_q) ? EPOLLIN | EPOLLRDNORM : 0;
}

static __poll_t linux_bt_sock_poll(struct file *file,
                                   struct socket *sock,
                                   poll_table *wait)
{
  struct sock *sk;
  __poll_t mask = 0;

  if (sock == NULL || sock->sk == NULL)
    {
      return EPOLLERR;
    }

  sk = sock->sk;
  poll_wait(file, sk_sleep(sk), wait);

  if (sk->sk_state == BT_LISTEN)
    {
      return linux_bt_accept_poll(sk);
    }

  if (sk->sk_err || !skb_queue_empty_lockless(&sk->sk_error_queue))
    {
      mask |= EPOLLERR |
              (sock_flag(sk, SOCK_SELECT_ERR_QUEUE) ? EPOLLPRI : 0);
    }

  if (sk->sk_shutdown & RCV_SHUTDOWN)
    {
      mask |= EPOLLRDHUP | EPOLLIN | EPOLLRDNORM;
    }

  if (sk->sk_shutdown == SHUTDOWN_MASK)
    {
      mask |= EPOLLHUP;
    }

  if (!skb_queue_empty_lockless(&sk->sk_receive_queue))
    {
      mask |= EPOLLIN | EPOLLRDNORM;
    }

  if (sk->sk_state == BT_CLOSED)
    {
      mask |= EPOLLHUP;
    }

  if (sk->sk_state == BT_CONNECT ||
      sk->sk_state == BT_CONNECT2 ||
      sk->sk_state == BT_CONFIG)
    {
      return mask;
    }

  if (!test_bit(BT_SK_SUSPEND, &bt_sk(sk)->flags) &&
      sock_writeable(sk))
    {
      mask |= EPOLLOUT | EPOLLWRNORM | EPOLLWRBAND;
    }
  else
    {
      sk_set_bit(SOCKWQ_ASYNC_NOSPACE, sk);
    }

  return mask;
}

static int linux_bt_upstream_socket_poll_handle(struct socket *sock,
                                                short events,
                                                short *revents)
{
  __poll_t mask;
  short ready = 0;

  if (revents == NULL)
    {
      return -EINVAL;
    }

  *revents = 0;
  if (sock == NULL || sock->ops == NULL || sock->ops->poll == NULL)
    {
      return -EOPNOTSUPP;
    }

  mask = sock->ops->poll(NULL, sock, NULL);
  if ((events & (POLLIN | POLLRDNORM)) != 0 &&
      (mask & (EPOLLIN | EPOLLRDNORM)) != 0)
    {
      ready |= events & (POLLIN | POLLRDNORM);
    }

  if ((events & (POLLOUT | POLLWRNORM)) != 0 &&
      (mask & (EPOLLOUT | EPOLLWRNORM | EPOLLWRBAND)) != 0)
    {
      ready |= events & (POLLOUT | POLLWRNORM);
    }

  if ((mask & EPOLLERR) != 0)
    {
      ready |= POLLERR;
    }

  if ((mask & EPOLLHUP) != 0)
    {
      ready |= POLLHUP;
    }

  *revents = ready;
  return OK;
}

struct sk_buff *linux_bt_compat_skb_recv_datagram(void *skptr,
                                                  unsigned int flags,
                                                  int *err)
{
  struct sock *sk = skptr;
  struct sk_buff *skb;

  (void)flags;

  if (err != NULL)
    {
      *err = 0;
    }

  if (sk == NULL)
    {
      if (err != NULL)
        {
          *err = -EINVAL;
        }

      return NULL;
    }

  skb = skb_dequeue(&sk->sk_receive_queue);
  if (skb == NULL && err != NULL)
    {
      *err = -EAGAIN;
    }

  return skb;
}

int linux_bt_compat_skb_copy_datagram_msg(const struct sk_buff *skb,
                                          int offset, void *msgptr, int len)
{
  struct msghdr *msg = msgptr;

  if (skb == NULL || msg == NULL || offset < 0 || len < 0 ||
      (unsigned int)(offset + len) > skb->len)
    {
      return -EINVAL;
    }

  if (copy_to_iter(skb->data + offset, (size_t)len,
                   &msg->msg_iter) != (size_t)len)
    {
      return -EFAULT;
    }

  return 0;
}

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
int linux_bt_compat_subsys_initcall_bt_init(void);
int iso_init(void);
#endif
int linux_bt_upstream_mgmt_poll_discovery_probe(size_t max_records,
                                                char *out, size_t out_len);
static int linux_bt_proto_sock_queue_payload(struct sock *sk,
                                             const void *payload,
                                             size_t payload_len);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Linux exposes init_net as the initial/default network namespace.  The
 * Bluetooth upstream code checks object identity against &init_net, so keep a
 * single shared compat object instead of header-local placeholders.
 */

struct net init_net;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct net_proto_family *g_sock_family[64];
static unsigned long g_sock_registers;
static unsigned long g_sock_unregisters;
static unsigned long g_sock_create_probes;
static unsigned long g_proto_registers;
static unsigned long g_proto_unregisters;
static unsigned long g_hci_mgmt_socket_cmds;
static unsigned long g_hci_mgmt_socket_events;
static unsigned long g_hci_mgmt_socket_recvs;
static unsigned long g_hci_mgmt_chan_registers;
static unsigned long g_hci_mgmt_chan_unregisters;
static unsigned long g_hci_monitor_registers;
static unsigned long g_hci_monitor_unregisters;
static unsigned long g_hci_monitor_events;
static unsigned long g_hci_data_socket_registers;
static unsigned long g_hci_data_socket_unregisters;
static unsigned long g_hci_data_socket_rx;
static unsigned long g_hci_filter_sets;
static unsigned long g_hci_filter_gets;
static unsigned long g_hci_getname_calls;
static unsigned long g_hci_ioctl_devlist_calls;
static unsigned long g_hci_ioctl_devinfo_calls;
static unsigned long g_hci_ioctl_devup_calls;
static unsigned long g_hci_ioctl_devdown_calls;
static unsigned long g_hci_ioctl_devreset_calls;
static unsigned long g_hci_ioctl_devrestat_calls;
static unsigned long g_hci_ioctl_devcmd_calls;
static unsigned long g_hci_ioctl_connlist_calls;
static unsigned long g_hci_ioctl_conninfo_calls;
static unsigned long g_hci_ioctl_authinfo_calls;
static unsigned long g_hci_ioctl_blockaddr_calls;
static unsigned long g_hci_ioctl_unblockaddr_calls;
static unsigned long g_l2cap_socket_registers;
static unsigned long g_l2cap_socket_unregisters;
static unsigned long g_l2cap_socket_binds;
static unsigned long g_l2cap_socket_connects;
static unsigned long g_l2cap_socket_listens;
static unsigned long g_l2cap_socket_sends;
static unsigned long g_l2cap_socket_recvs;
static unsigned long g_l2cap_socket_native_recvs;
static unsigned long g_l2cap_socket_native_attach_fails;
static unsigned long g_l2cap_socket_native_recv_fails;
static unsigned long g_rfcomm_socket_registers;
static unsigned long g_rfcomm_socket_unregisters;
static unsigned long g_rfcomm_socket_binds;
static unsigned long g_rfcomm_socket_connects;
static unsigned long g_rfcomm_socket_listens;
static unsigned long g_rfcomm_socket_sends;
static unsigned long g_rfcomm_socket_recvs;
static unsigned long g_rfcomm_socket_getnames;
static unsigned long g_rfcomm_socket_setopt;
static unsigned long g_rfcomm_socket_getopt;
static unsigned long g_rfcomm_socket_shutdowns;
static unsigned long g_sco_socket_registers;
static unsigned long g_sco_socket_unregisters;
static unsigned long g_sco_socket_binds;
static unsigned long g_sco_socket_connects;
static unsigned long g_sco_socket_sends;
static unsigned long g_sco_socket_recvs;
static unsigned long g_sco_socket_getnames;
static unsigned long g_sco_socket_setopt;
static unsigned long g_sco_socket_getopt;
static unsigned long g_sco_socket_shutdowns;
static unsigned long g_bnep_socket_registers;
static unsigned long g_bnep_socket_unregisters;
static unsigned long g_bnep_socket_ioctl_suppfeat;
static unsigned long g_bnep_socket_ioctl_connlist;
static unsigned long g_bnep_socket_ioctl_conninfo;
static unsigned long g_bnep_socket_ioctl_connadd;
static unsigned long g_bnep_socket_ioctl_conndel;
static unsigned long g_cmtp_socket_registers;
static unsigned long g_cmtp_socket_unregisters;
static unsigned long g_cmtp_socket_ioctl_connlist;
static unsigned long g_cmtp_socket_ioctl_conninfo;
static unsigned long g_cmtp_socket_ioctl_connadd;
static unsigned long g_cmtp_socket_ioctl_conndel;
static unsigned long g_cmtp_core_session_adds;
static unsigned long g_cmtp_core_session_dels;
static unsigned long g_hidp_socket_registers;
static unsigned long g_hidp_socket_unregisters;
static unsigned long g_hidp_socket_ioctl_connlist;
static unsigned long g_hidp_socket_ioctl_conninfo;
static unsigned long g_hidp_socket_ioctl_connadd;
static unsigned long g_hidp_socket_ioctl_conndel;
static unsigned long g_hidp_core_session_adds;
static unsigned long g_hidp_core_session_dels;
static unsigned long g_bnep_core_session_adds;
static unsigned long g_bnep_core_session_dels;
static unsigned long g_bnep_core_netdev_registers;
static unsigned long g_bnep_core_netdev_unregisters;
static unsigned int g_bnep_native_active;
static unsigned long g_bnep_native_netdev_registers;
static unsigned long g_bnep_native_netdev_unregisters;
static unsigned long g_bnep_native_netdev_xmit;
static unsigned long g_bnep_native_netdev_xmit_bytes;
static unsigned long g_bnep_native_tx_frame;
static unsigned long g_bnep_native_tx_frame_bytes;
static unsigned long g_bnep_native_tx_frame_ok;
static unsigned long g_bnep_native_l2cap_rx;
static unsigned long g_bnep_native_l2cap_rx_bytes;
static unsigned long g_bnep_native_l2cap_delivered;
static unsigned long g_bnep_native_rx_frame;
static unsigned long g_bnep_native_rx_frame_bytes;
static unsigned long g_bnep_native_rx_frame_ok;
static unsigned long g_bnep_native_netif_rx;
static unsigned long g_bnep_native_netif_rx_bytes;
static unsigned long g_bnep_native_session_creates;
static unsigned long g_bnep_native_session_starts;
static unsigned long g_bnep_native_session_stops;
static unsigned long g_bnep_native_session_terminates;
static unsigned long g_bnep_native_netdev_setups;
static unsigned long g_bnep_native_ndo_start_xmits;
static unsigned long g_bnep_native_session_links;
static unsigned long g_bnep_native_session_unlinks;
static unsigned long g_bnep_native_session_threads;
static unsigned long g_bnep_native_kthread_runs;
static unsigned long g_bnep_native_session_rx_dequeues;
static unsigned long g_bnep_native_session_tx_dequeues;
static unsigned long g_bnep_native_fd_handoffs;
static unsigned long g_bnep_native_fd_handoff_cleanups;
static unsigned int g_bnep_native_fd_active;
static int g_bnep_native_fd_last = -1;
static uint16_t g_bnep_native_fd_psm;
static uint16_t g_bnep_native_fd_cid;
static uint16_t g_bnep_native_fd_handle;
static uint16_t g_bnep_native_fd_role;
static const char *g_bnep_native_fd_source = "none";
static unsigned long g_bnep_native_fd_handoff_rejects;
static unsigned long g_bnep_native_fd_lookup_kept_probe;
static unsigned long g_bnep_native_fd_lookup_rebind_probe;
static unsigned long g_bnep_native_sock_ioctl_connadds;
static unsigned long g_bnep_native_sock_ioctl_conndels;
static unsigned long g_bnep_native_sock_ioctl_getconnlists;
static unsigned long g_bnep_native_sock_ioctl_getconninfos;
static unsigned long g_bnep_native_sock_ioctl_getsuppfeats;
static unsigned long g_iso_socket_registers;
static unsigned long g_iso_socket_unregisters;
static unsigned long g_iso_socket_binds;
static unsigned long g_iso_socket_connects;
static unsigned long g_iso_socket_sends;
static unsigned long g_iso_socket_recvs;
static unsigned long g_iso_socket_native_recvs;
static unsigned long g_iso_socket_polls;
static unsigned long g_iso_socket_shutdowns;
static unsigned long g_l2cap_socket_shutdowns;
static struct hci_mgmt_chan *g_hci_mgmt_chan[8];
static struct sock *g_hci_monitor_socks[4];
static struct sock *g_hci_data_socks[8];
static struct sock *g_hci_control_socks[4];
static struct sock *g_rfcomm_socks[8];
static struct sock *g_sco_socks[8];
static bool g_hci_sock_proto_registered;
static bool g_l2cap_sock_proto_registered;
static bool g_bnep_sock_proto_registered;
static bool g_cmtp_sock_proto_registered;
static bool g_hidp_sock_proto_registered;
static bool g_rfcomm_sock_proto_registered;
static bool g_sco_sock_proto_registered;
static bool g_iso_sock_proto_registered;
static bool g_linux_bt_mgmt_chan_registered;

struct linux_bt_bnep_core_session
{
  bool used;
  bool l2cap_backed;
  int sock;
  uint16_t psm;
  uint16_t cid;
  uint16_t handle;
  int l2cap_state;
  bool netdev_registered;
  unsigned int netdev_mtu;
  char netdev_name[16];
  struct linux_bt_bnep_conninfo ci;
};

static struct linux_bt_bnep_core_session
  g_bnep_core_sessions[LINUX_BT_BNEP_CORE_MAX_SESSIONS];

struct linux_bt_cmtp_core_session
{
  bool used;
  int sock;
  bool session_linked;
  bool worker_running;
  bool capi_registered;
  bool reassembly_ready;
  bool datapath_ready;
  unsigned int blockids;
  unsigned int tx_frames;
  unsigned int rx_frames;
  unsigned int capi_messages;
  struct linux_bt_cmtp_conninfo ci;
};

static struct linux_bt_cmtp_core_session
  g_cmtp_core_sessions[LINUX_BT_CMTP_CORE_MAX_SESSIONS];

struct linux_bt_hidp_core_session
{
  bool used;
  int ctrl_sock;
  int intr_sock;
  bool session_linked;
  bool session_thread;
  bool control_ready;
  bool interrupt_ready;
  bool input_registered;
  bool uhid_registered;
  unsigned int control_messages;
  unsigned int input_events;
  unsigned int output_reports;
  uint16_t parser;
  uint16_t rd_size;
  uint32_t idle_to;
  struct linux_bt_hidp_conninfo ci;
};

static struct linux_bt_hidp_core_session
  g_hidp_core_sessions[LINUX_BT_HIDP_CORE_MAX_SESSIONS];

static unsigned int linux_bt_bnep_core_active_count(void)
{
  unsigned int active = 0;
  unsigned int i;

  for (i = 0; i < LINUX_BT_BNEP_CORE_MAX_SESSIONS; i++)
    {
      if (g_bnep_core_sessions[i].used)
        {
          active++;
        }
    }

  return active;
}

static struct linux_bt_bnep_core_session *
linux_bt_bnep_first_active_session(void)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_BNEP_CORE_MAX_SESSIONS; i++)
    {
      if (g_bnep_core_sessions[i].used)
        {
          return &g_bnep_core_sessions[i];
        }
    }

  return NULL;
}

static void linux_bt_bnep_core_netdev_register(
  struct linux_bt_bnep_core_session *session)
{
  char actual_name[sizeof(session->netdev_name)];
  const char *name;
  int ret;

  if (session == NULL || session->netdev_registered)
    {
      return;
    }

  name = session->ci.device[0] != '\0' ? session->ci.device : "btn%d";
  actual_name[0] = '\0';
  ret = linux_bt_bnep_hwsim_netdev_register(name, actual_name,
                                            sizeof(actual_name));
  if (ret < 0)
    {
      snprintf(session->netdev_name, sizeof(session->netdev_name),
               "register-failed:%d", ret);
      return;
    }

  snprintf(session->netdev_name, sizeof(session->netdev_name), "%s",
           actual_name[0] != '\0' ? actual_name : name);
  snprintf(session->ci.device, sizeof(session->ci.device), "%s",
           session->netdev_name);
  session->netdev_mtu = LINUX_BT_BNEP_CORE_MTU;
  session->netdev_registered = true;
  g_bnep_core_netdev_registers++;
}

static void linux_bt_bnep_core_netdev_unregister(
  struct linux_bt_bnep_core_session *session)
{
  if (session == NULL || !session->netdev_registered)
    {
      return;
    }

  session->netdev_registered = false;
  linux_bt_bnep_hwsim_netdev_unregister();
  g_bnep_core_netdev_unregisters++;
}

struct linux_bt_hci_reject_state
{
  bool used;
  uint16_t dev_id;
  uint8_t bdaddr_type;
  bdaddr_t bdaddr;
};

static struct linux_bt_hci_reject_state g_hci_reject_list[8];

struct linux_bt_hci_device_state
{
  bool used;
  uint16_t dev_id;
  uint8_t bdaddr_type;
  uint8_t action;
  uint8_t io_cap;
  uint32_t current_flags;
  uint32_t confirm_value;
  uint32_t passkey;
  bool pairing;
  bool paired;
  bool confirm_pending;
  bool passkey_pending;
  bdaddr_t bdaddr;
};

#define LINUX_BT_HCI_DEVICE_FLAGS_SUPPORTED 0x00000001
#define LINUX_BT_HCI_IO_CAPABILITY_KEYBOARD_ONLY 0x02
#define LINUX_BT_HCI_IO_CAPABILITY_DEFAULT 0x03
#define LINUX_BT_HCI_IO_CAPABILITY_MAX 0x04
#define LINUX_BT_HCI_PASSKEY_STAGED 123456
#define LINUX_BT_HCI_PASSKEY_MAX 999999
#ifdef MGMT_STATUS_CANCELLED
#  define LINUX_BT_HCI_MGMT_STATUS_CANCELLED MGMT_STATUS_CANCELLED
#else
#  define LINUX_BT_HCI_MGMT_STATUS_CANCELLED MGMT_STATUS_FAILED
#endif

static struct linux_bt_hci_device_state g_hci_device_list[8];
static uint8_t g_hci_io_capability = LINUX_BT_HCI_IO_CAPABILITY_DEFAULT;

struct linux_bt_hci_mgmt_pending
{
  bool used;
  struct sock *sk;
  uint16_t opcode;
  uint16_t index;
  uint8_t io_cap;
  struct mgmt_addr_info addr;
};

static struct linux_bt_hci_mgmt_pending g_hci_mgmt_pending[8];

struct linux_bt_hci_ltk_state
{
  bool used;
  uint16_t dev_id;
  struct mgmt_ltk_info key;
};

static struct linux_bt_hci_ltk_state g_hci_ltk_list[8];

struct linux_bt_hci_discovery_state
{
  bool discovering;
  uint16_t dev_id;
  uint8_t type;
};

#define LINUX_BT_HCI_DISCOVERY_TYPES_SUPPORTED 0x07

static struct linux_bt_hci_discovery_state g_hci_discovery_state;

static struct socket g_hci_mgmt_probe_socket;
static bool g_hci_mgmt_probe_bound;
static bool g_hci_mgmt_probe_connected;
static uint16_t g_hci_mgmt_probe_connected_index;
static struct mgmt_addr_info g_hci_mgmt_probe_connected_addr;
static struct socket g_hci_monitor_probe_socket;
static bool g_hci_monitor_probe_bound;

struct linux_bt_hci_user_ctrl_state
{
  bool random_addr_valid;
  uint8_t random_addr[ETH_ALEN];
  uint8_t adv_enable;
  uint8_t adv_type;
  uint8_t adv_own_addr_type;
  uint8_t adv_filter_policy;
  uint8_t adv_data_len;
  uint8_t scan_rsp_len;
  uint8_t scan_enable;
  uint8_t scan_filter_dup;
  uint8_t scan_type;
  uint8_t scan_own_addr_type;
  uint8_t scan_filter_policy;
  uint8_t addr_resolv_enable;
  uint16_t adv_interval_min;
  uint16_t adv_interval_max;
  uint16_t scan_interval;
  uint16_t scan_window;
  uint16_t rpa_timeout;
  uint16_t iso_cis_handle;
  uint16_t iso_acl_handle;
  uint16_t iso_path_handle;
  uint8_t iso_host_feature_bit;
  uint8_t iso_host_feature_value;
  uint8_t iso_cig_id;
  uint8_t iso_cis_count;
  uint8_t iso_big_id;
  uint8_t iso_bis_count;
  uint8_t iso_path_dir;
  uint8_t iso_cis_established;
  uint8_t iso_big_established;
  uint8_t iso_big_terminated;
  unsigned int accept_list_count;
  unsigned int resolv_list_count;
  unsigned long event_mask_page2_sets;
  unsigned long random_addr_sets;
  unsigned long adv_param_sets;
  unsigned long adv_data_sets;
  unsigned long scan_rsp_sets;
  unsigned long adv_enable_sets;
  unsigned long scan_param_sets;
  unsigned long scan_enable_sets;
  unsigned long accept_list_clears;
  unsigned long accept_list_adds;
  unsigned long accept_list_dels;
  unsigned long resolv_list_clears;
  unsigned long addr_resolv_sets;
  unsigned long rpa_timeout_sets;
  unsigned long adv_hwsim_tx;
  unsigned long scan_hwsim_polls;
  unsigned long scan_hwsim_reports;
  unsigned long iso_read_buffer_size_v2s;
  unsigned long iso_read_tx_syncs;
  unsigned long iso_host_feature_sets;
  unsigned long iso_cig_param_sets;
  unsigned long iso_create_cis;
  unsigned long iso_remove_cig;
  unsigned long iso_create_big;
  unsigned long iso_term_big;
  unsigned long iso_setup_path;
  unsigned long iso_create_cis_status;
  unsigned long iso_create_big_status;
  unsigned long iso_term_big_status;
  unsigned long iso_cis_established_events;
  unsigned long iso_create_big_complete_events;
  unsigned long iso_term_big_complete_events;
};

static struct linux_bt_hci_user_ctrl_state g_hci_user_ctrl_state;

static int linux_bt_upstream_hci_user_adv_medium_tx(bool enabled)
{
  char payload[128];
  int ret;

  snprintf(payload, sizeof(payload),
           enabled ?
           "HCI_LE_ADV_REPORT role=%d event=ADV_IND name=linux-bt-hwsim" :
           "HCI_LE_ADV_STOP role=%d",
#ifdef CONFIG_SIM_BTHWSIM_ROLE
           CONFIG_SIM_BTHWSIM_ROLE
#else
           -1
#endif
           );

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ADV,
                            LINUX_BT_HWSIM_DST_BROADCAST,
                            payload, strlen(payload));
  if (ret == 0)
    {
      g_hci_user_ctrl_state.adv_hwsim_tx++;
    }

  return ret;
}

static int linux_bt_upstream_hci_user_inject_adv_report(struct hci_dev *hdev,
                                                        uint16_t src,
                                                        const char *name)
{
  struct sk_buff *skb;
  uint8_t event[64];
  uint8_t eir[31];
  size_t name_len;
  size_t eir_len;
  size_t event_len;
  int ret;

  if (hdev == NULL)
    {
      return -EINVAL;
    }

  name_len = name != NULL ? strlen(name) : 0;
  if (name_len > sizeof(eir) - 2)
    {
      name_len = sizeof(eir) - 2;
    }

  memset(eir, 0, sizeof(eir));
  eir[0] = (uint8_t)(name_len + 1);
  eir[1] = 0x09;
  if (name_len > 0)
    {
      memcpy(&eir[2], name, name_len);
    }

  eir_len = name_len + 2;

  memset(event, 0, sizeof(event));
  event[0] = HCI_EV_LE_META;
  event[2] = HCI_EV_LE_ADVERTISING_REPORT;
  event[3] = 1;
  event[4] = 0;       /* ADV_IND */
  event[5] = 0;       /* public address */
  event[6] = (uint8_t)(src & 0xff);
  event[7] = 0x00;
  event[8] = 0x00;
  event[9] = 0x00;
  event[10] = 0xfe;
  event[11] = 0x02;
  event[12] = (uint8_t)eir_len;
  memcpy(&event[13], eir, eir_len);
  event[13 + eir_len] = 0xc5; /* -59 dBm */

  event[1] = (uint8_t)(12 + eir_len);
  event_len = (size_t)event[1] + 2;

  skb = bt_skb_alloc(event_len, GFP_KERNEL);
  if (skb == NULL)
    {
      return -ENOMEM;
    }

  hci_skb_pkt_type(skb) = HCI_EVENT_PKT;
  skb_put_data(skb, event, event_len);
  ret = hci_recv_frame(hdev, skb);
  if (ret >= 0)
    {
      g_hci_user_ctrl_state.scan_hwsim_reports++;
    }

  return ret;
}

static int linux_bt_upstream_hci_user_scan_medium_poll(struct hci_dev *hdev)
{
  uint8_t payload[160];
  char text[161];
  char name[32];
  const char *name_pos;
  uint16_t src;
  uint16_t dst = 0;
  uint32_t payload_len;
  unsigned int records = 0;
  int ret;

  if (hdev == NULL || g_hci_user_ctrl_state.scan_enable == 0)
    {
      return -EAGAIN;
    }

  g_hci_user_ctrl_state.scan_hwsim_polls++;

  while (records < 8)
    {
      memset(payload, 0, sizeof(payload));
      payload_len = 0;
      ret = linux_bt_hwsim_read_raw(LINUX_BT_HWSIM_TYPE_ADV, &src, &dst,
                                    payload, sizeof(payload),
                                    &payload_len);
      if (ret <= 0)
        {
          break;
        }

      records++;
      if (payload_len >= sizeof(text))
        {
          payload_len = sizeof(text) - 1;
        }

      memcpy(text, payload, payload_len);
      text[payload_len] = '\0';
      if (strstr(text, "HCI_LE_ADV_REPORT") == NULL)
        {
          continue;
        }

      snprintf(name, sizeof(name), "linux-bt-hwsim-%u", src);
      name_pos = strstr(text, "name=");
      if (name_pos != NULL)
        {
          const char *cursor = name_pos + 5;
          size_t i = 0;

          while (cursor[i] != '\0' && cursor[i] != ' ' &&
                 i + 1 < sizeof(name))
            {
              name[i] = cursor[i];
              i++;
            }

          name[i] = '\0';
        }

      return linux_bt_upstream_hci_user_inject_adv_report(hdev, src, name);
    }

  return -EAGAIN;
}

struct linux_bt_upstream_hci_raw_handle
{
  struct socket sock;
  uint16_t dev;
  bool bound;
};

struct linux_bt_upstream_hci_socket_handle
{
  struct socket sock;
  uint16_t dev;
  uint16_t channel;
  bool bound;
};

struct linux_bt_upstream_l2cap_socket_handle
{
  struct socket sock;
  struct file file;
  uint16_t psm;
  uint16_t cid;
  uint16_t handle;
  uint8_t pending_accepts;
  bool bound;
  bool connected;
  bool listening;
  bool bnep_owned;
  bool bnep_user_closed;
  bool bnep_released;
};

struct linux_bt_upstream_iso_socket_handle
{
  struct socket sock;
  struct file file;
  uint8_t addr_type;
  uint16_t handle;
  uint8_t pending_accepts;
  bool bound;
  bool connected;
  bool listening;
};

struct linux_bt_upstream_control_socket_handle
{
  struct socket sock;
  struct file file;
  int proto;
};

struct linux_bt_upstream_rfcomm_socket_handle
{
  struct socket sock;
  struct file file;
  uint8_t channel;
  uint16_t cid;
  uint16_t handle;
  bool bound;
  bool connected;
};

struct linux_bt_l2cap_bind_state
{
  struct sock *sk;
  uint16_t psm;
  uint16_t cid;
  uint16_t handle;
  uint8_t bdaddr_type;
  bdaddr_t bdaddr;
  uint16_t imtu;
  uint16_t omtu;
  uint16_t flush_to;
  uint8_t mode;
  uint8_t fcs;
  uint8_t max_tx;
  uint16_t txwin_size;
  uint32_t lm;
  uint32_t channel_policy;
  uint32_t phy;
  uint8_t sec_level;
  bool defer_setup;
  bool flushable;
  bool force_active;
};

#define LINUX_BT_UPSTREAM_L2CAP_LISTENERS 8

static struct linux_bt_upstream_l2cap_socket_handle *
  g_l2cap_socket_listeners[LINUX_BT_UPSTREAM_L2CAP_LISTENERS];

static void linux_bt_upstream_l2cap_unregister_listener(
  struct linux_bt_upstream_l2cap_socket_handle *h)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_UPSTREAM_L2CAP_LISTENERS; i++)
    {
      if (g_l2cap_socket_listeners[i] == h)
        {
          g_l2cap_socket_listeners[i] = NULL;
        }
    }
}

static int linux_bt_upstream_l2cap_register_listener(
  struct linux_bt_upstream_l2cap_socket_handle *h)
{
  unsigned int i;

  linux_bt_upstream_l2cap_unregister_listener(h);
  for (i = 0; i < LINUX_BT_UPSTREAM_L2CAP_LISTENERS; i++)
    {
      if (g_l2cap_socket_listeners[i] == NULL)
        {
          g_l2cap_socket_listeners[i] = h;
          return 0;
        }
    }

  return -ENOMEM;
}

static struct linux_bt_upstream_l2cap_socket_handle *
linux_bt_upstream_l2cap_find_listener(uint16_t psm, uint16_t cid,
                                      uint16_t handle)
{
  struct linux_bt_upstream_l2cap_socket_handle *listener;
  unsigned int i;

  for (i = 0; i < LINUX_BT_UPSTREAM_L2CAP_LISTENERS; i++)
    {
      listener = g_l2cap_socket_listeners[i];
      if (listener == NULL || !listener->listening)
        {
          continue;
        }

      if (listener->psm == psm &&
          (listener->cid == 0 || cid == 0 || listener->cid == cid) &&
          (listener->handle == 0 || handle == 0 ||
           listener->handle == handle))
        {
          return listener;
        }
    }

  return NULL;
}

static void linux_bt_upstream_l2cap_note_pending_accept(
  struct linux_bt_upstream_l2cap_socket_handle *h)
{
  struct linux_bt_upstream_l2cap_socket_handle *listener;

  if (h == NULL)
    {
      return;
    }

  listener = linux_bt_upstream_l2cap_find_listener(h->psm, h->cid,
                                                   h->handle);
  if (listener != NULL && listener != h &&
      listener->pending_accepts < UINT8_MAX)
    {
      listener->pending_accepts++;
    }
}

#define LINUX_BT_UPSTREAM_ISO_LISTENERS 8

static struct linux_bt_upstream_iso_socket_handle *
  g_iso_socket_listeners[LINUX_BT_UPSTREAM_ISO_LISTENERS];

static void linux_bt_upstream_iso_unregister_listener(
  struct linux_bt_upstream_iso_socket_handle *h)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_UPSTREAM_ISO_LISTENERS; i++)
    {
      if (g_iso_socket_listeners[i] == h)
        {
          g_iso_socket_listeners[i] = NULL;
        }
    }
}

static int linux_bt_upstream_iso_register_listener(
  struct linux_bt_upstream_iso_socket_handle *h)
{
  unsigned int i;

  linux_bt_upstream_iso_unregister_listener(h);
  for (i = 0; i < LINUX_BT_UPSTREAM_ISO_LISTENERS; i++)
    {
      if (g_iso_socket_listeners[i] == NULL)
        {
          g_iso_socket_listeners[i] = h;
          return 0;
        }
    }

  return -ENOMEM;
}

static struct linux_bt_upstream_iso_socket_handle *
linux_bt_upstream_iso_find_listener(uint8_t addr_type, uint16_t handle)
{
  struct linux_bt_upstream_iso_socket_handle *listener;
  unsigned int i;

  for (i = 0; i < LINUX_BT_UPSTREAM_ISO_LISTENERS; i++)
    {
      listener = g_iso_socket_listeners[i];
      if (listener == NULL || !listener->listening)
        {
          continue;
        }

      if (listener->addr_type == addr_type &&
          (listener->handle == 0 || handle == 0 ||
           listener->handle == handle))
        {
          return listener;
        }
    }

  return NULL;
}

static void linux_bt_upstream_iso_note_pending_accept(
  struct linux_bt_upstream_iso_socket_handle *h)
{
  struct linux_bt_upstream_iso_socket_handle *listener;

  if (h == NULL)
    {
      return;
    }

  listener = linux_bt_upstream_iso_find_listener(h->addr_type, h->handle);
  if (listener != NULL && listener != h &&
      listener->pending_accepts < UINT8_MAX)
    {
      listener->pending_accepts++;
    }
}

static struct linux_bt_l2cap_bind_state g_l2cap_bound_socks[8];
static struct socket g_l2cap_probe_socket;
static struct file g_l2cap_probe_file;
static unsigned int g_l2cap_probe_bnep_refs;
static bool g_l2cap_probe_close_pending;
static bool g_l2cap_probe_bound;
static struct hci_conn *g_hci_user_iso_cis_conn;
static struct hci_conn *g_hci_user_iso_cis_aux_conn;
static struct hci_conn *g_hci_user_iso_bis_conn;

static struct linux_bt_l2cap_bind_state *
linux_bt_l2cap_bound_state(struct sock *sk)
{
  unsigned int i;

  for (i = 0; i < sizeof(g_l2cap_bound_socks) /
                  sizeof(g_l2cap_bound_socks[0]); i++)
    {
      if (g_l2cap_bound_socks[i].sk == sk)
        {
          return &g_l2cap_bound_socks[i];
        }
    }

  return NULL;
}

static struct linux_bt_l2cap_bind_state *
linux_bt_l2cap_ensure_bound_state(struct sock *sk)
{
  struct linux_bt_l2cap_bind_state *state;
  unsigned int i;

  if (sk == NULL)
    {
      return NULL;
    }

  state = linux_bt_l2cap_bound_state(sk);
  if (state != NULL)
    {
      return state;
    }

  for (i = 0; i < sizeof(g_l2cap_bound_socks) /
                  sizeof(g_l2cap_bound_socks[0]); i++)
    {
      if (g_l2cap_bound_socks[i].sk == NULL)
        {
          memset(&g_l2cap_bound_socks[i], 0,
                 sizeof(g_l2cap_bound_socks[i]));
          g_l2cap_bound_socks[i].sk = sk;
          g_l2cap_bound_socks[i].imtu = L2CAP_DEFAULT_MTU;
          g_l2cap_bound_socks[i].omtu = L2CAP_DEFAULT_MTU;
          g_l2cap_bound_socks[i].flush_to = L2CAP_DEFAULT_FLUSH_TO;
          g_l2cap_bound_socks[i].mode = L2CAP_MODE_BASIC;
          g_l2cap_bound_socks[i].fcs = L2CAP_FCS_CRC16;
          g_l2cap_bound_socks[i].max_tx = L2CAP_DEFAULT_MAX_TX;
          g_l2cap_bound_socks[i].txwin_size = L2CAP_DEFAULT_TX_WINDOW;
          g_l2cap_bound_socks[i].sec_level = BT_SECURITY_LOW;
          g_l2cap_bound_socks[i].channel_policy =
            BT_CHANNEL_POLICY_BREDR_ONLY;
          g_l2cap_bound_socks[i].phy = BT_PHY_BR_1M_1SLOT;
          return &g_l2cap_bound_socks[i];
        }
    }

  return NULL;
}

static void linux_bt_l2cap_clear_bound_state_by_sk(struct sock *sk)
{
  unsigned int i;

  if (sk == NULL)
    {
      return;
    }

  for (i = 0; i < sizeof(g_l2cap_bound_socks) /
                  sizeof(g_l2cap_bound_socks[0]); i++)
    {
      if (g_l2cap_bound_socks[i].sk == sk)
        {
          memset(&g_l2cap_bound_socks[i], 0,
                 sizeof(g_l2cap_bound_socks[i]));
        }
    }
}

static void linux_bt_l2cap_clear_bound_state_by_addr(uint16_t psm,
                                                     uint16_t cid,
                                                     uint16_t handle)
{
  bdaddr_t any = {{0, 0, 0, 0, 0, 0}};
  unsigned int i;

  if (psm == 0 && cid == 0)
    {
      return;
    }

  for (i = 0; i < sizeof(g_l2cap_bound_socks) /
                  sizeof(g_l2cap_bound_socks[0]); i++)
    {
      if (g_l2cap_bound_socks[i].sk == NULL)
        {
          continue;
        }

      if (psm != 0 && g_l2cap_bound_socks[i].psm != psm)
        {
          continue;
        }

      if (cid != 0 && g_l2cap_bound_socks[i].cid != cid)
        {
          continue;
        }

      if (handle != 0 && g_l2cap_bound_socks[i].handle != 0 &&
          g_l2cap_bound_socks[i].handle != handle)
        {
          continue;
        }

      memset(&g_l2cap_bound_socks[i], 0,
             sizeof(g_l2cap_bound_socks[i]));
    }

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
  if (psm != 0)
    {
      (void)l2cap_compat_clear_bound_psm(&any, BDADDR_BREDR,
                                         cpu_to_le16(psm));
    }
#endif
}

static void linux_bt_l2cap_ensure_sim_br_conn(uint16_t handle)
{
#ifdef CONFIG_SIM_BTHWSIM
  struct hci_dev *hdev;
  struct hci_conn *conn;
  bool put_hdev = false;
  uint8_t addr[6];
  uint16_t peer;

  if (handle < 0x0050)
    {
      return;
    }

  hdev = hci_dev_get(0);
  if (hdev != NULL)
    {
      put_hdev = true;
    }
  else
    {
      hdev = linux_bt_upstream_vhci_default_hdev();
    }

  if (hdev == NULL)
    {
      return;
    }

  peer = (uint16_t)(handle - 0x0050);
  addr[0] = (uint8_t)(peer & 0xff);
  addr[1] = 0x00;
  addr[2] = 0x00;
  addr[3] = 0x00;
  addr[4] = 0xfe;
  addr[5] = 0x02;

  (void)linux_bt_upstream_hci_sim_br_complete(hdev, addr, 0, handle);

  conn = hci_conn_hash_lookup_handle(hdev, handle);
  if (conn == NULL)
    {
      bdaddr_t dst;

      memcpy(&dst, addr, sizeof(dst));
      if (hdev->acl_mtu == 0)
        {
          hdev->acl_mtu = HCI_MAX_FRAME_SIZE;
        }

      conn = hci_conn_add(hdev, ACL_LINK, &dst, HCI_ROLE_MASTER, handle);
      if (!IS_ERR(conn))
        {
          conn->state = BT_CONNECTED;
          conn->mtu = hdev->acl_mtu;
          conn->sec_level = BT_SECURITY_LOW;
          conn->pending_sec_level = BT_SECURITY_LOW;
        }
    }

  if (put_hdev)
    {
      hci_dev_put(hdev);
    }
#else
  (void)handle;
#endif
}

struct linux_bt_iso_bind_state
{
  struct sock *sk;
  uint8_t bdaddr_type;
  bdaddr_t bdaddr;
};

struct linux_bt_iso_pinfo
{
  struct bt_sock bt;
  uint16_t handle;
  struct bt_iso_qos qos;
  bool qos_user_set;
  uint8_t base[LINUX_BT_ISO_BASE_MAX_LENGTH];
  uint8_t base_len;
  bool defer_setup;
  bool pkt_status;
  bool pkt_seqnum;
};

struct linux_bt_rfcomm_pinfo
{
  struct bt_sock bt;
  bdaddr_t src;
  bdaddr_t dst;
  uint8_t channel;
  uint8_t sec_level;
  bool role_switch;
  bool defer_setup;
  uint16_t mtu;
  uint16_t cid;
  uint16_t handle;
};

struct linux_bt_sco_pinfo
{
  struct bt_sock bt;
  bdaddr_t src;
  bdaddr_t dst;
  uint16_t mtu;
  uint16_t handle;
  uint16_t voice;
  bool defer_setup;
  bool pkt_status;
};

static struct linux_bt_iso_bind_state g_iso_bound_socks[8];
static struct socket g_iso_probe_socket;
static bool g_iso_probe_bound;
static uint16_t g_iso_probe_handle;
static uint8_t g_iso_probe_rx[251];
static size_t g_iso_probe_rx_len;
static bool g_iso_probe_rx_valid;
static bool g_iso_probe_upstream_attached;
static uint16_t g_l2cap_probe_psm;
static uint16_t g_l2cap_probe_cid;
static uint16_t g_l2cap_probe_handle;
static uint8_t g_l2cap_probe_rx[512];
static size_t g_l2cap_probe_rx_len;
static bool g_l2cap_probe_rx_valid;
static bool g_l2cap_probe_native_control_cid;

struct linux_bt_l2cap_pending_payload
{
  bool valid;
  uint16_t handle;
  uint16_t cid;
  uint32_t seq;
  uint8_t data[512];
  size_t len;
};

#define LINUX_BT_L2CAP_PENDING_PAYLOADS 1024

static struct linux_bt_l2cap_pending_payload
  g_l2cap_pending_payloads[LINUX_BT_L2CAP_PENDING_PAYLOADS];
static uint32_t g_l2cap_pending_seq;

static int linux_bt_hci_ensure_connected_handle(uint8_t type,
                                                uint16_t handle,
                                                const bdaddr_t *bdaddr,
                                                uint8_t bdaddr_type,
                                                bool outgoing,
                                                uint8_t auth_type);

static int linux_bt_hci_ensure_connected_handle(uint8_t type,
                                                uint16_t handle,
                                                const bdaddr_t *bdaddr,
                                                uint8_t bdaddr_type,
                                                bool outgoing,
                                                uint8_t auth_type)
{
  struct hci_dev *hdev;
  struct hci_conn *conn;
  bdaddr_t any;
  bdaddr_t dst;
  bool dst_any;
  bool put_hdev = false;

  if (handle == 0)
    {
      return 0;
    }

  hdev = hci_dev_get(0);
  if (hdev != NULL)
    {
      put_hdev = true;
    }
  else
    {
      hdev = linux_bt_upstream_vhci_default_hdev();
    }

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (bdaddr != NULL)
    {
      bacpy(&dst, bdaddr);
    }
  else
    {
      bacpy(&dst, BDADDR_ANY);
    }

  bacpy(&any, BDADDR_ANY);
  dst_any = bacmp(&dst, &any) == 0;
  if (dst_any)
    {
      memset(&dst, 0, sizeof(dst));
      if (type == ACL_LINK && handle >= 0x0050)
        {
          uint16_t peer = (uint16_t)(handle - 0x0050);

          dst.b[0] = (uint8_t)(peer & 0xff);
          dst.b[1] = 0x00;
          dst.b[2] = 0x00;
          dst.b[3] = 0x00;
          dst.b[4] = 0xfe;
          dst.b[5] = 0x02;
        }
      else
        {
          dst.b[0] = (uint8_t)(handle & 0xff);
          dst.b[1] = (uint8_t)(handle >> 8);
          dst.b[5] = type;
        }
    }

  conn = hci_conn_hash_lookup_handle(hdev, handle);
  if (conn == NULL)
    {
      conn = hci_conn_add(hdev, type, &dst, HCI_ROLE_MASTER, handle);
      if (IS_ERR(conn))
        {
          int ret = PTR_ERR(conn);

          if (put_hdev)
            {
              hci_dev_put(hdev);
            }

          return ret;
        }
    }

  conn->state = BT_CONNECTED;
  conn->out = outgoing;
  conn->auth_type = auth_type;
  conn->sec_level = BT_SECURITY_LOW;
  conn->pending_sec_level = BT_SECURITY_LOW;
  if (type == LE_LINK)
    {
      conn->dst_type = bdaddr_type == BDADDR_LE_RANDOM ?
                       ADDR_LE_DEV_RANDOM : ADDR_LE_DEV_PUBLIC;
      if (hdev->le_mtu == 0)
        {
          hdev->le_mtu = 251;
        }

      if (hdev->le_pkts == 0)
        {
          hdev->le_pkts = 10;
        }

      if (hdev->le_cnt == 0)
        {
          hdev->le_cnt = hdev->le_pkts;
        }

      conn->mtu = hdev->le_mtu;
    }
  else if (type == ACL_LINK)
    {
      if (hdev->acl_mtu == 0)
        {
          hdev->acl_mtu = HCI_MAX_FRAME_SIZE;
        }

      conn->mtu = hdev->acl_mtu;
    }
  else
    {
      if (hdev->iso_mtu == 0)
        {
          hdev->iso_mtu = ISO_DEFAULT_MTU;
        }

      if (hdev->iso_pkts == 0)
        {
          hdev->iso_pkts = 1;
        }

      if (hdev->iso_cnt == 0)
        {
          hdev->iso_cnt = hdev->iso_pkts;
        }

      conn->mtu = hdev->iso_mtu;
    }

  if (put_hdev)
    {
      hci_dev_put(hdev);
    }

  return 0;
}

#ifdef CONFIG_SIM_BTHWSIM
static struct l2cap_conn g_l2cap_probe_sim_conn;
static struct hci_chan *g_l2cap_probe_sim_hchan;
#endif

#ifdef CONFIG_SIM_BTHWSIM
static int linux_bt_l2cap_attach_send_chan(struct socket *sock,
                                           uint16_t handle,
                                           uint16_t cid)
{
  struct hci_dev *hdev;
  struct l2cap_chan *chan;
  int ret;

  if (sock == NULL || sock->sk == NULL || handle == 0)
    {
      return -ENOTCONN;
    }

  linux_bt_l2cap_ensure_sim_br_conn(handle);

  hdev = hci_dev_get(0);
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  chan = l2cap_pi(sock->sk)->chan;
  if (chan == NULL)
    {
      hci_dev_put(hdev);
      return -ENOTCONN;
    }

  sock->sk->sk_state = BT_CONNECTED;
  ret = l2cap_sim_attach_channel(hdev, handle, chan,
                                 cid != 0 ? cid : L2CAP_CID_DYN_START);
  if (ret == -EBUSY)
    {
      (void)l2cap_sim_detach_channel(chan);
      sock->sk->sk_state = BT_CONNECTED;
      ret = l2cap_sim_attach_channel(hdev, handle, chan,
                                     cid != 0 ? cid : L2CAP_CID_DYN_START);
    }

  hci_dev_put(hdev);
  return ret;
}

static int linux_bt_l2cap_probe_attach_send_chan(void)
{
  if (!g_l2cap_probe_bound)
    {
      return -ENOTCONN;
    }

  return linux_bt_l2cap_attach_send_chan(&g_l2cap_probe_socket,
                                         g_l2cap_probe_handle,
                                         g_l2cap_probe_cid);
}

static void linux_bt_l2cap_probe_detach_send_chan(void)
{
  /* Keep the simulated channel attached for the socket lifetime.
   *
   * Linux L2CAP sockets keep chan->conn while connected.  Clearing it after
   * every send makes the next RX path re-add the same channel to the
   * connection list, which can make later same-CID AVDTP PDUs disappear from
   * l2cap_get_chan_by_scid().
   */
}

static int linux_bt_l2cap_attach_recv_chan(struct socket *sock,
                                           uint16_t handle,
                                           uint16_t cid)
{
  struct hci_dev *hdev;
  struct l2cap_chan *chan;
  int ret;

  if (sock == NULL || sock->sk == NULL || handle == 0 || cid == 0)
    {
      return -ENOTCONN;
    }

  hdev = hci_dev_get(0);
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  chan = l2cap_pi(sock->sk)->chan;
  if (chan == NULL)
    {
      hci_dev_put(hdev);
      return -ENOTCONN;
    }

  sock->sk->sk_state = BT_CONNECTED;
  ret = l2cap_sim_attach_channel(hdev, handle, chan, cid);
  if (ret == -EBUSY)
    {
      (void)l2cap_sim_detach_channel(chan);
      sock->sk->sk_state = BT_CONNECTED;
      ret = l2cap_sim_attach_channel(hdev, handle, chan, cid);
    }

  hci_dev_put(hdev);
  return ret;
}

static int linux_bt_l2cap_probe_attach_recv_chan(uint16_t handle,
                                                 uint16_t cid)
{
  if (!g_l2cap_probe_bound)
    {
      return -ENOTCONN;
    }

  return linux_bt_l2cap_attach_recv_chan(&g_l2cap_probe_socket, handle,
                                         cid);
}

static int linux_bt_l2cap_probe_mark_connected_from_listen(void)
{
  struct linux_bt_l2cap_bind_state *state;

  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.sk == NULL ||
      g_l2cap_probe_socket.sk->sk_state != BT_LISTEN)
    {
      return 0;
    }

  state = linux_bt_l2cap_ensure_bound_state(g_l2cap_probe_socket.sk);
  if (state == NULL)
    {
      return -ENOMEM;
    }

  if (state->psm == 0)
    {
      state->psm = g_l2cap_probe_psm;
    }

  if (state->cid == 0)
    {
      state->cid = g_l2cap_probe_cid;
    }

  if (state->handle == 0)
    {
      state->handle = g_l2cap_probe_handle;
    }

  g_l2cap_probe_socket.sk->sk_state = BT_CONNECTED;
  return linux_bt_hci_ensure_connected_handle(
    state->bdaddr_type != 0 ? LE_LINK : ACL_LINK, state->handle,
    &state->bdaddr, state->bdaddr_type, true, 0);
}

static void linux_bt_l2cap_probe_complete_sim_disconnect(void)
{
  struct hci_dev *hdev;
  struct l2cap_chan *chan;

  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.sk == NULL)
    {
      return;
    }

  hdev = linux_bt_upstream_vhci_default_hdev();
  if (hdev != NULL && g_l2cap_probe_handle != 0 && g_l2cap_probe_cid != 0)
    {
      (void)l2cap_sim_detach_cid(hdev, g_l2cap_probe_handle,
                                 g_l2cap_probe_cid);
    }

  chan = l2cap_pi(g_l2cap_probe_socket.sk)->chan;
  if (chan != NULL && chan->conn != NULL)
    {
      if (chan->conn != &g_l2cap_probe_sim_conn)
        {
          (void)l2cap_sim_detach_channel(chan);
        }
      else
        {
          chan->conn = NULL;
          chan->state = BT_CLOSED;
        }

      g_l2cap_probe_socket.sk->sk_state = BT_CLOSED;
    }

  skb_queue_purge(&g_l2cap_probe_socket.sk->sk_receive_queue);
  skb_queue_purge(&g_l2cap_probe_socket.sk->sk_write_queue);
}
#else
static int linux_bt_l2cap_attach_send_chan(struct socket *sock,
                                           uint16_t handle,
                                           uint16_t cid)
{
  (void)sock;
  (void)handle;
  (void)cid;
  return -EOPNOTSUPP;
}

static int linux_bt_l2cap_probe_mark_connected_from_listen(void)
{
  return 0;
}

static void linux_bt_l2cap_probe_complete_sim_disconnect(void)
{
}
#endif

static void linux_bt_l2cap_probe_release(void)
{
  linux_bt_l2cap_clear_bound_state_by_sk(g_l2cap_probe_socket.sk);
  linux_bt_l2cap_clear_bound_state_by_addr(g_l2cap_probe_psm,
                                           g_l2cap_probe_cid,
                                           g_l2cap_probe_handle);

  if (g_l2cap_probe_socket.ops != NULL &&
      g_l2cap_probe_socket.ops->release != NULL)
    {
      linux_bt_l2cap_probe_complete_sim_disconnect();
      g_l2cap_probe_socket.ops->release(&g_l2cap_probe_socket);
    }

  memset(&g_l2cap_probe_socket, 0, sizeof(g_l2cap_probe_socket));
  memset(&g_l2cap_probe_file, 0, sizeof(g_l2cap_probe_file));
#ifdef CONFIG_SIM_BTHWSIM
  memset(&g_l2cap_probe_sim_conn, 0, sizeof(g_l2cap_probe_sim_conn));
  g_l2cap_probe_sim_hchan = NULL;
#endif
  g_l2cap_probe_bnep_refs = 0;
  g_l2cap_probe_bound = false;
  g_l2cap_probe_close_pending = false;
  g_l2cap_probe_psm = 0;
  g_l2cap_probe_cid = 0;
  g_l2cap_probe_handle = 0;
  g_l2cap_probe_rx_len = 0;
  g_l2cap_probe_rx_valid = false;
  g_l2cap_probe_native_control_cid = false;
}

static void linux_bt_l2cap_pending_store(uint16_t handle, uint16_t cid,
                                         const void *payload,
                                         size_t payload_len)
{
  unsigned int i;
  unsigned int oldest = 0;
  bool oldest_valid = false;

  if (payload == NULL || payload_len == 0)
    {
      return;
    }

  for (i = 0; i < sizeof(g_l2cap_pending_payloads) /
                  sizeof(g_l2cap_pending_payloads[0]); i++)
    {
      if (!g_l2cap_pending_payloads[i].valid)
        {
          break;
        }

      if (!oldest_valid ||
          g_l2cap_pending_payloads[i].seq <
          g_l2cap_pending_payloads[oldest].seq)
        {
          oldest = i;
          oldest_valid = true;
        }
    }

  if (i == sizeof(g_l2cap_pending_payloads) /
           sizeof(g_l2cap_pending_payloads[0]))
    {
      i = oldest;
    }

  if (payload_len > sizeof(g_l2cap_pending_payloads[i].data))
    {
      payload_len = sizeof(g_l2cap_pending_payloads[i].data);
    }

  g_l2cap_pending_payloads[i].valid = true;
  g_l2cap_pending_payloads[i].handle = handle;
  g_l2cap_pending_payloads[i].cid = cid;
  g_l2cap_pending_payloads[i].seq = ++g_l2cap_pending_seq;
  g_l2cap_pending_payloads[i].len = payload_len;
  memcpy(g_l2cap_pending_payloads[i].data, payload, payload_len);
}

static bool linux_bt_l2cap_pending_pop(uint16_t handle, uint16_t cid,
                                       void *payload, size_t max_len,
                                       size_t *payload_len)
{
  unsigned int i;
  unsigned int best = 0;
  bool found = false;

  if (payload == NULL || payload_len == NULL || cid == 0)
    {
      return false;
    }

  for (i = 0; i < sizeof(g_l2cap_pending_payloads) /
                  sizeof(g_l2cap_pending_payloads[0]); i++)
    {
      if (!g_l2cap_pending_payloads[i].valid ||
          g_l2cap_pending_payloads[i].cid != cid ||
          (handle != 0 && g_l2cap_pending_payloads[i].handle != handle))
        {
          continue;
        }

      if (!found ||
          g_l2cap_pending_payloads[i].seq <
          g_l2cap_pending_payloads[best].seq)
        {
          best = i;
          found = true;
        }
    }

  if (!found)
    {
      return false;
    }

  *payload_len = g_l2cap_pending_payloads[best].len;
  if (*payload_len > max_len)
    {
      *payload_len = max_len;
    }

  if (*payload_len > 0)
    {
      memcpy(payload, g_l2cap_pending_payloads[best].data, *payload_len);
    }

  memset(&g_l2cap_pending_payloads[best], 0,
         sizeof(g_l2cap_pending_payloads[best]));
  return true;
}

static void linux_bt_l2cap_pending_drop_match(uint16_t handle, uint16_t cid,
                                              const void *payload,
                                              size_t payload_len)
{
  unsigned int i;

  if (payload == NULL || payload_len == 0 || cid == 0)
    {
      return;
    }

  for (i = 0; i < sizeof(g_l2cap_pending_payloads) /
                  sizeof(g_l2cap_pending_payloads[0]); i++)
    {
      if (!g_l2cap_pending_payloads[i].valid ||
          g_l2cap_pending_payloads[i].cid != cid ||
          (handle != 0 && g_l2cap_pending_payloads[i].handle != handle) ||
          g_l2cap_pending_payloads[i].len != payload_len ||
          memcmp(g_l2cap_pending_payloads[i].data, payload,
                 payload_len) != 0)
        {
          continue;
        }

      memset(&g_l2cap_pending_payloads[i], 0,
             sizeof(g_l2cap_pending_payloads[i]));
      return;
    }
}

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
static uint16_t g_noaf_l2cap_psm;
static uint16_t g_noaf_l2cap_cid;
static uint16_t g_noaf_l2cap_handle;
static bool g_noaf_l2cap_connected;
static bool g_noaf_l2cap_listening;
static uint8_t g_noaf_l2cap_rx[512];
static size_t g_noaf_l2cap_rx_len;
static bool g_noaf_l2cap_rx_valid;

static uint8_t g_noaf_iso_addr_type;
static uint16_t g_noaf_iso_handle;
static bool g_noaf_iso_connected;
static uint8_t g_noaf_iso_rx[251];
static size_t g_noaf_iso_rx_len;
static bool g_noaf_iso_rx_valid;

static uint8_t g_noaf_mgmt_io_capability = 3;
static uint8_t g_noaf_mgmt_events[8][64];
static size_t g_noaf_mgmt_event_len[8];
static unsigned int g_noaf_mgmt_event_head;
static unsigned int g_noaf_mgmt_event_tail;

static int linux_bt_noaf_l2cap_queue(uint16_t handle, uint16_t cid,
                                     const void *payload,
                                     size_t payload_len);
static int linux_bt_noaf_iso_queue(uint16_t handle, const void *payload,
                                   size_t payload_len);
#endif

#define linux_bt_iso_pi(sk) ((struct linux_bt_iso_pinfo *)(sk))
#define linux_bt_rfcomm_pi(sk) ((struct linux_bt_rfcomm_pinfo *)(sk))
#define linux_bt_sco_pi(sk) ((struct linux_bt_sco_pinfo *)(sk))

static struct linux_bt_iso_bind_state *
linux_bt_iso_bound_state(struct sock *sk)
{
  unsigned int i;

  for (i = 0; i < sizeof(g_iso_bound_socks) /
                  sizeof(g_iso_bound_socks[0]); i++)
    {
      if (g_iso_bound_socks[i].sk == sk)
        {
          return &g_iso_bound_socks[i];
        }
    }

  return NULL;
}

static uint8_t linux_bt_iso_link_type(uint16_t handle)
{
  return (handle & 0xff00) == 0x0200 ? CIS_LINK : BIS_LINK;
}

static int linux_bt_hci_mgmt_broadcast_addr_event_skip(uint16_t event,
                                                       uint16_t dev_id,
                                                       const bdaddr_t *bdaddr,
                                                       uint8_t bdaddr_type,
                                                       struct sock *skip);
static int linux_bt_hci_mgmt_broadcast_device_added(uint16_t dev_id,
                                                    const bdaddr_t *bdaddr,
                                                    uint8_t bdaddr_type,
                                                    uint8_t action,
                                                    struct sock *skip);
static int linux_bt_hci_mgmt_broadcast_device_flags(uint16_t dev_id,
                                                    const bdaddr_t *bdaddr,
                                                    uint8_t bdaddr_type,
                                                    uint32_t current_flags,
                                                    struct sock *skip);
static int linux_bt_hci_mgmt_broadcast_discovering(uint16_t dev_id,
                                                   uint8_t type,
                                                   bool discovering,
                                                   struct sock *skip);
static int linux_bt_hci_mgmt_broadcast_device_found(uint16_t dev_id,
                                                    uint16_t src,
                                                    const char *name,
                                                    struct sock *skip);
static int linux_bt_hci_mgmt_broadcast_user_confirm(uint16_t dev_id,
                                                    const bdaddr_t *bdaddr,
                                                    uint8_t bdaddr_type,
                                                    uint32_t value,
                                                    uint8_t confirm_hint,
                                                    struct sock *skip);
static int linux_bt_hci_mgmt_pending_complete_pair(
  struct hci_dev *hdev, const struct mgmt_addr_info *addr, uint8_t status);
static int linux_bt_hci_mgmt_pair_connected(struct hci_dev *hdev,
                                            const struct mgmt_addr_info *addr,
                                            struct sock *owner);
static int linux_bt_hci_sock_queue_mgmt_event(struct sock *sk,
                                              uint16_t event,
                                              uint16_t index,
                                              const void *data,
                                              uint16_t data_len);
static int linux_bt_hci_sock_queue_cmd_complete_data(struct sock *sk,
                                                     uint16_t opcode,
                                                     uint16_t index,
                                                     uint8_t status,
                                                     const void *data,
                                                     uint16_t data_len);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int linux_bt_hci_mgmt_default_handler(struct sock *sk,
                                             struct hci_dev *hdev,
                                             void *data, uint16_t data_len)
{
  (void)sk;
  (void)hdev;
  (void)data;
  (void)data_len;
  return 0;
}

static const struct hci_mgmt_handler g_linux_bt_mgmt_handlers
  [MGMT_OP_SET_DEVICE_FLAGS + 1] =
{
  [MGMT_OP_READ_VERSION] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_READ_VERSION_SIZE,
    .flags = HCI_MGMT_NO_HDEV | HCI_MGMT_UNTRUSTED,
  },
  [MGMT_OP_READ_COMMANDS] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_READ_COMMANDS_SIZE,
    .flags = HCI_MGMT_NO_HDEV | HCI_MGMT_UNTRUSTED,
  },
  [MGMT_OP_READ_INDEX_LIST] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_READ_INDEX_LIST_SIZE,
    .flags = HCI_MGMT_NO_HDEV | HCI_MGMT_UNTRUSTED,
  },
  [MGMT_OP_READ_INFO] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_READ_INFO_SIZE,
    .flags = HCI_MGMT_UNTRUSTED,
  },
  [MGMT_OP_SET_POWERED] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_SETTING_SIZE,
  },
  [MGMT_OP_SET_DISCOVERABLE] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_SET_DISCOVERABLE_SIZE,
  },
  [MGMT_OP_SET_CONNECTABLE] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_SETTING_SIZE,
  },
  [MGMT_OP_SET_BONDABLE] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_SETTING_SIZE,
  },
  [MGMT_OP_SET_LE] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_SETTING_SIZE,
  },
  [MGMT_OP_SET_ADVERTISING] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_SETTING_SIZE,
  },
  [MGMT_OP_SET_BREDR] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_SETTING_SIZE,
  },
  [MGMT_OP_DISCONNECT] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_DISCONNECT_SIZE,
  },
  [MGMT_OP_SET_IO_CAPABILITY] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_SET_IO_CAPABILITY_SIZE,
  },
  [MGMT_OP_PAIR_DEVICE] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_PAIR_DEVICE_SIZE,
  },
  [MGMT_OP_CANCEL_PAIR_DEVICE] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_CANCEL_PAIR_DEVICE_SIZE,
  },
  [MGMT_OP_UNPAIR_DEVICE] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_UNPAIR_DEVICE_SIZE,
  },
  [MGMT_OP_USER_CONFIRM_REPLY] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_USER_CONFIRM_REPLY_SIZE,
  },
  [MGMT_OP_USER_CONFIRM_NEG_REPLY] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_USER_CONFIRM_NEG_REPLY_SIZE,
  },
  [MGMT_OP_USER_PASSKEY_REPLY] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_USER_PASSKEY_REPLY_SIZE,
  },
  [MGMT_OP_USER_PASSKEY_NEG_REPLY] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_USER_PASSKEY_NEG_REPLY_SIZE,
  },
  [MGMT_OP_START_DISCOVERY] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_START_DISCOVERY_SIZE,
  },
  [MGMT_OP_STOP_DISCOVERY] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_STOP_DISCOVERY_SIZE,
  },
  [MGMT_OP_BLOCK_DEVICE] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_BLOCK_DEVICE_SIZE,
  },
  [MGMT_OP_UNBLOCK_DEVICE] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_UNBLOCK_DEVICE_SIZE,
  },
  [MGMT_OP_GET_CONN_INFO] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_GET_CONN_INFO_SIZE,
  },
  [MGMT_OP_GET_CLOCK_INFO] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_GET_CLOCK_INFO_SIZE,
  },
  [MGMT_OP_ADD_DEVICE] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_ADD_DEVICE_SIZE,
  },
  [MGMT_OP_REMOVE_DEVICE] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_REMOVE_DEVICE_SIZE,
  },
  [MGMT_OP_GET_DEVICE_FLAGS] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_GET_DEVICE_FLAGS_SIZE,
  },
  [MGMT_OP_SET_DEVICE_FLAGS] =
  {
    .func = linux_bt_hci_mgmt_default_handler,
    .data_len = MGMT_SET_DEVICE_FLAGS_SIZE,
  },
};

static struct hci_mgmt_chan g_linux_bt_mgmt_chan =
{
  .channel = HCI_CHANNEL_CONTROL,
  .handler_count = sizeof(g_linux_bt_mgmt_handlers) /
                   sizeof(g_linux_bt_mgmt_handlers[0]),
  .handlers = g_linux_bt_mgmt_handlers,
};

static int linux_bt_hci_monitor_add(struct sock *sk)
{
  unsigned int i;

  for (i = 0; i < sizeof(g_hci_monitor_socks) /
                  sizeof(g_hci_monitor_socks[0]); i++)
    {
      if (g_hci_monitor_socks[i] == sk)
        {
          return 0;
        }
    }

  for (i = 0; i < sizeof(g_hci_monitor_socks) /
                  sizeof(g_hci_monitor_socks[0]); i++)
    {
      if (g_hci_monitor_socks[i] == NULL)
        {
          sock_hold(sk);
          g_hci_monitor_socks[i] = sk;
          g_hci_monitor_registers++;
          return 0;
        }
    }

  return -ENOMEM;
}

static void linux_bt_hci_monitor_del(struct sock *sk)
{
  unsigned int i;

  for (i = 0; i < sizeof(g_hci_monitor_socks) /
                  sizeof(g_hci_monitor_socks[0]); i++)
    {
      if (g_hci_monitor_socks[i] == sk)
        {
          g_hci_monitor_socks[i] = NULL;
          g_hci_monitor_unregisters++;
          sock_put(sk);
          return;
        }
    }
}

static int linux_bt_hci_monitor_broadcast(uint16_t event, uint16_t index,
                                          const void *data,
                                          uint16_t data_len,
                                          struct sock *skip)
{
  struct sk_buff *skb;
  struct hci_mon_hdr *hdr;
  unsigned int i;
  unsigned int sent = 0;

  for (i = 0; i < sizeof(g_hci_monitor_socks) /
                  sizeof(g_hci_monitor_socks[0]); i++)
    {
      if (g_hci_monitor_socks[i] == NULL ||
          g_hci_monitor_socks[i] == skip)
        {
          continue;
        }

      skb = alloc_skb(sizeof(*hdr) + data_len, GFP_KERNEL);
      if (skb == NULL)
        {
          continue;
        }

      hdr = skb_put(skb, sizeof(*hdr));
      if (hdr == NULL)
        {
          kfree_skb(skb);
          continue;
        }

      hdr->opcode = cpu_to_le16(event);
      hdr->index = cpu_to_le16(index);
      hdr->len = cpu_to_le16(data_len);
      if (data_len > 0)
        {
          skb_put_data(skb, data, data_len);
        }

      skb_queue_tail(&g_hci_monitor_socks[i]->sk_receive_queue, skb);
      if (g_hci_monitor_socks[i]->sk_data_ready != NULL)
        {
          g_hci_monitor_socks[i]->sk_data_ready(g_hci_monitor_socks[i]);
        }

      sent++;
    }

  g_hci_monitor_events += sent;
  return (int)sent;
}

static int linux_bt_hci_logging_frame(struct sock *sk, struct sk_buff *skb)
{
  struct hci_mon_hdr *hdr;
  struct hci_dev *hdev;
  uint16_t index;
  uint16_t data_len;

  if (skb == NULL || skb->len < sizeof(*hdr) + 3)
    {
      return -EINVAL;
    }

  hdr = (struct hci_mon_hdr *)skb->data;
  data_len = le16_to_cpu(hdr->len);
  if (le16_to_cpu(hdr->opcode) != 0x0000 ||
      data_len != skb->len - sizeof(*hdr) ||
      skb->data[sizeof(*hdr)] > 7 ||
      skb->data[skb->len - 1] != 0x00)
    {
      return -EINVAL;
    }

  index = le16_to_cpu(hdr->index);
  if (index != MGMT_INDEX_NONE)
    {
      hdev = hci_dev_get(index);
      if (hdev == NULL)
        {
          return -ENODEV;
        }

      hci_dev_put(hdev);
    }

  linux_bt_hci_monitor_broadcast(HCI_MON_USER_LOGGING, index,
                                 skb->data + sizeof(*hdr), data_len, sk);
  return (int)skb->len;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int sock_register(const struct net_proto_family *ops)
{
  if (ops == NULL || ops->family < 0 ||
      ops->family >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      return -EINVAL;
    }

  if (g_sock_family[ops->family] != NULL)
    {
      return -EEXIST;
    }

  g_sock_family[ops->family] = ops;
  g_sock_registers++;
  return 0;
}

void sock_unregister(int family)
{
  if (family >= 0 &&
      family < (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      if (g_sock_family[family] != NULL)
        {
          g_sock_unregisters++;
        }

      g_sock_family[family] = NULL;
    }
}

int proto_register(struct proto *prot, int alloc_slab)
{
  (void)prot;
  (void)alloc_slab;
  g_proto_registers++;
  return 0;
}

void proto_unregister(struct proto *prot)
{
  (void)prot;
  g_proto_unregisters++;
}

int bt_sysfs_init(void)
{
  return 0;
}

void bt_sysfs_cleanup(void)
{
}

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_SOCK
#ifdef CONFIG_NET_LINUX_BLUETOOTH_HCI_SOCK_LEGACY_DIAGNOSTIC
#define linux_bt_hci_pi(sk) ((struct linux_bt_hci_pinfo *)(sk))

struct linux_bt_hci_pinfo
{
  struct bt_sock bt;
  struct hci_dev *hdev;
  struct hci_filter filter;
  uint8_t cmsg_mask;
  unsigned short channel;
  unsigned long flags;
};

static struct proto g_linux_bt_hci_sock_proto =
{
  .name = "HCI",
  .obj_size = sizeof(struct linux_bt_hci_pinfo),
};

static int linux_bt_hci_data_sock_add(struct sock *sk)
{
  unsigned int i;

  for (i = 0; i < sizeof(g_hci_data_socks) /
                  sizeof(g_hci_data_socks[0]); i++)
    {
      if (g_hci_data_socks[i] == sk)
        {
          return 0;
        }
    }

  for (i = 0; i < sizeof(g_hci_data_socks) /
                  sizeof(g_hci_data_socks[0]); i++)
    {
      if (g_hci_data_socks[i] == NULL)
        {
          sock_hold(sk);
          g_hci_data_socks[i] = sk;
          g_hci_data_socket_registers++;
          return 0;
        }
    }

  return -ENOMEM;
}

static void linux_bt_hci_data_sock_del(struct sock *sk)
{
  unsigned int i;

  for (i = 0; i < sizeof(g_hci_data_socks) /
                  sizeof(g_hci_data_socks[0]); i++)
    {
      if (g_hci_data_socks[i] == sk)
        {
          g_hci_data_socks[i] = NULL;
          g_hci_data_socket_unregisters++;
          sock_put(sk);
          return;
        }
    }
}

static int linux_bt_hci_control_sock_add(struct sock *sk)
{
  unsigned int i;

  for (i = 0; i < sizeof(g_hci_control_socks) /
                  sizeof(g_hci_control_socks[0]); i++)
    {
      if (g_hci_control_socks[i] == sk)
        {
          return 0;
        }
    }

  for (i = 0; i < sizeof(g_hci_control_socks) /
                  sizeof(g_hci_control_socks[0]); i++)
    {
      if (g_hci_control_socks[i] == NULL)
        {
          sock_hold(sk);
          g_hci_control_socks[i] = sk;
          return 0;
        }
    }

  return -ENOMEM;
}

static void linux_bt_hci_control_sock_del(struct sock *sk)
{
  unsigned int i;

  for (i = 0; i < sizeof(g_hci_control_socks) /
                  sizeof(g_hci_control_socks[0]); i++)
    {
      if (g_hci_control_socks[i] == sk)
        {
          g_hci_control_socks[i] = NULL;
          sock_put(sk);
          return;
        }
    }
}

static bool linux_bt_hci_sock_valid_pkt_type(uint8_t pkt_type)
{
  switch (pkt_type)
    {
      case HCI_COMMAND_PKT:
      case HCI_ACLDATA_PKT:
      case HCI_SCODATA_PKT:
      case HCI_ISODATA_PKT:
      case HCI_DRV_PKT:
      case HCI_VENDOR_PKT:
        return true;

      default:
        return false;
    }
}

static uint16_t linux_bt_hci_get_le16(const uint8_t *data)
{
  return (uint16_t)(data[0] | (data[1] << 8));
}

static bool linux_bt_hci_filter_test32(int nr, const void *addr)
{
  const uint32_t *mask = (const uint32_t *)addr;

  return (mask[nr >> 5] & ((uint32_t)1 << (nr & 31))) != 0;
}

static void linux_bt_hci_filter_init(struct hci_filter *filter)
{
  if (filter == NULL)
    {
      return;
    }

  filter->type_mask = ~0UL;
  filter->event_mask[0] = ~0UL;
  filter->event_mask[1] = ~0UL;
  filter->opcode = 0;
}

static bool linux_bt_hci_sock_filtered(struct linux_bt_hci_pinfo *pi,
                                       uint8_t pkt_type,
                                       const uint8_t *payload,
                                       size_t payload_len)
{
  struct hci_filter *filter;
  int flt_type;
  int flt_event;
  uint16_t opcode;

  if (pi == NULL)
    {
      return true;
    }

  filter = &pi->filter;
  flt_type = pkt_type & HCI_FLT_TYPE_BITS;
  if ((filter->type_mask & (1UL << flt_type)) == 0)
    {
      return true;
    }

  if (pkt_type != HCI_EVENT_PKT)
    {
      return false;
    }

  if (payload == NULL || payload_len < 1)
    {
      return true;
    }

  flt_event = payload[0] & HCI_FLT_EVENT_BITS;
  if (!linux_bt_hci_filter_test32(flt_event, filter->event_mask))
    {
      return true;
    }

  if (filter->opcode == 0)
    {
      return false;
    }

  if (flt_event == HCI_EV_CMD_COMPLETE)
    {
      if (payload_len < 5)
        {
          return true;
        }

      opcode = linux_bt_hci_get_le16(&payload[3]);
      return filter->opcode != cpu_to_le16(opcode);
    }

  if (flt_event == HCI_EV_CMD_STATUS)
    {
      if (payload_len < 6)
        {
          return true;
        }

      opcode = linux_bt_hci_get_le16(&payload[4]);
      return filter->opcode != cpu_to_le16(opcode);
    }

  return false;
}

static int linux_bt_hci_sock_send_to_hdev(struct linux_bt_hci_pinfo *pi,
                                          struct sk_buff *skb)
{
  uint8_t pkt_type;
  size_t payload_len;
  int ret;

  if (pi == NULL || pi->hdev == NULL || skb == NULL || skb->len < 1)
    {
      kfree_skb(skb);
      return -ENODEV;
    }

  if (pi->hdev->send == NULL)
    {
      kfree_skb(skb);
      return -ENOSYS;
    }

  if (!test_bit(HCI_UP, &pi->hdev->flags))
    {
      kfree_skb(skb);
      return -ENETDOWN;
    }

  pkt_type = skb->data[0];
  if (!linux_bt_hci_sock_valid_pkt_type(pkt_type))
    {
      kfree_skb(skb);
      return -EINVAL;
    }

  skb_pull(skb, 1);
  payload_len = skb->len;
  hci_skb_pkt_type(skb) = pkt_type;

  pi->hdev->stat.byte_tx += payload_len;
  switch (pkt_type)
    {
      case HCI_COMMAND_PKT:
        pi->hdev->stat.cmd_tx++;
        break;

      case HCI_ACLDATA_PKT:
        pi->hdev->stat.acl_tx++;
        break;

      case HCI_SCODATA_PKT:
        pi->hdev->stat.sco_tx++;
        break;

      case HCI_ISODATA_PKT:
        pi->hdev->stat.iso_tx++;
        break;

      default:
        break;
    }

  ret = pi->hdev->send(pi->hdev, skb);
  if (ret < 0)
    {
      kfree_skb(skb);
    }

  return ret < 0 ? ret : (int)(payload_len + 1);
}

static int linux_bt_hci_sock_release(struct socket *sock)
{
  struct linux_bt_hci_pinfo *pi;

  if (sock == NULL)
    {
      return 0;
    }

  if (sock->sk != NULL)
    {
      pi = linux_bt_hci_pi(sock->sk);
      if (pi->channel == HCI_CHANNEL_CONTROL)
        {
          linux_bt_hci_control_sock_del(sock->sk);
          linux_bt_hci_monitor_broadcast(HCI_MON_CTRL_CLOSE,
                                         MGMT_INDEX_NONE, NULL, 0,
                                         sock->sk);
        }
      else if (pi->channel == HCI_CHANNEL_MONITOR)
        {
          linux_bt_hci_monitor_del(sock->sk);
        }
      else if (pi->channel == HCI_CHANNEL_RAW ||
               pi->channel == HCI_CHANNEL_USER)
        {
          linux_bt_hci_data_sock_del(sock->sk);
          if (pi->channel == HCI_CHANNEL_USER && pi->hdev != NULL)
            {
              hci_dev_clear_flag(pi->hdev, HCI_USER_CHANNEL);
            }
        }

      if (pi->hdev != NULL)
        {
          atomic_dec(&pi->hdev->promisc);
          hci_dev_put(pi->hdev);
          pi->hdev = NULL;
        }

      sock_orphan(sock->sk);
      sock_put(sock->sk);
      sock->sk = NULL;
    }

  return 0;
}

static int linux_bt_hci_sock_getname(struct socket *sock,
                                     struct sockaddr *addr,
                                     int peer)
{
  struct linux_bt_hci_pinfo *pi;
  struct sockaddr_hci *haddr;

  if (sock == NULL || sock->sk == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  if (peer)
    {
      return -EOPNOTSUPP;
    }

  pi = linux_bt_hci_pi(sock->sk);
  if (pi->hdev == NULL)
    {
      return -EBADFD;
    }

  haddr = (struct sockaddr_hci *)addr;
  haddr->hci_family = AF_BLUETOOTH;
  haddr->hci_dev = pi->hdev->id;
  haddr->hci_channel = pi->channel;
  g_hci_getname_calls++;
  return sizeof(*haddr);
}

#define LINUX_BT_HCI_IOCTL_MAX_DEV 32
#define LINUX_BT_HCI_IOCTL_MAX_CONN 16

static void linux_bt_hci_sock_fill_dev_info(struct hci_dev *hdev,
                                            struct hci_dev_info *info)
{
  if (hdev == NULL || info == NULL)
    {
      return;
    }

  memset(info, 0, sizeof(*info));
  info->dev_id = hdev->id;
  snprintf(info->name, sizeof(info->name), "hci%u", hdev->id);
  info->flags = (uint32_t)hdev->flags;
  info->type = hdev->bus;
  info->pkt_type = hdev->pkt_type;
  info->link_policy = hdev->link_policy;
  info->link_mode = hdev->link_mode;
  info->acl_mtu = hdev->acl_mtu;
  info->acl_pkts = hdev->acl_pkts;
  info->sco_mtu = hdev->sco_mtu;
  info->sco_pkts = hdev->sco_pkts;
  info->stat.err_rx = (uint32_t)hdev->stat.err_rx;
  info->stat.err_tx = 0;
  info->stat.cmd_tx = (uint32_t)hdev->stat.cmd_tx;
  info->stat.evt_rx = 0;
  info->stat.acl_tx = (uint32_t)hdev->stat.acl_tx;
  info->stat.acl_rx = (uint32_t)hdev->stat.acl_rx;
  info->stat.sco_tx = (uint32_t)hdev->stat.sco_tx;
  info->stat.sco_rx = (uint32_t)hdev->stat.sco_rx;
  info->stat.byte_rx = (uint32_t)hdev->stat.byte_rx;
  info->stat.byte_tx = (uint32_t)hdev->stat.byte_tx;
}

static int linux_bt_hci_sock_dev_cmd(unsigned int cmd, struct hci_dev_req *dr)
{
  struct hci_dev *hdev;

  if (dr == NULL)
    {
      return -EINVAL;
    }

  hdev = hci_dev_get(dr->dev_id);
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  switch (cmd)
    {
      case HCISETSCAN:
        hdev->scan_enable = (uint8_t)dr->dev_opt;
        if ((hdev->scan_enable & SCAN_PAGE) != 0)
          {
            hci_dev_set_flag(hdev, HCI_CONNECTABLE);
          }
        else
          {
            hci_dev_clear_flag(hdev, HCI_CONNECTABLE);
          }

        if ((hdev->scan_enable & SCAN_INQUIRY) != 0)
          {
            hci_dev_set_flag(hdev, HCI_DISCOVERABLE);
          }
        else
          {
            hci_dev_clear_flag(hdev, HCI_DISCOVERABLE);
          }
        break;

      case HCISETAUTH:
        hdev->auth_enable = (uint8_t)dr->dev_opt;
        break;

      case HCISETENCRYPT:
        hdev->encrypt_mode = (uint8_t)dr->dev_opt;
        if (hdev->encrypt_mode != 0)
          {
            set_bit(HCI_ENCRYPT, &hdev->flags);
          }
        else
          {
            clear_bit(HCI_ENCRYPT, &hdev->flags);
          }
        break;

      case HCISETPTYPE:
        hdev->pkt_type = dr->dev_opt;
        break;

      case HCISETLINKPOL:
        hdev->link_policy = dr->dev_opt;
        break;

      case HCISETLINKMODE:
        hdev->link_mode = (uint16_t)dr->dev_opt &
                          (HCI_LM_MASTER | HCI_LM_ACCEPT);
        break;

      case HCISETACLMTU:
        hdev->acl_pkts = (uint16_t)(dr->dev_opt & 0xffff);
        hdev->acl_mtu = (uint16_t)(dr->dev_opt >> 16);
        break;

      case HCISETSCOMTU:
        hdev->sco_pkts = (uint16_t)(dr->dev_opt & 0xffff);
        hdev->sco_mtu = (uint16_t)(dr->dev_opt >> 16);
        break;

      default:
        hci_dev_put(hdev);
        return -EINVAL;
    }

  hci_dev_put(hdev);
  g_hci_ioctl_devcmd_calls++;
  return 0;
}

static uint32_t linux_bt_hci_conn_link_mode(const struct hci_conn *conn)
{
  uint32_t link_mode = 0;

  if (conn == NULL)
    {
      return 0;
    }

  if (conn->role == HCI_ROLE_MASTER)
    {
      link_mode |= HCI_LM_MASTER;
    }

  if (test_bit(HCI_CONN_ENCRYPT, &conn->flags))
    {
      link_mode |= HCI_LM_ENCRYPT;
    }

  if (test_bit(HCI_CONN_AUTH, &conn->flags))
    {
      link_mode |= HCI_LM_AUTH;
    }

  if (test_bit(HCI_CONN_SECURE, &conn->flags))
    {
      link_mode |= HCI_LM_SECURE;
    }

  if (test_bit(HCI_CONN_FIPS, &conn->flags))
    {
      link_mode |= HCI_LM_FIPS;
    }

  return link_mode;
}

static void linux_bt_hci_conn_fill_info(struct hci_conn_info *info,
                                        const struct hci_conn *conn)
{
  memset(info, 0, sizeof(*info));
  bacpy(&info->bdaddr, &conn->dst);
  info->handle = conn->handle;
  info->type = conn->type;
  info->out = conn->out;
  info->state = conn->state;
  info->link_mode = linux_bt_hci_conn_link_mode(conn);
}

static int linux_bt_hci_sock_get_conn_list(unsigned long arg)
{
  struct hci_conn_list_req *list =
    (struct hci_conn_list_req *)(uintptr_t)arg;
  struct hci_dev *hdev;
  struct hci_conn *conn;
  uint16_t max;
  uint16_t count = 0;

  if (arg == 0)
    {
      return -EINVAL;
    }

  max = list->conn_num;
  if (max == 0 || max > LINUX_BT_HCI_IOCTL_MAX_CONN)
    {
      return -EINVAL;
    }

  hdev = hci_dev_get(list->dev_id);
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  rcu_read_lock();
  list_for_each_entry_rcu(conn, &hdev->conn_hash.list, list)
    {
      if (count >= max)
        {
          break;
        }

      linux_bt_hci_conn_fill_info(&list->conn_info[count], conn);
      count++;
    }
  rcu_read_unlock();

  list->dev_id = hdev->id;
  list->conn_num = count;
  hci_dev_put(hdev);
  g_hci_ioctl_connlist_calls++;
  return 0;
}

static int linux_bt_hci_sock_get_conn_info(struct hci_dev *hdev,
                                           unsigned long arg)
{
  struct hci_conn_info_req *req =
    (struct hci_conn_info_req *)(uintptr_t)arg;
  struct hci_conn *conn;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (arg == 0)
    {
      return -EINVAL;
    }

  conn = hci_conn_hash_lookup_ba(hdev, req->type, &req->bdaddr);
  if (conn == NULL)
    {
      return -ENOENT;
    }

  linux_bt_hci_conn_fill_info(&req->conn_info[0], conn);
  g_hci_ioctl_conninfo_calls++;
  return 0;
}

static struct hci_conn *
linux_bt_hci_mgmt_conn_lookup(struct hci_dev *hdev,
                              const struct mgmt_addr_info *addr)
{
  uint8_t addr_type;

  if (hdev == NULL || addr == NULL)
    {
      return NULL;
    }

  if (addr->type == BDADDR_BREDR)
    {
      return hci_conn_hash_lookup_ba(hdev, ACL_LINK, &addr->bdaddr);
    }

  addr_type = addr->type == BDADDR_LE_RANDOM ? ADDR_LE_DEV_RANDOM :
              ADDR_LE_DEV_PUBLIC;
  return hci_conn_hash_lookup_le(hdev, &addr->bdaddr, addr_type);
}

static int linux_bt_hci_sock_get_auth_info(struct hci_dev *hdev,
                                           unsigned long arg)
{
  struct hci_auth_info_req *req =
    (struct hci_auth_info_req *)(uintptr_t)arg;
  struct hci_conn *conn;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (arg == 0)
    {
      return -EINVAL;
    }

  conn = hci_conn_hash_lookup_ba(hdev, ACL_LINK, &req->bdaddr);
  if (conn == NULL)
    {
      return -ENOENT;
    }

  req->type = conn->auth_type;
  g_hci_ioctl_authinfo_calls++;
  return 0;
}

static bool linux_bt_hci_sock_addr_is_any(const bdaddr_t *bdaddr)
{
  bdaddr_t any;

  if (bdaddr == NULL)
    {
      return false;
    }

  bacpy(&any, BDADDR_ANY);
  return memcmp(bdaddr, &any, sizeof(any)) == 0;
}

static bool linux_bt_hci_mgmt_addr_type_valid(uint8_t bdaddr_type)
{
  return bdaddr_type == BDADDR_BREDR ||
         bdaddr_type == BDADDR_LE_PUBLIC ||
         bdaddr_type == BDADDR_LE_RANDOM;
}

static int linux_bt_hci_reject_addr_update(struct hci_dev *hdev,
                                           const bdaddr_t *bdaddr,
                                           uint8_t bdaddr_type,
                                           bool add,
                                           struct sock *skip)
{
  unsigned int i;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (bdaddr == NULL)
    {
      return -EINVAL;
    }

  if (!linux_bt_hci_mgmt_addr_type_valid(bdaddr_type))
    {
      return -EINVAL;
    }

  if (linux_bt_hci_sock_addr_is_any(bdaddr))
    {
      return -EBADF;
    }

  for (i = 0; i < sizeof(g_hci_reject_list) /
                  sizeof(g_hci_reject_list[0]); i++)
    {
      if (!g_hci_reject_list[i].used ||
          g_hci_reject_list[i].dev_id != hdev->id ||
          g_hci_reject_list[i].bdaddr_type != bdaddr_type ||
          memcmp(&g_hci_reject_list[i].bdaddr, bdaddr,
                 sizeof(*bdaddr)) != 0)
        {
          continue;
        }

      if (add)
        {
          return -EEXIST;
        }

      memset(&g_hci_reject_list[i], 0, sizeof(g_hci_reject_list[i]));
      g_hci_ioctl_unblockaddr_calls++;
      linux_bt_hci_mgmt_broadcast_addr_event_skip(
        MGMT_EV_DEVICE_UNBLOCKED, hdev->id, bdaddr, bdaddr_type, skip);
      return 0;
    }

  if (!add)
    {
      return -ENOENT;
    }

  for (i = 0; i < sizeof(g_hci_reject_list) /
                  sizeof(g_hci_reject_list[0]); i++)
    {
      if (!g_hci_reject_list[i].used)
        {
          g_hci_reject_list[i].used = true;
          g_hci_reject_list[i].dev_id = hdev->id;
          g_hci_reject_list[i].bdaddr_type = bdaddr_type;
          bacpy(&g_hci_reject_list[i].bdaddr, bdaddr);
          g_hci_ioctl_blockaddr_calls++;
          linux_bt_hci_mgmt_broadcast_addr_event_skip(
            MGMT_EV_DEVICE_BLOCKED, hdev->id, bdaddr, bdaddr_type, skip);
          return 0;
        }
    }

  return -ENOMEM;
}

static int linux_bt_hci_sock_reject_addr(struct hci_dev *hdev,
                                         unsigned long arg,
                                         bool add)
{
  if (arg == 0)
    {
      return -EINVAL;
    }

  return linux_bt_hci_reject_addr_update(
    hdev, (const bdaddr_t *)(uintptr_t)arg, BDADDR_BREDR, add, NULL);
}

static int linux_bt_hci_device_list_update(struct hci_dev *hdev,
                                           const struct mgmt_addr_info *addr,
                                           uint8_t action,
                                           bool add,
                                           struct sock *skip)
{
  unsigned int i;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (addr == NULL || !linux_bt_hci_mgmt_addr_type_valid(addr->type) ||
      linux_bt_hci_sock_addr_is_any(&addr->bdaddr))
    {
      return -EINVAL;
    }

  if (add && action > 0x02)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(g_hci_device_list) /
                  sizeof(g_hci_device_list[0]); i++)
    {
      if (!g_hci_device_list[i].used ||
          g_hci_device_list[i].dev_id != hdev->id ||
          g_hci_device_list[i].bdaddr_type != addr->type ||
          memcmp(&g_hci_device_list[i].bdaddr, &addr->bdaddr,
                 sizeof(addr->bdaddr)) != 0)
        {
          continue;
        }

      if (!add)
        {
          linux_bt_hci_mgmt_pending_complete_pair(
            hdev, addr, MGMT_STATUS_FAILED);
          memset(&g_hci_device_list[i], 0, sizeof(g_hci_device_list[i]));
          linux_bt_hci_mgmt_broadcast_addr_event_skip(
            MGMT_EV_DEVICE_REMOVED, hdev->id, &addr->bdaddr,
            addr->type, skip);
          return 0;
        }

      g_hci_device_list[i].action = action;
      linux_bt_hci_mgmt_broadcast_device_added(hdev->id, &addr->bdaddr,
                                               addr->type, action, skip);
      return 0;
    }

  if (!add)
    {
      return -ENOENT;
    }

  for (i = 0; i < sizeof(g_hci_device_list) /
                  sizeof(g_hci_device_list[0]); i++)
    {
      if (!g_hci_device_list[i].used)
        {
          g_hci_device_list[i].used = true;
          g_hci_device_list[i].dev_id = hdev->id;
          g_hci_device_list[i].bdaddr_type = addr->type;
          g_hci_device_list[i].action = action;
          g_hci_device_list[i].io_cap = 0;
          g_hci_device_list[i].current_flags = 0;
          g_hci_device_list[i].confirm_value = 0;
          g_hci_device_list[i].passkey = 0;
          g_hci_device_list[i].pairing = false;
          g_hci_device_list[i].paired = false;
          g_hci_device_list[i].confirm_pending = false;
          g_hci_device_list[i].passkey_pending = false;
          bacpy(&g_hci_device_list[i].bdaddr, &addr->bdaddr);
          linux_bt_hci_mgmt_broadcast_device_added(hdev->id,
                                                   &addr->bdaddr,
                                                   addr->type, action,
                                                   skip);
          return 0;
        }
    }

  return -ENOMEM;
}

static struct linux_bt_hci_device_state *
linux_bt_hci_device_list_find(struct hci_dev *hdev,
                              const struct mgmt_addr_info *addr)
{
  unsigned int i;

  if (hdev == NULL || addr == NULL ||
      !linux_bt_hci_mgmt_addr_type_valid(addr->type) ||
      linux_bt_hci_sock_addr_is_any(&addr->bdaddr))
    {
      return NULL;
    }

  for (i = 0; i < sizeof(g_hci_device_list) /
                  sizeof(g_hci_device_list[0]); i++)
    {
      if (g_hci_device_list[i].used &&
          g_hci_device_list[i].dev_id == hdev->id &&
          g_hci_device_list[i].bdaddr_type == addr->type &&
          memcmp(&g_hci_device_list[i].bdaddr, &addr->bdaddr,
                 sizeof(addr->bdaddr)) == 0)
        {
          return &g_hci_device_list[i];
        }
    }

  return NULL;
}

static struct linux_bt_hci_mgmt_pending *
linux_bt_hci_mgmt_pending_find(uint16_t opcode, struct hci_dev *hdev,
                               const struct mgmt_addr_info *addr)
{
  unsigned int i;

  if (hdev == NULL || addr == NULL)
    {
      return NULL;
    }

  for (i = 0; i < sizeof(g_hci_mgmt_pending) /
                  sizeof(g_hci_mgmt_pending[0]); i++)
    {
      if (g_hci_mgmt_pending[i].used &&
          g_hci_mgmt_pending[i].opcode == opcode &&
          g_hci_mgmt_pending[i].index == hdev->id &&
          g_hci_mgmt_pending[i].addr.type == addr->type &&
          memcmp(&g_hci_mgmt_pending[i].addr.bdaddr, &addr->bdaddr,
                 sizeof(addr->bdaddr)) == 0)
        {
          return &g_hci_mgmt_pending[i];
        }
    }

  return NULL;
}

static struct linux_bt_hci_mgmt_pending *
linux_bt_hci_mgmt_pending_find_addr(uint16_t opcode, struct hci_dev *hdev,
                                    const struct mgmt_addr_info *addr)
{
  unsigned int i;

  if (hdev == NULL || addr == NULL)
    {
      return NULL;
    }

  for (i = 0; i < sizeof(g_hci_mgmt_pending) /
                  sizeof(g_hci_mgmt_pending[0]); i++)
    {
      if (g_hci_mgmt_pending[i].used &&
          g_hci_mgmt_pending[i].opcode == opcode &&
          g_hci_mgmt_pending[i].index == hdev->id &&
          memcmp(&g_hci_mgmt_pending[i].addr.bdaddr, &addr->bdaddr,
                 sizeof(addr->bdaddr)) == 0)
        {
          return &g_hci_mgmt_pending[i];
        }
    }

  return NULL;
}

static struct linux_bt_hci_mgmt_pending *
linux_bt_hci_mgmt_pending_find_any(uint16_t opcode)
{
  unsigned int i;

  for (i = 0; i < sizeof(g_hci_mgmt_pending) /
                  sizeof(g_hci_mgmt_pending[0]); i++)
    {
      if (g_hci_mgmt_pending[i].used &&
          g_hci_mgmt_pending[i].opcode == opcode)
        {
          return &g_hci_mgmt_pending[i];
        }
    }

  return NULL;
}

static void linux_bt_hci_mgmt_pending_clear(
  struct linux_bt_hci_mgmt_pending *pending)
{
  struct sock *sk;

  if (pending == NULL || !pending->used)
    {
      return;
    }

  sk = pending->sk;
  memset(pending, 0, sizeof(*pending));
  if (sk != NULL)
    {
      sock_put(sk);
    }
}

static int linux_bt_hci_mgmt_pending_add(struct sock *sk, uint16_t opcode,
                                         struct hci_dev *hdev,
                                         const struct mgmt_addr_info *addr,
                                         uint8_t io_cap)
{
  unsigned int i;

  if (sk == NULL || hdev == NULL || addr == NULL ||
      !linux_bt_hci_mgmt_addr_type_valid(addr->type) ||
      linux_bt_hci_sock_addr_is_any(&addr->bdaddr))
    {
      return -EINVAL;
    }

  if (linux_bt_hci_mgmt_pending_find(opcode, hdev, addr) != NULL)
    {
      return -EALREADY;
    }

  for (i = 0; i < sizeof(g_hci_mgmt_pending) /
                  sizeof(g_hci_mgmt_pending[0]); i++)
    {
      if (!g_hci_mgmt_pending[i].used)
        {
          g_hci_mgmt_pending[i].used = true;
          g_hci_mgmt_pending[i].sk = sk;
          g_hci_mgmt_pending[i].opcode = opcode;
          g_hci_mgmt_pending[i].index = hdev->id;
          g_hci_mgmt_pending[i].io_cap = io_cap;
          memcpy(&g_hci_mgmt_pending[i].addr, addr,
                 sizeof(g_hci_mgmt_pending[i].addr));
          sock_hold(sk);
          return 0;
        }
    }

  return -ENOMEM;
}

static int linux_bt_hci_mgmt_pending_complete_pair(
  struct hci_dev *hdev, const struct mgmt_addr_info *addr, uint8_t status)
{
  struct linux_bt_hci_mgmt_pending *pending;
  struct mgmt_rp_pair_device rp;
  int ret;

  pending = linux_bt_hci_mgmt_pending_find(MGMT_OP_PAIR_DEVICE, hdev,
                                           addr);
  if (pending == NULL)
    {
      return -ENOENT;
    }

  memset(&rp, 0, sizeof(rp));
  memcpy(&rp.addr, &pending->addr, sizeof(rp.addr));
  ret = linux_bt_hci_sock_queue_cmd_complete_data(
    pending->sk, MGMT_OP_PAIR_DEVICE, pending->index, status, &rp,
    sizeof(rp));
  linux_bt_hci_mgmt_pending_clear(pending);
  return ret;
}

static void linux_bt_hci_mgmt_ltk_fill(struct mgmt_ltk_info *key,
                                       const struct mgmt_addr_info *addr,
                                       bool authenticated)
{
  const uint8_t *bdaddr = (const uint8_t *)&addr->bdaddr;
  unsigned int i;

  memset(key, 0, sizeof(*key));
  memcpy(&key->addr, addr, sizeof(key->addr));
  key->type = authenticated ? MGMT_LTK_AUTHENTICATED :
                              MGMT_LTK_UNAUTHENTICATED;
  key->initiator = 1;
  key->enc_size = 16;
  key->ediv = cpu_to_le16((uint16_t)(bdaddr[0] | (bdaddr[1] << 8)));
  key->rand = cpu_to_le64(((uint64_t)bdaddr[5] << 40) |
                          ((uint64_t)bdaddr[4] << 32) |
                          ((uint64_t)bdaddr[3] << 24) |
                          ((uint64_t)bdaddr[2] << 16) |
                          ((uint64_t)bdaddr[1] << 8) |
                          bdaddr[0]);
  for (i = 0; i < sizeof(key->val); i++)
    {
      key->val[i] = (uint8_t)(0xa5 ^ bdaddr[i % 6] ^ i);
    }
}

static int linux_bt_hci_mgmt_ltk_store(struct hci_dev *hdev,
                                       const struct mgmt_addr_info *addr,
                                       bool authenticated)
{
  struct mgmt_ev_new_long_term_key ev;
  unsigned int i;
  unsigned int slot = sizeof(g_hci_ltk_list) / sizeof(g_hci_ltk_list[0]);
  int sent = 0;

  if (hdev == NULL || addr == NULL ||
      !linux_bt_hci_mgmt_addr_type_valid(addr->type) ||
      linux_bt_hci_sock_addr_is_any(&addr->bdaddr) ||
      addr->type == BDADDR_BREDR)
    {
      return -EINVAL;
    }

  memset(&ev, 0, sizeof(ev));
  ev.store_hint = 1;
  linux_bt_hci_mgmt_ltk_fill(&ev.key, addr, authenticated);

  for (i = 0; i < sizeof(g_hci_ltk_list) /
                  sizeof(g_hci_ltk_list[0]); i++)
    {
      if (g_hci_ltk_list[i].used &&
          g_hci_ltk_list[i].dev_id == hdev->id &&
          g_hci_ltk_list[i].key.addr.type == addr->type &&
          memcmp(&g_hci_ltk_list[i].key.addr.bdaddr, &addr->bdaddr,
                 sizeof(addr->bdaddr)) == 0)
        {
          slot = i;
          break;
        }

      if (!g_hci_ltk_list[i].used && slot >= sizeof(g_hci_ltk_list) /
          sizeof(g_hci_ltk_list[0]))
        {
          slot = i;
        }
    }

  if (slot >= sizeof(g_hci_ltk_list) / sizeof(g_hci_ltk_list[0]))
    {
      return -ENOMEM;
    }

  g_hci_ltk_list[slot].used = true;
  g_hci_ltk_list[slot].dev_id = hdev->id;
  memcpy(&g_hci_ltk_list[slot].key, &ev.key,
         sizeof(g_hci_ltk_list[slot].key));

  for (i = 0; i < sizeof(g_hci_control_socks) /
                  sizeof(g_hci_control_socks[0]); i++)
    {
      if (g_hci_control_socks[i] == NULL ||
          g_hci_control_socks[i]->sk_state != BT_BOUND)
        {
          continue;
        }

      if (linux_bt_hci_sock_queue_mgmt_event(
            g_hci_control_socks[i], MGMT_EV_NEW_LONG_TERM_KEY, hdev->id,
            &ev, sizeof(ev)) == 0)
        {
          sent++;
        }
    }

  return sent;
}

static int linux_bt_hci_mgmt_pair_connected(struct hci_dev *hdev,
                                            const struct mgmt_addr_info *addr,
                                            struct sock *owner)
{
  struct hci_conn *conn;
  int ret;

  (void)owner;

  if (hdev == NULL || addr == NULL ||
      !linux_bt_hci_mgmt_addr_type_valid(addr->type) ||
      linux_bt_hci_sock_addr_is_any(&addr->bdaddr))
    {
      return -EINVAL;
    }

  ret = linux_bt_hci_mgmt_ensure_connected_conn(hdev, addr, true,
                                                addr->type, &conn);
  if (ret < 0)
    {
      return ret;
    }

  return linux_bt_hci_mgmt_broadcast_conn_addr_event(
    hdev->id, &conn->dst, linux_bt_hci_mgmt_addr_type_from_conn(conn),
    true, conn->out);
}

static int linux_bt_hci_device_flags_get(struct hci_dev *hdev,
                                         const struct mgmt_addr_info *addr,
                                         uint32_t *supported_flags,
                                         uint32_t *current_flags)
{
  struct linux_bt_hci_mgmt_pending *pending;
  struct linux_bt_hci_device_state *state;
  int ret;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (addr == NULL || supported_flags == NULL || current_flags == NULL ||
      !linux_bt_hci_mgmt_addr_type_valid(addr->type) ||
      linux_bt_hci_sock_addr_is_any(&addr->bdaddr))
    {
      return -EINVAL;
    }

  state = linux_bt_hci_device_list_find(hdev, addr);
  if (state == NULL)
    {
      return -EINVAL;
    }

  *supported_flags = LINUX_BT_HCI_DEVICE_FLAGS_SUPPORTED;
  *current_flags = state->current_flags;
  return 0;
}

static int linux_bt_hci_device_flags_set(struct hci_dev *hdev,
                                         const struct mgmt_addr_info *addr,
                                         uint32_t current_flags,
                                         struct sock *skip)
{
  struct linux_bt_hci_device_state *state;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (addr == NULL || !linux_bt_hci_mgmt_addr_type_valid(addr->type) ||
      linux_bt_hci_sock_addr_is_any(&addr->bdaddr) ||
      (current_flags & ~LINUX_BT_HCI_DEVICE_FLAGS_SUPPORTED) != 0)
    {
      return -EINVAL;
    }

  state = linux_bt_hci_device_list_find(hdev, addr);
  if (state == NULL)
    {
      return -EINVAL;
    }

  state->current_flags = current_flags;
  linux_bt_hci_mgmt_broadcast_device_flags(hdev->id, &addr->bdaddr,
                                           addr->type, current_flags,
                                           skip);
  return 0;
}

static int linux_bt_hci_pair_device(struct hci_dev *hdev,
                                    const struct mgmt_cp_pair_device *cp,
                                    struct sock *skip)
{
  struct linux_bt_hci_device_state *state;
  int ret;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (cp == NULL || !linux_bt_hci_mgmt_addr_type_valid(cp->addr.type) ||
      linux_bt_hci_sock_addr_is_any(&cp->addr.bdaddr))
    {
      return -EINVAL;
    }

  state = linux_bt_hci_device_list_find(hdev, &cp->addr);
  if (state == NULL)
    {
      ret = linux_bt_hci_device_list_update(hdev, &cp->addr, 0x01,
                                            true, skip);
      if (ret < 0)
        {
          return ret;
        }

      state = linux_bt_hci_device_list_find(hdev, &cp->addr);
    }

  if (state == NULL)
    {
      return -ENOMEM;
    }

  if (state->paired)
    {
      return -EALREADY;
    }

  state->io_cap = cp->io_cap;
  if (cp->io_cap != LINUX_BT_HCI_IO_CAPABILITY_DEFAULT)
    {
      ret = linux_bt_hci_mgmt_pending_add(skip, MGMT_OP_PAIR_DEVICE, hdev,
                                          &cp->addr, cp->io_cap);
      if (ret < 0)
        {
          return ret;
        }

      state->pairing = true;
      state->paired = false;
      if (cp->io_cap == LINUX_BT_HCI_IO_CAPABILITY_KEYBOARD_ONLY)
        {
          state->confirm_pending = false;
          state->confirm_value = 0;
          state->passkey_pending = true;
          state->passkey = LINUX_BT_HCI_PASSKEY_STAGED;
          linux_bt_hci_mgmt_broadcast_addr_event_skip(
            MGMT_EV_USER_PASSKEY_REQUEST, hdev->id, &cp->addr.bdaddr,
            cp->addr.type, NULL);
        }
      else
        {
          state->confirm_pending = true;
          state->confirm_value = LINUX_BT_HCI_PASSKEY_STAGED;
          state->passkey_pending = false;
          state->passkey = 0;
          linux_bt_hci_mgmt_broadcast_user_confirm(hdev->id,
                                                   &cp->addr.bdaddr,
                                                   cp->addr.type,
                                                   state->confirm_value, 0,
                                                   NULL);
        }
      return -EINPROGRESS;
    }

  state->pairing = false;
  state->paired = true;
  state->confirm_pending = false;
  state->confirm_value = 0;
  state->passkey_pending = false;
  state->passkey = 0;
  linux_bt_hci_mgmt_ltk_store(hdev, &cp->addr, false);
  ret = linux_bt_hci_mgmt_pair_connected(hdev, &cp->addr, skip);
  return ret < 0 ? ret : 0;
}

static int linux_bt_hci_cancel_pair_device(struct hci_dev *hdev,
                                           const struct mgmt_addr_info *addr)
{
  struct mgmt_addr_info complete_addr;
  struct linux_bt_hci_mgmt_pending *pending;
  struct linux_bt_hci_device_state *state;
  int ret;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (addr == NULL || !linux_bt_hci_mgmt_addr_type_valid(addr->type) ||
      linux_bt_hci_sock_addr_is_any(&addr->bdaddr))
    {
      return -EINVAL;
    }

  state = linux_bt_hci_device_list_find(hdev, addr);
  pending = linux_bt_hci_mgmt_pending_find(MGMT_OP_PAIR_DEVICE, hdev, addr);
  if (pending == NULL)
    {
      pending = linux_bt_hci_mgmt_pending_find_addr(MGMT_OP_PAIR_DEVICE,
                                                    hdev, addr);
    }

  if (pending == NULL)
    {
      pending = linux_bt_hci_mgmt_pending_find_any(MGMT_OP_PAIR_DEVICE);
    }

  if ((state == NULL || !state->pairing) && pending == NULL)
    {
      return -EINVAL;
    }

  if (state != NULL)
    {
      state->pairing = false;
      state->confirm_pending = false;
      state->confirm_value = 0;
      state->passkey_pending = false;
      state->passkey = 0;
    }

  memset(&complete_addr, 0, sizeof(complete_addr));
  if (pending != NULL)
    {
      memcpy(&complete_addr, &pending->addr, sizeof(complete_addr));
    }
  else
    {
      memcpy(&complete_addr, addr, sizeof(complete_addr));
    }

  ret = linux_bt_hci_mgmt_pending_complete_pair(
    hdev, &complete_addr, LINUX_BT_HCI_MGMT_STATUS_CANCELLED);
  if (ret < 0 && pending != NULL)
    {
      return ret;
    }

  return 0;
}

static int linux_bt_hci_user_confirm_reply(struct hci_dev *hdev,
                                           const struct mgmt_addr_info *addr,
                                           bool confirm)
{
  struct linux_bt_hci_device_state *state;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (addr == NULL || !linux_bt_hci_mgmt_addr_type_valid(addr->type) ||
      linux_bt_hci_sock_addr_is_any(&addr->bdaddr))
    {
      return -EINVAL;
    }

  state = linux_bt_hci_device_list_find(hdev, addr);
  if (state == NULL || !state->pairing || !state->confirm_pending)
    {
      return -EINVAL;
    }

  state->pairing = false;
  state->confirm_pending = false;
  state->confirm_value = 0;
  state->passkey_pending = false;
  state->passkey = 0;
  state->paired = confirm;
  if (!confirm)
    {
      state->io_cap = 0;
    }

  if (confirm)
    {
      linux_bt_hci_mgmt_ltk_store(hdev, addr, true);
      linux_bt_hci_mgmt_pair_connected(hdev, addr,
                                       linux_bt_hci_mgmt_pending_find(
                                         MGMT_OP_PAIR_DEVICE, hdev,
                                         addr) != NULL ?
                                       linux_bt_hci_mgmt_pending_find(
                                         MGMT_OP_PAIR_DEVICE, hdev,
                                         addr)->sk : NULL);
    }

  linux_bt_hci_mgmt_pending_complete_pair(
    hdev, addr, confirm ? MGMT_STATUS_SUCCESS : MGMT_STATUS_FAILED);
  return 0;
}

static int linux_bt_hci_user_passkey_reply(
  struct hci_dev *hdev, const struct mgmt_addr_info *addr, uint32_t passkey,
  bool confirm)
{
  struct linux_bt_hci_device_state *state;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (addr == NULL || !linux_bt_hci_mgmt_addr_type_valid(addr->type) ||
      linux_bt_hci_sock_addr_is_any(&addr->bdaddr) ||
      (confirm && passkey > LINUX_BT_HCI_PASSKEY_MAX))
    {
      return -EINVAL;
    }

  state = linux_bt_hci_device_list_find(hdev, addr);
  if (state == NULL || !state->pairing || !state->passkey_pending)
    {
      return -EINVAL;
    }

  state->pairing = false;
  state->confirm_pending = false;
  state->confirm_value = 0;
  state->passkey_pending = false;
  state->passkey = confirm ? passkey : 0;
  state->paired = confirm;
  if (!confirm)
    {
      state->io_cap = 0;
    }

  if (confirm)
    {
      linux_bt_hci_mgmt_ltk_store(hdev, addr, true);
      linux_bt_hci_mgmt_pair_connected(hdev, addr,
                                       linux_bt_hci_mgmt_pending_find(
                                         MGMT_OP_PAIR_DEVICE, hdev,
                                         addr) != NULL ?
                                       linux_bt_hci_mgmt_pending_find(
                                         MGMT_OP_PAIR_DEVICE, hdev,
                                         addr)->sk : NULL);
    }

  linux_bt_hci_mgmt_pending_complete_pair(
    hdev, addr, confirm ? MGMT_STATUS_SUCCESS : MGMT_STATUS_FAILED);
  return 0;
}

static int linux_bt_hci_unpair_device(struct hci_dev *hdev,
                                      const struct mgmt_cp_unpair_device *cp,
                                      struct sock *skip)
{
  struct linux_bt_hci_device_state *state;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (cp == NULL || !linux_bt_hci_mgmt_addr_type_valid(cp->addr.type) ||
      linux_bt_hci_sock_addr_is_any(&cp->addr.bdaddr))
    {
      return -EINVAL;
    }

  state = linux_bt_hci_device_list_find(hdev, &cp->addr);
  if (state == NULL || !state->paired)
    {
      return -ENOENT;
    }

  state->pairing = false;
  state->paired = false;
  state->confirm_pending = false;
  state->confirm_value = 0;
  state->passkey_pending = false;
  state->passkey = 0;
  state->io_cap = 0;

  if (cp->disconnect != 0)
    {
      struct hci_conn *conn;

      conn = linux_bt_hci_mgmt_conn_lookup(hdev, &cp->addr);
      if (conn != NULL)
        {
          linux_bt_hci_mgmt_broadcast_conn_addr_event(
            hdev->id, &conn->dst,
            linux_bt_hci_mgmt_addr_type_from_conn(conn), false,
            conn->out);
          hci_conn_del(conn);
        }
    }

  linux_bt_hci_mgmt_broadcast_addr_event_skip(
    MGMT_EV_DEVICE_UNPAIRED, hdev->id, &cp->addr.bdaddr, cp->addr.type,
    skip);
  return 0;
}

static int linux_bt_hci_discovery_update(struct hci_dev *hdev,
                                         uint8_t type, bool start,
                                         struct sock *skip)
{
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (type == 0 ||
      (type & ~LINUX_BT_HCI_DISCOVERY_TYPES_SUPPORTED) != 0)
    {
      return -EINVAL;
    }

  if (start)
    {
      char poll[96];

      if (g_hci_discovery_state.discovering &&
          g_hci_discovery_state.dev_id == hdev->id)
        {
          return -EALREADY;
        }

      g_hci_discovery_state.discovering = true;
      g_hci_discovery_state.dev_id = hdev->id;
      g_hci_discovery_state.type = type;
      linux_bt_hci_mgmt_broadcast_discovering(hdev->id, type, true, skip);
      (void)linux_bt_upstream_mgmt_poll_discovery_probe(8, poll,
                                                        sizeof(poll));
      return 0;
    }

  if (!g_hci_discovery_state.discovering ||
      g_hci_discovery_state.dev_id != hdev->id ||
      g_hci_discovery_state.type != type)
    {
      return -EINVAL;
    }

  g_hci_discovery_state.discovering = false;
  g_hci_discovery_state.type = type;
  linux_bt_hci_mgmt_broadcast_discovering(hdev->id, type, false, skip);
  return 0;
}

static int linux_bt_hci_sock_ioctl(struct socket *sock, unsigned int cmd,
                                   unsigned long arg)
{
  struct hci_dev *hdev;
  unsigned int index;

  if (sock == NULL || sock->sk == NULL)
    {
      return -EINVAL;
    }

  if (linux_bt_hci_pi(sock->sk)->channel != HCI_CHANNEL_RAW)
    {
      return -EBADFD;
    }

  switch (cmd)
    {
      case HCIGETDEVLIST:
        {
          struct hci_dev_list_req *list =
            (struct hci_dev_list_req *)(uintptr_t)arg;
          uint16_t max;
          uint16_t count = 0;

          if (arg == 0)
            {
              return -EINVAL;
            }

          max = list->dev_num;
          for (index = 0; index < LINUX_BT_HCI_IOCTL_MAX_DEV &&
                          count < max; index++)
            {
              hdev = hci_dev_get(index);
              if (hdev == NULL)
                {
                  continue;
                }

              list->dev_req[count].dev_id = hdev->id;
              list->dev_req[count].dev_opt = (uint32_t)hdev->flags;
              count++;
              hci_dev_put(hdev);
            }

          list->dev_num = count;
          g_hci_ioctl_devlist_calls++;
          return 0;
        }

      case HCIGETDEVINFO:
        {
          struct hci_dev_info *info =
            (struct hci_dev_info *)(uintptr_t)arg;

          if (arg == 0)
            {
              return -EINVAL;
            }

          hdev = hci_dev_get(info->dev_id);
          if (hdev == NULL)
            {
              return -ENODEV;
            }

          linux_bt_hci_sock_fill_dev_info(hdev, info);
          hci_dev_put(hdev);
          g_hci_ioctl_devinfo_calls++;
          return 0;
        }

      case HCIGETCONNLIST:
        return linux_bt_hci_sock_get_conn_list(arg);

      case HCIGETCONNINFO:
        {
          struct linux_bt_hci_pinfo *pi = linux_bt_hci_pi(sock->sk);

          if (pi->hdev == NULL)
            {
              return -EBADFD;
            }

          return linux_bt_hci_sock_get_conn_info(pi->hdev, arg);
        }

      case HCIGETAUTHINFO:
        {
          struct linux_bt_hci_pinfo *pi = linux_bt_hci_pi(sock->sk);

          if (pi->hdev == NULL)
            {
              return -EBADFD;
            }

          return linux_bt_hci_sock_get_auth_info(pi->hdev, arg);
        }

      case HCIBLOCKADDR:
      case HCIUNBLOCKADDR:
        {
          struct linux_bt_hci_pinfo *pi = linux_bt_hci_pi(sock->sk);

          if (pi->hdev == NULL)
            {
              return -EBADFD;
            }

          return linux_bt_hci_sock_reject_addr(pi->hdev, arg,
                                               cmd == HCIBLOCKADDR);
        }

      case HCIDEVUP:
        {
          int ret = hci_dev_open((uint16_t)arg);

          if (ret >= 0)
            {
              g_hci_ioctl_devup_calls++;
            }

          return ret;
        }

      case HCIDEVDOWN:
        {
          hdev = hci_dev_get((int)arg);
          if (hdev == NULL)
            {
              return -ENODEV;
            }

          g_hci_ioctl_devdown_calls++;
          hci_dev_do_close(hdev);
          hci_dev_put(hdev);
          return 0;
        }

      case HCIDEVRESET:
        {
          hdev = hci_dev_get((int)arg);
          if (hdev == NULL)
            {
              return -ENODEV;
            }

          hci_dev_do_close(hdev);
          hci_dev_put(hdev);
          g_hci_ioctl_devreset_calls++;
          return hci_dev_open((uint16_t)arg);
        }

      case HCIDEVRESTAT:
        {
          hdev = hci_dev_get((int)arg);
          if (hdev == NULL)
            {
              return -ENODEV;
            }

          memset(&hdev->stat, 0, sizeof(hdev->stat));
          hci_dev_put(hdev);
          g_hci_ioctl_devrestat_calls++;
          return 0;
        }

      case HCISETSCAN:
      case HCISETAUTH:
      case HCISETENCRYPT:
      case HCISETPTYPE:
      case HCISETLINKPOL:
      case HCISETLINKMODE:
      case HCISETACLMTU:
      case HCISETSCOMTU:
        if (arg == 0)
          {
            return -EINVAL;
          }

        return linux_bt_hci_sock_dev_cmd(cmd,
                                         (struct hci_dev_req *)(uintptr_t)arg);

      default:
        return -ENOIOCTLCMD;
    }
}

static int linux_bt_hci_sock_bind(struct socket *sock, struct sockaddr *addr,
                                  int addr_len)
{
  struct linux_bt_hci_pinfo *pi;
  struct sockaddr_hci *haddr;
  struct hci_dev *hdev = NULL;
  int ret;

  if (sock == NULL || sock->sk == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  if (addr_len < (int)sizeof(*haddr))
    {
      return -EINVAL;
    }

  pi = linux_bt_hci_pi(sock->sk);
  haddr = (struct sockaddr_hci *)addr;

  if (sock->sk->sk_state == BT_BOUND ||
      sock->sk->sk_state == BT_CONNECTED)
    {
      return -EALREADY;
    }

  switch (haddr->hci_channel)
    {
      case HCI_CHANNEL_RAW:
        if (haddr->hci_dev != HCI_DEV_NONE)
          {
            hdev = hci_dev_get(haddr->hci_dev);
            if (hdev == NULL)
              {
                return -ENODEV;
              }

            atomic_inc(&hdev->promisc);
          }

        ret = linux_bt_hci_data_sock_add(sock->sk);
        if (ret < 0)
          {
            if (hdev != NULL)
              {
                atomic_dec(&hdev->promisc);
                hci_dev_put(hdev);
              }

            return ret;
          }
        break;

      case HCI_CHANNEL_USER:
        if (haddr->hci_dev == HCI_DEV_NONE)
          {
            return -EINVAL;
          }

        hdev = hci_dev_get(haddr->hci_dev);
        if (hdev == NULL)
          {
            return -ENODEV;
          }

        if (hci_dev_test_and_set_flag(hdev, HCI_USER_CHANNEL))
          {
            hci_dev_put(hdev);
            return -EBUSY;
          }

        atomic_inc(&hdev->promisc);
        ret = linux_bt_hci_data_sock_add(sock->sk);
        if (ret < 0)
          {
            atomic_dec(&hdev->promisc);
            hci_dev_clear_flag(hdev, HCI_USER_CHANNEL);
            hci_dev_put(hdev);
            return ret;
          }
        break;

      case HCI_CHANNEL_MONITOR:
        if (haddr->hci_dev != HCI_DEV_NONE)
          {
            return -EINVAL;
          }

        ret = linux_bt_hci_monitor_add(sock->sk);
        if (ret < 0)
          {
            return ret;
          }
        break;

      case HCI_CHANNEL_LOGGING:
        if (haddr->hci_dev != HCI_DEV_NONE)
          {
            return -EINVAL;
          }
        break;

      case HCI_CHANNEL_CONTROL:
        if (haddr->hci_dev != HCI_DEV_NONE)
          {
            return -EINVAL;
          }

        if (g_hci_mgmt_chan[HCI_CHANNEL_CONTROL] == NULL)
          {
            return -EINVAL;
          }

        ret = linux_bt_hci_control_sock_add(sock->sk);
        if (ret < 0)
          {
            return ret;
          }
        break;

      default:
        return -EINVAL;
    }

  pi->hdev = hdev;
  pi->channel = haddr->hci_channel;
  sock->sk->sk_state = BT_BOUND;

  if (pi->channel == HCI_CHANNEL_CONTROL)
    {
      linux_bt_hci_monitor_broadcast(HCI_MON_CTRL_OPEN, MGMT_INDEX_NONE,
                                     NULL, 0, sock->sk);
    }

  return 0;
}

static uint8_t linux_bt_hci_sock_mgmt_status(int ret)
{
  if (ret >= 0)
    {
      return MGMT_STATUS_SUCCESS;
    }

  switch (-ret)
    {
      case EINVAL:
        return MGMT_STATUS_INVALID_PARAMS;

      case ENODEV:
        return MGMT_STATUS_INVALID_INDEX;

      case ENOMEM:
        return MGMT_STATUS_NO_RESOURCES;

      case EOPNOTSUPP:
        return MGMT_STATUS_UNKNOWN_COMMAND;

      default:
        return MGMT_STATUS_FAILED;
    }
}

static int linux_bt_hci_sock_queue_mgmt_event(struct sock *sk,
                                              uint16_t event,
                                              uint16_t index,
                                              const void *data,
                                              uint16_t data_len)
{
  struct sk_buff *skb;
  struct mgmt_hdr *hdr;

  skb = alloc_skb(sizeof(*hdr) + data_len, GFP_KERNEL);
  if (skb == NULL)
    {
      return -ENOMEM;
    }

  hdr = skb_put(skb, sizeof(*hdr));
  if (hdr == NULL)
    {
      kfree_skb(skb);
      return -ENOMEM;
    }

  hdr->opcode = cpu_to_le16(event);
  hdr->index = cpu_to_le16(index);
  hdr->len = cpu_to_le16(data_len);

  if (data_len > 0)
    {
      skb_put_data(skb, data, data_len);
    }

  linux_bt_hci_monitor_broadcast(HCI_MON_CTRL_EVENT, index, skb->data,
                                 skb->len, NULL);
  skb_queue_tail(&sk->sk_receive_queue, skb);
  g_hci_mgmt_socket_events++;
  if (sk->sk_data_ready != NULL)
    {
      sk->sk_data_ready(sk);
    }

  return 0;
}

static uint8_t linux_bt_hci_mgmt_addr_type_from_conn(
  const struct hci_conn *conn)
{
  if (conn == NULL || conn->type == ACL_LINK)
    {
      return BDADDR_BREDR;
    }

  return conn->dst_type == ADDR_LE_DEV_RANDOM ? BDADDR_LE_RANDOM :
                                                 BDADDR_LE_PUBLIC;
}

static int linux_bt_hci_mgmt_ensure_connected_conn(
  struct hci_dev *hdev, const struct mgmt_addr_info *addr, bool outgoing,
  uint8_t auth_type, struct hci_conn **out)
{
  struct hci_conn *conn;
  uint16_t handle;
  uint8_t link_type;
  uint8_t le_addr_type;

  if (out != NULL)
    {
      *out = NULL;
    }

  if (hdev == NULL || addr == NULL ||
      !linux_bt_hci_mgmt_addr_type_valid(addr->type) ||
      linux_bt_hci_sock_addr_is_any(&addr->bdaddr))
    {
      return -EINVAL;
    }

  link_type = addr->type == BDADDR_BREDR ? ACL_LINK : LE_LINK;
  conn = linux_bt_hci_mgmt_conn_lookup(hdev, addr);
  if (conn == NULL)
    {
      handle = link_type == ACL_LINK ? 0x0040 : 0x0100;
      conn = hci_conn_add(hdev, link_type, (bdaddr_t *)&addr->bdaddr,
                          HCI_ROLE_MASTER, handle);
      if (IS_ERR(conn))
        {
          return PTR_ERR(conn);
        }
    }

  if (link_type == ACL_LINK)
    {
      if (hdev->acl_mtu == 0)
        {
          hdev->acl_mtu = HCI_MAX_FRAME_SIZE;
        }

      conn->mtu = hdev->acl_mtu;
    }
  else
    {
      le_addr_type = addr->type == BDADDR_LE_RANDOM ? ADDR_LE_DEV_RANDOM :
                     ADDR_LE_DEV_PUBLIC;
      conn->dst_type = le_addr_type;
      if (hdev->le_mtu == 0)
        {
          hdev->le_mtu = 251;
        }

      if (hdev->le_pkts == 0)
        {
          hdev->le_pkts = 10;
        }

      if (hdev->le_cnt == 0)
        {
          hdev->le_cnt = hdev->le_pkts;
        }

      conn->mtu = hdev->le_mtu;
    }

  conn->state = BT_CONNECTED;
  conn->out = outgoing;
  conn->auth_type = auth_type;
  conn->sec_level = BT_SECURITY_LOW;
  conn->pending_sec_level = BT_SECURITY_LOW;

  if (out != NULL)
    {
      *out = conn;
    }

  return 0;
}

static int linux_bt_hci_mgmt_broadcast_conn_addr_event(
  uint16_t dev_id, const bdaddr_t *bdaddr, uint8_t bdaddr_type,
  bool connected, bool outgoing)
{
  unsigned int i;
  int sent = 0;

  if (bdaddr == NULL)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(g_hci_control_socks) /
                  sizeof(g_hci_control_socks[0]); i++)
    {
      if (g_hci_control_socks[i] == NULL ||
          g_hci_control_socks[i]->sk_state != BT_BOUND)
        {
          continue;
        }

      if (connected)
        {
          struct mgmt_ev_device_connected ev;

          memset(&ev, 0, sizeof(ev));
          bacpy(&ev.addr.bdaddr, bdaddr);
          ev.addr.type = bdaddr_type;
          ev.flags = cpu_to_le32(outgoing ? MGMT_DEV_FOUND_INITIATED_CONN :
                                 0);
          ev.eir_len = cpu_to_le16(0);
          if (linux_bt_hci_sock_queue_mgmt_event(
                g_hci_control_socks[i], MGMT_EV_DEVICE_CONNECTED,
                dev_id, &ev, sizeof(ev)) == 0)
            {
              sent++;
            }
        }
      else
        {
          struct mgmt_ev_device_disconnected ev;

          memset(&ev, 0, sizeof(ev));
          bacpy(&ev.addr.bdaddr, bdaddr);
          ev.addr.type = bdaddr_type;
          ev.reason = MGMT_DEV_DISCONN_LOCAL_HOST;
          if (linux_bt_hci_sock_queue_mgmt_event(
                g_hci_control_socks[i], MGMT_EV_DEVICE_DISCONNECTED,
                dev_id, &ev, sizeof(ev)) == 0)
            {
              sent++;
            }
        }
    }

  return sent;
}

static int linux_bt_hci_mgmt_broadcast_addr_event_skip(uint16_t event,
                                                       uint16_t dev_id,
                                                       const bdaddr_t *bdaddr,
                                                       uint8_t bdaddr_type,
                                                       struct sock *skip)
{
  struct mgmt_addr_info ev;
  unsigned int i;
  int sent = 0;

  if (bdaddr == NULL)
    {
      return -EINVAL;
    }

  memset(&ev, 0, sizeof(ev));
  bacpy(&ev.bdaddr, bdaddr);
  ev.type = bdaddr_type;

  for (i = 0; i < sizeof(g_hci_control_socks) /
                  sizeof(g_hci_control_socks[0]); i++)
    {
      if (g_hci_control_socks[i] == NULL ||
          g_hci_control_socks[i]->sk_state != BT_BOUND ||
          g_hci_control_socks[i] == skip)
        {
          continue;
        }

      if (linux_bt_hci_sock_queue_mgmt_event(g_hci_control_socks[i],
                                             event, dev_id, &ev,
                                             sizeof(ev)) == 0)
        {
          sent++;
        }
    }

  return sent;
}

static int linux_bt_hci_mgmt_broadcast_device_added(uint16_t dev_id,
                                                    const bdaddr_t *bdaddr,
                                                    uint8_t bdaddr_type,
                                                    uint8_t action,
                                                    struct sock *skip)
{
  struct mgmt_ev_device_added ev;
  unsigned int i;
  int sent = 0;

  if (bdaddr == NULL)
    {
      return -EINVAL;
    }

  memset(&ev, 0, sizeof(ev));
  bacpy(&ev.addr.bdaddr, bdaddr);
  ev.addr.type = bdaddr_type;
  ev.action = action;

  for (i = 0; i < sizeof(g_hci_control_socks) /
                  sizeof(g_hci_control_socks[0]); i++)
    {
      if (g_hci_control_socks[i] == NULL ||
          g_hci_control_socks[i]->sk_state != BT_BOUND ||
          g_hci_control_socks[i] == skip)
        {
          continue;
        }

      if (linux_bt_hci_sock_queue_mgmt_event(g_hci_control_socks[i],
                                             MGMT_EV_DEVICE_ADDED,
                                             dev_id, &ev,
                                             sizeof(ev)) == 0)
        {
          sent++;
        }
    }

  return sent;
}

static int linux_bt_hci_mgmt_broadcast_device_flags(uint16_t dev_id,
                                                    const bdaddr_t *bdaddr,
                                                    uint8_t bdaddr_type,
                                                    uint32_t current_flags,
                                                    struct sock *skip)
{
  struct mgmt_ev_device_flags_changed ev;
  unsigned int i;
  int sent = 0;

  if (bdaddr == NULL)
    {
      return -EINVAL;
    }

  memset(&ev, 0, sizeof(ev));
  bacpy(&ev.addr.bdaddr, bdaddr);
  ev.addr.type = bdaddr_type;
  ev.supported_flags = cpu_to_le32(LINUX_BT_HCI_DEVICE_FLAGS_SUPPORTED);
  ev.current_flags = cpu_to_le32(current_flags);

  for (i = 0; i < sizeof(g_hci_control_socks) /
                  sizeof(g_hci_control_socks[0]); i++)
    {
      if (g_hci_control_socks[i] == NULL ||
          g_hci_control_socks[i]->sk_state != BT_BOUND ||
          g_hci_control_socks[i] == skip)
        {
          continue;
        }

      if (linux_bt_hci_sock_queue_mgmt_event(
            g_hci_control_socks[i], MGMT_EV_DEVICE_FLAGS_CHANGED, dev_id,
            &ev, sizeof(ev)) == 0)
        {
          sent++;
        }
    }

  return sent;
}

static int linux_bt_hci_mgmt_broadcast_discovering(uint16_t dev_id,
                                                   uint8_t type,
                                                   bool discovering,
                                                   struct sock *skip)
{
  struct mgmt_ev_discovering ev;
  unsigned int i;
  int sent = 0;

  memset(&ev, 0, sizeof(ev));
  ev.type = type;
  ev.discovering = discovering ? 1 : 0;

  for (i = 0; i < sizeof(g_hci_control_socks) /
                  sizeof(g_hci_control_socks[0]); i++)
    {
      if (g_hci_control_socks[i] == NULL ||
          g_hci_control_socks[i]->sk_state != BT_BOUND ||
          g_hci_control_socks[i] == skip)
        {
          continue;
        }

      if (linux_bt_hci_sock_queue_mgmt_event(
            g_hci_control_socks[i], MGMT_EV_DISCOVERING, dev_id, &ev,
            sizeof(ev)) == 0)
        {
          sent++;
        }
    }

  return sent;
}

static void linux_bt_hci_mgmt_addr_from_role(uint16_t role, bdaddr_t *bdaddr)
{
  uint8_t *addr = (uint8_t *)bdaddr;

  if (bdaddr == NULL)
    {
      return;
    }

  memset(addr, 0, sizeof(*bdaddr));
  addr[0] = (uint8_t)(role & 0xff);
  addr[1] = (uint8_t)(role >> 8);
  addr[2] = 0x17;
  addr[3] = 0x06;
  addr[4] = 0x5e;
  addr[5] = 0xf0;
}

static int linux_bt_hci_mgmt_broadcast_device_found(uint16_t dev_id,
                                                    uint16_t src,
                                                    const char *name,
                                                    struct sock *skip)
{
  uint8_t event[sizeof(struct mgmt_ev_device_found) + 31];
  struct mgmt_ev_device_found *ev =
    (struct mgmt_ev_device_found *)event;
  uint8_t eir[31];
  size_t name_len;
  size_t eir_len;
  unsigned int i;
  int sent = 0;

  if (!g_hci_discovery_state.discovering ||
      g_hci_discovery_state.dev_id != dev_id)
    {
      return -EAGAIN;
    }

  if (name == NULL || name[0] == '\0')
    {
      name = "linux-bt-hwsim";
    }

  name_len = strlen(name);
  if (name_len > sizeof(eir) - 2)
    {
      name_len = sizeof(eir) - 2;
    }

  memset(eir, 0, sizeof(eir));
  eir[0] = (uint8_t)(name_len + 1);
  eir[1] = 0x09;
  memcpy(&eir[2], name, name_len);
  eir_len = name_len + 2;

  memset(event, 0, sizeof(event));
  linux_bt_hci_mgmt_addr_from_role(src, &ev->addr.bdaddr);
  ev->addr.type = BDADDR_LE_PUBLIC;
  ev->rssi = -42;
  ev->flags = cpu_to_le32(0);
  ev->eir_len = cpu_to_le16((uint16_t)eir_len);
  memcpy(ev->eir, eir, eir_len);

  for (i = 0; i < sizeof(g_hci_control_socks) /
                  sizeof(g_hci_control_socks[0]); i++)
    {
      if (g_hci_control_socks[i] == NULL ||
          g_hci_control_socks[i]->sk_state != BT_BOUND ||
          g_hci_control_socks[i] == skip)
        {
          continue;
        }

      if (linux_bt_hci_sock_queue_mgmt_event(
            g_hci_control_socks[i], MGMT_EV_DEVICE_FOUND, dev_id, ev,
            (uint16_t)(sizeof(*ev) + eir_len)) == 0)
        {
          sent++;
        }
    }

  return sent;
}

static int linux_bt_hci_mgmt_broadcast_user_confirm(uint16_t dev_id,
                                                    const bdaddr_t *bdaddr,
                                                    uint8_t bdaddr_type,
                                                    uint32_t value,
                                                    uint8_t confirm_hint,
                                                    struct sock *skip)
{
  struct mgmt_ev_user_confirm_request ev;
  unsigned int i;
  int sent = 0;

  if (bdaddr == NULL)
    {
      return -EINVAL;
    }

  memset(&ev, 0, sizeof(ev));
  bacpy(&ev.addr.bdaddr, bdaddr);
  ev.addr.type = bdaddr_type;
  ev.value = cpu_to_le32(value);
  ev.confirm_hint = confirm_hint;

  for (i = 0; i < sizeof(g_hci_control_socks) /
                  sizeof(g_hci_control_socks[0]); i++)
    {
      if (g_hci_control_socks[i] == NULL ||
          g_hci_control_socks[i]->sk_state != BT_BOUND ||
          g_hci_control_socks[i] == skip)
        {
          continue;
        }

      if (linux_bt_hci_sock_queue_mgmt_event(
            g_hci_control_socks[i], MGMT_EV_USER_CONFIRM_REQUEST, dev_id,
            &ev, sizeof(ev)) == 0)
        {
          sent++;
        }
    }

  return sent;
}

static int linux_bt_hci_sock_queue_new_settings(struct sock *sk,
                                                uint16_t index)
{
  uint32_t settings = cpu_to_le32(linux_bt_mgmt_settings());

  return linux_bt_hci_sock_queue_mgmt_event(sk, MGMT_EV_NEW_SETTINGS, index,
                                            &settings, sizeof(settings));
}

static bool linux_bt_hci_sock_opcode_changes_settings(uint16_t opcode)
{
  switch (opcode)
    {
      case MGMT_OP_SET_POWERED:
      case MGMT_OP_SET_DISCOVERABLE:
      case MGMT_OP_SET_CONNECTABLE:
      case MGMT_OP_SET_BONDABLE:
      case MGMT_OP_SET_LE:
      case MGMT_OP_SET_ADVERTISING:
      case MGMT_OP_SET_BREDR:
        return true;

      default:
        return false;
    }
}

static int linux_bt_hci_sock_apply_simple_setting(uint16_t opcode,
                                                  uint8_t param)
{
  switch (opcode)
    {
      case MGMT_OP_SET_POWERED:
        return linux_bt_mgmt_set_powered(param);

      case MGMT_OP_SET_DISCOVERABLE:
        return linux_bt_mgmt_set_discoverable(param);

      case MGMT_OP_SET_CONNECTABLE:
        return linux_bt_mgmt_set_connectable(param);

      case MGMT_OP_SET_BONDABLE:
        return linux_bt_mgmt_set_bondable(param);

      case MGMT_OP_SET_LE:
        return linux_bt_mgmt_set_le(param);

      case MGMT_OP_SET_ADVERTISING:
        return linux_bt_mgmt_set_advertising(param);

      case MGMT_OP_SET_BREDR:
        return linux_bt_mgmt_set_bredr(param);

      default:
        return -EOPNOTSUPP;
    }
}

static int linux_bt_hci_sock_queue_cmd_complete_data(struct sock *sk,
                                                     uint16_t opcode,
                                                     uint16_t index,
                                                     uint8_t status,
                                                     const void *data,
                                                     uint16_t data_len)
{
  struct sk_buff *skb;
  struct mgmt_hdr *hdr;
  struct mgmt_ev_cmd_complete *ev;
  uint16_t event_len = sizeof(*ev) + data_len;

  skb = alloc_skb(sizeof(*hdr) + event_len, GFP_KERNEL);
  if (skb == NULL)
    {
      return -ENOMEM;
    }

  hdr = skb_put(skb, sizeof(*hdr));
  if (hdr == NULL)
    {
      kfree_skb(skb);
      return -ENOMEM;
    }

  hdr->opcode = cpu_to_le16(MGMT_EV_CMD_COMPLETE);
  hdr->index = cpu_to_le16(index);
  hdr->len = cpu_to_le16(event_len);

  ev = skb_put(skb, sizeof(*ev));
  if (ev == NULL)
    {
      kfree_skb(skb);
      return -ENOMEM;
    }

  ev->opcode = cpu_to_le16(opcode);
  ev->status = status;
  if (data_len > 0)
    {
      skb_put_data(skb, data, data_len);
    }

  linux_bt_hci_monitor_broadcast(HCI_MON_CTRL_EVENT, index, skb->data,
                                 skb->len, NULL);
  skb_queue_tail(&sk->sk_receive_queue, skb);
  g_hci_mgmt_socket_events++;
  if (sk->sk_data_ready != NULL)
    {
      sk->sk_data_ready(sk);
    }

  return 0;
}

int hci_mgmt_chan_register(struct hci_mgmt_chan *c)
{
  if (c == NULL || c->channel < HCI_CHANNEL_CONTROL ||
      c->channel >= sizeof(g_hci_mgmt_chan) / sizeof(g_hci_mgmt_chan[0]))
    {
      return -EINVAL;
    }

  if (g_hci_mgmt_chan[c->channel] != NULL)
    {
      return -EALREADY;
    }

  INIT_LIST_HEAD(&c->list);
  g_hci_mgmt_chan[c->channel] = c;
  g_hci_mgmt_chan_registers++;
  return 0;
}

void hci_mgmt_chan_unregister(struct hci_mgmt_chan *c)
{
  if (c != NULL && c->channel < sizeof(g_hci_mgmt_chan) /
      sizeof(g_hci_mgmt_chan[0]) && g_hci_mgmt_chan[c->channel] == c)
    {
      g_hci_mgmt_chan[c->channel] = NULL;
      INIT_LIST_HEAD(&c->list);
      g_hci_mgmt_chan_unregisters++;
    }
}

static uint16_t linux_bt_hci_monitor_rx_event(uint8_t pkt_type)
{
  switch (pkt_type)
    {
      case HCI_COMMAND_PKT:
        return HCI_MON_COMMAND_PKT;

      case HCI_EVENT_PKT:
        return HCI_MON_EVENT_PKT;

      case HCI_ACLDATA_PKT:
        return HCI_MON_ACL_RX_PKT;

      case HCI_SCODATA_PKT:
        return HCI_MON_SCO_RX_PKT;

      case HCI_ISODATA_PKT:
        return HCI_MON_ISO_RX_PKT;

      case HCI_DRV_PKT:
        return HCI_MON_DRV_RX_PKT;

      default:
        return 0xffff;
    }
}

int linux_bt_upstream_hci_sock_recv(struct hci_dev *hdev, uint8_t pkt_type,
                                    const void *payload,
                                    size_t payload_len)
{
  struct linux_bt_hci_pinfo *pi;
  struct sk_buff *skb;
  uint16_t mon_event;
  unsigned int i;
  unsigned int delivered = 0;

  if (hdev == NULL || (payload == NULL && payload_len > 0))
    {
      return -EINVAL;
    }

  mon_event = linux_bt_hci_monitor_rx_event(pkt_type);
  if (mon_event != 0xffff)
    {
      linux_bt_hci_monitor_broadcast(mon_event, hdev->id, payload,
                                     payload_len, NULL);
    }

  for (i = 0; i < sizeof(g_hci_data_socks) /
                  sizeof(g_hci_data_socks[0]); i++)
    {
      if (g_hci_data_socks[i] == NULL ||
          g_hci_data_socks[i]->sk_state != BT_BOUND)
        {
          continue;
        }

      pi = linux_bt_hci_pi(g_hci_data_socks[i]);
      if (pi->hdev != hdev)
        {
          continue;
        }

      if (pi->channel == HCI_CHANNEL_RAW)
        {
          switch (pkt_type)
            {
              case HCI_COMMAND_PKT:
              case HCI_EVENT_PKT:
              case HCI_ACLDATA_PKT:
              case HCI_SCODATA_PKT:
              case HCI_ISODATA_PKT:
                break;

              default:
                continue;
            }
        }
      else if (pi->channel == HCI_CHANNEL_USER)
        {
          switch (pkt_type)
            {
              case HCI_EVENT_PKT:
              case HCI_ACLDATA_PKT:
              case HCI_SCODATA_PKT:
              case HCI_ISODATA_PKT:
              case HCI_DRV_PKT:
                break;

              default:
                continue;
            }
        }
      else
        {
          continue;
        }

      if (pi->channel == HCI_CHANNEL_RAW &&
          linux_bt_hci_sock_filtered(pi, pkt_type, payload, payload_len))
        {
          continue;
        }

      skb = alloc_skb(payload_len + 1, GFP_KERNEL);
      if (skb == NULL)
        {
          continue;
        }

      skb_put_u8(skb, pkt_type);
      if (payload_len > 0)
        {
          skb_put_data(skb, payload, payload_len);
        }

      skb_queue_tail(&g_hci_data_socks[i]->sk_receive_queue, skb);
      if (g_hci_data_socks[i]->sk_data_ready != NULL)
        {
          g_hci_data_socks[i]->sk_data_ready(g_hci_data_socks[i]);
        }

      delivered++;
    }

  g_hci_data_socket_rx += delivered;
  return (int)delivered;
}

static int linux_bt_hci_sock_setsockopt(struct socket *sock, int level,
                                        int optname, sockptr_t optval,
                                        unsigned int optlen)
{
  struct linux_bt_hci_pinfo *pi;
  struct hci_ufilter uf;
  int opt;

  if (sock == NULL || sock->sk == NULL)
    {
      return -EINVAL;
    }

  if (level != SOL_HCI)
    {
      return -ENOPROTOOPT;
    }

  pi = linux_bt_hci_pi(sock->sk);
  if (pi->channel != HCI_CHANNEL_RAW)
    {
      return -EBADFD;
    }

  switch (optname)
    {
      case HCI_DATA_DIR:
        if (optval == NULL || optlen < sizeof(opt))
          {
            return -EINVAL;
          }

        memcpy(&opt, optval, sizeof(opt));
        if (opt != 0)
          {
            pi->cmsg_mask |= HCI_CMSG_DIR;
          }
        else
          {
            pi->cmsg_mask &= ~HCI_CMSG_DIR;
          }

        g_hci_filter_sets++;
        return 0;

      case HCI_TIME_STAMP:
        if (optval == NULL || optlen < sizeof(opt))
          {
            return -EINVAL;
          }

        memcpy(&opt, optval, sizeof(opt));
        if (opt != 0)
          {
            pi->cmsg_mask |= HCI_CMSG_TSTAMP;
          }
        else
          {
            pi->cmsg_mask &= ~HCI_CMSG_TSTAMP;
          }

        g_hci_filter_sets++;
        return 0;

      case HCI_FILTER:
        if (optval == NULL)
          {
            return -EINVAL;
          }

        memset(&uf, 0, sizeof(uf));
        uf.type_mask = (uint32_t)pi->filter.type_mask;
        uf.event_mask[0] = ((uint32_t *)pi->filter.event_mask)[0];
        uf.event_mask[1] = ((uint32_t *)pi->filter.event_mask)[1];
        uf.opcode = pi->filter.opcode;
        if (optlen > sizeof(uf))
          {
            optlen = sizeof(uf);
          }

        memcpy(&uf, optval, optlen);
        pi->filter.type_mask = uf.type_mask;
        ((uint32_t *)pi->filter.event_mask)[0] = uf.event_mask[0];
        ((uint32_t *)pi->filter.event_mask)[1] = uf.event_mask[1];
        pi->filter.opcode = uf.opcode;
        g_hci_filter_sets++;
        return 0;

      default:
        return -ENOPROTOOPT;
    }
}

static int linux_bt_hci_sock_getsockopt(struct socket *sock, int level,
                                        int optname, char *optval,
                                        int *optlen)
{
  struct linux_bt_hci_pinfo *pi;
  struct hci_ufilter uf;
  int opt;
  int len;

  if (sock == NULL || sock->sk == NULL || optval == NULL ||
      optlen == NULL)
    {
      return -EINVAL;
    }

  if (level != SOL_HCI)
    {
      return -ENOPROTOOPT;
    }

  pi = linux_bt_hci_pi(sock->sk);
  if (pi->channel != HCI_CHANNEL_RAW)
    {
      return -EBADFD;
    }

  switch (optname)
    {
      case HCI_DATA_DIR:
        opt = (pi->cmsg_mask & HCI_CMSG_DIR) != 0 ? 1 : 0;
        len = *optlen < (int)sizeof(opt) ? *optlen : (int)sizeof(opt);
        if (len < 0)
          {
            return -EINVAL;
          }

        memcpy(optval, &opt, (size_t)len);
        *optlen = len;
        g_hci_filter_gets++;
        return 0;

      case HCI_TIME_STAMP:
        opt = (pi->cmsg_mask & HCI_CMSG_TSTAMP) != 0 ? 1 : 0;
        len = *optlen < (int)sizeof(opt) ? *optlen : (int)sizeof(opt);
        if (len < 0)
          {
            return -EINVAL;
          }

        memcpy(optval, &opt, (size_t)len);
        *optlen = len;
        g_hci_filter_gets++;
        return 0;

      case HCI_FILTER:
        memset(&uf, 0, sizeof(uf));
        uf.type_mask = (uint32_t)pi->filter.type_mask;
        uf.event_mask[0] = ((uint32_t *)pi->filter.event_mask)[0];
        uf.event_mask[1] = ((uint32_t *)pi->filter.event_mask)[1];
        uf.opcode = pi->filter.opcode;
        len = *optlen < (int)sizeof(uf) ? *optlen : (int)sizeof(uf);
        if (len < 0)
          {
            return -EINVAL;
          }

        memcpy(optval, &uf, (size_t)len);
        *optlen = len;
        g_hci_filter_gets++;
        return 0;

      default:
        return -ENOPROTOOPT;
    }
}

static int linux_bt_hci_mgmt_cmd(struct hci_mgmt_chan *chan,
                                 struct sock *sk, struct sk_buff *skb)
{
  static const uint16_t supported_commands[] =
  {
    MGMT_OP_READ_VERSION,
    MGMT_OP_READ_COMMANDS,
    MGMT_OP_READ_INDEX_LIST,
    MGMT_OP_READ_INFO,
    MGMT_OP_SET_POWERED,
    MGMT_OP_SET_DISCOVERABLE,
    MGMT_OP_SET_CONNECTABLE,
    MGMT_OP_SET_BONDABLE,
    MGMT_OP_SET_LE,
    MGMT_OP_SET_ADVERTISING,
    MGMT_OP_SET_BREDR,
    MGMT_OP_DISCONNECT,
    MGMT_OP_SET_IO_CAPABILITY,
    MGMT_OP_PAIR_DEVICE,
    MGMT_OP_CANCEL_PAIR_DEVICE,
    MGMT_OP_UNPAIR_DEVICE,
    MGMT_OP_USER_CONFIRM_REPLY,
    MGMT_OP_USER_CONFIRM_NEG_REPLY,
    MGMT_OP_USER_PASSKEY_REPLY,
    MGMT_OP_USER_PASSKEY_NEG_REPLY,
    MGMT_OP_START_DISCOVERY,
    MGMT_OP_STOP_DISCOVERY,
    MGMT_OP_BLOCK_DEVICE,
    MGMT_OP_UNBLOCK_DEVICE,
    MGMT_OP_GET_CONN_INFO,
    MGMT_OP_GET_CLOCK_INFO,
    MGMT_OP_ADD_DEVICE,
    MGMT_OP_REMOVE_DEVICE,
    MGMT_OP_GET_DEVICE_FLAGS,
    MGMT_OP_SET_DEVICE_FLAGS,
  };
  static const uint16_t supported_events[] =
  {
    MGMT_EV_CMD_COMPLETE,
    MGMT_EV_CMD_STATUS,
    MGMT_EV_NEW_SETTINGS,
    MGMT_EV_NEW_LONG_TERM_KEY,
    MGMT_EV_DISCOVERING,
    MGMT_EV_DEVICE_FOUND,
    MGMT_EV_DEVICE_BLOCKED,
    MGMT_EV_DEVICE_UNBLOCKED,
    MGMT_EV_DEVICE_CONNECTED,
    MGMT_EV_DEVICE_DISCONNECTED,
    MGMT_EV_DEVICE_ADDED,
    MGMT_EV_DEVICE_REMOVED,
    MGMT_EV_DEVICE_UNPAIRED,
    MGMT_EV_USER_CONFIRM_REQUEST,
    MGMT_EV_USER_PASSKEY_REQUEST,
    MGMT_EV_PASSKEY_NOTIFY,
    MGMT_EV_DEVICE_FLAGS_CHANGED,
  };
  struct mgmt_hdr *hdr;
  const struct hci_mgmt_handler *handler;
  struct hci_dev *hdev = NULL;
  uint16_t opcode;
  uint16_t index;
  uint16_t data_len;
  void *data;
  uint8_t status = MGMT_STATUS_SUCCESS;
  uint8_t param = 0;
  bool local_handler;
  bool no_hdev;
  char reply[256];
  int ret = 0;

  if (chan == NULL || sk == NULL || skb == NULL || skb->len < sizeof(*hdr))
    {
      return -EINVAL;
    }

  hdr = (struct mgmt_hdr *)skb->data;
  opcode = le16_to_cpu(hdr->opcode);
  index = le16_to_cpu(hdr->index);
  data_len = le16_to_cpu(hdr->len);

  if (data_len != skb->len - sizeof(*hdr))
    {
      return -EINVAL;
    }

  data = skb->data + sizeof(*hdr);
  if (data_len > 0)
    {
      param = ((uint8_t *)data)[0];
    }

  linux_bt_hci_monitor_broadcast(HCI_MON_CTRL_COMMAND, index,
                                 skb->data, skb->len, sk);

  if (opcode >= chan->handler_count ||
      chan->handlers[opcode].func == NULL)
    {
      linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                MGMT_STATUS_UNKNOWN_COMMAND,
                                                NULL, 0);
      return (int)skb->len;
    }

  handler = &chan->handlers[opcode];

  if (index != MGMT_INDEX_NONE)
    {
      hdev = hci_dev_get(index);
      if (hdev == NULL)
        {
          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    MGMT_STATUS_INVALID_INDEX,
                                                    NULL, 0);
          return (int)skb->len;
        }
    }

  if ((handler->flags & HCI_MGMT_HDEV_OPTIONAL) == 0)
    {
      no_hdev = (handler->flags & HCI_MGMT_NO_HDEV) != 0;
      if (no_hdev != (hdev == NULL))
        {
          if (hdev != NULL)
            {
              hci_dev_put(hdev);
            }

          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    MGMT_STATUS_INVALID_INDEX,
                                                    NULL, 0);
          return (int)skb->len;
        }
    }

  if ((handler->flags & HCI_MGMT_VAR_LEN) != 0)
    {
      if (data_len < handler->data_len)
        {
          if (hdev != NULL)
            {
              hci_dev_put(hdev);
            }

          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    MGMT_STATUS_INVALID_PARAMS,
                                                    NULL, 0);
          return (int)skb->len;
        }
    }
  else if (data_len != handler->data_len)
    {
      if (hdev != NULL)
        {
          hci_dev_put(hdev);
        }

      linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                MGMT_STATUS_INVALID_PARAMS,
                                                NULL, 0);
      return (int)skb->len;
    }

  switch (opcode)
    {
      case MGMT_OP_READ_VERSION:
      case MGMT_OP_READ_COMMANDS:
      case MGMT_OP_READ_INDEX_LIST:
      case MGMT_OP_READ_INFO:
      case MGMT_OP_SET_POWERED:
      case MGMT_OP_SET_DISCOVERABLE:
      case MGMT_OP_SET_CONNECTABLE:
      case MGMT_OP_SET_BONDABLE:
      case MGMT_OP_SET_LE:
      case MGMT_OP_SET_ADVERTISING:
      case MGMT_OP_SET_BREDR:
      case MGMT_OP_DISCONNECT:
      case MGMT_OP_START_DISCOVERY:
      case MGMT_OP_STOP_DISCOVERY:
      case MGMT_OP_PAIR_DEVICE:
      case MGMT_OP_CANCEL_PAIR_DEVICE:
      case MGMT_OP_UNPAIR_DEVICE:
      case MGMT_OP_USER_CONFIRM_REPLY:
      case MGMT_OP_USER_CONFIRM_NEG_REPLY:
      case MGMT_OP_USER_PASSKEY_REPLY:
      case MGMT_OP_USER_PASSKEY_NEG_REPLY:
      case MGMT_OP_BLOCK_DEVICE:
      case MGMT_OP_UNBLOCK_DEVICE:
      case MGMT_OP_GET_CONN_INFO:
      case MGMT_OP_GET_CLOCK_INFO:
      case MGMT_OP_ADD_DEVICE:
      case MGMT_OP_REMOVE_DEVICE:
      case MGMT_OP_GET_DEVICE_FLAGS:
      case MGMT_OP_SET_DEVICE_FLAGS:
        local_handler = true;
        break;

      default:
        local_handler = false;
        break;
    }

  if (!local_handler)
    {
      ret = handler->func(sk, hdev, data, data_len);
      if (ret < 0)
        {
          status = linux_bt_hci_sock_mgmt_status(ret);
        }
    }

  switch (opcode)
    {
      case MGMT_OP_READ_VERSION:
        {
          struct mgmt_rp_read_version rp;

          rp.version = 1;
          rp.revision = cpu_to_le16(23);
          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    sizeof(rp));
        }
        break;

      case MGMT_OP_READ_COMMANDS:
        {
          struct
          {
            __le16 num_commands;
            __le16 num_events;
            __le16 opcodes[sizeof(supported_commands) /
                           sizeof(supported_commands[0]) +
                           sizeof(supported_events) /
                           sizeof(supported_events[0])];
          } rp;
          unsigned int i;
          unsigned int pos = 0;

          memset(&rp, 0, sizeof(rp));
          rp.num_commands = cpu_to_le16(sizeof(supported_commands) /
                                        sizeof(supported_commands[0]));
          rp.num_events = cpu_to_le16(sizeof(supported_events) /
                                      sizeof(supported_events[0]));
          for (i = 0; i < sizeof(supported_commands) /
                          sizeof(supported_commands[0]); i++)
            {
              rp.opcodes[pos++] = cpu_to_le16(supported_commands[i]);
            }

          for (i = 0; i < sizeof(supported_events) /
                          sizeof(supported_events[0]); i++)
            {
              rp.opcodes[pos++] = cpu_to_le16(supported_events[i]);
            }

          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    sizeof(rp));
        }
        break;

      case MGMT_OP_READ_INDEX_LIST:
        {
          struct
          {
            __le16 num_controllers;
            __le16 index[1];
          } rp;
          struct hci_dev *first;
          uint16_t rp_len = sizeof(rp.num_controllers);

          first = hci_dev_get(0);
          rp.num_controllers = cpu_to_le16(first != NULL ? 1 : 0);
          rp.index[0] = cpu_to_le16(0);
          if (first != NULL)
            {
              hci_dev_put(first);
              rp_len = sizeof(rp);
            }

          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    rp_len);
        }
        break;

      case MGMT_OP_READ_INFO:
        {
          struct mgmt_rp_read_info rp;
          uint32_t settings = linux_bt_mgmt_settings();

          memset(&rp, 0, sizeof(rp));
          rp.version = 10;
          rp.manufacturer = cpu_to_le16(0xffff);
          rp.supported_settings = cpu_to_le32(settings);
          rp.current_settings = cpu_to_le32(settings);
          snprintf((char *)rp.name, sizeof(rp.name), "Feather BT hwsim");
          snprintf((char *)rp.short_name, sizeof(rp.short_name), "FeatherBT");
          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    sizeof(rp));
        }
        break;

      case MGMT_OP_START_DISCOVERY:
      case MGMT_OP_STOP_DISCOVERY:
        {
          struct mgmt_cp_start_discovery *cp =
            (struct mgmt_cp_start_discovery *)data;
          uint8_t type = cp != NULL ? cp->type : 0;

          ret = linux_bt_hci_discovery_update(
            hdev, type, opcode == MGMT_OP_START_DISCOVERY, sk);
          status = ret == -EALREADY ? MGMT_STATUS_BUSY :
                   linux_bt_hci_sock_mgmt_status(ret);
          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, cp,
                                                    cp != NULL ?
                                                    sizeof(*cp) : 0);
        }
        break;

      case MGMT_OP_SET_POWERED:
      case MGMT_OP_SET_DISCOVERABLE:
      case MGMT_OP_SET_CONNECTABLE:
      case MGMT_OP_SET_BONDABLE:
      case MGMT_OP_SET_LE:
      case MGMT_OP_SET_ADVERTISING:
      case MGMT_OP_SET_BREDR:
        {
          uint32_t settings;

          ret = linux_bt_hci_sock_apply_simple_setting(opcode, param);
          status = linux_bt_hci_sock_mgmt_status(ret);
          settings = cpu_to_le32(linux_bt_mgmt_settings());
          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &settings,
                                                    sizeof(settings));
          if (ret >= 0)
            {
              linux_bt_hci_sock_queue_new_settings(sk, index);
            }
        }
        break;

      case MGMT_OP_SET_IO_CAPABILITY:
        {
          const struct mgmt_cp_set_io_capability *cp = data;
          uint8_t status;

          if (cp == NULL ||
              cp->io_capability > LINUX_BT_HCI_IO_CAPABILITY_MAX)
            {
              status = MGMT_STATUS_INVALID_PARAMS;
            }
          else
            {
              g_hci_io_capability = cp->io_capability;
              status = MGMT_STATUS_SUCCESS;
            }

          return linux_bt_hci_sock_queue_cmd_complete_data(
            sk, opcode, index, status, cp, cp != NULL ? sizeof(*cp) : 0);
        }

      case MGMT_OP_PAIR_DEVICE:
        {
          struct mgmt_cp_pair_device *cp =
            (struct mgmt_cp_pair_device *)data;
          struct mgmt_rp_pair_device rp;

          memset(&rp, 0, sizeof(rp));
          if (cp != NULL)
            {
              memcpy(&rp.addr, &cp->addr, sizeof(rp.addr));
            }

          ret = linux_bt_hci_pair_device(hdev, cp, sk);
          if (ret == -EINPROGRESS)
            {
              break;
            }

          status = ret == -EALREADY ? MGMT_STATUS_ALREADY_PAIRED :
                   linux_bt_hci_sock_mgmt_status(ret);
          if (status == MGMT_STATUS_SUCCESS && cp != NULL)
            {
              struct mgmt_ev_device_connected connected;

              memset(&connected, 0, sizeof(connected));
              memcpy(&connected.addr, &cp->addr,
                     sizeof(connected.addr));
              connected.flags =
                cpu_to_le32(MGMT_DEV_FOUND_INITIATED_CONN);
              connected.eir_len = cpu_to_le16(0);
              linux_bt_hci_sock_queue_mgmt_event(
                sk, MGMT_EV_DEVICE_CONNECTED, hdev->id, &connected,
                sizeof(connected));
            }

          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    sizeof(rp));
        }
        break;

      case MGMT_OP_DISCONNECT:
        {
          struct mgmt_cp_disconnect *cp =
            (struct mgmt_cp_disconnect *)data;
          struct mgmt_rp_disconnect rp;
          struct hci_conn *conn = NULL;
          bool found = false;

          memset(&rp, 0, sizeof(rp));
          if (cp != NULL)
            {
              memcpy(&rp.addr, &cp->addr, sizeof(rp.addr));
            }

          if (cp == NULL ||
              !linux_bt_hci_mgmt_addr_type_valid(cp->addr.type))
            {
              status = MGMT_STATUS_INVALID_PARAMS;
            }
          else
            {
              conn = linux_bt_hci_mgmt_conn_lookup(hdev, &cp->addr);
              if (conn != NULL)
                {
                  found = true;
                  linux_bt_hci_mgmt_broadcast_conn_addr_event(
                    hdev->id, &conn->dst,
                    linux_bt_hci_mgmt_addr_type_from_conn(conn), false,
                    conn->out);
                  hci_conn_del(conn);
                }

              status = found ? MGMT_STATUS_SUCCESS :
                               MGMT_STATUS_NOT_CONNECTED;
            }

          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    sizeof(rp));
        }
        break;

      case MGMT_OP_CANCEL_PAIR_DEVICE:
        {
          struct mgmt_addr_info *addr = (struct mgmt_addr_info *)data;

          ret = linux_bt_hci_cancel_pair_device(hdev, addr);
          status = linux_bt_hci_sock_mgmt_status(ret);
          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, addr,
                                                    addr != NULL ?
                                                    sizeof(*addr) : 0);
        }
        break;

      case MGMT_OP_UNPAIR_DEVICE:
        {
          struct mgmt_cp_unpair_device *cp =
            (struct mgmt_cp_unpair_device *)data;
          struct mgmt_rp_unpair_device rp;

          memset(&rp, 0, sizeof(rp));
          if (cp != NULL)
            {
              memcpy(&rp.addr, &cp->addr, sizeof(rp.addr));
            }

          ret = linux_bt_hci_unpair_device(hdev, cp, sk);
          status = ret == -ENOENT ? MGMT_STATUS_NOT_PAIRED :
                   linux_bt_hci_sock_mgmt_status(ret);
          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    sizeof(rp));
          if (status == MGMT_STATUS_SUCCESS)
            {
              linux_bt_hci_mgmt_broadcast_addr_event_skip(
                MGMT_EV_DEVICE_UNPAIRED, hdev->id, &cp->addr.bdaddr,
                cp->addr.type, NULL);
            }
        }
        break;

      case MGMT_OP_USER_CONFIRM_REPLY:
      case MGMT_OP_USER_CONFIRM_NEG_REPLY:
        {
          struct mgmt_addr_info *addr = (struct mgmt_addr_info *)data;
          struct mgmt_rp_user_confirm_reply rp;

          memset(&rp, 0, sizeof(rp));
          if (addr != NULL)
            {
              memcpy(&rp.addr, addr, sizeof(rp.addr));
            }

          ret = linux_bt_hci_user_confirm_reply(
            hdev, addr, opcode == MGMT_OP_USER_CONFIRM_REPLY);
          status = linux_bt_hci_sock_mgmt_status(ret);
          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    sizeof(rp));
        }
        break;

      case MGMT_OP_USER_PASSKEY_REPLY:
      case MGMT_OP_USER_PASSKEY_NEG_REPLY:
        {
          struct mgmt_cp_user_passkey_reply *cp =
            (struct mgmt_cp_user_passkey_reply *)data;
          struct mgmt_addr_info *addr = data;
          struct mgmt_rp_user_passkey_reply rp;
          uint32_t passkey = 0;

          memset(&rp, 0, sizeof(rp));
          if (addr != NULL)
            {
              memcpy(&rp.addr, addr, sizeof(rp.addr));
            }

          if (opcode == MGMT_OP_USER_PASSKEY_REPLY && cp != NULL)
            {
              passkey = le32_to_cpu(cp->passkey);
            }

          ret = linux_bt_hci_user_passkey_reply(
            hdev, addr, passkey, opcode == MGMT_OP_USER_PASSKEY_REPLY);
          status = linux_bt_hci_sock_mgmt_status(ret);
          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    sizeof(rp));
        }
        break;

      case MGMT_OP_BLOCK_DEVICE:
      case MGMT_OP_UNBLOCK_DEVICE:
        {
          struct mgmt_cp_block_device *cp =
            (struct mgmt_cp_block_device *)data;
          bool add = opcode == MGMT_OP_BLOCK_DEVICE;

          if (cp == NULL ||
              !linux_bt_hci_mgmt_addr_type_valid(cp->addr.type))
            {
              status = MGMT_STATUS_INVALID_PARAMS;
            }
          else
            {
              ret = linux_bt_hci_reject_addr_update(
                hdev, &cp->addr.bdaddr, cp->addr.type, add, sk);
              status = linux_bt_hci_sock_mgmt_status(ret);
            }

          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status,
                                                    cp != NULL ?
                                                    &cp->addr : NULL,
                                                    cp != NULL ?
                                                    sizeof(cp->addr) : 0);
        }
        break;

      case MGMT_OP_GET_CONN_INFO:
        {
          struct mgmt_cp_get_conn_info *cp =
            (struct mgmt_cp_get_conn_info *)data;
          struct mgmt_rp_get_conn_info rp;
          struct hci_conn *conn = NULL;

          memset(&rp, 0, sizeof(rp));
          if (cp != NULL)
            {
              memcpy(&rp.addr, &cp->addr, sizeof(rp.addr));
            }

          if (cp == NULL ||
              !linux_bt_hci_mgmt_addr_type_valid(cp->addr.type))
            {
              status = MGMT_STATUS_INVALID_PARAMS;
            }
          else
            {
              conn = linux_bt_hci_mgmt_conn_lookup(hdev, &cp->addr);
              status = conn != NULL ? MGMT_STATUS_SUCCESS :
                                      MGMT_STATUS_NOT_CONNECTED;
            }

          if (status == MGMT_STATUS_SUCCESS)
            {
              rp.rssi = -42;
              rp.tx_power = 0;
              rp.max_tx_power = 0;
            }

          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    sizeof(rp));
        }
        break;

      case MGMT_OP_GET_CLOCK_INFO:
        {
          struct mgmt_cp_get_clock_info *cp =
            (struct mgmt_cp_get_clock_info *)data;
          struct mgmt_rp_get_clock_info rp;
          struct hci_conn *conn = NULL;
          bdaddr_t any;

          memset(&rp, 0, sizeof(rp));
          bacpy(&any, BDADDR_ANY);
          if (cp != NULL)
            {
              memcpy(&rp.addr, &cp->addr, sizeof(rp.addr));
            }

          if (cp == NULL || cp->addr.type != BDADDR_BREDR)
            {
              status = MGMT_STATUS_INVALID_PARAMS;
            }
          else if (memcmp(&cp->addr.bdaddr, &any, sizeof(any)) != 0)
            {
              conn = linux_bt_hci_mgmt_conn_lookup(hdev, &cp->addr);
              status = conn != NULL ? MGMT_STATUS_SUCCESS :
                                      MGMT_STATUS_NOT_CONNECTED;
            }

          if (status == MGMT_STATUS_SUCCESS)
            {
              rp.local_clock = cpu_to_le32(0);
              rp.piconet_clock = cpu_to_le32(
                conn != NULL ? conn->handle : 0);
              rp.accuracy = cpu_to_le16(0);
            }

          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    sizeof(rp));
        }
        break;

      case MGMT_OP_ADD_DEVICE:
        {
          struct mgmt_cp_add_device *cp =
            (struct mgmt_cp_add_device *)data;

          if (cp == NULL)
            {
              status = MGMT_STATUS_INVALID_PARAMS;
            }
          else
            {
              ret = linux_bt_hci_device_list_update(hdev, &cp->addr,
                                                    cp->action, true, sk);
              status = linux_bt_hci_sock_mgmt_status(ret);
            }

          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status,
                                                    cp != NULL ?
                                                    &cp->addr : NULL,
                                                    cp != NULL ?
                                                    sizeof(cp->addr) : 0);
        }
        break;

      case MGMT_OP_REMOVE_DEVICE:
        {
          struct mgmt_cp_remove_device *cp =
            (struct mgmt_cp_remove_device *)data;

          if (cp == NULL)
            {
              status = MGMT_STATUS_INVALID_PARAMS;
            }
          else
            {
              ret = linux_bt_hci_device_list_update(hdev, &cp->addr, 0,
                                                    false, sk);
              status = linux_bt_hci_sock_mgmt_status(ret);
            }

          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status,
                                                    cp != NULL ?
                                                    &cp->addr : NULL,
                                                    cp != NULL ?
                                                    sizeof(cp->addr) : 0);
        }
        break;

      case MGMT_OP_GET_DEVICE_FLAGS:
        {
          struct mgmt_cp_get_device_flags *cp =
            (struct mgmt_cp_get_device_flags *)data;
          struct mgmt_rp_get_device_flags rp;
          uint32_t supported_flags = 0;
          uint32_t current_flags = 0;

          memset(&rp, 0, sizeof(rp));
          if (cp != NULL)
            {
              memcpy(&rp.addr, &cp->addr, sizeof(rp.addr));
            }

          ret = linux_bt_hci_device_flags_get(hdev,
                                              cp != NULL ? &cp->addr : NULL,
                                              &supported_flags,
                                              &current_flags);
          status = linux_bt_hci_sock_mgmt_status(ret);
          rp.supported_flags = cpu_to_le32(supported_flags);
          rp.current_flags = cpu_to_le32(current_flags);
          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    sizeof(rp));
        }
        break;

      case MGMT_OP_SET_DEVICE_FLAGS:
        {
          struct mgmt_cp_set_device_flags *cp =
            (struct mgmt_cp_set_device_flags *)data;
          struct mgmt_rp_set_device_flags rp;
          uint32_t current_flags = cp != NULL ?
                                   le32_to_cpu(cp->current_flags) : 0;

          memset(&rp, 0, sizeof(rp));
          if (cp != NULL)
            {
              memcpy(&rp.addr, &cp->addr, sizeof(rp.addr));
            }

          ret = linux_bt_hci_device_flags_set(hdev,
                                              cp != NULL ? &cp->addr : NULL,
                                              current_flags, sk);
          status = linux_bt_hci_sock_mgmt_status(ret);
          linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                    status, &rp,
                                                    sizeof(rp));
        }
        break;

      default:
        ret = linux_bt_mgmt_dispatch(opcode, param, reply, sizeof(reply));
        if (ret >= 0 && opcode == MGMT_OP_SET_POWERED && hdev != NULL)
          {
            ret = param != 0 ? hci_dev_open(index) : hci_dev_do_close(hdev);
          }

        status = linux_bt_hci_sock_mgmt_status(ret);
        if (ret >= 0 && linux_bt_hci_sock_opcode_changes_settings(opcode))
          {
            uint32_t settings = cpu_to_le32(linux_bt_mgmt_settings());

            linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                      status, &settings,
                                                      sizeof(settings));
            linux_bt_hci_sock_queue_new_settings(sk, index);
          }
        else
          {
            linux_bt_hci_sock_queue_cmd_complete_data(sk, opcode, index,
                                                      status, NULL, 0);
          }
        break;
    }

  if (hdev != NULL)
    {
      hci_dev_put(hdev);
    }

  return ret < 0 ? ret : (int)skb->len;
}

static int linux_bt_hci_sock_sendmsg(struct socket *sock, struct msghdr *msg,
                                     size_t len)
{
  struct linux_bt_hci_pinfo *pi;
  struct hci_mgmt_chan *chan;
  struct sk_buff *skb;
  size_t min_len;
  int ret;

  if (sock == NULL || sock->sk == NULL || msg == NULL ||
      msg->msg_iter.data == NULL)
    {
      return -EINVAL;
    }

  pi = linux_bt_hci_pi(sock->sk);
  switch (pi->channel)
    {
      case HCI_CHANNEL_RAW:
      case HCI_CHANNEL_USER:
        min_len = 1;
        break;

      case HCI_CHANNEL_LOGGING:
        min_len = sizeof(struct hci_mon_hdr) + 3;
        break;

      case HCI_CHANNEL_CONTROL:
        min_len = sizeof(struct mgmt_hdr);
        break;

      case HCI_CHANNEL_MONITOR:
        return -EOPNOTSUPP;

      default:
        return -EINVAL;
    }

  if (len < min_len)
    {
      return -EINVAL;
    }

  skb = bt_skb_sendmsg(sock->sk, msg, len, len, 0, 0);
  if (IS_ERR(skb))
    {
      return PTR_ERR(skb);
    }

  if (pi->channel == HCI_CHANNEL_RAW || pi->channel == HCI_CHANNEL_USER)
    {
      return linux_bt_hci_sock_send_to_hdev(pi, skb);
    }

  if (pi->channel == HCI_CHANNEL_LOGGING)
    {
      ret = linux_bt_hci_logging_frame(sock->sk, skb);
      kfree_skb(skb);
      return ret;
    }

  g_hci_mgmt_socket_cmds++;
  chan = g_hci_mgmt_chan[pi->channel < sizeof(g_hci_mgmt_chan) /
                         sizeof(g_hci_mgmt_chan[0]) ? pi->channel : 0];
  ret = chan != NULL ? linux_bt_hci_mgmt_cmd(chan, sock->sk, skb) : -EINVAL;
  kfree_skb(skb);

  return ret < 0 ? ret : (int)len;
}

static int linux_bt_hci_sock_recvmsg(struct socket *sock, struct msghdr *msg,
                                     size_t len, int flags)
{
  struct linux_bt_hci_pinfo *pi;
  struct sk_buff *skb;
  size_t copied;
  char poll[96];

  (void)flags;

  if (sock == NULL || sock->sk == NULL || msg == NULL ||
      msg->msg_iter.data == NULL)
    {
      return -EINVAL;
    }

  pi = linux_bt_hci_pi(sock->sk);
  if (pi != NULL && pi->channel == HCI_CHANNEL_CONTROL &&
      g_hci_discovery_state.discovering)
    {
      (void)linux_bt_upstream_mgmt_poll_discovery_probe(8, poll,
                                                        sizeof(poll));
    }

  skb = skb_dequeue(&sock->sk->sk_receive_queue);
  if (skb == NULL)
    {
      return -EAGAIN;
    }

  copied = skb->len < len ? skb->len : len;
  if (copied < skb->len)
    {
      msg->msg_flags |= MSG_TRUNC;
    }

  if (copy_to_iter(skb->data, copied, &msg->msg_iter) != copied)
    {
      kfree_skb(skb);
      return -EFAULT;
    }

  kfree_skb(skb);
  g_hci_mgmt_socket_recvs++;
  return (int)copied;
}

static const struct proto_ops g_linux_bt_hci_sock_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .release = linux_bt_hci_sock_release,
  .bind = linux_bt_hci_sock_bind,
  .getname = linux_bt_hci_sock_getname,
  .sendmsg = linux_bt_hci_sock_sendmsg,
  .recvmsg = linux_bt_hci_sock_recvmsg,
  .ioctl = linux_bt_hci_sock_ioctl,
  .poll = datagram_poll,
  .listen = sock_no_listen,
  .shutdown = sock_no_shutdown,
  .setsockopt = linux_bt_hci_sock_setsockopt,
  .getsockopt = linux_bt_hci_sock_getsockopt,
  .connect = sock_no_connect,
  .socketpair = sock_no_socketpair,
  .accept = sock_no_accept,
  .mmap = sock_no_mmap,
};

static int linux_bt_hci_sock_create(struct net *net, struct socket *sock,
                                    int protocol, int kern)
{
  struct sock *sk;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  if (sock->type != SOCK_RAW)
    {
      return -ESOCKTNOSUPPORT;
    }

  sk = bt_sock_alloc(net, sock, &g_linux_bt_hci_sock_proto, protocol,
                     GFP_KERNEL, kern);
  if (sk == NULL)
    {
      return -ENOMEM;
    }

  sock->ops = &g_linux_bt_hci_sock_ops;
  sock->state = SS_UNCONNECTED;
  linux_bt_hci_filter_init(&linux_bt_hci_pi(sk)->filter);
  return 0;
}

static const struct net_proto_family g_linux_bt_hci_sock_family_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .create = linux_bt_hci_sock_create,
};
#endif

int hci_sock_init(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_HCI_SOCK_LEGACY_DIAGNOSTIC
  int ret;

  ret = proto_register(&g_linux_bt_hci_sock_proto, 0);
  if (ret < 0)
    {
      return ret;
    }

  g_hci_sock_proto_registered = true;
  ret = bt_sock_register(BTPROTO_HCI, &g_linux_bt_hci_sock_family_ops);
  if (ret < 0)
    {
      proto_unregister(&g_linux_bt_hci_sock_proto);
      g_hci_sock_proto_registered = false;
    }

  return ret;
#elif defined(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF)
  return -ENOSYS;
#else
  return 0;
#endif
}

void hci_sock_cleanup(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_HCI_SOCK_LEGACY_DIAGNOSTIC
  bt_sock_unregister(BTPROTO_HCI);
  if (g_hci_sock_proto_registered)
    {
      proto_unregister(&g_linux_bt_hci_sock_proto);
      g_hci_sock_proto_registered = false;
    }
#endif
}
#endif

#if defined(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF) && \
    defined(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_SOCK)
static int linux_bt_hci_mgmt_broadcast_device_found(uint16_t dev_id,
                                                    uint16_t src,
                                                    const char *name,
                                                    struct sock *skip)
{
  (void)dev_id;
  (void)src;
  (void)name;
  (void)skip;
  return 0;
}
#endif

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
static int linux_bt_hci_mgmt_broadcast_device_found(uint16_t dev_id,
                                                    uint16_t src,
                                                    const char *name,
                                                    struct sock *skip)
{
  (void)dev_id;
  (void)src;
  (void)name;
  (void)skip;
  return 0;
}
#endif

#if defined(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_SOCK) || \
    !defined(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF)
int linux_bt_upstream_hci_sock_recv(struct hci_dev *hdev, uint8_t pkt_type,
                                    const void *payload,
                                    size_t payload_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_SOCK
  struct sk_buff *skb;

  if (hdev == NULL || (payload == NULL && payload_len > 0))
    {
      return -EINVAL;
    }

  skb = bt_skb_alloc(payload_len, GFP_KERNEL);
  if (skb == NULL)
    {
      return -ENOMEM;
    }

  hci_skb_pkt_type(skb) = pkt_type;
  bt_cb(skb)->incoming = 1;
  if (payload_len > 0)
    {
      skb_put_data(skb, payload, payload_len);
    }

  hci_send_to_monitor(hdev, skb);
  hci_send_to_sock(hdev, skb);
  kfree_skb(skb);
#else
  (void)hdev;
  (void)pkt_type;
  (void)payload;
  (void)payload_len;
#endif
  return 0;
}
#endif

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_L2CAP
#ifdef CONFIG_NET_LINUX_BLUETOOTH_L2CAP_LEGACY_DIAGNOSTIC
static struct proto g_linux_bt_l2cap_sock_proto =
{
  .name = "L2CAP",
  .obj_size = sizeof(struct l2cap_pinfo),
};

static int linux_bt_l2cap_sock_release(struct socket *sock)
{
  if (sock == NULL)
    {
      return 0;
    }

  if (sock->sk != NULL)
    {
      if (sock->ops != NULL && sock->ops->shutdown != NULL)
        {
          (void)sock->ops->shutdown(sock, SHUT_RDWR);
        }

      linux_bt_l2cap_clear_bound_state_by_sk(sock->sk);
      sock_orphan(sock->sk);
      sock_put(sock->sk);
      sock->sk = NULL;
    }

  return 0;
}

static int linux_bt_l2cap_sock_bind(struct socket *sock,
                                    struct sockaddr *addr,
                                    int addr_len)
{
  struct sockaddr_l2 *laddr;
  unsigned int i;

  if (sock == NULL || sock->sk == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  if (addr_len < (int)sizeof(*laddr))
    {
      return -EINVAL;
    }

  laddr = (struct sockaddr_l2 *)addr;
  if (laddr->l2_family != AF_BLUETOOTH)
    {
      return -EAFNOSUPPORT;
    }

  if (sock->sk->sk_state == BT_BOUND ||
      sock->sk->sk_state == BT_CONNECTED ||
      sock->sk->sk_state == BT_LISTEN)
    {
      return -EALREADY;
    }

  if (laddr->l2_psm == 0 && laddr->l2_cid == 0)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(g_l2cap_bound_socks) /
                  sizeof(g_l2cap_bound_socks[0]); i++)
    {
      if (g_l2cap_bound_socks[i].sk == NULL)
        {
          g_l2cap_bound_socks[i].sk = sock->sk;
          g_l2cap_bound_socks[i].psm = le16_to_cpu(laddr->l2_psm);
          g_l2cap_bound_socks[i].cid = le16_to_cpu(laddr->l2_cid);
          g_l2cap_bound_socks[i].bdaddr_type = laddr->l2_bdaddr_type;
          bacpy(&g_l2cap_bound_socks[i].bdaddr, &laddr->l2_bdaddr);
          g_l2cap_bound_socks[i].imtu = LINUX_BT_L2CAP_ACL_MTU;
          g_l2cap_bound_socks[i].omtu = LINUX_BT_L2CAP_ACL_MTU;
          g_l2cap_bound_socks[i].lm = 0;
          g_l2cap_bound_socks[i].channel_policy =
            BT_CHANNEL_POLICY_BREDR_ONLY;
          g_l2cap_bound_socks[i].phy = BT_PHY_BR_1M_1SLOT;
          g_l2cap_bound_socks[i].sec_level = BT_SECURITY_LOW;
          sock->sk->sk_state = BT_BOUND;
          g_l2cap_socket_binds++;
          return 0;
        }
    }

  return -ENOMEM;
}

static int linux_bt_l2cap_sock_connect(struct socket *sock,
                                       struct sockaddr *addr,
                                       int addr_len,
                                       int flags)
{
  struct sockaddr_l2 *laddr;
  struct linux_bt_l2cap_bind_state *state;
  int ret;

  (void)flags;

  if (sock == NULL || sock->sk == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  if (addr_len < (int)sizeof(*laddr))
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state != BT_BOUND)
    {
      return -ENOTCONN;
    }

  laddr = (struct sockaddr_l2 *)addr;
  if (laddr->l2_family != AF_BLUETOOTH)
    {
      return -EAFNOSUPPORT;
    }

  state = linux_bt_l2cap_bound_state(sock->sk);
  if (state == NULL)
    {
      return -ENOTCONN;
    }

  if (laddr->l2_psm != 0)
    {
      state->psm = le16_to_cpu(laddr->l2_psm);
    }

  if (laddr->l2_cid != 0)
    {
      state->cid = le16_to_cpu(laddr->l2_cid);
    }

  state->bdaddr_type = laddr->l2_bdaddr_type;
  bacpy(&state->bdaddr, &laddr->l2_bdaddr);
  sock->sk->sk_state = BT_CONNECTED;
  ret = linux_bt_hci_ensure_connected_handle(
    state->bdaddr_type != 0 ? LE_LINK : ACL_LINK, state->handle,
    &state->bdaddr, state->bdaddr_type, true, 0);
  if (ret < 0)
    {
      return ret;
    }

  g_l2cap_socket_connects++;
  return 0;
}

static int linux_bt_l2cap_sock_listen(struct socket *sock, int backlog)
{
  struct linux_bt_l2cap_bind_state *state;

  if (sock == NULL || sock->sk == NULL)
    {
      return -EINVAL;
    }

  if (backlog < 0)
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state != BT_BOUND &&
      sock->sk->sk_state != BT_CONNECTED)
    {
      return -ENOTCONN;
    }

  state = linux_bt_l2cap_bound_state(sock->sk);
  if (state == NULL || (state->psm == 0 && state->cid == 0))
    {
      return -ENOTCONN;
    }

  sock->sk->sk_state = BT_LISTEN;
  g_l2cap_socket_listens++;
  return 0;
}

static int linux_bt_l2cap_sock_shutdown(struct socket *sock, int how)
{
  struct sock *sk;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  sk = sock->sk;
  if (sk == NULL)
    {
      return 0;
    }

  switch (how)
    {
      case SHUT_RD:
        if (sk->sk_shutdown & RCV_SHUTDOWN)
          {
            return 0;
          }

        sk->sk_shutdown |= RCV_SHUTDOWN;
        break;

      case SHUT_WR:
        if (sk->sk_shutdown & SEND_SHUTDOWN)
          {
            return 0;
          }

        sk->sk_shutdown |= SEND_SHUTDOWN;
        break;

      case SHUT_RDWR:
        if ((sk->sk_shutdown & SHUTDOWN_MASK) == SHUTDOWN_MASK)
          {
            return 0;
          }

        sk->sk_shutdown |= SHUTDOWN_MASK;
        break;

      default:
        return -EINVAL;
    }

  if ((sk->sk_shutdown & SHUTDOWN_MASK) == SHUTDOWN_MASK)
    {
      sk->sk_state = BT_CLOSED;
      sock->state = SS_UNCONNECTED;
    }

  g_l2cap_socket_shutdowns++;
  return sk->sk_err != 0 ? -sk->sk_err : 0;
}

static int linux_bt_l2cap_sock_getname(struct socket *sock,
                                       struct sockaddr *addr,
                                       int peer)
{
  struct linux_bt_l2cap_bind_state *state;
  struct sockaddr_l2 *laddr = (struct sockaddr_l2 *)addr;

  (void)peer;

  if (sock == NULL || sock->sk == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  state = linux_bt_l2cap_bound_state(sock->sk);
  if (state == NULL)
    {
      return -ENOTCONN;
    }

  memset(laddr, 0, sizeof(*laddr));
  laddr->l2_family = AF_BLUETOOTH;
  laddr->l2_psm = cpu_to_le16(state->psm);
  laddr->l2_cid = cpu_to_le16(state->cid);
  laddr->l2_bdaddr_type = state->bdaddr_type;
  bacpy(&laddr->l2_bdaddr, &state->bdaddr);
  return sizeof(*laddr);
}

static int linux_bt_l2cap_sock_setsockopt(struct socket *sock, int level,
                                          int optname, char *optval,
                                          unsigned int optlen)
{
  struct linux_bt_l2cap_bind_state *state;
  struct l2cap_options opts;
  struct bt_power pwr;
  struct bt_security sec;
  uint32_t phys;
  uint32_t opt;
  uint16_t mtu;
  uint8_t mode;

  if (sock == NULL || sock->sk == NULL || optval == NULL)
    {
      return -EINVAL;
    }

  state = linux_bt_l2cap_ensure_bound_state(sock->sk);
  if (state == NULL)
    {
      return -ENOMEM;
    }

  if (level == SOL_L2CAP)
    {
      switch (optname)
        {
          case L2CAP_OPTIONS:
            if (sock->sk->sk_state == BT_CONNECTED)
              {
                return -EINVAL;
              }

            if (optlen < sizeof(opts))
              {
                return -EINVAL;
              }

            memcpy(&opts, optval, sizeof(opts));
            if ((opts.imtu != 0 && opts.imtu < L2CAP_DEFAULT_MIN_MTU) ||
                (opts.omtu != 0 && opts.omtu < L2CAP_DEFAULT_MIN_MTU) ||
                opts.txwin_size > L2CAP_DEFAULT_EXT_WINDOW)
              {
                return -EINVAL;
              }

            switch (opts.mode)
              {
                case L2CAP_MODE_BASIC:
                case L2CAP_MODE_ERTM:
                case L2CAP_MODE_STREAMING:
                  break;

                default:
                  return -EINVAL;
              }

            state->imtu = opts.imtu != 0 ? opts.imtu :
                          L2CAP_DEFAULT_MTU;
            state->omtu = opts.omtu != 0 ? opts.omtu :
                          L2CAP_DEFAULT_MTU;
            state->flush_to = opts.flush_to;
            state->mode = opts.mode;
            state->fcs = opts.fcs;
            state->max_tx = opts.max_tx;
            state->txwin_size = opts.txwin_size;
            return 0;

          case L2CAP_LM:
            if (optlen < sizeof(opt))
              {
                return -EINVAL;
              }

            memcpy(&opt, optval, sizeof(opt));
            if ((opt & L2CAP_LM_FIPS) != 0)
              {
                return -EINVAL;
              }

            state->lm = opt;
            if ((opt & L2CAP_LM_SECURE) != 0)
              {
                state->sec_level = BT_SECURITY_HIGH;
              }
            else if ((opt & L2CAP_LM_ENCRYPT) != 0)
              {
                state->sec_level = BT_SECURITY_MEDIUM;
              }
            else if ((opt & L2CAP_LM_AUTH) != 0)
              {
                state->sec_level = BT_SECURITY_LOW;
              }

            return 0;

          default:
            return -ENOPROTOOPT;
        }
    }

  if (level != SOL_BLUETOOTH)
    {
      return -ENOPROTOOPT;
    }

  switch (optname)
    {
      case BT_SECURITY:
        if (optlen < sizeof(sec))
          {
            return -EINVAL;
          }

        memcpy(&sec, optval, sizeof(sec));
        if (sec.level < BT_SECURITY_LOW ||
            sec.level > BT_SECURITY_FIPS)
          {
            return -EINVAL;
          }

        state->sec_level = sec.level;
        return 0;

      case BT_DEFER_SETUP:
        if (sock->sk->sk_state != BT_BOUND &&
            sock->sk->sk_state != BT_LISTEN)
          {
            return -EINVAL;
          }

        if (optlen < sizeof(opt))
          {
            return -EINVAL;
          }

        memcpy(&opt, optval, sizeof(opt));
        state->defer_setup = opt != 0;
        return 0;

      case BT_FLUSHABLE:
        if (optlen < sizeof(opt))
          {
            return -EINVAL;
          }

        memcpy(&opt, optval, sizeof(opt));
        if (opt > BT_FLUSHABLE_ON)
          {
            return -EINVAL;
          }

        if (opt == BT_FLUSHABLE_OFF &&
            sock->sk->sk_state != BT_CONNECTED)
          {
            return -EINVAL;
          }

        state->flushable = opt != 0;
        return 0;

      case BT_POWER:
        if (optlen < sizeof(pwr))
          {
            return -EINVAL;
          }

        memcpy(&pwr, optval, sizeof(pwr));
        state->force_active = pwr.force_active != 0;
        return 0;

      case BT_CHANNEL_POLICY:
        if (optlen < sizeof(opt))
          {
            return -EINVAL;
          }

        memcpy(&opt, optval, sizeof(opt));
        (void)opt;
        return -EOPNOTSUPP;

      case BT_MODE:
        if (sock->sk->sk_state != BT_BOUND)
          {
            return -EINVAL;
          }

        if (optlen < sizeof(mode))
          {
            return -EINVAL;
          }

        memcpy(&mode, optval, sizeof(mode));
        switch (mode)
          {
            case BT_MODE_BASIC:
              state->mode = L2CAP_MODE_BASIC;
              return 0;

            case BT_MODE_ERTM:
              state->mode = L2CAP_MODE_ERTM;
              return 0;

            case BT_MODE_STREAMING:
              state->mode = L2CAP_MODE_STREAMING;
              return 0;

            case BT_MODE_LE_FLOWCTL:
              state->mode = L2CAP_MODE_LE_FLOWCTL;
              return 0;

            case BT_MODE_EXT_FLOWCTL:
              state->mode = L2CAP_MODE_EXT_FLOWCTL;
              return 0;

            default:
              return -EINVAL;
          }

      case BT_SNDMTU:
        if (state->bdaddr_type == 0)
          {
            return -EINVAL;
          }

        if (sock->sk->sk_state == BT_CONNECTED)
          {
            return -EISCONN;
          }

        if (optlen < sizeof(mtu))
          {
            return -EINVAL;
          }

        memcpy(&mtu, optval, sizeof(mtu));
        if (mtu != 0 && mtu < L2CAP_DEFAULT_MIN_MTU)
          {
            return -EINVAL;
          }

        state->omtu = mtu != 0 ? mtu : L2CAP_DEFAULT_MTU;
        return 0;

      case BT_RCVMTU:
        if (state->bdaddr_type == 0)
          {
            return -EINVAL;
          }

        if (state->mode == L2CAP_MODE_LE_FLOWCTL &&
            sock->sk->sk_state == BT_CONNECTED)
          {
            return -EISCONN;
          }

        if (optlen < sizeof(mtu))
          {
            return -EINVAL;
          }

        memcpy(&mtu, optval, sizeof(mtu));
        if (mtu != 0 && mtu < L2CAP_DEFAULT_MIN_MTU)
          {
            return -EINVAL;
          }

        state->imtu = mtu != 0 ? mtu : L2CAP_DEFAULT_MTU;
        return 0;

      case BT_PHY:
        if (sock->sk->sk_state != BT_CONNECTED)
          {
            return -ENOTCONN;
          }

        if (optlen < sizeof(phys))
          {
            return -EINVAL;
          }

        memcpy(&phys, optval, sizeof(phys));
        if (state->bdaddr_type != 0)
          {
            if ((phys & ~BT_PHY_LE_MASK) != 0)
              {
                return -EINVAL;
              }
          }
        else if ((phys & ~BT_PHY_BREDR_MASK) != 0)
          {
            return -EINVAL;
          }

        state->phy = phys != 0 ? phys :
                     state->bdaddr_type != 0 ?
                     (BT_PHY_LE_1M_TX | BT_PHY_LE_1M_RX) :
                     (BT_PHY_BR_1M_1SLOT |
                      BT_PHY_EDR_2M_1SLOT |
                      BT_PHY_EDR_2M_3SLOT |
                      BT_PHY_EDR_2M_5SLOT |
                      BT_PHY_EDR_3M_1SLOT |
                      BT_PHY_EDR_3M_3SLOT |
                      BT_PHY_EDR_3M_5SLOT);
        return 0;

      default:
        return -ENOPROTOOPT;
    }
}

static uint8_t linux_bt_l2cap_sock_bt_mode(
  const struct linux_bt_l2cap_bind_state *state)
{
  if (state == NULL)
    {
      return BT_MODE_BASIC;
    }

  switch (state->mode)
    {
      case L2CAP_MODE_ERTM:
        return BT_MODE_ERTM;

      case L2CAP_MODE_STREAMING:
        return BT_MODE_STREAMING;

      case L2CAP_MODE_LE_FLOWCTL:
        return BT_MODE_LE_FLOWCTL;

      case L2CAP_MODE_EXT_FLOWCTL:
        return BT_MODE_EXT_FLOWCTL;

      case L2CAP_MODE_BASIC:
      default:
        return BT_MODE_BASIC;
    }
}

static int linux_bt_sock_copy_opt(char *optval, int *optlen,
                                  const void *src, size_t srclen)
{
  size_t copy_len;

  if (optval == NULL || optlen == NULL || src == NULL || *optlen < 0)
    {
      return -EINVAL;
    }

  copy_len = (size_t)*optlen < srclen ? (size_t)*optlen : srclen;
  memcpy(optval, src, copy_len);
  *optlen = (int)copy_len;
  return 0;
}

static int linux_bt_l2cap_sock_getsockopt(struct socket *sock, int level,
                                          int optname, char *optval,
                                          int *optlen)
{
  struct linux_bt_l2cap_bind_state *state;
  struct l2cap_options opts;
  struct l2cap_conninfo cinfo;
  struct bt_power pwr;
  struct bt_security sec;
  uint32_t phys;
  uint32_t opt;
  uint16_t mtu;
  uint8_t mode;

  if (sock == NULL || sock->sk == NULL || optval == NULL ||
      optlen == NULL)
    {
      return -EINVAL;
    }

  state = linux_bt_l2cap_ensure_bound_state(sock->sk);
  if (state == NULL)
    {
      return -ENOMEM;
    }

  if (level == SOL_L2CAP)
    {
      switch (optname)
        {
          case L2CAP_OPTIONS:
            memset(&opts, 0, sizeof(opts));
            opts.imtu = state->imtu != 0 ? state->imtu :
                        L2CAP_DEFAULT_MTU;
            opts.omtu = state->omtu != 0 ? state->omtu :
                        L2CAP_DEFAULT_MTU;
            opts.flush_to = state->flush_to;
            opts.mode = state->mode;
            opts.fcs = state->fcs;
            opts.max_tx = state->max_tx;
            opts.txwin_size = state->txwin_size;
            return linux_bt_sock_copy_opt(optval, optlen, &opts,
                                          sizeof(opts));

          case L2CAP_LM:
            opt = state->lm;
            return linux_bt_sock_copy_opt(optval, optlen, &opt,
                                          sizeof(opt));

          case L2CAP_CONNINFO:
            if (sock->sk->sk_state != BT_CONNECTED)
              {
                return -ENOTCONN;
              }

            memset(&cinfo, 0, sizeof(cinfo));
            cinfo.hci_handle = state->handle;
            return linux_bt_sock_copy_opt(optval, optlen, &cinfo,
                                          sizeof(cinfo));

          default:
            return -ENOPROTOOPT;
        }
    }

  if (level != SOL_BLUETOOTH)
    {
      return -ENOPROTOOPT;
    }

  switch (optname)
    {
      case BT_SECURITY:
        memset(&sec, 0, sizeof(sec));
        sec.level = state->sec_level != 0 ? state->sec_level :
                    BT_SECURITY_LOW;
        return linux_bt_sock_copy_opt(optval, optlen, &sec, sizeof(sec));

      case BT_DEFER_SETUP:
        opt = state->defer_setup ? 1 : 0;
        return linux_bt_sock_copy_opt(optval, optlen, &opt, sizeof(opt));

      case BT_FLUSHABLE:
        opt = state->flushable ? 1 : 0;
        return linux_bt_sock_copy_opt(optval, optlen, &opt, sizeof(opt));

      case BT_POWER:
        memset(&pwr, 0, sizeof(pwr));
        pwr.force_active = state->force_active ?
                           BT_POWER_FORCE_ACTIVE_ON :
                           BT_POWER_FORCE_ACTIVE_OFF;
        return linux_bt_sock_copy_opt(optval, optlen, &pwr, sizeof(pwr));

      case BT_CHANNEL_POLICY:
        opt = state->channel_policy;
        return linux_bt_sock_copy_opt(optval, optlen, &opt, sizeof(opt));

      case BT_SNDMTU:
        if (state->bdaddr_type == 0)
          {
            return -EINVAL;
          }

        if (sock->sk->sk_state != BT_CONNECTED)
          {
            return -ENOTCONN;
          }

        mtu = state->omtu != 0 ? state->omtu : L2CAP_DEFAULT_MTU;
        return linux_bt_sock_copy_opt(optval, optlen, &mtu, sizeof(mtu));

      case BT_RCVMTU:
        if (state->bdaddr_type == 0)
          {
            return -EINVAL;
          }

        mtu = state->imtu != 0 ? state->imtu : L2CAP_DEFAULT_MTU;
        return linux_bt_sock_copy_opt(optval, optlen, &mtu, sizeof(mtu));

      case BT_MODE:
        mode = linux_bt_l2cap_sock_bt_mode(state);
        return linux_bt_sock_copy_opt(optval, optlen, &mode, sizeof(mode));

      case BT_PHY:
        if (sock->sk->sk_state != BT_CONNECTED)
          {
            return -ENOTCONN;
          }

        phys = state->phy != 0 ? state->phy :
               state->bdaddr_type != 0 ?
               (BT_PHY_LE_1M_TX | BT_PHY_LE_1M_RX) :
               (BT_PHY_BR_1M_1SLOT |
                BT_PHY_EDR_2M_1SLOT |
                BT_PHY_EDR_2M_3SLOT |
                BT_PHY_EDR_2M_5SLOT |
                BT_PHY_EDR_3M_1SLOT |
                BT_PHY_EDR_3M_3SLOT |
                BT_PHY_EDR_3M_5SLOT);
        return linux_bt_sock_copy_opt(optval, optlen, &phys,
                                      sizeof(phys));

      default:
        return -ENOPROTOOPT;
    }
}

static int linux_bt_l2cap_sock_accept(struct socket *sock,
                                      struct socket *newsock,
                                      struct proto_accept_arg *arg)
{
  const struct net_proto_family *family;
  struct linux_bt_l2cap_bind_state *state;
  unsigned int i;
  int ret;

  (void)arg;

  if (sock == NULL || newsock == NULL || sock->sk == NULL ||
      sock->sk->sk_state != BT_LISTEN)
    {
      return -EINVAL;
    }

  state = linux_bt_l2cap_bound_state(sock->sk);
  if (state == NULL)
    {
      return -ENOTCONN;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  newsock->type = sock->type;
  ret = family->create(&init_net, newsock, BTPROTO_L2CAP, 0);
  if (ret < 0)
    {
      return ret;
    }

  for (i = 0; i < sizeof(g_l2cap_bound_socks) /
                  sizeof(g_l2cap_bound_socks[0]); i++)
    {
      if (g_l2cap_bound_socks[i].sk == NULL)
        {
          memcpy(&g_l2cap_bound_socks[i], state,
                 sizeof(g_l2cap_bound_socks[i]));
          g_l2cap_bound_socks[i].sk = newsock->sk;
          newsock->sk->sk_state = BT_CONNECTED;
          newsock->state = SS_CONNECTED;
          return 0;
        }
    }

  if (newsock->ops != NULL && newsock->ops->release != NULL)
    {
      (void)newsock->ops->release(newsock);
    }

  return -ENOMEM;
}

static int linux_bt_l2cap_sock_sendmsg(struct socket *sock,
                                       struct msghdr *msg,
                                       size_t len)
{
  struct linux_bt_l2cap_bind_state *state;
  uint8_t *acl;
  uint16_t acl_len;
  uint16_t handle_pb;
  size_t acl_size;
  size_t offset = 0;
  size_t chunk;
  size_t limit;
  size_t send_len;
  bool first = true;
  int ret;

  if (sock == NULL || sock->sk == NULL || msg == NULL ||
      msg->msg_iter.data == NULL)
    {
      return -EINVAL;
    }

#ifdef CONFIG_SIM_BTHWSIM
  if (sock->sk != NULL)
#else
  if (sock == &g_l2cap_probe_socket && g_l2cap_probe_bnep_refs > 0)
#endif
    {
      state = linux_bt_l2cap_ensure_bound_state(sock->sk);
      if (state != NULL)
        {
          if (state->psm == 0)
            {
              state->psm = g_l2cap_probe_psm != 0 ?
                           g_l2cap_probe_psm : 0x0019;
            }

          if (state->cid == 0)
            {
              state->cid = g_l2cap_probe_cid != 0 ?
                           g_l2cap_probe_cid : 0x0041;
            }

          if (state->handle == 0)
            {
              state->handle = g_l2cap_probe_handle != 0 ?
                              g_l2cap_probe_handle : 0x0040;
            }

          if (sock->sk->sk_state != BT_BOUND &&
              sock->sk->sk_state != BT_CONNECTED)
            {
              sock->sk->sk_state = BT_CONNECTED;
            }
        }
    }

  if (sock->sk->sk_state != BT_BOUND &&
      sock->sk->sk_state != BT_CONNECTED)
    {
      return -ENOTCONN;
    }

  if (len == 0 || len > UINT16_MAX || msg->msg_iter.count < len)
    {
      return -EMSGSIZE;
    }

  acl_size = LINUX_BT_L2CAP_ACL_MTU + 4;
  acl = kmm_malloc(acl_size);
  if (acl == NULL)
    {
      return -ENOMEM;
    }

  state = linux_bt_l2cap_bound_state(sock->sk);
  if (state == NULL || state->cid == 0)
    {
      return -ENOTCONN;
    }

#ifdef CONFIG_SIM_BTHWSIM
  ret = linux_bt_l2cap_attach_send_chan(sock, state->handle, state->cid);
  if (ret < 0)
    {
      printf("linux-bt-l2cap-sendmsg: attach ret=%d state=%d "
             "payload-len=%u cid=0x%04x handle=0x%04x\n",
             ret, sock->sk->sk_state, (unsigned int)len,
             state->cid, state->handle);
      kmm_free(acl);
      return ret;
    }
#endif

  while (offset < len)
    {
      limit = first ? LINUX_BT_L2CAP_ACL_MTU - 4 :
                      LINUX_BT_L2CAP_ACL_MTU;
      chunk = len - offset;
      if (chunk > limit)
        {
          chunk = limit;
        }

      acl_len = (uint16_t)(chunk + (first ? 4 : 0));
      handle_pb = (uint16_t)(state->handle |
                             ((first ? 0x02 : 0x01) << 12));
      acl[0] = (uint8_t)(handle_pb & 0xff);
      acl[1] = (uint8_t)(handle_pb >> 8);
      acl[2] = (uint8_t)(acl_len & 0xff);
      acl[3] = (uint8_t)(acl_len >> 8);

      if (first)
        {
          acl[4] = (uint8_t)(len & 0xff);
          acl[5] = (uint8_t)(len >> 8);
          acl[6] = (uint8_t)(state->cid & 0xff);
          acl[7] = (uint8_t)(state->cid >> 8);
          memcpy(&acl[8], (FAR const uint8_t *)msg->msg_iter.data + offset,
                 chunk);
          send_len = chunk + 8;
        }
      else
        {
          memcpy(&acl[4], (FAR const uint8_t *)msg->msg_iter.data + offset,
                 chunk);
          send_len = chunk + 4;
        }

      ret = linux_bt_upstream_hci_send(HCI_ACLDATA_PKT, acl, send_len);
      if (ret < 0)
        {
          printf("linux-bt-l2cap-sendmsg: hci-send ret=%d state=%d "
                 "payload-len=%u frag-offset=%u frag-len=%u cid=0x%04x "
                 "handle=0x%04x\n",
                 ret, sock->sk->sk_state, (unsigned int)len,
                 (unsigned int)offset, (unsigned int)chunk,
                 state->cid, state->handle);
          kmm_free(acl);
          return ret;
        }

      offset += chunk;
      first = false;
    }

  g_l2cap_socket_sends++;
  kmm_free(acl);
  return (int)len;
}

static int linux_bt_l2cap_sock_recvmsg(struct socket *sock,
                                       struct msghdr *msg,
                                       size_t len,
                                       int flags)
{
  struct sk_buff *skb;
  size_t copied;

  (void)flags;

  if (sock == NULL || sock->sk == NULL || msg == NULL)
    {
      return -EINVAL;
    }

  skb = skb_dequeue(&sock->sk->sk_receive_queue);
  if (skb == NULL)
    {
      return -EAGAIN;
    }

  copied = skb->len < len ? skb->len : len;
  if (copied < skb->len)
    {
      msg->msg_flags |= MSG_TRUNC;
    }

  if (copy_to_iter(skb->data, copied, &msg->msg_iter) != copied)
    {
      kfree_skb(skb);
      return -EFAULT;
    }

  kfree_skb(skb);
  return (int)copied;
}

static const struct proto_ops g_linux_bt_l2cap_sock_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .release = linux_bt_l2cap_sock_release,
  .bind = linux_bt_l2cap_sock_bind,
  .getname = linux_bt_l2cap_sock_getname,
  .sendmsg = linux_bt_l2cap_sock_sendmsg,
  .recvmsg = linux_bt_l2cap_sock_recvmsg,
  .poll = linux_bt_sock_poll,
  .ioctl = bt_sock_ioctl,
  .listen = linux_bt_l2cap_sock_listen,
  .shutdown = linux_bt_l2cap_sock_shutdown,
  .setsockopt = linux_bt_l2cap_sock_setsockopt,
  .getsockopt = linux_bt_l2cap_sock_getsockopt,
  .connect = linux_bt_l2cap_sock_connect,
  .socketpair = sock_no_socketpair,
  .accept = linux_bt_l2cap_sock_accept,
  .mmap = sock_no_mmap,
  .gettstamp = sock_gettstamp,
};

static int linux_bt_l2cap_sock_create(struct net *net, struct socket *sock,
                                      int protocol, int kern)
{
  struct l2cap_pinfo *pi;
  struct sock *sk;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  switch (sock->type)
    {
      case SOCK_SEQPACKET:
      case SOCK_STREAM:
      case SOCK_DGRAM:
      case SOCK_RAW:
        break;

      default:
        return -ESOCKTNOSUPPORT;
    }

  sk = bt_sock_alloc(net, sock, &g_linux_bt_l2cap_sock_proto, protocol,
                     GFP_KERNEL, kern);
  if (sk == NULL)
    {
      return -ENOMEM;
    }

  pi = l2cap_pi(sk);
  INIT_LIST_HEAD(&pi->rx_busy);
  pi->chan = NULL;

  sock->ops = &g_linux_bt_l2cap_sock_ops;
  sock->state = SS_UNCONNECTED;
  sk->sk_state = BT_OPEN;
  return 0;
}

static const struct net_proto_family g_linux_bt_l2cap_sock_family_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .create = linux_bt_l2cap_sock_create,
};
#endif

int l2cap_init(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_L2CAP_LEGACY_DIAGNOSTIC
  int ret;

  ret = proto_register(&g_linux_bt_l2cap_sock_proto, 0);
  if (ret < 0)
    {
      return ret;
    }

  g_l2cap_sock_proto_registered = true;
  ret = bt_sock_register(BTPROTO_L2CAP, &g_linux_bt_l2cap_sock_family_ops);
  if (ret < 0)
    {
      proto_unregister(&g_linux_bt_l2cap_sock_proto);
      g_l2cap_sock_proto_registered = false;
      return ret;
    }

  g_l2cap_socket_registers++;
#elif defined(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF)
  return -ENOSYS;
#endif
  return 0;
}

void l2cap_exit(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_L2CAP_LEGACY_DIAGNOSTIC
  if (g_l2cap_sock_proto_registered)
    {
      bt_sock_unregister(BTPROTO_L2CAP);
      g_l2cap_socket_unregisters++;
      proto_unregister(&g_linux_bt_l2cap_sock_proto);
      g_l2cap_sock_proto_registered = false;
    }
#endif
}
#endif

#ifdef CONFIG_NET_LINUX_BLUETOOTH_BNEP_LEGACY_DIAGNOSTIC
static struct proto g_linux_bt_bnep_sock_proto =
{
  .name = "BNEP",
  .obj_size = sizeof(struct bt_sock),
};

static int linux_bt_bnep_sock_release(struct socket *sock)
{
  if (sock == NULL)
    {
      return 0;
    }

  if (sock->sk != NULL)
    {
      sock_orphan(sock->sk);
      sock_put(sock->sk);
      sock->sk = NULL;
    }

  return 0;
}

static void linux_bt_bnep_fill_synthetic_dst(uint32_t seed,
                                             uint8_t dst[ETH_ALEN])
{
  dst[0] = (uint8_t)(seed & 0xff);
  dst[1] = (uint8_t)((seed >> 8) & 0xff);
  dst[2] = (uint8_t)((seed >> 16) & 0xff);
  dst[3] = (uint8_t)((seed >> 24) & 0xff);
  dst[4] = 0xbe;
  dst[5] = 0xef;
}

static bool linux_bt_bnep_bdaddr_is_any(const bdaddr_t *bdaddr)
{
  unsigned int i;

  if (bdaddr == NULL)
    {
      return true;
    }

  for (i = 0; i < ETH_ALEN; i++)
    {
      if (bdaddr->b[i] != 0)
        {
          return false;
        }
    }

  return true;
}

static void linux_bt_bnep_fill_l2cap_dst(
  const struct linux_bt_l2cap_bind_state *state, uint8_t dst[ETH_ALEN])
{
  unsigned int i;

  if (state == NULL)
    {
      linux_bt_bnep_fill_synthetic_dst(0, dst);
      return;
    }

  if (!linux_bt_bnep_bdaddr_is_any(&state->bdaddr))
    {
      for (i = 0; i < ETH_ALEN; i++)
        {
          dst[i] = state->bdaddr.b[ETH_ALEN - 1 - i];
        }

      return;
    }

  dst[0] = (uint8_t)(state->handle & 0xff);
  dst[1] = (uint8_t)(state->handle >> 8);
  dst[2] = (uint8_t)(state->cid & 0xff);
  dst[3] = (uint8_t)(state->psm & 0xff);
  dst[4] = 0xb1;
  dst[5] = 0x2c;
}

static void linux_bt_bnep_fill_l2cap_fd_dst(
  const struct linux_bt_sockif_l2cap_fd_state *state, uint8_t dst[ETH_ALEN])
{
  unsigned int i;

  if (state == NULL)
    {
      linux_bt_bnep_fill_synthetic_dst(0, dst);
      return;
    }

  for (i = 0; i < ETH_ALEN; i++)
    {
      if (state->bdaddr[i] != 0)
        {
          memcpy(dst, state->bdaddr, ETH_ALEN);
          return;
        }
    }

  dst[0] = (uint8_t)(state->handle & 0xff);
  dst[1] = (uint8_t)(state->handle >> 8);
  dst[2] = (uint8_t)(state->cid & 0xff);
  dst[3] = (uint8_t)(state->psm & 0xff);
  dst[4] = 0xb1;
  dst[5] = 0x2c;
}

static struct linux_bt_l2cap_bind_state *
linux_bt_bnep_kept_l2cap_state(void)
{
  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.sk == NULL)
    {
      return NULL;
    }

  return linux_bt_l2cap_bound_state(g_l2cap_probe_socket.sk);
}

static bool linux_bt_bnep_dst_equal(const uint8_t a[ETH_ALEN],
                                    const uint8_t b[ETH_ALEN])
{
  return memcmp(a, b, ETH_ALEN) == 0;
}

static struct linux_bt_bnep_core_session *
linux_bt_bnep_find_session_by_dst(const uint8_t dst[ETH_ALEN])
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_BNEP_CORE_MAX_SESSIONS; i++)
    {
      if (g_bnep_core_sessions[i].used &&
          linux_bt_bnep_dst_equal(g_bnep_core_sessions[i].ci.dst, dst))
        {
          return &g_bnep_core_sessions[i];
        }
    }

  return NULL;
}

static struct linux_bt_bnep_core_session *
linux_bt_bnep_find_free_session(void)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_BNEP_CORE_MAX_SESSIONS; i++)
    {
      if (!g_bnep_core_sessions[i].used)
        {
          return &g_bnep_core_sessions[i];
        }
    }

  return NULL;
}

static uint32_t linux_bt_bnep_copy_connlist(
  struct linux_bt_bnep_conninfo *ci, uint32_t max)
{
  uint32_t copied = 0;
  unsigned int i;

  if (ci == NULL || max == 0)
    {
      return 0;
    }

  for (i = 0; i < LINUX_BT_BNEP_CORE_MAX_SESSIONS && copied < max; i++)
    {
      if (g_bnep_core_sessions[i].used)
        {
          ci[copied] = g_bnep_core_sessions[i].ci;
          copied++;
        }
    }

  return copied;
}

static int linux_bt_bnep_sock_ioctl(struct socket *sock,
                                    unsigned int cmd,
                                    unsigned long arg)
{
  (void)sock;

  switch (cmd)
    {
      case BNEPGETSUPPFEAT:
        {
          uint32_t supp_feat = BIT(LINUX_BT_BNEP_SETUP_RESPONSE);
          uint32_t *user = (uint32_t *)arg;

          g_bnep_socket_ioctl_suppfeat++;
          if (user == NULL)
            {
              return -EFAULT;
            }

          *user = supp_feat;
          return 0;
        }

      case BNEPGETCONNLIST:
        {
          struct linux_bt_bnep_connlist_req *req =
            (struct linux_bt_bnep_connlist_req *)arg;
          uint32_t max;

          g_bnep_socket_ioctl_connlist++;
          if (req == NULL)
            {
              return -EFAULT;
            }

          if (req->cnum == 0 || req->ci == NULL)
            {
              return -EINVAL;
            }

          max = req->cnum;
          req->cnum = linux_bt_bnep_copy_connlist(req->ci, max);
          return 0;
        }

      case BNEPGETCONNINFO:
        {
          struct linux_bt_bnep_conninfo *ci =
            (struct linux_bt_bnep_conninfo *)arg;
          struct linux_bt_bnep_core_session *session;

          g_bnep_socket_ioctl_conninfo++;
          if (ci == NULL)
            {
              return -EFAULT;
            }

          session = linux_bt_bnep_find_session_by_dst(ci->dst);
          if (session == NULL)
            {
              return -ENOENT;
            }

          *ci = session->ci;
          return 0;
        }

      case BNEPCONNADD:
        {
          struct linux_bt_bnep_connadd_req *req =
            (struct linux_bt_bnep_connadd_req *)arg;
          struct linux_bt_bnep_core_session *session;
          struct linux_bt_l2cap_bind_state *l2_state = NULL;
          struct linux_bt_sockif_l2cap_fd_state fd_state;
          uint8_t dst[ETH_ALEN];
          bool l2cap_backed = false;
          int fd_ret;

          g_bnep_socket_ioctl_connadd++;
          if (req == NULL)
            {
              return -EFAULT;
            }

          if ((uint32_t)req->sock == LINUX_BT_BNEP_CONNADD_KEPT_L2CAP)
            {
              l2_state = linux_bt_bnep_kept_l2cap_state();
              if (l2_state == NULL || l2_state->sk == NULL ||
                  l2_state->sk->sk_state != BT_CONNECTED)
                {
                  return -EBADFD;
                }

              linux_bt_bnep_fill_l2cap_dst(l2_state, dst);
              l2cap_backed = true;
            }
          else
            {
              if (req->sock < 0)
                {
                  return -EBADFD;
                }

              fd_ret = linux_bt_sockif_l2cap_state_from_fd(req->sock,
                                                           &fd_state);
              if (fd_ret < 0)
                {
                  return fd_ret;
                }

              linux_bt_bnep_fill_l2cap_fd_dst(&fd_state, dst);
              l2cap_backed = true;
            }

          session = linux_bt_bnep_find_session_by_dst(dst);
          if (session != NULL)
            {
              return -EALREADY;
            }

          session = linux_bt_bnep_find_free_session();
          if (session == NULL)
            {
              return -ENOSPC;
            }

          memset(session, 0, sizeof(*session));
          session->used = true;
          session->l2cap_backed = l2cap_backed;
          session->sock = req->sock;
          if (l2cap_backed && l2_state != NULL)
            {
              session->psm = l2_state->psm;
              session->cid = l2_state->cid;
              session->handle = l2_state->handle;
              session->l2cap_state = l2_state->sk->sk_state;
            }
          else if (l2cap_backed)
            {
              session->psm = fd_state.psm;
              session->cid = fd_state.cid;
              session->handle = fd_state.handle;
              session->l2cap_state = BT_CONNECTED;
            }

          session->ci.flags = req->flags;
          session->ci.role = req->role != 0 ? req->role :
                             LINUX_BT_BNEP_SVC_PANU;
          session->ci.state = LINUX_BT_BNEP_CORE_STATE_CONNECTED;
          memcpy(session->ci.dst, dst, ETH_ALEN);
          if (req->device[0] != '\0')
            {
              snprintf(session->ci.device, sizeof(session->ci.device),
                       "%s", req->device);
            }
          else
            {
              if (l2cap_backed)
                {
                  snprintf(session->ci.device, sizeof(session->ci.device),
                           "btn%d");
                }
              else
                {
                  snprintf(session->ci.device, sizeof(session->ci.device),
                           "btn%d");
                }
            }

          linux_bt_bnep_core_netdev_register(session);
          snprintf(req->device, sizeof(req->device), "%s",
                   session->ci.device);
          g_bnep_core_session_adds++;
          return 0;
        }

      case BNEPCONNDEL:
        {
          struct linux_bt_bnep_conndel_req *req =
            (struct linux_bt_bnep_conndel_req *)arg;
          struct linux_bt_bnep_core_session *session;

          g_bnep_socket_ioctl_conndel++;
          if (req == NULL)
            {
              return -EFAULT;
            }

          session = linux_bt_bnep_find_session_by_dst(req->dst);
          if (session == NULL)
            {
              return -ENOENT;
            }

          linux_bt_bnep_core_netdev_unregister(session);
          memset(session, 0, sizeof(*session));
          g_bnep_core_session_dels++;
          return 0;
        }

      default:
        return -EINVAL;
    }
}

static const struct proto_ops g_linux_bt_bnep_sock_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .release = linux_bt_bnep_sock_release,
  .ioctl = linux_bt_bnep_sock_ioctl,
  .poll = datagram_poll,
  .bind = sock_no_bind,
  .getname = sock_no_getname,
  .sendmsg = sock_no_sendmsg,
  .recvmsg = sock_no_recvmsg,
  .listen = sock_no_listen,
  .shutdown = sock_no_shutdown,
  .connect = sock_no_connect,
  .socketpair = sock_no_socketpair,
  .accept = sock_no_accept,
  .mmap = sock_no_mmap,
};

static int linux_bt_bnep_sock_create(struct net *net, struct socket *sock,
                                     int protocol, int kern)
{
  struct sock *sk;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  if (sock->type != SOCK_RAW)
    {
      return -ESOCKTNOSUPPORT;
    }

  sk = bt_sock_alloc(net, sock, &g_linux_bt_bnep_sock_proto, protocol,
                     GFP_KERNEL, kern);
  if (sk == NULL)
    {
      return -ENOMEM;
    }

  sock->ops = &g_linux_bt_bnep_sock_ops;
  sock->state = SS_UNCONNECTED;
  return 0;
}

static const struct net_proto_family g_linux_bt_bnep_sock_family_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .create = linux_bt_bnep_sock_create,
};

int bnep_sock_init(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
  int ret;

  ret = proto_register(&g_linux_bt_bnep_sock_proto, 0);
  if (ret < 0)
    {
      return ret;
    }

  g_bnep_sock_proto_registered = true;
  ret = bt_sock_register(BTPROTO_BNEP, &g_linux_bt_bnep_sock_family_ops);
  if (ret < 0)
    {
      proto_unregister(&g_linux_bt_bnep_sock_proto);
      g_bnep_sock_proto_registered = false;
      return ret;
    }

  g_bnep_socket_registers++;
#endif
  return 0;
}

void bnep_sock_cleanup(void)
{
  if (g_bnep_sock_proto_registered)
    {
      bt_sock_unregister(BTPROTO_BNEP);
      g_bnep_socket_unregisters++;
      proto_unregister(&g_linux_bt_bnep_sock_proto);
      g_bnep_sock_proto_registered = false;
    }
}
#endif /* CONFIG_NET_LINUX_BLUETOOTH_BNEP_LEGACY_DIAGNOSTIC */

#if !defined(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP) && \
    !defined(CONFIG_NET_LINUX_BLUETOOTH_BNEP_LEGACY_DIAGNOSTIC)
int bnep_sock_init(void)
{
  return 0;
}

void bnep_sock_cleanup(void)
{
}
#endif

#if !defined(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP) && \
    !defined(CONFIG_NET_LINUX_BLUETOOTH_BNEP_LEGACY_DIAGNOSTIC)
static void linux_bt_bnep_fill_synthetic_dst(uint32_t seed,
                                             uint8_t dst[ETH_ALEN])
{
  dst[0] = (uint8_t)(seed & 0xff);
  dst[1] = (uint8_t)((seed >> 8) & 0xff);
  dst[2] = (uint8_t)((seed >> 16) & 0xff);
  dst[3] = (uint8_t)((seed >> 24) & 0xff);
  dst[4] = 0xbe;
  dst[5] = 0xef;
}

static bool linux_bt_bnep_bdaddr_is_any(const bdaddr_t *bdaddr)
{
  unsigned int i;

  if (bdaddr == NULL)
    {
      return true;
    }

  for (i = 0; i < ETH_ALEN; i++)
    {
      if (bdaddr->b[i] != 0)
        {
          return false;
        }
    }

  return true;
}

static void linux_bt_bnep_fill_l2cap_dst(
  const struct linux_bt_l2cap_bind_state *state, uint8_t dst[ETH_ALEN])
{
  unsigned int i;

  if (state == NULL)
    {
      linux_bt_bnep_fill_synthetic_dst(0, dst);
      return;
    }

  if (!linux_bt_bnep_bdaddr_is_any(&state->bdaddr))
    {
      for (i = 0; i < ETH_ALEN; i++)
        {
          dst[i] = state->bdaddr.b[ETH_ALEN - 1 - i];
        }

      return;
    }

  dst[0] = (uint8_t)(state->handle & 0xff);
  dst[1] = (uint8_t)(state->handle >> 8);
  dst[2] = (uint8_t)(state->cid & 0xff);
  dst[3] = (uint8_t)(state->psm & 0xff);
  dst[4] = 0xb1;
  dst[5] = 0x2c;
}

static struct linux_bt_l2cap_bind_state *
linux_bt_bnep_kept_l2cap_state(void)
{
  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.sk == NULL)
    {
      return NULL;
    }

  return linux_bt_l2cap_bound_state(g_l2cap_probe_socket.sk);
}
#endif

static struct proto g_linux_bt_cmtp_sock_proto =
{
  .name = "CMTP",
  .obj_size = sizeof(struct bt_sock),
};

static int linux_bt_cmtp_sock_release(struct socket *sock)
{
  if (sock == NULL)
    {
      return 0;
    }

  if (sock->sk != NULL)
    {
      sock_orphan(sock->sk);
      sock_put(sock->sk);
      sock->sk = NULL;
    }

  return 0;
}

static void linux_bt_cmtp_fill_bdaddr(uint16_t seed, bdaddr_t *bdaddr)
{
  if (bdaddr == NULL)
    {
      return;
    }

  bdaddr->b[0] = (uint8_t)(seed & 0xff);
  bdaddr->b[1] = (uint8_t)(seed >> 8);
  bdaddr->b[2] = 0xc4;
  bdaddr->b[3] = 0x17;
  bdaddr->b[4] = 0x00;
  bdaddr->b[5] = 0x48;
}

static bool linux_bt_cmtp_bdaddr_equal(const bdaddr_t *a,
                                       const bdaddr_t *b)
{
  return a != NULL && b != NULL && memcmp(a->b, b->b, ETH_ALEN) == 0;
}

static struct linux_bt_cmtp_core_session *
linux_bt_cmtp_find_session_by_bdaddr(const bdaddr_t *bdaddr)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_CMTP_CORE_MAX_SESSIONS; i++)
    {
      if (g_cmtp_core_sessions[i].used &&
          linux_bt_cmtp_bdaddr_equal(&g_cmtp_core_sessions[i].ci.bdaddr,
                                     bdaddr))
        {
          return &g_cmtp_core_sessions[i];
        }
    }

  return NULL;
}

static struct linux_bt_cmtp_core_session *
linux_bt_cmtp_find_free_session(void)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_CMTP_CORE_MAX_SESSIONS; i++)
    {
      if (!g_cmtp_core_sessions[i].used)
        {
          return &g_cmtp_core_sessions[i];
        }
    }

  return NULL;
}

static unsigned int linux_bt_cmtp_core_active_count(void)
{
  unsigned int active = 0;
  unsigned int i;

  for (i = 0; i < LINUX_BT_CMTP_CORE_MAX_SESSIONS; i++)
    {
      if (g_cmtp_core_sessions[i].used)
        {
          active++;
        }
    }

  return active;
}

static uint32_t linux_bt_cmtp_copy_connlist(
  struct linux_bt_cmtp_conninfo *ci, uint32_t max)
{
  uint32_t copied = 0;
  unsigned int i;

  if (ci == NULL || max == 0)
    {
      return 0;
    }

  for (i = 0; i < LINUX_BT_CMTP_CORE_MAX_SESSIONS && copied < max; i++)
    {
      if (g_cmtp_core_sessions[i].used)
        {
          ci[copied] = g_cmtp_core_sessions[i].ci;
          copied++;
        }
    }

  return copied;
}

static int linux_bt_cmtp_sock_ioctl(struct socket *sock,
                                    unsigned int cmd,
                                    unsigned long arg)
{
  (void)sock;

  switch (cmd)
    {
      case LINUX_BT_NATIVE_CMTPGETCONNLIST:
        {
          struct linux_bt_cmtp_connlist_req *req =
            (struct linux_bt_cmtp_connlist_req *)arg;
          uint32_t max;

          g_cmtp_socket_ioctl_connlist++;
          if (req == NULL)
            {
              return -EFAULT;
            }

          if (req->cnum == 0 || req->ci == NULL)
            {
              return -EINVAL;
            }

          max = req->cnum;
          req->cnum = linux_bt_cmtp_copy_connlist(req->ci, max);
          return 0;
        }

      case LINUX_BT_NATIVE_CMTPGETCONNINFO:
        {
          struct linux_bt_cmtp_conninfo *ci =
            (struct linux_bt_cmtp_conninfo *)arg;
          struct linux_bt_cmtp_core_session *session;

          g_cmtp_socket_ioctl_conninfo++;
          if (ci == NULL)
            {
              return -EFAULT;
            }

          session = linux_bt_cmtp_find_session_by_bdaddr(&ci->bdaddr);
          if (session == NULL)
            {
              return -ENOENT;
            }

          *ci = session->ci;
          return 0;
        }

      case LINUX_BT_NATIVE_CMTPCONNADD:
        {
          struct linux_bt_cmtp_connadd_req *req =
            (struct linux_bt_cmtp_connadd_req *)arg;
          struct linux_bt_cmtp_core_session *session;
          bdaddr_t bdaddr;
          unsigned int i;

          g_cmtp_socket_ioctl_connadd++;
          if (req == NULL)
            {
              return -EFAULT;
            }

          if (req->sock < 0)
            {
              return -EBADFD;
            }

          linux_bt_cmtp_fill_bdaddr((uint16_t)req->sock, &bdaddr);
          session = linux_bt_cmtp_find_session_by_bdaddr(&bdaddr);
          if (session != NULL)
            {
              return -EALREADY;
            }

          session = linux_bt_cmtp_find_free_session();
          if (session == NULL)
            {
              return -ENOSPC;
            }

          memset(session, 0, sizeof(*session));
          session->used = true;
          session->sock = req->sock;
          session->session_linked = true;
          session->worker_running = true;
          session->capi_registered = true;
          session->reassembly_ready = true;
          session->datapath_ready = true;
          session->blockids = 1;
          session->tx_frames = 1;
          session->rx_frames = 1;
          session->capi_messages = 1;
          session->ci.bdaddr = bdaddr;
          session->ci.flags = req->flags & BIT(LINUX_BT_CMTP_LOOPBACK);
          session->ci.state = LINUX_BT_CMTP_CORE_STATE_CONNECTED;
          session->ci.num = 1;
          for (i = 0; i < LINUX_BT_CMTP_CORE_MAX_SESSIONS; i++)
            {
              if (g_cmtp_core_sessions[i].used &&
                  &g_cmtp_core_sessions[i] != session &&
                  g_cmtp_core_sessions[i].ci.num >= session->ci.num)
                {
                  session->ci.num = g_cmtp_core_sessions[i].ci.num + 1;
                }
            }

          g_cmtp_core_session_adds++;
          return 0;
        }

      case LINUX_BT_NATIVE_CMTPCONNDEL:
        {
          struct linux_bt_cmtp_conndel_req *req =
            (struct linux_bt_cmtp_conndel_req *)arg;
          struct linux_bt_cmtp_core_session *session;

          g_cmtp_socket_ioctl_conndel++;
          if (req == NULL)
            {
              return -EFAULT;
            }

          session = linux_bt_cmtp_find_session_by_bdaddr(&req->bdaddr);
          if (session == NULL)
            {
              return -ENOENT;
            }

          memset(session, 0, sizeof(*session));
          g_cmtp_core_session_dels++;
          return 0;
        }

      default:
        return -EINVAL;
    }
}

static const struct proto_ops g_linux_bt_cmtp_sock_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .release = linux_bt_cmtp_sock_release,
  .ioctl = linux_bt_cmtp_sock_ioctl,
  .bind = sock_no_bind,
  .getname = sock_no_getname,
  .sendmsg = sock_no_sendmsg,
  .recvmsg = sock_no_recvmsg,
  .listen = sock_no_listen,
  .shutdown = sock_no_shutdown,
  .connect = sock_no_connect,
  .socketpair = sock_no_socketpair,
  .accept = sock_no_accept,
  .mmap = sock_no_mmap,
};

static int linux_bt_cmtp_sock_create(struct net *net, struct socket *sock,
                                     int protocol, int kern)
{
  struct sock *sk;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  if (sock->type != SOCK_RAW)
    {
      return -ESOCKTNOSUPPORT;
    }

  sk = bt_sock_alloc(net, sock, &g_linux_bt_cmtp_sock_proto, protocol,
                     GFP_KERNEL, kern);
  if (sk == NULL)
    {
      return -ENOMEM;
    }

  sock->ops = &g_linux_bt_cmtp_sock_ops;
  sock->state = SS_UNCONNECTED;
  return 0;
}

static const struct net_proto_family g_linux_bt_cmtp_sock_family_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .create = linux_bt_cmtp_sock_create,
};

int cmtp_sock_init(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
  int ret;

  ret = proto_register(&g_linux_bt_cmtp_sock_proto, 0);
  if (ret < 0)
    {
      return ret;
    }

  g_cmtp_sock_proto_registered = true;
  ret = bt_sock_register(BTPROTO_CMTP, &g_linux_bt_cmtp_sock_family_ops);
  if (ret < 0)
    {
      proto_unregister(&g_linux_bt_cmtp_sock_proto);
      g_cmtp_sock_proto_registered = false;
      return ret;
    }

  g_cmtp_socket_registers++;
#endif
  return 0;
}

void cmtp_sock_cleanup(void)
{
  if (g_cmtp_sock_proto_registered)
    {
      bt_sock_unregister(BTPROTO_CMTP);
      g_cmtp_socket_unregisters++;
      proto_unregister(&g_linux_bt_cmtp_sock_proto);
      g_cmtp_sock_proto_registered = false;
    }
}

static struct proto g_linux_bt_hidp_sock_proto =
{
  .name = "HIDP",
  .obj_size = sizeof(struct bt_sock),
};

static int linux_bt_hidp_sock_release(struct socket *sock)
{
  if (sock == NULL)
    {
      return 0;
    }

  if (sock->sk != NULL)
    {
      sock_orphan(sock->sk);
      sock_put(sock->sk);
      sock->sk = NULL;
    }

  return 0;
}

static void linux_bt_hidp_fill_bdaddr(uint16_t seed, bdaddr_t *bdaddr)
{
  if (bdaddr == NULL)
    {
      return;
    }

  bdaddr->b[0] = (uint8_t)(seed & 0xff);
  bdaddr->b[1] = (uint8_t)(seed >> 8);
  bdaddr->b[2] = 0x24;
  bdaddr->b[3] = 0x11;
  bdaddr->b[4] = 0x00;
  bdaddr->b[5] = 0x48;
}

static bool linux_bt_hidp_bdaddr_equal(const bdaddr_t *a,
                                       const bdaddr_t *b)
{
  return a != NULL && b != NULL && memcmp(a->b, b->b, ETH_ALEN) == 0;
}

static struct linux_bt_hidp_core_session *
linux_bt_hidp_find_session_by_bdaddr(const bdaddr_t *bdaddr)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_HIDP_CORE_MAX_SESSIONS; i++)
    {
      if (g_hidp_core_sessions[i].used &&
          linux_bt_hidp_bdaddr_equal(&g_hidp_core_sessions[i].ci.bdaddr,
                                     bdaddr))
        {
          return &g_hidp_core_sessions[i];
        }
    }

  return NULL;
}

static struct linux_bt_hidp_core_session *
linux_bt_hidp_find_free_session(void)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_HIDP_CORE_MAX_SESSIONS; i++)
    {
      if (!g_hidp_core_sessions[i].used)
        {
          return &g_hidp_core_sessions[i];
        }
    }

  return NULL;
}

static unsigned int linux_bt_hidp_core_active_count(void)
{
  unsigned int active = 0;
  unsigned int i;

  for (i = 0; i < LINUX_BT_HIDP_CORE_MAX_SESSIONS; i++)
    {
      if (g_hidp_core_sessions[i].used)
        {
          active++;
        }
    }

  return active;
}

static uint32_t linux_bt_hidp_copy_connlist(
  struct linux_bt_hidp_conninfo *ci, uint32_t max)
{
  uint32_t copied = 0;
  unsigned int i;

  if (ci == NULL || max == 0)
    {
      return 0;
    }

  for (i = 0; i < LINUX_BT_HIDP_CORE_MAX_SESSIONS && copied < max; i++)
    {
      if (g_hidp_core_sessions[i].used)
        {
          ci[copied] = g_hidp_core_sessions[i].ci;
          copied++;
        }
    }

  return copied;
}

static int linux_bt_hidp_sock_ioctl(struct socket *sock,
                                    unsigned int cmd,
                                    unsigned long arg)
{
  (void)sock;

  switch (cmd)
    {
      case LINUX_BT_NATIVE_HIDPGETCONNLIST:
        {
          struct linux_bt_hidp_connlist_req *req =
            (struct linux_bt_hidp_connlist_req *)arg;
          uint32_t max;

          g_hidp_socket_ioctl_connlist++;
          if (req == NULL)
            {
              return -EFAULT;
            }

          if (req->cnum == 0 || req->ci == NULL)
            {
              return -EINVAL;
            }

          max = req->cnum;
          req->cnum = linux_bt_hidp_copy_connlist(req->ci, max);
          return 0;
        }

      case LINUX_BT_NATIVE_HIDPGETCONNINFO:
        {
          struct linux_bt_hidp_conninfo *ci =
            (struct linux_bt_hidp_conninfo *)arg;
          struct linux_bt_hidp_core_session *session;

          g_hidp_socket_ioctl_conninfo++;
          if (ci == NULL)
            {
              return -EFAULT;
            }

          session = linux_bt_hidp_find_session_by_bdaddr(&ci->bdaddr);
          if (session == NULL)
            {
              return -ENOENT;
            }

          *ci = session->ci;
          return 0;
        }

      case LINUX_BT_NATIVE_HIDPCONNADD:
        {
          struct linux_bt_hidp_connadd_req *req =
            (struct linux_bt_hidp_connadd_req *)arg;
          struct linux_bt_hidp_core_session *session;
          bdaddr_t bdaddr;

          g_hidp_socket_ioctl_connadd++;
          if (req == NULL)
            {
              return -EFAULT;
            }

          if (req->ctrl_sock < 0 || req->intr_sock < 0)
            {
              return -EBADFD;
            }

          linux_bt_hidp_fill_bdaddr((uint16_t)req->ctrl_sock, &bdaddr);
          session = linux_bt_hidp_find_session_by_bdaddr(&bdaddr);
          if (session != NULL)
            {
              return -EALREADY;
            }

          session = linux_bt_hidp_find_free_session();
          if (session == NULL)
            {
              return -ENOSPC;
            }

          memset(session, 0, sizeof(*session));
          session->used = true;
          session->ctrl_sock = req->ctrl_sock;
          session->intr_sock = req->intr_sock;
          session->session_linked = true;
          session->session_thread = true;
          session->control_ready = true;
          session->interrupt_ready = true;
          session->input_registered = true;
          session->uhid_registered = true;
          session->control_messages = 1;
          session->input_events = 1;
          session->output_reports = 1;
          session->parser = req->parser;
          session->rd_size = req->rd_size;
          session->idle_to = req->idle_to;
          session->ci.bdaddr = bdaddr;
          session->ci.flags = req->flags;
          session->ci.state = LINUX_BT_HIDP_CORE_STATE_CONNECTED;
          session->ci.vendor = req->vendor;
          session->ci.product = req->product;
          session->ci.version = req->version;
          snprintf(session->ci.name, sizeof(session->ci.name), "%s",
                   req->name[0] != '\0' ? req->name :
                   "Feather HIDP hwsim device");
          g_hidp_core_session_adds++;
          return 0;
        }

      case LINUX_BT_NATIVE_HIDPCONNDEL:
        {
          struct linux_bt_hidp_conndel_req *req =
            (struct linux_bt_hidp_conndel_req *)arg;
          struct linux_bt_hidp_core_session *session;

          g_hidp_socket_ioctl_conndel++;
          if (req == NULL)
            {
              return -EFAULT;
            }

          session = linux_bt_hidp_find_session_by_bdaddr(&req->bdaddr);
          if (session == NULL)
            {
              return -ENOENT;
            }

          memset(session, 0, sizeof(*session));
          g_hidp_core_session_dels++;
          return 0;
        }

      default:
        return -EINVAL;
    }
}

static const struct proto_ops g_linux_bt_hidp_sock_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .release = linux_bt_hidp_sock_release,
  .ioctl = linux_bt_hidp_sock_ioctl,
  .bind = sock_no_bind,
  .getname = sock_no_getname,
  .sendmsg = sock_no_sendmsg,
  .recvmsg = sock_no_recvmsg,
  .listen = sock_no_listen,
  .shutdown = sock_no_shutdown,
  .connect = sock_no_connect,
  .socketpair = sock_no_socketpair,
  .accept = sock_no_accept,
  .mmap = sock_no_mmap,
};

static int linux_bt_hidp_sock_create(struct net *net, struct socket *sock,
                                     int protocol, int kern)
{
  struct sock *sk;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  if (sock->type != SOCK_RAW)
    {
      return -ESOCKTNOSUPPORT;
    }

  sk = bt_sock_alloc(net, sock, &g_linux_bt_hidp_sock_proto, protocol,
                     GFP_KERNEL, kern);
  if (sk == NULL)
    {
      return -ENOMEM;
    }

  sock->ops = &g_linux_bt_hidp_sock_ops;
  sock->state = SS_UNCONNECTED;
  return 0;
}

static const struct net_proto_family g_linux_bt_hidp_sock_family_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .create = linux_bt_hidp_sock_create,
};

int hidp_sock_init(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
  int ret;

  ret = proto_register(&g_linux_bt_hidp_sock_proto, 0);
  if (ret < 0)
    {
      return ret;
    }

  g_hidp_sock_proto_registered = true;
  ret = bt_sock_register(BTPROTO_HIDP, &g_linux_bt_hidp_sock_family_ops);
  if (ret < 0)
    {
      proto_unregister(&g_linux_bt_hidp_sock_proto);
      g_hidp_sock_proto_registered = false;
      return ret;
    }

  g_hidp_socket_registers++;
#endif
  return 0;
}

void hidp_sock_cleanup(void)
{
  if (g_hidp_sock_proto_registered)
    {
      bt_sock_unregister(BTPROTO_HIDP);
      g_hidp_socket_unregisters++;
      proto_unregister(&g_linux_bt_hidp_sock_proto);
      g_hidp_sock_proto_registered = false;
    }
}

static int linux_bt_upstream_control_socket_open(int proto,
                                                 void **out_handle)
{
  const struct net_proto_family *family = NULL;
  struct linux_bt_upstream_control_socket_handle *h;
  int ret;

  if (out_handle == NULL)
    {
      return -EINVAL;
    }

  *out_handle = NULL;
  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  h = kmm_zalloc(sizeof(*h));
  if (h == NULL)
    {
      return -ENOMEM;
    }

  h->sock.type = SOCK_RAW;
  h->sock.file = &h->file;
  h->file.private_data = h;
  ret = family->create(&init_net, &h->sock, proto, 0);
  if (ret < 0)
    {
      kmm_free(h);
      return ret;
    }

  h->proto = proto;
  *out_handle = h;
  return 0;
}

static int linux_bt_upstream_control_socket_ioctl_handle(void *handle,
                                                        unsigned int cmd,
                                                        unsigned long arg)
{
  struct linux_bt_upstream_control_socket_handle *h = handle;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->ioctl == NULL)
    {
      return -EOPNOTSUPP;
    }

  return h->sock.ops->ioctl(&h->sock, cmd, arg);
}

static int linux_bt_upstream_control_socket_close_handle(void *handle)
{
  struct linux_bt_upstream_control_socket_handle *h = handle;
  int close_ret = 0;

  if (h == NULL)
    {
      return -EINVAL;
    }

  if (h->sock.ops != NULL && h->sock.ops->release != NULL)
    {
      close_ret = h->sock.ops->release(&h->sock);
    }
  else if (h->sock.sk != NULL)
    {
      sock_put(h->sock.sk);
    }

  h->file.private_data = NULL;
  kmm_free(h);
  return close_ret;
}

int linux_bt_upstream_cmtp_socket_open(void **out_handle)
{
  return linux_bt_upstream_control_socket_open(BTPROTO_CMTP, out_handle);
}

int linux_bt_upstream_cmtp_socket_ioctl_handle(void *handle,
                                               unsigned int cmd,
                                               unsigned long arg)
{
  return linux_bt_upstream_control_socket_ioctl_handle(handle, cmd, arg);
}

int linux_bt_upstream_cmtp_socket_close_handle(void *handle)
{
  return linux_bt_upstream_control_socket_close_handle(handle);
}

int linux_bt_upstream_hidp_socket_open(void **out_handle)
{
  return linux_bt_upstream_control_socket_open(BTPROTO_HIDP, out_handle);
}

int linux_bt_upstream_hidp_socket_ioctl_handle(void *handle,
                                               unsigned int cmd,
                                               unsigned long arg)
{
  return linux_bt_upstream_control_socket_ioctl_handle(handle, cmd, arg);
}

int linux_bt_upstream_hidp_socket_close_handle(void *handle)
{
  return linux_bt_upstream_control_socket_close_handle(handle);
}

#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
static struct proto g_linux_bt_rfcomm_sock_proto =
{
  .name = "RFCOMM",
  .obj_size = sizeof(struct linux_bt_rfcomm_pinfo),
};

static bool linux_bt_rfcomm_valid_channel(uint8_t channel)
{
  return channel <= LINUX_BT_RFCOMM_MAX_CHANNEL;
}

static bool linux_bt_rfcomm_bdaddr_any(const bdaddr_t *bdaddr)
{
  unsigned int i;

  if (bdaddr == NULL)
    {
      return true;
    }

  for (i = 0; i < sizeof(bdaddr->b); i++)
    {
      if (bdaddr->b[i] != 0)
        {
          return false;
        }
    }

  return true;
}

static int linux_bt_rfcomm_sock_release(struct socket *sock)
{
  unsigned int i;

  if (sock == NULL)
    {
      return 0;
    }

  if (sock->sk != NULL)
    {
      if (sock->ops != NULL && sock->ops->shutdown != NULL)
        {
          (void)sock->ops->shutdown(sock, SHUT_RDWR);
        }

      for (i = 0; i < sizeof(g_rfcomm_socks) /
                      sizeof(g_rfcomm_socks[0]); i++)
        {
          if (g_rfcomm_socks[i] == sock->sk)
            {
              g_rfcomm_socks[i] = NULL;
              break;
            }
        }

      sock_orphan(sock->sk);
      sock_put(sock->sk);
      sock->sk = NULL;
    }

  return 0;
}

static int linux_bt_rfcomm_sock_shutdown(struct socket *sock, int how)
{
  struct sock *sk;

  (void)how;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  sk = sock->sk;
  if (sk == NULL)
    {
      return 0;
    }

  if ((sk->sk_shutdown & SHUTDOWN_MASK) == SHUTDOWN_MASK)
    {
      return sk->sk_err != 0 ? -sk->sk_err : 0;
    }

  sk->sk_shutdown = SHUTDOWN_MASK;
  sk->sk_state = BT_CLOSED;
  sock->state = SS_UNCONNECTED;
  g_rfcomm_socket_shutdowns++;
  return sk->sk_err != 0 ? -sk->sk_err : 0;
}

static int linux_bt_rfcomm_sock_bind(struct socket *sock,
                                     struct sockaddr *addr,
                                     int addr_len)
{
  struct linux_bt_sockaddr_rc *raddr;
  struct linux_bt_rfcomm_pinfo *pi;

  if (sock == NULL || sock->sk == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  if (addr_len < (int)sizeof(*raddr))
    {
      return -EINVAL;
    }

  raddr = (struct linux_bt_sockaddr_rc *)addr;
  if (raddr->rc_family != AF_BLUETOOTH)
    {
      return -EAFNOSUPPORT;
    }

  if (!linux_bt_rfcomm_valid_channel(raddr->rc_channel))
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state != BT_OPEN)
    {
      return -EALREADY;
    }

  pi = linux_bt_rfcomm_pi(sock->sk);
  bacpy(&pi->src, &raddr->rc_bdaddr);
  pi->channel = raddr->rc_channel != 0 ? raddr->rc_channel : 1;
  pi->cid = LINUX_BT_RFCOMM_DEFAULT_CID + pi->channel;
  pi->handle = LINUX_BT_RFCOMM_DEFAULT_HANDLE;
  sock->sk->sk_state = BT_BOUND;
  g_rfcomm_socket_binds++;
  return 0;
}

static int linux_bt_rfcomm_sock_connect(struct socket *sock,
                                        struct sockaddr *addr,
                                        int addr_len,
                                        int flags)
{
  struct linux_bt_sockaddr_rc *raddr;
  struct linux_bt_rfcomm_pinfo *pi;
  int ret;

  (void)flags;

  if (sock == NULL || sock->sk == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  if (addr_len < (int)sizeof(*raddr))
    {
      return -EINVAL;
    }

  raddr = (struct linux_bt_sockaddr_rc *)addr;
  if (raddr->rc_family != AF_BLUETOOTH)
    {
      return -EAFNOSUPPORT;
    }

  if (raddr->rc_channel == 0 ||
      !linux_bt_rfcomm_valid_channel(raddr->rc_channel))
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state == BT_CONNECTED)
    {
      return -EISCONN;
    }

  pi = linux_bt_rfcomm_pi(sock->sk);
  if (sock->sk->sk_state == BT_OPEN)
    {
      bacpy(&pi->src, BDADDR_ANY);
      sock->sk->sk_state = BT_BOUND;
      g_rfcomm_socket_binds++;
    }

  bacpy(&pi->dst, &raddr->rc_bdaddr);
  pi->channel = raddr->rc_channel;
  pi->cid = LINUX_BT_RFCOMM_DEFAULT_CID + pi->channel;
  pi->handle = LINUX_BT_RFCOMM_DEFAULT_HANDLE;
  ret = linux_bt_hci_ensure_connected_handle(
    ACL_LINK, pi->handle,
    linux_bt_rfcomm_bdaddr_any(&pi->dst) ? &pi->src : &pi->dst,
    0, true, 0);
  if (ret < 0)
    {
      return ret;
    }

  sock->sk->sk_state = BT_CONNECTED;
  sock->state = SS_CONNECTED;
  g_rfcomm_socket_connects++;
  return 0;
}

static int linux_bt_rfcomm_sock_listen(struct socket *sock, int backlog)
{
  struct linux_bt_rfcomm_pinfo *pi;

  if (sock == NULL || sock->sk == NULL)
    {
      return -EINVAL;
    }

  if (backlog < 0)
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state != BT_BOUND)
    {
      return -EINVAL;
    }

  pi = linux_bt_rfcomm_pi(sock->sk);
  if (pi->channel == 0)
    {
      pi->channel = 1;
      pi->cid = LINUX_BT_RFCOMM_DEFAULT_CID + pi->channel;
    }

  sock->sk->sk_state = BT_LISTEN;
  g_rfcomm_socket_listens++;
  return 0;
}

static int linux_bt_rfcomm_sock_accept(struct socket *sock,
                                       struct socket *newsock,
                                       struct proto_accept_arg *arg)
{
  const struct net_proto_family *family;
  struct linux_bt_rfcomm_pinfo *pi;
  struct linux_bt_rfcomm_pinfo *new_pi;
  int ret;

  (void)arg;

  if (sock == NULL || newsock == NULL || sock->sk == NULL ||
      sock->sk->sk_state != BT_LISTEN)
    {
      return -EINVAL;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  pi = linux_bt_rfcomm_pi(sock->sk);
  if (pi->channel == 0)
    {
      return -ENOTCONN;
    }

  newsock->type = sock->type;
  ret = family->create(&init_net, newsock, BTPROTO_RFCOMM, 0);
  if (ret < 0)
    {
      return ret;
    }

  new_pi = linux_bt_rfcomm_pi(newsock->sk);
  bacpy(&new_pi->src, &pi->src);
  bacpy(&new_pi->dst, &pi->dst);
  new_pi->channel = pi->channel;
  new_pi->cid = pi->cid;
  new_pi->handle = pi->handle;
  new_pi->mtu = pi->mtu;
  new_pi->sec_level = pi->sec_level;
  new_pi->role_switch = pi->role_switch;
  new_pi->defer_setup = pi->defer_setup;

  ret = linux_bt_hci_ensure_connected_handle(
    ACL_LINK, new_pi->handle,
    linux_bt_rfcomm_bdaddr_any(&new_pi->dst) ? &new_pi->src :
                                               &new_pi->dst,
    0, true, 0);
  if (ret < 0)
    {
      if (newsock->ops != NULL && newsock->ops->release != NULL)
        {
          (void)newsock->ops->release(newsock);
        }

      return ret;
    }

  newsock->sk->sk_state = BT_CONNECTED;
  newsock->state = SS_CONNECTED;
  g_rfcomm_socket_connects++;
  return 0;
}

static int linux_bt_rfcomm_sock_getname(struct socket *sock,
                                        struct sockaddr *addr,
                                        int peer)
{
  struct linux_bt_sockaddr_rc *raddr;
  struct linux_bt_rfcomm_pinfo *pi;

  if (sock == NULL || sock->sk == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  raddr = (struct linux_bt_sockaddr_rc *)addr;
  pi = linux_bt_rfcomm_pi(sock->sk);
  if (peer && sock->sk->sk_state != BT_CONNECTED &&
      sock->sk->sk_state != BT_CONNECT &&
      sock->sk->sk_state != BT_CONNECT2)
    {
      return -ENOTCONN;
    }

  memset(raddr, 0, sizeof(*raddr));
  raddr->rc_family = AF_BLUETOOTH;
  raddr->rc_channel = pi->channel;
  bacpy(&raddr->rc_bdaddr, peer ? &pi->dst : &pi->src);
  g_rfcomm_socket_getnames++;
  return sizeof(*raddr);
}

static int linux_bt_rfcomm_sock_setsockopt(struct socket *sock, int level,
                                           int optname, sockptr_t optval,
                                           unsigned int optlen)
{
  struct linux_bt_rfcomm_pinfo *pi;
  struct bt_security sec;
  uint32_t opt;
  uint16_t mtu;
  uint32_t lm;

  if (sock == NULL || sock->sk == NULL || optval == NULL)
    {
      return -EINVAL;
    }

  pi = linux_bt_rfcomm_pi(sock->sk);
  if (level == SOL_BLUETOOTH)
    {
      switch (optname)
        {
          case BT_SECURITY:
            if (optlen < sizeof(sec))
              {
                return -EINVAL;
              }

            memcpy(&sec, optval, sizeof(sec));
            if (sec.level < BT_SECURITY_LOW ||
                sec.level > BT_SECURITY_HIGH)
              {
                return -EINVAL;
              }

            pi->sec_level = sec.level;
            g_rfcomm_socket_setopt++;
            return 0;

          case BT_DEFER_SETUP:
            if (sock->sk->sk_state != BT_BOUND &&
                sock->sk->sk_state != BT_LISTEN)
              {
                return -EINVAL;
              }

            if (optlen < sizeof(opt))
              {
                return -EINVAL;
              }

            memcpy(&opt, optval, sizeof(opt));
            pi->defer_setup = opt != 0;
            g_rfcomm_socket_setopt++;
            return 0;

          default:
            return -ENOPROTOOPT;
        }
    }

  if (level != SOL_RFCOMM)
    {
      return -ENOPROTOOPT;
    }

  switch (optname)
    {
      case SO_RFCOMM_MTU:
        if (optlen < sizeof(mtu))
          {
            return -EINVAL;
          }

        memcpy(&mtu, optval, sizeof(mtu));
        if (mtu == 0)
          {
            return -EINVAL;
          }

        pi->mtu = mtu;
        g_rfcomm_socket_setopt++;
        return 0;

      case SO_RFCOMM_LM:
      case RFCOMM_LM:
        if (optlen < sizeof(lm))
          {
            return -EINVAL;
          }

        memcpy(&lm, optval, sizeof(lm));
        if ((lm & RFCOMM_LM_FIPS) != 0)
          {
            return -EINVAL;
          }

        if ((lm & RFCOMM_LM_AUTH) != 0)
          {
            pi->sec_level = BT_SECURITY_LOW;
          }

        if ((lm & RFCOMM_LM_ENCRYPT) != 0)
          {
            pi->sec_level = BT_SECURITY_MEDIUM;
          }

        if ((lm & RFCOMM_LM_SECURE) != 0)
          {
            pi->sec_level = BT_SECURITY_HIGH;
          }

        pi->role_switch = (lm & RFCOMM_LM_MASTER) != 0;
        g_rfcomm_socket_setopt++;
        return 0;

      default:
        return -ENOPROTOOPT;
    }
}

static int linux_bt_rfcomm_sock_getsockopt(struct socket *sock, int level,
                                           int optname, char *optval,
                                           int *optlen)
{
  struct linux_bt_rfcomm_pinfo *pi;
  struct bt_security sec;
  struct linux_bt_rfcomm_conninfo cinfo;
  uint32_t opt;
  uint16_t mtu;
  uint32_t lm;
  int len;

  if (sock == NULL || sock->sk == NULL || optval == NULL ||
      optlen == NULL || *optlen < 0)
    {
      return -EINVAL;
    }

  pi = linux_bt_rfcomm_pi(sock->sk);
  if (level == SOL_BLUETOOTH)
    {
      switch (optname)
        {
          case BT_SECURITY:
            memset(&sec, 0, sizeof(sec));
            sec.level = pi->sec_level != 0 ? pi->sec_level :
                        BT_SECURITY_LOW;
            len = *optlen < (int)sizeof(sec) ? *optlen : (int)sizeof(sec);
            memcpy(optval, &sec, (size_t)len);
            *optlen = len;
            g_rfcomm_socket_getopt++;
            return 0;

          case BT_DEFER_SETUP:
            opt = pi->defer_setup ? 1 : 0;
            len = *optlen < (int)sizeof(opt) ? *optlen : (int)sizeof(opt);
            memcpy(optval, &opt, (size_t)len);
            *optlen = len;
            g_rfcomm_socket_getopt++;
            return 0;

          default:
            return -ENOPROTOOPT;
        }
    }

  if (level != SOL_RFCOMM)
    {
      return -ENOPROTOOPT;
    }

  switch (optname)
    {
      case SO_RFCOMM_MTU:
        mtu = pi->mtu;
        len = *optlen < (int)sizeof(mtu) ? *optlen : (int)sizeof(mtu);
        memcpy(optval, &mtu, (size_t)len);
        *optlen = len;
        g_rfcomm_socket_getopt++;
        return 0;

      case SO_RFCOMM_LM:
      case RFCOMM_LM:
        switch (pi->sec_level)
          {
            case BT_SECURITY_LOW:
              lm = RFCOMM_LM_AUTH;
              break;

            case BT_SECURITY_MEDIUM:
              lm = RFCOMM_LM_AUTH | RFCOMM_LM_ENCRYPT;
              break;

            case BT_SECURITY_HIGH:
              lm = RFCOMM_LM_AUTH | RFCOMM_LM_ENCRYPT |
                   RFCOMM_LM_SECURE;
              break;

            case BT_SECURITY_FIPS:
              lm = RFCOMM_LM_AUTH | RFCOMM_LM_ENCRYPT |
                   RFCOMM_LM_SECURE | RFCOMM_LM_FIPS;
              break;

            default:
              lm = 0;
              break;
          }

        if (pi->role_switch)
          {
            lm |= RFCOMM_LM_MASTER;
          }

        len = *optlen < (int)sizeof(lm) ? *optlen : (int)sizeof(lm);
        memcpy(optval, &lm, (size_t)len);
        *optlen = len;
        g_rfcomm_socket_getopt++;
        return 0;

      case RFCOMM_CONNINFO:
        if (sock->sk->sk_state != BT_CONNECTED && !pi->defer_setup)
          {
            return -ENOTCONN;
          }

        memset(&cinfo, 0, sizeof(cinfo));
        cinfo.hci_handle = pi->handle;
        len = *optlen < (int)sizeof(cinfo) ? *optlen :
              (int)sizeof(cinfo);
        memcpy(optval, &cinfo, (size_t)len);
        *optlen = len;
        g_rfcomm_socket_getopt++;
        return 0;

      default:
        return -ENOPROTOOPT;
    }
}

static int linux_bt_rfcomm_sock_ioctl(struct socket *sock,
                                      unsigned int cmd,
                                      unsigned long arg)
{
  int ret;

  ret = bt_sock_ioctl(sock, cmd, arg);
  if (ret == -ENOIOCTLCMD)
    {
      ret = -EOPNOTSUPP;
    }

  return ret;
}

static int linux_bt_rfcomm_sock_sendmsg(struct socket *sock,
                                        struct msghdr *msg,
                                        size_t len)
{
  struct linux_bt_rfcomm_pinfo *pi;
  uint8_t *acl;
  uint8_t *frame;
  uint16_t rfcomm_len;
  uint16_t l2_len;
  uint16_t handle_pb;
  size_t frame_len;
  int ret;

  if (sock == NULL || sock->sk == NULL || msg == NULL ||
      msg->msg_iter.data == NULL)
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state != BT_CONNECTED)
    {
      return -ENOTCONN;
    }

  pi = linux_bt_rfcomm_pi(sock->sk);
  if (len == 0 || len > pi->mtu || len > UINT16_MAX - 8 ||
      msg->msg_iter.count < len)
    {
      return -EMSGSIZE;
    }

  frame_len = len + 3;
  acl = kmm_malloc(frame_len + 8);
  if (acl == NULL)
    {
      return -ENOMEM;
    }

  frame = &acl[8];
  rfcomm_len = (uint16_t)len;
  frame[0] = (uint8_t)(((pi->channel << 1) << 2) | 0x03);
  frame[1] = 0xef;
  frame[2] = (uint8_t)((rfcomm_len << 1) | 0x01);
  memcpy(&frame[3], msg->msg_iter.data, len);

  l2_len = (uint16_t)frame_len;
  handle_pb = (uint16_t)(pi->handle | (0x02 << 12));
  acl[0] = (uint8_t)(handle_pb & 0xff);
  acl[1] = (uint8_t)(handle_pb >> 8);
  acl[2] = (uint8_t)((l2_len + 4) & 0xff);
  acl[3] = (uint8_t)((l2_len + 4) >> 8);
  acl[4] = (uint8_t)(l2_len & 0xff);
  acl[5] = (uint8_t)(l2_len >> 8);
  acl[6] = (uint8_t)(pi->cid & 0xff);
  acl[7] = (uint8_t)(pi->cid >> 8);

  ret = linux_bt_upstream_hci_send(HCI_ACLDATA_PKT, acl, frame_len + 8);
  if (ret >= 0)
    {
      (void)linux_bt_proto_sock_queue_payload(sock->sk, msg->msg_iter.data,
                                              len);
      g_rfcomm_socket_sends++;
      ret = (int)len;
    }

  kmm_free(acl);
  return ret;
}

static int linux_bt_rfcomm_sock_recvmsg(struct socket *sock,
                                        struct msghdr *msg,
                                        size_t len,
                                        int flags)
{
  struct sk_buff *skb;
  size_t copied;

  (void)flags;

  if (sock == NULL || sock->sk == NULL || msg == NULL)
    {
      return -EINVAL;
    }

  skb = skb_dequeue(&sock->sk->sk_receive_queue);
  if (skb == NULL)
    {
      return -EAGAIN;
    }

  copied = skb->len < len ? skb->len : len;
  if (copied < skb->len)
    {
      msg->msg_flags |= MSG_TRUNC;
    }

  if (copy_to_iter(skb->data, copied, &msg->msg_iter) != copied)
    {
      kfree_skb(skb);
      return -EFAULT;
    }

  kfree_skb(skb);
  g_rfcomm_socket_recvs++;
  return (int)copied;
}

static const struct proto_ops g_linux_bt_rfcomm_sock_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .release = linux_bt_rfcomm_sock_release,
  .bind = linux_bt_rfcomm_sock_bind,
  .getname = linux_bt_rfcomm_sock_getname,
  .sendmsg = linux_bt_rfcomm_sock_sendmsg,
  .recvmsg = linux_bt_rfcomm_sock_recvmsg,
  .ioctl = linux_bt_rfcomm_sock_ioctl,
  .poll = linux_bt_sock_poll,
  .listen = linux_bt_rfcomm_sock_listen,
  .shutdown = linux_bt_rfcomm_sock_shutdown,
  .setsockopt = linux_bt_rfcomm_sock_setsockopt,
  .getsockopt = linux_bt_rfcomm_sock_getsockopt,
  .connect = linux_bt_rfcomm_sock_connect,
  .socketpair = sock_no_socketpair,
  .accept = linux_bt_rfcomm_sock_accept,
  .mmap = sock_no_mmap,
  .gettstamp = sock_gettstamp,
};

static int linux_bt_rfcomm_sock_create(struct net *net,
                                       struct socket *sock,
                                       int protocol,
                                       int kern)
{
  struct linux_bt_rfcomm_pinfo *pi;
  struct sock *sk;
  unsigned int i;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  switch (sock->type)
    {
      case SOCK_STREAM:
      case SOCK_RAW:
        break;

      default:
        return -ESOCKTNOSUPPORT;
    }

  sk = bt_sock_alloc(net, sock, &g_linux_bt_rfcomm_sock_proto, protocol,
                     GFP_KERNEL, kern);
  if (sk == NULL)
    {
      return -ENOMEM;
    }

  pi = linux_bt_rfcomm_pi(sk);
  bacpy(&pi->src, BDADDR_ANY);
  bacpy(&pi->dst, BDADDR_ANY);
  pi->channel = 0;
  pi->sec_level = BT_SECURITY_LOW;
  pi->role_switch = false;
  pi->defer_setup = false;
  pi->mtu = LINUX_BT_RFCOMM_DEFAULT_MTU;
  pi->cid = LINUX_BT_RFCOMM_DEFAULT_CID;
  pi->handle = LINUX_BT_RFCOMM_DEFAULT_HANDLE;
  sock->ops = &g_linux_bt_rfcomm_sock_ops;
  sock->state = SS_UNCONNECTED;
  for (i = 0; i < sizeof(g_rfcomm_socks) / sizeof(g_rfcomm_socks[0]); i++)
    {
      if (g_rfcomm_socks[i] == NULL)
        {
          g_rfcomm_socks[i] = sk;
          return 0;
        }
    }

  sock_orphan(sk);
  sock_put(sk);
  sock->sk = NULL;
  sock->ops = NULL;
  return -ENOMEM;
}

static const struct net_proto_family g_linux_bt_rfcomm_sock_family_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .create = linux_bt_rfcomm_sock_create,
};

int rfcomm_sock_init(void)
{
  int ret;

  ret = proto_register(&g_linux_bt_rfcomm_sock_proto, 0);
  if (ret < 0)
    {
      return ret;
    }

  g_rfcomm_sock_proto_registered = true;
  ret = bt_sock_register(BTPROTO_RFCOMM,
                         &g_linux_bt_rfcomm_sock_family_ops);
  if (ret < 0)
    {
      proto_unregister(&g_linux_bt_rfcomm_sock_proto);
      g_rfcomm_sock_proto_registered = false;
      return ret;
    }

  g_rfcomm_socket_registers++;
  return 0;
}

void rfcomm_sock_cleanup(void)
{
  if (g_rfcomm_sock_proto_registered)
    {
      bt_sock_unregister(BTPROTO_RFCOMM);
      g_rfcomm_socket_unregisters++;
      proto_unregister(&g_linux_bt_rfcomm_sock_proto);
      g_rfcomm_sock_proto_registered = false;
    }
}
#else
int rfcomm_sock_init(void)
{
  return 0;
}

void rfcomm_sock_cleanup(void)
{
}
#endif

#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
static struct proto g_linux_bt_sco_sock_proto =
{
  .name = "SCO",
  .obj_size = sizeof(struct linux_bt_sco_pinfo),
};

static int linux_bt_sco_sock_release(struct socket *sock)
{
  unsigned int i;
  int ret = 0;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(g_sco_socks) / sizeof(g_sco_socks[0]); i++)
    {
      if (g_sco_socks[i] == sock->sk)
        {
          g_sco_socks[i] = NULL;
        }
    }

  if (sock->sk != NULL)
    {
      if (sock->ops != NULL && sock->ops->shutdown != NULL)
        {
          ret = sock->ops->shutdown(sock, SHUT_RDWR);
        }

      sock_orphan(sock->sk);
      sock_put(sock->sk);
      sock->sk = NULL;
    }

  sock->ops = NULL;
  sock->state = SS_UNCONNECTED;
  return ret;
}

static int linux_bt_sco_sock_bind(struct socket *sock,
                                  struct sockaddr *addr,
                                  int addr_len)
{
  struct linux_bt_sockaddr_sco *saddr =
    (struct linux_bt_sockaddr_sco *)addr;
  struct linux_bt_sco_pinfo *pi;

  if (sock == NULL || sock->sk == NULL || addr == NULL ||
      addr_len < (int)sizeof(struct linux_bt_sockaddr_sco) ||
      addr->sa_family != AF_BLUETOOTH)
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state != BT_OPEN)
    {
      return -EBADFD;
    }

  pi = linux_bt_sco_pi(sock->sk);
  bacpy(&pi->src, &saddr->sco_bdaddr);
  sock->sk->sk_state = BT_BOUND;
  g_sco_socket_binds++;
  return 0;
}

static int linux_bt_sco_sock_connect(struct socket *sock,
                                     struct sockaddr *addr,
                                     int addr_len, int flags)
{
  struct linux_bt_sockaddr_sco *saddr =
    (struct linux_bt_sockaddr_sco *)addr;
  struct linux_bt_sco_pinfo *pi;

  (void)flags;

  if (sock == NULL || sock->sk == NULL || addr == NULL ||
      addr_len < (int)sizeof(struct linux_bt_sockaddr_sco) ||
      addr->sa_family != AF_BLUETOOTH)
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state != BT_OPEN && sock->sk->sk_state != BT_BOUND)
    {
      return -EBADFD;
    }

  pi = linux_bt_sco_pi(sock->sk);
  bacpy(&pi->dst, &saddr->sco_bdaddr);
  if (pi->handle == 0)
    {
      pi->handle = LINUX_BT_SCO_DEFAULT_HANDLE;
    }

  if (pi->mtu == 0)
    {
      pi->mtu = LINUX_BT_SCO_DEFAULT_MTU;
    }

  (void)linux_bt_hci_ensure_connected_handle(SCO_LINK, pi->handle,
                                             &pi->dst, 0, true, 0);
  sock->sk->sk_state = BT_CONNECTED;
  sock->state = SS_CONNECTED;
  g_sco_socket_connects++;
  return 0;
}

static int linux_bt_sco_sock_listen(struct socket *sock, int backlog)
{
  if (sock == NULL || sock->sk == NULL)
    {
      return -EINVAL;
    }

  if (backlog < 0)
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state != BT_BOUND)
    {
      return -EINVAL;
    }

  sock->sk->sk_state = BT_LISTEN;
  sock->state = SS_UNCONNECTED;
  return 0;
}

static int linux_bt_sco_sock_accept(struct socket *sock,
                                    struct socket *newsock,
                                    struct proto_accept_arg *arg)
{
  const struct net_proto_family *family;
  struct linux_bt_sco_pinfo *pi;
  struct linux_bt_sco_pinfo *new_pi;
  int ret;

  (void)arg;

  if (sock == NULL || newsock == NULL || sock->sk == NULL ||
      sock->sk->sk_state != BT_LISTEN)
    {
      return -EINVAL;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  pi = linux_bt_sco_pi(sock->sk);
  newsock->type = sock->type;
  ret = family->create(&init_net, newsock, BTPROTO_SCO, 0);
  if (ret < 0)
    {
      return ret;
    }

  new_pi = linux_bt_sco_pi(newsock->sk);
  bacpy(&new_pi->src, &pi->src);
  bacpy(&new_pi->dst, &pi->dst);
  new_pi->handle = pi->handle != 0 ? pi->handle :
                   LINUX_BT_SCO_DEFAULT_HANDLE;
  new_pi->mtu = pi->mtu != 0 ? pi->mtu : LINUX_BT_SCO_DEFAULT_MTU;
  new_pi->voice = pi->voice;
  new_pi->defer_setup = pi->defer_setup;
  new_pi->pkt_status = pi->pkt_status;

  ret = linux_bt_hci_ensure_connected_handle(SCO_LINK, new_pi->handle,
                                             &new_pi->dst, 0, true, 0);
  if (ret < 0)
    {
      if (newsock->ops != NULL && newsock->ops->release != NULL)
        {
          (void)newsock->ops->release(newsock);
        }

      return ret;
    }

  newsock->sk->sk_state = BT_CONNECTED;
  newsock->state = SS_CONNECTED;
  g_sco_socket_connects++;
  return 0;
}

static int linux_bt_sco_sock_getname(struct socket *sock,
                                     struct sockaddr *addr,
                                     int peer)
{
  struct linux_bt_sockaddr_sco *saddr =
    (struct linux_bt_sockaddr_sco *)addr;
  struct linux_bt_sco_pinfo *pi;

  if (sock == NULL || sock->sk == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  pi = linux_bt_sco_pi(sock->sk);
  memset(saddr, 0, sizeof(*saddr));
  saddr->sco_family = AF_BLUETOOTH;
  if (peer)
    {
      bacpy(&saddr->sco_bdaddr, &pi->dst);
    }
  else
    {
      bacpy(&saddr->sco_bdaddr, &pi->src);
    }

  g_sco_socket_getnames++;
  return sizeof(*saddr);
}

static int linux_bt_sco_sock_setsockopt(struct socket *sock, int level,
                                        int optname, char *optval,
                                        unsigned int optlen)
{
  struct linux_bt_sco_pinfo *pi;
  uint32_t opt;

  if (sock == NULL || sock->sk == NULL || optval == NULL)
    {
      return -EINVAL;
    }

  pi = linux_bt_sco_pi(sock->sk);
  if (level != SOL_BLUETOOTH)
    {
      return -ENOPROTOOPT;
    }

  switch (optname)
    {
      case BT_DEFER_SETUP:
        if (sock->sk->sk_state != BT_BOUND &&
            sock->sk->sk_state != BT_LISTEN)
          {
            return -EINVAL;
          }

        if (optlen < sizeof(opt))
          {
            return -EINVAL;
          }

        memcpy(&opt, optval, sizeof(opt));
        pi->defer_setup = opt != 0;
        g_sco_socket_setopt++;
        return 0;

      case BT_VOICE:
        if (sock->sk->sk_state != BT_OPEN &&
            sock->sk->sk_state != BT_BOUND &&
            sock->sk->sk_state != BT_CONNECT2)
          {
            return -EINVAL;
          }

        if (optlen < sizeof(pi->voice))
          {
            return -EINVAL;
          }

        memcpy(&pi->voice, optval, sizeof(pi->voice));
        g_sco_socket_setopt++;
        return 0;

      case BT_PKT_STATUS:
        if (optlen < sizeof(opt))
          {
            return -EINVAL;
          }

        memcpy(&opt, optval, sizeof(opt));
        pi->pkt_status = opt != 0;
        g_sco_socket_setopt++;
        return 0;

      case BT_CODEC:
        if (sock->sk->sk_state != BT_OPEN &&
            sock->sk->sk_state != BT_BOUND &&
            sock->sk->sk_state != BT_CONNECT2)
          {
            return -EINVAL;
          }

        return -EOPNOTSUPP;

      default:
        return -ENOPROTOOPT;
    }
}

static int linux_bt_sco_sock_getsockopt(struct socket *sock, int level,
                                        int optname, char *optval,
                                        int *optlen)
{
  struct linux_bt_sco_pinfo *pi;
  struct linux_bt_sco_conninfo cinfo;
  struct linux_bt_sco_options opts;
  uint32_t opt;
  int len;

  if (sock == NULL || sock->sk == NULL || optval == NULL ||
      optlen == NULL)
    {
      return -EINVAL;
    }

  pi = linux_bt_sco_pi(sock->sk);
  if (level == SOL_SCO && optname == SCO_OPTIONS)
    {
      if (sock->sk->sk_state != BT_CONNECTED &&
          !(sock->sk->sk_state == BT_CONNECT2 && pi->defer_setup))
        {
          return -ENOTCONN;
        }

      opts.mtu = pi->mtu;
      len = *optlen < (int)sizeof(opts) ? *optlen : (int)sizeof(opts);
      memcpy(optval, &opts, (size_t)len);
      *optlen = len;
      g_sco_socket_getopt++;
      return 0;
    }

  if (level == SOL_SCO && optname == SCO_CONNINFO)
    {
      if (sock->sk->sk_state != BT_CONNECTED &&
          !(sock->sk->sk_state == BT_CONNECT2 && pi->defer_setup))
        {
          return -ENOTCONN;
        }

      memset(&cinfo, 0, sizeof(cinfo));
      cinfo.hci_handle = pi->handle;
      len = *optlen < (int)sizeof(cinfo) ? *optlen : (int)sizeof(cinfo);
      memcpy(optval, &cinfo, (size_t)len);
      *optlen = len;
      g_sco_socket_getopt++;
      return 0;
    }

  if (level == SOL_SCO && optname == SO_SCO_MTU)
    {
      return -ENOPROTOOPT;
    }

  if (level == SOL_SCO && optname == SO_SCO_HANDLE)
    {
      return -ENOPROTOOPT;
    }

  if (*optlen < (int)sizeof(uint16_t))
    {
      return -EINVAL;
    }

  if (level == SOL_BLUETOOTH && optname == BT_DEFER_SETUP)
    {
      if (sock->sk->sk_state != BT_BOUND &&
          sock->sk->sk_state != BT_LISTEN)
        {
          return -EINVAL;
        }

      if (*optlen < (int)sizeof(opt))
        {
          return -EINVAL;
        }

      opt = pi->defer_setup ? 1 : 0;
      memcpy(optval, &opt, sizeof(opt));
      *optlen = sizeof(opt);
      g_sco_socket_getopt++;
      return 0;
    }

  if (level == SOL_BLUETOOTH && optname == BT_VOICE)
    {
      uint16_t voice = pi->voice;

      memcpy(optval, &voice, sizeof(voice));
      *optlen = sizeof(voice);
      g_sco_socket_getopt++;
      return 0;
    }

  if (level == SOL_BLUETOOTH && optname == BT_PKT_STATUS)
    {
      if (*optlen < (int)sizeof(opt))
        {
          return -EINVAL;
        }

      opt = pi->pkt_status ? 1 : 0;
      memcpy(optval, &opt, sizeof(opt));
      *optlen = sizeof(opt);
      g_sco_socket_getopt++;
      return 0;
    }

  if (level == SOL_BLUETOOTH &&
      (optname == BT_SNDMTU || optname == BT_RCVMTU))
    {
      if (sock->sk->sk_state != BT_CONNECTED)
        {
          return -ENOTCONN;
        }

      if (*optlen < (int)sizeof(opt))
        {
          return -EINVAL;
        }

      opt = pi->mtu;
      memcpy(optval, &opt, sizeof(opt));
      *optlen = sizeof(opt);
      g_sco_socket_getopt++;
      return 0;
    }

  if (level == SOL_BLUETOOTH && optname == BT_CODEC)
    {
      return -EOPNOTSUPP;
    }

  return -ENOPROTOOPT;
}

static int linux_bt_sco_sock_sendmsg(struct socket *sock,
                                     struct msghdr *msg, size_t len)
{
  struct linux_bt_sco_pinfo *pi;
  int ret;

  if (sock == NULL || sock->sk == NULL || msg == NULL ||
      (msg->msg_iter.data == NULL && len > 0))
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state != BT_CONNECTED)
    {
      return -ENOTCONN;
    }

  pi = linux_bt_sco_pi(sock->sk);
  if (pi->handle == 0)
    {
      pi->handle = LINUX_BT_SCO_DEFAULT_HANDLE;
    }

  ret = linux_bt_upstream_hci_send(HCI_SCODATA_PKT, msg->msg_iter.data,
                                   len);
  if (ret >= 0)
    {
      g_sco_socket_sends++;
    }

  return ret < 0 ? ret : (int)len;
}

static int linux_bt_sco_sock_recvmsg(struct socket *sock,
                                     struct msghdr *msg, size_t len,
                                     int flags)
{
  int ret;

  if (sock == NULL || sock->sk == NULL || msg == NULL)
    {
      return -EINVAL;
    }

  ret = bt_sock_recvmsg(sock, msg, len, flags);
  if (ret >= 0)
    {
      g_sco_socket_recvs++;
    }

  return ret;
}

static int linux_bt_sco_sock_shutdown(struct socket *sock, int how)
{
  struct sock *sk;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  sk = sock->sk;
  if (sk == NULL)
    {
      return 0;
    }

  switch (how)
    {
      case SHUT_RD:
        if (sk->sk_shutdown & RCV_SHUTDOWN)
          {
            return 0;
          }

        sk->sk_shutdown |= RCV_SHUTDOWN;
        break;

      case SHUT_WR:
        if (sk->sk_shutdown & SEND_SHUTDOWN)
          {
            return 0;
          }

        sk->sk_shutdown |= SEND_SHUTDOWN;
        break;

      case SHUT_RDWR:
        if ((sk->sk_shutdown & SHUTDOWN_MASK) == SHUTDOWN_MASK)
          {
            return 0;
          }

        sk->sk_shutdown |= SHUTDOWN_MASK;
        sk->sk_state = BT_CLOSED;
        sock->state = SS_UNCONNECTED;
        break;

      default:
        return -EINVAL;
    }

  g_sco_socket_shutdowns++;
  return sk->sk_err != 0 ? -sk->sk_err : 0;
}

static const struct proto_ops g_linux_bt_sco_sock_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .release = linux_bt_sco_sock_release,
  .bind = linux_bt_sco_sock_bind,
  .getname = linux_bt_sco_sock_getname,
  .sendmsg = linux_bt_sco_sock_sendmsg,
  .recvmsg = linux_bt_sco_sock_recvmsg,
  .ioctl = bt_sock_ioctl,
  .poll = linux_bt_sock_poll,
  .connect = linux_bt_sco_sock_connect,
  .listen = linux_bt_sco_sock_listen,
  .accept = linux_bt_sco_sock_accept,
  .mmap = sock_no_mmap,
  .socketpair = sock_no_socketpair,
  .shutdown = linux_bt_sco_sock_shutdown,
  .setsockopt = linux_bt_sco_sock_setsockopt,
  .getsockopt = linux_bt_sco_sock_getsockopt,
  .gettstamp = sock_gettstamp,
};

static int linux_bt_sco_sock_create(struct net *net, struct socket *sock,
                                    int protocol, int kern)
{
  struct linux_bt_sco_pinfo *pi;
  struct sock *sk;
  unsigned int i;

  (void)kern;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  if (sock->type != SOCK_SEQPACKET)
    {
      return -ESOCKTNOSUPPORT;
    }

  sk = bt_sock_alloc(net, sock, &g_linux_bt_sco_sock_proto, protocol,
                     GFP_ATOMIC, 0);
  if (sk == NULL)
    {
      return -ENOMEM;
    }

  sock->ops = &g_linux_bt_sco_sock_ops;
  sock->state = SS_UNCONNECTED;
  sk->sk_state = BT_OPEN;
  sk->sk_type = sock->type;
  pi = linux_bt_sco_pi(sk);
  bacpy(&pi->src, BDADDR_ANY);
  bacpy(&pi->dst, BDADDR_ANY);
  pi->mtu = LINUX_BT_SCO_DEFAULT_MTU;
  pi->handle = LINUX_BT_SCO_DEFAULT_HANDLE;
  pi->voice = BT_VOICE_CVSD_16BIT;
  pi->defer_setup = false;
  pi->pkt_status = false;

  for (i = 0; i < sizeof(g_sco_socks) / sizeof(g_sco_socks[0]); i++)
    {
      if (g_sco_socks[i] == NULL)
        {
          g_sco_socks[i] = sk;
          break;
        }
    }

  return 0;
}

static const struct net_proto_family g_linux_bt_sco_sock_family_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .create = linux_bt_sco_sock_create,
};

int sco_init(void)
{
  int ret;

  if (g_sco_sock_proto_registered)
    {
      return -EALREADY;
    }

  ret = proto_register(&g_linux_bt_sco_sock_proto, 0);
  if (ret < 0)
    {
      return ret;
    }

  g_sco_sock_proto_registered = true;
  ret = bt_sock_register(BTPROTO_SCO,
                         &g_linux_bt_sco_sock_family_ops);
  if (ret < 0)
    {
      proto_unregister(&g_linux_bt_sco_sock_proto);
      g_sco_sock_proto_registered = false;
      return ret;
    }

  g_sco_socket_registers++;
  return 0;
}

void sco_exit(void)
{
  if (g_sco_sock_proto_registered)
    {
      bt_sock_unregister(BTPROTO_SCO);
      g_sco_socket_unregisters++;
      proto_unregister(&g_linux_bt_sco_sock_proto);
      g_sco_sock_proto_registered = false;
    }
}
#else
int sco_init(void)
{
  return 0;
}

void sco_exit(void)
{
}
#endif

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_MGMT
int mgmt_init(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
  int ret;

  ret = hci_mgmt_chan_register(&g_linux_bt_mgmt_chan);
  if (ret >= 0)
    {
      g_linux_bt_mgmt_chan_registered = true;
    }

  return ret;
#else
  return 0;
#endif
}

void mgmt_exit(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
  if (g_linux_bt_mgmt_chan_registered)
    {
      hci_mgmt_chan_unregister(&g_linux_bt_mgmt_chan);
      g_linux_bt_mgmt_chan_registered = false;
    }
#endif
}
#endif

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_ISO
#ifdef CONFIG_NET_LINUX_BLUETOOTH_ISO_LEGACY_DIAGNOSTIC
static struct proto g_linux_bt_iso_sock_proto =
{
  .name = "ISO",
  .obj_size = sizeof(struct linux_bt_iso_pinfo),
};

static int linux_bt_iso_sock_release(struct socket *sock)
{
  unsigned int i;

  if (sock == NULL)
    {
      return 0;
    }

  if (sock->sk != NULL)
    {
      for (i = 0; i < sizeof(g_iso_bound_socks) /
                      sizeof(g_iso_bound_socks[0]); i++)
        {
          if (g_iso_bound_socks[i].sk == sock->sk)
            {
              memset(&g_iso_bound_socks[i], 0,
                     sizeof(g_iso_bound_socks[i]));
              break;
            }
        }

      sock_orphan(sock->sk);
      sock_put(sock->sk);
      sock->sk = NULL;
    }

  return 0;
}

static int linux_bt_iso_sock_bind(struct socket *sock,
                                  struct sockaddr *addr,
                                  int addr_len)
{
  struct sockaddr_iso *iaddr;
  struct linux_bt_iso_pinfo *pi;
  unsigned int i;

  if (sock == NULL || sock->sk == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  if (addr_len < (int)sizeof(*iaddr))
    {
      return -EINVAL;
    }

  iaddr = (struct sockaddr_iso *)addr;
  if (iaddr->iso_family != AF_BLUETOOTH)
    {
      return -EAFNOSUPPORT;
    }

  if (sock->sk->sk_state == BT_BOUND ||
      sock->sk->sk_state == BT_CONNECTED)
    {
      return -EALREADY;
    }

  for (i = 0; i < sizeof(g_iso_bound_socks) /
                  sizeof(g_iso_bound_socks[0]); i++)
    {
      if (g_iso_bound_socks[i].sk == NULL)
        {
          g_iso_bound_socks[i].sk = sock->sk;
          g_iso_bound_socks[i].bdaddr_type = iaddr->iso_bdaddr_type;
          bacpy(&g_iso_bound_socks[i].bdaddr, &iaddr->iso_bdaddr);
          sock->sk->sk_state = BT_BOUND;
          pi = linux_bt_iso_pi(sock->sk);
          (void)pi;
          g_iso_socket_binds++;
          return 0;
        }
    }

  return -ENOMEM;
}

static int linux_bt_iso_sock_connect(struct socket *sock,
                                     struct sockaddr *addr,
                                     int addr_len,
                                     int flags)
{
  struct sockaddr_iso *iaddr;
  struct linux_bt_iso_pinfo *pi;
  struct linux_bt_iso_bind_state *state = NULL;
  unsigned int i;
  int ret;

  (void)flags;

  if (sock == NULL || sock->sk == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  if (addr_len < (int)sizeof(*iaddr))
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state != BT_BOUND)
    {
      return -ENOTCONN;
    }

  iaddr = (struct sockaddr_iso *)addr;
  if (iaddr->iso_family != AF_BLUETOOTH)
    {
      return -EAFNOSUPPORT;
    }

  for (i = 0; i < sizeof(g_iso_bound_socks) /
                  sizeof(g_iso_bound_socks[0]); i++)
    {
      if (g_iso_bound_socks[i].sk == sock->sk)
        {
          state = &g_iso_bound_socks[i];
          break;
        }
    }

  if (state == NULL)
    {
      return -ENOTCONN;
    }

  sock->sk->sk_state = BT_CONNECTED;
  state->bdaddr_type = iaddr->iso_bdaddr_type;
  bacpy(&state->bdaddr, &iaddr->iso_bdaddr);
  pi = linux_bt_iso_pi(sock->sk);
  ret = linux_bt_hci_ensure_connected_handle(
    linux_bt_iso_link_type(pi->handle), pi->handle, &state->bdaddr,
    state->bdaddr_type, true, 0);
  if (ret < 0)
    {
      return ret;
    }

  g_iso_socket_connects++;
  return 0;
}

static int linux_bt_iso_sock_listen(struct socket *sock, int backlog)
{
  if (sock == NULL || sock->sk == NULL)
    {
      return -EINVAL;
    }

  if (backlog < 0)
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state != BT_BOUND &&
      sock->sk->sk_state != BT_CONNECTED)
    {
      return -ENOTCONN;
    }

  sock->sk->sk_state = BT_LISTEN;
  return 0;
}

static int linux_bt_iso_sock_getname(struct socket *sock,
                                     struct sockaddr *addr,
                                     int peer)
{
  struct sockaddr_iso *iaddr = (struct sockaddr_iso *)addr;
  struct linux_bt_iso_bind_state *state = NULL;
  unsigned int i;

  (void)peer;

  if (sock == NULL || sock->sk == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(g_iso_bound_socks) /
                  sizeof(g_iso_bound_socks[0]); i++)
    {
      if (g_iso_bound_socks[i].sk == sock->sk)
        {
          state = &g_iso_bound_socks[i];
          break;
        }
    }

  if (state == NULL)
    {
      return -ENOTCONN;
    }

  memset(iaddr, 0, sizeof(*iaddr));
  iaddr->iso_family = AF_BLUETOOTH;
  iaddr->iso_bdaddr_type = state->bdaddr_type;
  bacpy(&iaddr->iso_bdaddr, &state->bdaddr);
  return sizeof(*iaddr);
}

static int linux_bt_iso_sock_accept(struct socket *sock,
                                    struct socket *newsock,
                                    struct proto_accept_arg *arg)
{
  const struct net_proto_family *family;
  struct linux_bt_iso_bind_state *state = NULL;
  unsigned int i;
  int ret;

  (void)arg;

  if (sock == NULL || newsock == NULL || sock->sk == NULL ||
      sock->sk->sk_state != BT_LISTEN)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(g_iso_bound_socks) /
                  sizeof(g_iso_bound_socks[0]); i++)
    {
      if (g_iso_bound_socks[i].sk == sock->sk)
        {
          state = &g_iso_bound_socks[i];
          break;
        }
    }

  if (state == NULL)
    {
      return -ENOTCONN;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  newsock->type = sock->type;
  ret = family->create(&init_net, newsock, BTPROTO_ISO, 0);
  if (ret < 0)
    {
      return ret;
    }

  for (i = 0; i < sizeof(g_iso_bound_socks) /
                  sizeof(g_iso_bound_socks[0]); i++)
    {
      if (g_iso_bound_socks[i].sk == NULL)
        {
          memcpy(&g_iso_bound_socks[i], state,
                 sizeof(g_iso_bound_socks[i]));
          g_iso_bound_socks[i].sk = newsock->sk;
          newsock->sk->sk_state = BT_CONNECTED;
          newsock->state = SS_CONNECTED;
          return 0;
        }
    }

  if (newsock->ops != NULL && newsock->ops->release != NULL)
    {
      (void)newsock->ops->release(newsock);
    }

  return -ENOMEM;
}

static int linux_bt_iso_sock_setsockopt(struct socket *sock, int level,
                                        int optname, char *optval,
                                        unsigned int optlen)
{
  struct linux_bt_iso_pinfo *pi;
  struct bt_iso_qos qos;
  uint32_t opt;

  if (sock == NULL || sock->sk == NULL || optval == NULL)
    {
      return -EINVAL;
    }

  if (level != SOL_BLUETOOTH)
    {
      return -ENOPROTOOPT;
    }

  pi = linux_bt_iso_pi(sock->sk);

  switch (optname)
    {
      case BT_DEFER_SETUP:
        if (sock->sk->sk_state != BT_BOUND &&
            sock->sk->sk_state != BT_LISTEN)
          {
            return -EINVAL;
          }

        if (optlen < sizeof(opt))
          {
            return -EINVAL;
          }

        memcpy(&opt, optval, sizeof(opt));
        pi->defer_setup = opt != 0;
        return 0;

      case BT_PKT_STATUS:
        if (optlen < sizeof(opt))
          {
            return -EINVAL;
          }

        memcpy(&opt, optval, sizeof(opt));
        pi->pkt_status = opt != 0;
        return 0;

      case BT_PKT_SEQNUM:
        if (optlen < sizeof(opt))
          {
            return -EINVAL;
          }

        memcpy(&opt, optval, sizeof(opt));
        pi->pkt_seqnum = opt != 0;
        return 0;

      case BT_ISO_QOS:
        if (sock->sk->sk_state != BT_OPEN &&
            sock->sk->sk_state != BT_BOUND &&
            sock->sk->sk_state != BT_CONNECT2 &&
            sock->sk->sk_state != BT_CONNECTED)
          {
            return -EINVAL;
          }

        if (optlen < sizeof(qos))
          {
            return -EINVAL;
          }

        memcpy(&qos, optval, sizeof(qos));
        pi->qos = qos;
        pi->qos_user_set = true;
        return 0;

      case BT_ISO_BASE:
        if (sock->sk->sk_state != BT_OPEN &&
            sock->sk->sk_state != BT_BOUND &&
            sock->sk->sk_state != BT_CONNECT2)
          {
            return -EINVAL;
          }

        if (optlen > sizeof(pi->base))
          {
            return -EINVAL;
          }

        memcpy(pi->base, optval, optlen);
        pi->base_len = (uint8_t)optlen;
        return 0;

      default:
        return -ENOPROTOOPT;
    }
}

static int linux_bt_iso_sock_getsockopt(struct socket *sock, int level,
                                        int optname, char *optval,
                                        int *optlen)
{
  struct linux_bt_iso_pinfo *pi;
  uint32_t opt;

  if (sock == NULL || sock->sk == NULL || optval == NULL ||
      optlen == NULL)
    {
      return -EINVAL;
    }

  if (level != SOL_BLUETOOTH)
    {
      return -ENOPROTOOPT;
    }

  pi = linux_bt_iso_pi(sock->sk);

  switch (optname)
    {
      case BT_DEFER_SETUP:
        if (sock->sk->sk_state == BT_CONNECTED)
          {
            return -EINVAL;
          }

        opt = pi->defer_setup ? 1 : 0;
        return linux_bt_sock_copy_opt(optval, optlen, &opt, sizeof(opt));

      case BT_PKT_STATUS:
        opt = pi->pkt_status ? 1 : 0;
        return linux_bt_sock_copy_opt(optval, optlen, &opt, sizeof(opt));

      case BT_PKT_SEQNUM:
        opt = pi->pkt_seqnum ? 1 : 0;
        return linux_bt_sock_copy_opt(optval, optlen, &opt, sizeof(opt));

      case BT_ISO_QOS:
        return linux_bt_sock_copy_opt(optval, optlen, &pi->qos,
                                      sizeof(pi->qos));

      case BT_ISO_BASE:
        return linux_bt_sock_copy_opt(optval, optlen, pi->base,
                                      pi->base_len);

      default:
        return -ENOPROTOOPT;
    }
}

static int linux_bt_iso_sock_sendmsg(struct socket *sock,
                                     struct msghdr *msg,
                                     size_t len)
{
  struct linux_bt_iso_pinfo *pi;
  uint8_t iso[4 + 4 + 251];
  uint16_t iso_len;
  int ret;

  if (sock == NULL || sock->sk == NULL || msg == NULL)
    {
      return -EINVAL;
    }

  if (sock->sk->sk_state != BT_BOUND &&
      sock->sk->sk_state != BT_CONNECTED)
    {
      return -ENOTCONN;
    }

  if (len == 0 || len > sizeof(iso) - 8 || msg->msg_iter.count < len)
    {
      return -EMSGSIZE;
    }

  pi = linux_bt_iso_pi(sock->sk);
  iso_len = (uint16_t)(4 + len);
  iso[0] = (uint8_t)(pi->handle & 0xff);
  iso[1] = (uint8_t)(pi->handle >> 8);
  iso[2] = (uint8_t)(iso_len & 0xff);
  iso[3] = (uint8_t)(iso_len >> 8);
  iso[4] = 0x01;
  iso[5] = 0x00;
  iso[6] = (uint8_t)(len & 0xff);
  iso[7] = (uint8_t)(len >> 8);
  memcpy(&iso[8], msg->msg_iter.data, len);

  ret = linux_bt_upstream_hci_send(HCI_ISODATA_PKT, iso, len + 8);
  if (ret >= 0)
    {
#ifdef CONFIG_SIM_BTHWSIM
#  if CONFIG_SIM_BTHWSIM_ROLE == 3
      (void)linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ISO, 4, iso,
                                len + 8);
#  elif CONFIG_SIM_BTHWSIM_ROLE == 4
      (void)linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ISO, 3, iso,
                                len + 8);
#  else
      (void)linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ISO,
                                LINUX_BT_HWSIM_DST_BROADCAST, iso,
                                len + 8);
#  endif
#endif
      g_iso_socket_sends++;
      return (int)len;
    }

  return ret;
}

static int linux_bt_iso_sock_recvmsg(struct socket *sock,
                                     struct msghdr *msg,
                                     size_t len,
                                     int flags)
{
  struct sk_buff *skb;
  size_t copied;

  (void)flags;

  if (sock == NULL || sock->sk == NULL || msg == NULL)
    {
      return -EINVAL;
    }

  skb = skb_dequeue(&sock->sk->sk_receive_queue);
  if (skb == NULL)
    {
      return -EAGAIN;
    }

  copied = skb->len < len ? skb->len : len;
  if (copied < skb->len)
    {
      msg->msg_flags |= MSG_TRUNC;
    }

  if (copy_to_iter(skb->data, copied, &msg->msg_iter) != copied)
    {
      kfree_skb(skb);
      return -EFAULT;
    }

  kfree_skb(skb);
  return (int)copied;
}

static int linux_bt_iso_sock_shutdown(struct socket *sock, int how)
{
  struct sock *sk;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  sk = sock->sk;
  if (sk == NULL)
    {
      return 0;
    }

  switch (how)
    {
      case SHUT_RD:
        if (sk->sk_shutdown & RCV_SHUTDOWN)
          {
            return 0;
          }

        sk->sk_shutdown |= RCV_SHUTDOWN;
        break;

      case SHUT_WR:
        if (sk->sk_shutdown & SEND_SHUTDOWN)
          {
            return 0;
          }

        sk->sk_shutdown |= SEND_SHUTDOWN;
        break;

      case SHUT_RDWR:
        if ((sk->sk_shutdown & SHUTDOWN_MASK) == SHUTDOWN_MASK)
          {
            return 0;
          }

        sk->sk_shutdown |= SHUTDOWN_MASK;
        break;

      default:
        return -EINVAL;
    }

  if ((sk->sk_shutdown & SHUTDOWN_MASK) == SHUTDOWN_MASK)
    {
      sk->sk_state = BT_CLOSED;
      sock->state = SS_UNCONNECTED;
    }

  g_iso_socket_shutdowns++;
  return 0;
}

static const struct proto_ops g_linux_bt_iso_sock_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .release = linux_bt_iso_sock_release,
  .bind = linux_bt_iso_sock_bind,
  .getname = linux_bt_iso_sock_getname,
  .sendmsg = linux_bt_iso_sock_sendmsg,
  .recvmsg = linux_bt_iso_sock_recvmsg,
  .poll = linux_bt_sock_poll,
  .ioctl = bt_sock_ioctl,
  .listen = linux_bt_iso_sock_listen,
  .shutdown = linux_bt_iso_sock_shutdown,
  .connect = linux_bt_iso_sock_connect,
  .socketpair = sock_no_socketpair,
  .accept = linux_bt_iso_sock_accept,
  .setsockopt = linux_bt_iso_sock_setsockopt,
  .getsockopt = linux_bt_iso_sock_getsockopt,
  .mmap = sock_no_mmap,
};

static int linux_bt_iso_sock_create(struct net *net, struct socket *sock,
                                    int protocol, int kern)
{
  struct linux_bt_iso_pinfo *pi;
  struct sock *sk;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  switch (sock->type)
    {
      case SOCK_SEQPACKET:
        break;

      default:
        return -ESOCKTNOSUPPORT;
    }

  sk = bt_sock_alloc(net, sock, &g_linux_bt_iso_sock_proto, protocol,
                     GFP_KERNEL, kern);
  if (sk == NULL)
    {
      return -ENOMEM;
    }

  sock->ops = &g_linux_bt_iso_sock_ops;
  sock->state = SS_UNCONNECTED;
  sk->sk_state = BT_OPEN;
  pi = linux_bt_iso_pi(sk);
  pi->qos.ucast.cig = BT_ISO_QOS_CIG_UNSET;
  pi->qos.ucast.cis = BT_ISO_QOS_CIS_UNSET;
  return 0;
}

static const struct net_proto_family g_linux_bt_iso_sock_family_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .create = linux_bt_iso_sock_create,
};
#endif

int iso_init(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_ISO_LEGACY_DIAGNOSTIC
  int ret;

  ret = proto_register(&g_linux_bt_iso_sock_proto, 0);
  if (ret < 0)
    {
      return ret;
    }

  g_iso_sock_proto_registered = true;
  ret = bt_sock_register(BTPROTO_ISO, &g_linux_bt_iso_sock_family_ops);
  if (ret < 0)
    {
      proto_unregister(&g_linux_bt_iso_sock_proto);
      g_iso_sock_proto_registered = false;
      return ret;
    }

  g_iso_socket_registers++;
#elif defined(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF)
  return -ENOSYS;
#endif
  return 0;
}

int iso_exit(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_ISO_LEGACY_DIAGNOSTIC
  if (g_iso_sock_proto_registered)
    {
      bt_sock_unregister(BTPROTO_ISO);
      g_iso_socket_unregisters++;
      proto_unregister(&g_linux_bt_iso_sock_proto);
      g_iso_sock_proto_registered = false;
    }
#endif
  return 0;
}
#endif

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CORE
int hci_ethtool_ts_info(unsigned int index, int sk_proto,
                        struct kernel_ethtool_ts_info *ts_info)
{
  (void)index;
  (void)sk_proto;
  (void)ts_info;
  return -ENODEV;
}
#endif

static int linux_bt_proto_sock_queue_payload(struct sock *sk,
                                             const void *payload,
                                             size_t payload_len)
{
  struct sk_buff *skb;

  if (sk == NULL || (payload == NULL && payload_len > 0))
    {
      return -EINVAL;
    }

  skb = alloc_skb(payload_len, GFP_KERNEL);
  if (skb == NULL)
    {
      return -ENOMEM;
    }

  if (payload_len > 0)
    {
      skb_put_data(skb, payload, payload_len);
    }

  skb_queue_tail(&sk->sk_receive_queue, skb);
  if (sk->sk_data_ready != NULL)
    {
      sk->sk_data_ready(sk);
    }

  return 0;
}

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_L2CAP
static int linux_bt_upstream_l2cap_recv_native(const uint8_t *data,
                                               size_t payload_len,
                                               bool count_success)
{
  struct hci_dev *hdev;
  struct sk_buff *skb;
  uint16_t handle_flags;
  uint16_t handle;
  uint16_t flags;
  size_t acl_len;
  int ret;

  if (data == NULL || payload_len < 4)
    {
      return -EINVAL;
    }

  hdev = hci_dev_get(0);
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  handle_flags = (uint16_t)(data[0] | (data[1] << 8));
  handle = hci_handle(handle_flags);
  flags = hci_flags(handle_flags);
  if (flags == 0)
    {
      flags = ACL_START;
    }

  acl_len = payload_len - 4;
  skb = bt_skb_alloc(acl_len, GFP_ATOMIC);
  if (skb == NULL)
    {
      hci_dev_put(hdev);
      return -ENOMEM;
    }

  if (acl_len > 0)
    {
      memcpy(skb_put(skb, acl_len), &data[4], acl_len);
    }

  ret = l2cap_recv_acldata(hdev, handle, skb, flags);
  hci_dev_put(hdev);
  if (ret == 0 && count_success)
    {
      g_l2cap_socket_native_recvs++;
    }

  return ret;
}
#endif

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_ISO
static int linux_bt_upstream_iso_recv_native(const uint8_t *data,
                                             size_t payload_len)
{
  struct hci_dev *hdev;
  struct sk_buff *skb;
  uint16_t handle_flags;
  uint16_t handle;
  uint16_t flags;
  size_t iso_len;
  int ret;

  if (data == NULL || payload_len < 4)
    {
      return -EINVAL;
    }

  hdev = hci_dev_get(0);
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  handle_flags = (uint16_t)(data[0] | (data[1] << 8));
  handle = hci_handle(handle_flags);
  flags = hci_flags(handle_flags);
  iso_len = payload_len - 4;

  skb = bt_skb_alloc(iso_len, GFP_ATOMIC);
  if (skb == NULL)
    {
      hci_dev_put(hdev);
      return -ENOMEM;
    }

  if (iso_len > 0)
    {
      memcpy(skb_put(skb, iso_len), &data[4], iso_len);
    }

  ret = iso_recv(hdev, handle, skb, flags);
  hci_dev_put(hdev);
  if (ret == 0)
    {
      g_iso_socket_native_recvs++;
    }

  return ret;
}
#endif

int linux_bt_upstream_proto_sock_recv(uint8_t pkt_type,
                                      const void *payload,
                                      size_t payload_len)
{
  const uint8_t *data = payload;
  unsigned int i;
  unsigned int delivered = 0;
  bool l2cap_probe_queued = false;

  if (data == NULL && payload_len > 0)
    {
      return -EINVAL;
    }

  if (pkt_type == HCI_ACLDATA_PKT)
    {
      uint16_t handle;
      uint16_t l2cap_len;
      uint16_t cid;
      size_t data_len;
      bool l2cap_native_delivered = false;

      if (payload_len < 8)
        {
          return 0;
        }

      handle = (uint16_t)((data[0] | (data[1] << 8)) & 0x0fff);
      l2cap_len = (uint16_t)(data[4] | (data[5] << 8));
      cid = (uint16_t)(data[6] | (data[7] << 8));
      data_len = payload_len - 8;
      if (data_len > l2cap_len)
        {
          data_len = l2cap_len;
        }

#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
      for (i = 0; i < sizeof(g_rfcomm_socks) /
                      sizeof(g_rfcomm_socks[0]); i++)
        {
          struct linux_bt_rfcomm_pinfo *pi;
          const uint8_t *frame;
          size_t frame_len;
          size_t rfcomm_len;

          if (g_rfcomm_socks[i] == NULL ||
              (g_rfcomm_socks[i]->sk_state != BT_BOUND &&
               g_rfcomm_socks[i]->sk_state != BT_CONNECTED &&
               g_rfcomm_socks[i]->sk_state != BT_LISTEN))
            {
              continue;
            }

          pi = linux_bt_rfcomm_pi(g_rfcomm_socks[i]);
          if (pi->cid != cid)
            {
              continue;
            }

          if (pi->handle != 0 && pi->handle != handle)
            {
              continue;
            }

          if (data_len < 3)
            {
              continue;
            }

          frame = &data[8];
          frame_len = data_len;
          rfcomm_len = frame[2] >> 1;
          if (rfcomm_len > frame_len - 3)
            {
              rfcomm_len = frame_len - 3;
            }

          if (linux_bt_proto_sock_queue_payload(g_rfcomm_socks[i],
                                                &frame[3],
                                                rfcomm_len) == 0)
            {
              g_rfcomm_socket_recvs++;
              delivered++;
            }
        }
#endif

      if (g_l2cap_probe_bound &&
          g_l2cap_probe_socket.sk != NULL &&
          (g_l2cap_probe_socket.sk->sk_state == BT_BOUND ||
           g_l2cap_probe_socket.sk->sk_state == BT_CONNECTED ||
           g_l2cap_probe_socket.sk->sk_state == BT_LISTEN) &&
          (g_l2cap_probe_handle == 0 ||
           g_l2cap_probe_handle == handle) &&
          g_l2cap_probe_cid == cid)
        {
          if (linux_bt_proto_sock_queue_payload(g_l2cap_probe_socket.sk,
                                                &data[8], data_len) == 0)
            {
              g_l2cap_probe_rx_len = data_len;
              if (g_l2cap_probe_rx_len > sizeof(g_l2cap_probe_rx))
                {
                  g_l2cap_probe_rx_len = sizeof(g_l2cap_probe_rx);
                }

              if (g_l2cap_probe_rx_len > 0)
                {
                  memcpy(g_l2cap_probe_rx, &data[8],
                         g_l2cap_probe_rx_len);
                }

              g_l2cap_probe_rx_valid = true;
              l2cap_probe_queued = true;
              delivered++;
            }
        }

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_L2CAP
      if (g_l2cap_probe_bound &&
          g_l2cap_probe_socket.sk != NULL &&
          linux_bt_l2cap_bound_state(g_l2cap_probe_socket.sk) != NULL &&
          (cid != 0x0040 || g_l2cap_probe_native_control_cid) &&
          (g_l2cap_probe_cid == 0 || g_l2cap_probe_cid == cid) &&
          (g_l2cap_probe_handle == 0 ||
           g_l2cap_probe_handle == handle))
        {
          int attach_ret = linux_bt_l2cap_probe_attach_recv_chan(handle,
                                                                 cid);
          int native_ret = -ENOTCONN;

          if (attach_ret == 0)
            {
              native_ret = linux_bt_upstream_l2cap_recv_native(data,
                                                               payload_len,
                                                               true);
            }

          if (attach_ret == 0 && native_ret == 0)
            {
              if (linux_bt_proto_sock_queue_payload(g_l2cap_probe_socket.sk,
                                                    &data[8],
                                                    data_len) == 0)
                {
                  g_l2cap_probe_rx_len = data_len;
                  if (g_l2cap_probe_rx_len > sizeof(g_l2cap_probe_rx))
                    {
                      g_l2cap_probe_rx_len = sizeof(g_l2cap_probe_rx);
                    }

                  if (g_l2cap_probe_rx_len > 0)
                    {
                      memcpy(g_l2cap_probe_rx, &data[8],
                             g_l2cap_probe_rx_len);
                    }

                  g_l2cap_probe_rx_valid = true;
                  l2cap_probe_queued = true;
                  delivered++;
                }

              l2cap_native_delivered = true;
              delivered++;
              return (int)delivered;
            }
          else if (attach_ret < 0)
            {
              g_l2cap_socket_native_attach_fails++;
            }
          else
            {
              g_l2cap_socket_native_recv_fails++;
            }
        }
#endif

      for (i = 0; i < sizeof(g_l2cap_bound_socks) /
                      sizeof(g_l2cap_bound_socks[0]); i++)
        {
          if (g_l2cap_bound_socks[i].sk == NULL ||
              (g_l2cap_bound_socks[i].sk->sk_state != BT_BOUND &&
               g_l2cap_bound_socks[i].sk->sk_state != BT_CONNECTED &&
               g_l2cap_bound_socks[i].sk->sk_state != BT_LISTEN))
            {
              continue;
            }

          if (g_l2cap_bound_socks[i].cid != cid &&
              !(g_l2cap_bound_socks[i].cid == 0 &&
                g_l2cap_bound_socks[i].psm != 0 &&
                g_l2cap_bound_socks[i].sk->sk_state == BT_LISTEN &&
                cid == 0x0040))
            {
              continue;
            }

          if (g_l2cap_bound_socks[i].handle != 0 &&
              g_l2cap_bound_socks[i].handle != handle)
            {
              continue;
            }

          if (l2cap_native_delivered &&
              linux_bt_l2cap_bound_state(g_l2cap_bound_socks[i].sk) != NULL)
            {
              continue;
            }

#if defined(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_L2CAP) && \
    defined(CONFIG_SIM_BTHWSIM)
          if (!l2cap_native_delivered &&
              (cid != 0x0040 || g_l2cap_probe_native_control_cid) &&
              linux_bt_l2cap_bound_state(g_l2cap_bound_socks[i].sk) != NULL)
            {
              int attach_ret;
              int native_ret = -ENOTCONN;

              attach_ret = linux_bt_l2cap_attach_recv_chan(
                &(struct socket){ .sk = g_l2cap_bound_socks[i].sk },
                handle, cid);
              if (attach_ret == 0)
                {
                  native_ret = linux_bt_upstream_l2cap_recv_native(
                    data, payload_len, true);
                }

              if (attach_ret == 0 && native_ret == 0)
                {
                  if (g_l2cap_bound_socks[i].cid == 0)
                    {
                      g_l2cap_bound_socks[i].cid = cid;
                    }

                  if (g_l2cap_bound_socks[i].handle == 0)
                    {
                      g_l2cap_bound_socks[i].handle = handle;
                    }

                  (void)linux_bt_hci_ensure_connected_handle(
                    g_l2cap_bound_socks[i].bdaddr_type != 0 ? LE_LINK :
                    ACL_LINK, g_l2cap_bound_socks[i].handle,
                    &g_l2cap_bound_socks[i].bdaddr,
                    g_l2cap_bound_socks[i].bdaddr_type,
                    g_l2cap_bound_socks[i].sk->sk_state == BT_CONNECTED,
                    0);

                  if (g_l2cap_probe_socket.sk == g_l2cap_bound_socks[i].sk &&
                      linux_bt_proto_sock_queue_payload(
                        g_l2cap_bound_socks[i].sk, &data[8],
                        data_len) == 0)
                    {
                      g_l2cap_probe_rx_len = data_len;
                      if (g_l2cap_probe_rx_len > sizeof(g_l2cap_probe_rx))
                        {
                          g_l2cap_probe_rx_len = sizeof(g_l2cap_probe_rx);
                        }

                      if (g_l2cap_probe_rx_len > 0)
                        {
                          memcpy(g_l2cap_probe_rx, &data[8],
                                 g_l2cap_probe_rx_len);
                        }

                      g_l2cap_probe_rx_valid = true;
                      l2cap_probe_queued = true;
                      delivered++;
                    }

                  l2cap_native_delivered = true;
                  delivered++;
                  return (int)delivered;
                }
              else if (attach_ret < 0)
                {
                  g_l2cap_socket_native_attach_fails++;
                }
              else
                {
                  g_l2cap_socket_native_recv_fails++;
                }
            }
#endif

          if (linux_bt_proto_sock_queue_payload(g_l2cap_bound_socks[i].sk,
                                                &data[8], data_len) == 0)
            {
              if (g_l2cap_probe_socket.sk == g_l2cap_bound_socks[i].sk)
                {
                  l2cap_probe_queued = true;
                }

              delivered++;
            }
        }

      if (g_iso_probe_bound &&
          g_iso_probe_socket.sk != NULL &&
          linux_bt_iso_bound_state(g_iso_probe_socket.sk) == NULL &&
          (g_iso_probe_socket.sk->sk_state == BT_BOUND ||
           g_iso_probe_socket.sk->sk_state == BT_CONNECTED) &&
          (g_iso_probe_handle == 0 || g_iso_probe_handle == handle))
        {
          if (linux_bt_proto_sock_queue_payload(g_iso_probe_socket.sk,
                                                &data[8], data_len) == 0)
            {
              delivered++;
            }
        }

      if (g_l2cap_probe_bound &&
          !l2cap_probe_queued &&
          !l2cap_native_delivered &&
          g_l2cap_probe_socket.sk != NULL &&
#ifndef CONFIG_SIM_BTHWSIM
          linux_bt_l2cap_bound_state(g_l2cap_probe_socket.sk) == NULL &&
#endif
          (g_l2cap_probe_socket.sk->sk_state == BT_BOUND ||
           g_l2cap_probe_socket.sk->sk_state == BT_CONNECTED ||
           g_l2cap_probe_socket.sk->sk_state == BT_LISTEN) &&
          (g_l2cap_probe_handle == 0 ||
           g_l2cap_probe_handle == handle) &&
          (g_l2cap_probe_cid == cid ||
           (g_l2cap_probe_cid == 0 && g_l2cap_probe_psm != 0 &&
            g_l2cap_probe_socket.sk->sk_state == BT_LISTEN &&
            cid == 0x0040)))
        {
          if (linux_bt_proto_sock_queue_payload(g_l2cap_probe_socket.sk,
                                                &data[8], data_len) == 0)
            {
              g_l2cap_probe_rx_len = data_len;
              if (g_l2cap_probe_rx_len > sizeof(g_l2cap_probe_rx))
                {
                  g_l2cap_probe_rx_len = sizeof(g_l2cap_probe_rx);
                }

              if (g_l2cap_probe_rx_len > 0)
                {
                  memcpy(g_l2cap_probe_rx, &data[8],
                         g_l2cap_probe_rx_len);
                }

              g_l2cap_probe_rx_valid = true;
              delivered++;
            }
        }

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      delivered += linux_bt_noaf_l2cap_queue(handle, cid, &data[8],
                                             data_len);
#endif

      if (delivered == 0)
        {
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP
          delivered += bnep_recv_l2cap_payload(cid, &data[8], data_len);
#endif
#ifdef CONFIG_NET_LINUX_BLUETOOTH_6LOWPAN_BRIDGE
          delivered += linux_bt_6lowpan_recv_l2cap_payload(cid, &data[8],
                                                           data_len);
#endif
        }

      if (delivered == 0)
        {
          linux_bt_l2cap_pending_store(handle, cid, &data[8], data_len);
        }

      g_l2cap_socket_recvs += delivered;
      return (int)delivered;
    }

  if (pkt_type == HCI_ISODATA_PKT)
    {
      struct linux_bt_iso_pinfo *pi;
      uint16_t handle;
      uint16_t sdu_len;
      size_t data_off;
      size_t data_len;

      if (payload_len < 4)
        {
          return 0;
        }

      handle = (uint16_t)((data[0] | (data[1] << 8)) & 0x0fff);

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_ISO
      if (g_iso_probe_upstream_attached &&
          (g_iso_probe_handle == 0 || g_iso_probe_handle == handle) &&
          linux_bt_upstream_iso_recv_native(data, payload_len) == 0)
        {
          delivered++;
        }
#endif

      sdu_len = (uint16_t)(data[2] | (data[3] << 8));
      if (sdu_len == 0 && payload_len >= 8)
        {
          data_off = 8;
          sdu_len = (uint16_t)(data[6] | (data[7] << 8));
        }
      else
        {
          data_off = 4;
          if (sdu_len == payload_len)
            {
              sdu_len = (uint16_t)(payload_len - data_off);
            }
        }

      if (payload_len < data_off)
        {
          return 0;
        }

      data_len = payload_len - data_off;
      if (data_len > sdu_len)
        {
          data_len = sdu_len;
        }

      for (i = 0; i < sizeof(g_iso_bound_socks) /
                      sizeof(g_iso_bound_socks[0]); i++)
        {
          if (g_iso_bound_socks[i].sk == NULL ||
              (g_iso_bound_socks[i].sk->sk_state != BT_BOUND &&
               g_iso_bound_socks[i].sk->sk_state != BT_CONNECTED))
            {
              continue;
            }

          pi = linux_bt_iso_pi(g_iso_bound_socks[i].sk);
          if (linux_bt_iso_bound_state(g_iso_bound_socks[i].sk) != NULL)
            {
              continue;
            }

          if (pi->handle != 0 && pi->handle != handle)
            {
              continue;
            }

          if (linux_bt_proto_sock_queue_payload(g_iso_bound_socks[i].sk,
                                                &data[data_off],
                                                data_len) == 0)
            {
              delivered++;
            }
        }

      if (g_iso_probe_bound &&
          g_iso_probe_socket.sk != NULL &&
          linux_bt_iso_bound_state(g_iso_probe_socket.sk) == NULL &&
          (g_iso_probe_socket.sk->sk_state == BT_BOUND ||
           g_iso_probe_socket.sk->sk_state == BT_CONNECTED) &&
          (g_iso_probe_handle == 0 || g_iso_probe_handle == handle))
        {
          if (linux_bt_proto_sock_queue_payload(g_iso_probe_socket.sk,
                                                &data[data_off],
                                                data_len) == 0)
            {
              g_iso_probe_rx_len = data_len;
              if (g_iso_probe_rx_len > sizeof(g_iso_probe_rx))
                {
                  g_iso_probe_rx_len = sizeof(g_iso_probe_rx);
                }

              if (g_iso_probe_rx_len > 0)
                {
                  memcpy(g_iso_probe_rx, &data[data_off],
                         g_iso_probe_rx_len);
                }

              g_iso_probe_rx_valid = true;
              delivered++;
            }
        }

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      delivered += linux_bt_noaf_iso_queue(handle, &data[data_off],
                                           data_len);
#endif

      g_iso_socket_recvs += delivered;
      return (int)delivered;
    }

  if (pkt_type == HCI_SCODATA_PKT)
    {
      uint16_t handle = 0;
      const uint8_t *sco_payload = data;
      size_t sco_len = payload_len;

      if (payload_len >= 3)
        {
          handle = (uint16_t)((data[0] | (data[1] << 8)) & 0x0fff);
          sco_len = data[2];
          if (sco_len > payload_len - 3)
            {
              sco_len = payload_len - 3;
            }

          sco_payload = &data[3];
        }

#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
      for (i = 0; i < sizeof(g_sco_socks) / sizeof(g_sco_socks[0]); i++)
        {
          struct linux_bt_sco_pinfo *pi;

          if (g_sco_socks[i] == NULL ||
              g_sco_socks[i]->sk_state != BT_CONNECTED)
            {
              continue;
            }

          pi = linux_bt_sco_pi(g_sco_socks[i]);
          if (handle != 0 && pi->handle != 0 && pi->handle != handle)
            {
              continue;
            }

          if (linux_bt_proto_sock_queue_payload(g_sco_socks[i],
                                                sco_payload,
                                                sco_len) == 0)
            {
              g_sco_socket_recvs++;
              delivered++;
            }
        }
#endif

      return (int)delivered;
    }

  return 0;
}

#ifdef CONFIG_SIM_BTHWSIM
static int linux_bt_sim_l2cap_poll_one(void)
{
  uint8_t payload[LINUX_BT_HWSIM_ACL_RAW_MAX];
  uint16_t src;
  uint16_t dst;
  uint32_t len;
  int ret;

  ret = linux_bt_hwsim_read_raw_named(LINUX_BT_HWSIM_TYPE_ACL,
                                      "l2cap-native", &src, &dst,
                                      payload, sizeof(payload), &len);
  if (ret <= 0)
    {
      return ret;
    }

  return linux_bt_upstream_proto_sock_recv(HCI_ACLDATA_PKT, payload, len);
}

#endif

int linux_bt_upstream_af_init(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
  int ret;

  ret = linux_bt_compat_subsys_initcall_bt_init();
  if (ret < 0)
    {
      return ret;
    }

  ret = bnep_sock_init();
  if (ret < 0 && ret != -EALREADY)
    {
      return ret;
    }

  ret = cmtp_sock_init();
  if (ret < 0 && ret != -EALREADY)
    {
      return ret;
    }

  ret = hidp_sock_init();
  if (ret < 0 && ret != -EALREADY)
    {
      return ret;
    }

  ret = rfcomm_sock_init();
  if (ret < 0 && ret != -EALREADY)
    {
      return ret;
    }

  ret = sco_init();
  if (ret < 0 && ret != -EALREADY)
    {
      return ret;
    }

  ret = iso_init();
  return ret == -EALREADY ? 0 : ret;
#else
  return 0;
#endif
}

int linux_bt_upstream_af_status(char *out, size_t out_len)
{
  const struct net_proto_family *family = NULL;
  char user_random_addr[18];
  unsigned int count = 0;
  unsigned int i;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(g_sock_family) / sizeof(g_sock_family[0]); i++)
    {
      if (g_sock_family[i] != NULL)
        {
          count++;
        }
    }

  if (PF_BLUETOOTH >= 0 &&
      PF_BLUETOOTH < (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      family = g_sock_family[PF_BLUETOOTH];
    }

  snprintf(user_random_addr, sizeof(user_random_addr),
           "%02x:%02x:%02x:%02x:%02x:%02x",
           g_hci_user_ctrl_state.random_addr[5],
           g_hci_user_ctrl_state.random_addr[4],
           g_hci_user_ctrl_state.random_addr[3],
           g_hci_user_ctrl_state.random_addr[2],
           g_hci_user_ctrl_state.random_addr[1],
           g_hci_user_ctrl_state.random_addr[0]);

  snprintf(out, out_len,
           "upstream-af: selected=%u pf_bluetooth=%u registered=%u "
           "families=%u "
           "hci-sock-fallback-compiled=%u "
           "l2cap-sock-fallback-compiled=%u "
           "iso-sock-fallback-compiled=%u "
           "bnep-native-active=%u "
           "bnep-native-netdev-setup=%lu "
           "bnep-native-netdev-register=%lu "
           "bnep-native-session-create=%lu "
           "bnep-native-session-link=%lu "
           "bnep-native-session-thread=%lu "
           "bnep-native-kthread-run=%lu "
           "bnep-native-ndo-start-xmit=%lu "
           "bnep-native-netdev-xmit=%lu "
           "bnep-native-tx-frame=%lu "
           "bnep-native-tx-frame-ok=%lu "
           "bnep-native-session-rx-dequeue=%lu "
           "bnep-native-l2cap-rx=%lu "
           "bnep-native-l2cap-delivered=%lu "
           "bnep-native-rx-frame=%lu "
           "bnep-native-rx-frame-ok=%lu "
           "bnep-native-netif-rx=%lu "
           "l2cap-socket-register=%lu "
           "l2cap-socket-unregister=%lu "
           "l2cap-socket-bind=%lu "
           "l2cap-socket-connect=%lu "
           "l2cap-socket-listen=%lu "
           "l2cap-socket-send=%lu "
           "l2cap-socket-recv=%lu "
           "l2cap-socket-native-recv=%lu "
           "l2cap-socket-native-attach-fail=%lu "
           "l2cap-socket-native-recv-fail=%lu "
           "rfcomm-socket-register=%lu "
           "rfcomm-socket-unregister=%lu "
           "rfcomm-socket-bind=%lu "
           "rfcomm-socket-connect=%lu "
           "rfcomm-socket-listen=%lu "
           "rfcomm-socket-send=%lu "
           "rfcomm-socket-recv=%lu "
           "rfcomm-socket-getname=%lu "
           "rfcomm-socket-setopt=%lu "
           "rfcomm-socket-getopt=%lu "
           "iso-socket-register=%lu "
           "iso-socket-unregister=%lu "
           "iso-socket-bind=%lu "
           "iso-socket-connect=%lu "
           "iso-socket-send=%lu iso-socket-recv=%lu "
           "iso-socket-native-recv=%lu "
           "hci-user-iso-cis-count=%u "
           "hci-user-iso-cis-handle=0x%04x "
           "hci-user-iso-create-cis=%lu "
           "hci-user-iso-create-cis-status=%lu "
           "hci-user-iso-cis-established-event=%lu "
           "hci-user-iso-read-tx-sync=%lu "
           "hci-user-iso-create-big=%lu "
           "hci-user-iso-create-big-status=%lu "
           "hci-user-iso-create-big-complete-event=%lu "
           "hci-user-iso-term-big=%lu "
           "hci-user-iso-term-big-status=%lu "
           "hci-user-iso-term-big-complete-event=%lu "
           "hci-user-iso-setup-path=%lu "
           "bnep-native-sock-ioctl-getconnlist=%lu "
           "bnep-native-sock-ioctl-getconninfo=%lu "
           "bnep-native-sock-ioctl-getsuppfeat=%lu "
           "bnep-native-sock-ioctl-connadd=%lu "
           "bnep-native-sock-ioctl-conndel=%lu "
           "bnep-ioctl-connadd=%lu "
           "bnep-ioctl-conndel=%lu "
           "bnep-native-session-create=%lu "
           "bnep-native-session-start=%lu "
           "bnep-native-session-stop=%lu "
           "bnep-native-session-terminate=%lu "
           "bnep-native-netdev-register=%lu "
           "bnep-native-netdev-unregister=%lu "
           "bnep-native-contract-version=1 "
           "bnep-native-helper-contract="
           "sock_ioctl_connadd,bnep_add_connection,netdev_setup,"
           "session_thread,ndo_start_xmit,bnep_rx_frame,conndel_cleanup "
           "bnep-native-helper-owner=net_bluetooth/bnep "
           "upstream-link="
           "bluez-fd-to-imported-bnep "
           "bnep-native-source-map="
           "sock.c,core.c,netdev.c,linux_bt_bnep_netdev.c "
           "bnep-native-session-ownership="
           "sockfd_lookup,BNEPCONNADD,bnep_add_connection,alloc_netdev,"
           "register_netdev,kthread_run,bnep_session "
           "bnep-native-thread-ownership="
           "kthread_run,bnep_session,rx_wait,tx_wait,stop_wakeup,"
           "session_terminate "
           "bnep-native-netdev-ownership="
           "alloc_netdev,netdev_ops,ndo_open,ndo_stop,ndo_start_xmit,"
           "netif_rx,unregister_netdev,free_netdev "
           "bnep-native-state-ownership="
           "session_new,session_active,session_stopping,session_closed,"
           "active_zero "
           "bnep-native-lock-ownership="
           "session_list,session_ref,tx_queue,rx_queue,ioctl_serialization "
           "bnep-native-error-ownership="
           "bad_fd,bad_role,duplicate_session,tx_fail,rx_drop,"
           "conndel_missing,cleanup_after_error "
           "bnep-native-datapath-ownership="
           "NuttX-IP,ndo_start_xmit,bnep_tx_frame,L2CAP,hwsim,"
           "bnep_rx_frame,netif_rx,NuttX-IP-RX "
           "bnep-native-core-tx-path=kernel_sendmsg-only "
           "bnep-native-fd-ownership="
           "connected-l2cap-fd,bnep-control-fd,sockfd_lookup,"
           "sockfd_put-error,session-sock-owner "
           "bnep-native-cleanup-ownership="
           "BNEPCONNDEL,bnep_del_connection,unregister_netdev,"
           "session_stop,fd_cleanup "
           "sock-register=%lu sock-unregister=%lu "
           "sock-create-probe=%lu proto-register=%lu "
           "proto-unregister=%lu hci-mgmt-socket-cmd=%lu "
           "hci-mgmt-socket-event=%lu hci-mgmt-socket-recv=%lu "
           "hci-mgmt-chan-register=%lu hci-mgmt-chan-unregister=%lu "
           "hci-mgmt-control-contract-version=1 "
           "hci-mgmt-control-ownership="
           "socket,bind-control,hci_mgmt_chan_register,"
           "hci_mgmt_chan_find,hci_mgmt_cmd,handler-dispatch "
           "hci-mgmt-command-ownership="
           "READ_VERSION,READ_COMMANDS,READ_INDEX_LIST,READ_INFO,"
           "SET_POWERED,SET_CONNECTABLE,SET_DISCOVERABLE,SET_BONDABLE,"
           "SET_LE,SET_BREDR,SET_ADVERTISING,START_DISCOVERY,"
           "STOP_DISCOVERY,PAIR_DEVICE,GET_CONN_INFO,DISCONNECT,"
           "UNPAIR_DEVICE,SET_IO_CAPABILITY "
           "hci-mgmt-event-ownership="
           "CMD_COMPLETE,CMD_STATUS,NEW_SETTINGS,DISCOVERING,"
           "DEVICE_FOUND,DEVICE_CONNECTED,DEVICE_DISCONNECTED,"
           "DEVICE_UNPAIRED,NEW_LONG_TERM_KEY "
           "hci-mgmt-pending-ownership="
           "pending_cmd,pending_pair,pending_disconnect,pending_unpair,"
           "pending_error_status "
           "hci-mgmt-cleanup-ownership="
           "control-fd-release,pending-cmd-free,event-queue-drain,"
           "adapter-device-ref-zero "
           "hci-monitor-register=%lu hci-monitor-unregister=%lu "
           "hci-monitor-event=%lu hci-data-socket-register=%lu "
           "hci-data-socket-unregister=%lu hci-data-socket-rx=%lu "
           "hci-filter-set=%lu hci-filter-get=%lu "
           "hci-getname=%lu "
           "hci-ioctl-devlist=%lu hci-ioctl-devinfo=%lu "
           "hci-ioctl-devup=%lu hci-ioctl-devdown=%lu "
           "hci-ioctl-devreset=%lu "
           "hci-ioctl-devrestat=%lu hci-ioctl-devcmd=%lu "
           "hci-ioctl-connlist=%lu hci-ioctl-conninfo=%lu "
           "hci-ioctl-authinfo=%lu "
           "hci-ioctl-blockaddr=%lu hci-ioctl-unblockaddr=%lu "
           "hci-user-random-addr-valid=%u hci-user-random-addr=%s "
           "hci-user-adv-enable=%u hci-user-adv-type=%u "
           "hci-user-adv-own-addr-type=%u "
           "hci-user-adv-filter-policy=%u "
           "hci-user-adv-interval-min=%u "
           "hci-user-adv-interval-max=%u "
           "hci-user-adv-data-len=%u "
           "hci-user-scan-rsp-len=%u "
           "hci-user-scan-enable=%u "
           "hci-user-scan-filter-dup=%u "
           "hci-user-scan-type=%u "
           "hci-user-scan-own-addr-type=%u "
           "hci-user-scan-filter-policy=%u "
           "hci-user-scan-interval=%u "
           "hci-user-scan-window=%u "
           "hci-user-accept-list-count=%u "
           "hci-user-resolv-list-count=%u "
           "hci-user-addr-resolv-enable=%u "
           "hci-user-rpa-timeout=%u "
           "hci-user-event-mask-page2-set=%lu "
           "hci-user-random-addr-set=%lu "
           "hci-user-adv-param-set=%lu "
           "hci-user-adv-data-set=%lu "
           "hci-user-scan-rsp-set=%lu "
           "hci-user-adv-enable-set=%lu "
           "hci-user-scan-param-set=%lu "
           "hci-user-scan-enable-set=%lu "
           "hci-user-accept-list-clear=%lu "
           "hci-user-accept-list-add=%lu "
           "hci-user-accept-list-del=%lu "
           "hci-user-resolv-list-clear=%lu "
           "hci-user-addr-resolv-set=%lu "
           "hci-user-rpa-timeout-set=%lu "
          "hci-user-adv-hwsim-tx=%lu "
          "hci-user-scan-hwsim-poll=%lu "
          "hci-user-scan-hwsim-report=%lu "
          "hci-user-iso-host-feature-bit=%u "
          "hci-user-iso-host-feature-value=%u "
          "hci-user-iso-cig-id=%u "
          "hci-user-iso-cis-count=%u "
          "hci-user-iso-cis-handle=0x%04x "
          "hci-user-iso-acl-handle=0x%04x "
          "hci-user-iso-big-id=%u "
          "hci-user-iso-bis-count=%u "
          "hci-user-iso-path-handle=0x%04x "
          "hci-user-iso-path-dir=%u "
          "hci-user-iso-cis-established=%u "
          "hci-user-iso-big-established=%u "
          "hci-user-iso-big-terminated=%u "
          "hci-user-iso-read-buffer-size-v2=%lu "
          "hci-user-iso-read-tx-sync=%lu "
          "hci-user-iso-host-feature-set=%lu "
          "hci-user-iso-cig-param-set=%lu "
          "hci-user-iso-create-cis=%lu "
          "hci-user-iso-remove-cig=%lu "
          "hci-user-iso-create-big=%lu "
          "hci-user-iso-term-big=%lu "
          "hci-user-iso-setup-path=%lu "
          "hci-user-iso-create-cis-status=%lu "
          "hci-user-iso-create-big-status=%lu "
          "hci-user-iso-term-big-status=%lu "
          "hci-user-iso-cis-established-event=%lu "
          "hci-user-iso-create-big-complete-event=%lu "
          "hci-user-iso-term-big-complete-event=%lu "
          "l2cap-socket-register=%lu l2cap-socket-unregister=%lu "
           "l2cap-socket-bind=%lu l2cap-socket-connect=%lu "
           "l2cap-socket-listen=%lu "
           "l2cap-socket-send=%lu "
           "l2cap-socket-recv=%lu "
           "l2cap-socket-native-recv=%lu "
           "l2cap-socket-native-attach-fail=%lu "
           "l2cap-socket-native-recv-fail=%lu "
           "rfcomm-socket-register=%lu "
           "rfcomm-socket-unregister=%lu "
           "rfcomm-socket-bind=%lu "
           "rfcomm-socket-connect=%lu "
           "rfcomm-socket-listen=%lu "
           "rfcomm-socket-send=%lu "
           "rfcomm-socket-recv=%lu "
           "rfcomm-socket-getname=%lu "
           "rfcomm-socket-setopt=%lu "
           "rfcomm-socket-getopt=%lu "
           "bnep-socket-register=%lu "
           "bnep-socket-unregister=%lu "
           "bnep-ioctl-suppfeat=%lu "
           "bnep-ioctl-connlist=%lu "
           "bnep-ioctl-conninfo=%lu "
           "bnep-ioctl-connadd=%lu "
           "bnep-ioctl-conndel=%lu "
           "bnep-core-active=%u "
           "bnep-core-add=%lu "
           "bnep-core-del=%lu "
           "bnep-legacy-fallback-compiled=%u "
           "bnep-netdev-register=%lu "
           "bnep-netdev-unregister=%lu "
           "bnep-native-active=%u "
           "bnep-native-netdev-register=%lu "
           "bnep-native-netdev-unregister=%lu "
           "bnep-native-netdev-xmit=%lu "
           "bnep-native-netdev-xmit-bytes=%lu "
           "bnep-native-tx-frame=%lu "
           "bnep-native-tx-frame-bytes=%lu "
           "bnep-native-tx-frame-ok=%lu "
           "bnep-native-l2cap-rx=%lu "
           "bnep-native-l2cap-rx-bytes=%lu "
           "bnep-native-l2cap-delivered=%lu "
           "bnep-native-rx-frame=%lu "
           "bnep-native-rx-frame-bytes=%lu "
           "bnep-native-rx-frame-ok=%lu "
           "bnep-native-netif-rx=%lu "
           "bnep-native-netif-rx-bytes=%lu "
           "bnep-native-session-create=%lu "
           "bnep-native-session-start=%lu "
           "bnep-native-session-stop=%lu "
           "bnep-native-session-terminate=%lu "
           "bnep-native-netdev-setup=%lu "
           "bnep-native-ndo-start-xmit=%lu "
           "bnep-native-session-link=%lu "
           "bnep-native-session-unlink=%lu "
           "bnep-native-session-thread=%lu "
           "bnep-native-kthread-run=%lu "
           "bnep-native-session-rx-dequeue=%lu "
           "bnep-native-session-tx-dequeue=%lu "
           "bnep-native-fd-handoff=%lu "
           "bnep-native-fd-active=%u "
           "bnep-native-fd-cleanup=%lu "
           "bnep-native-fd-last=%d "
           "bnep-native-fd-psm=0x%04x "
           "bnep-native-fd-cid=0x%04x "
           "bnep-native-fd-handle=0x%04x "
           "bnep-native-fd-role=0x%04x "
           "bnep-native-fd-source=%s "
           "bnep-native-fd-handoff-reject=%lu "
           "bnep-native-fd-lookup-kept-probe=%lu "
           "bnep-native-fd-lookup-rebind-probe=%lu "
           "bnep-native-core-tx-path=kernel_sendmsg-only "
           "bnep-native-sock-ioctl-getconnlist=%lu "
           "bnep-native-sock-ioctl-getconninfo=%lu "
           "bnep-native-sock-ioctl-getsuppfeat=%lu "
           "bnep-native-sock-ioctl-connadd=%lu "
           "bnep-native-sock-ioctl-conndel=%lu "
           "iso-socket-register=%lu "
           "iso-socket-unregister=%lu iso-socket-bind=%lu "
           "iso-socket-connect=%lu "
           "iso-socket-send=%lu iso-socket-recv=%lu "
           "iso-socket-native-recv=%lu\n",
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
           1,
#else
           0,
#endif
           PF_BLUETOOTH, family != NULL ? 1 : 0, count,
#ifdef CONFIG_NET_LINUX_BLUETOOTH_HCI_SOCK_LEGACY_DIAGNOSTIC
           1,
#else
           0,
#endif
#ifdef CONFIG_NET_LINUX_BLUETOOTH_L2CAP_LEGACY_DIAGNOSTIC
           1,
#else
           0,
#endif
#ifdef CONFIG_NET_LINUX_BLUETOOTH_ISO_LEGACY_DIAGNOSTIC
           1,
#else
           0,
#endif
           g_bnep_native_active,
           g_bnep_native_netdev_setups,
           g_bnep_native_netdev_registers,
           g_bnep_native_session_creates,
           g_bnep_native_session_links,
           g_bnep_native_session_threads,
           g_bnep_native_kthread_runs,
           g_bnep_native_ndo_start_xmits,
           g_bnep_native_netdev_xmit,
           g_bnep_native_tx_frame,
           g_bnep_native_tx_frame_ok,
           g_bnep_native_session_rx_dequeues,
           g_bnep_native_l2cap_rx,
           g_bnep_native_l2cap_delivered,
           g_bnep_native_rx_frame,
           g_bnep_native_rx_frame_ok,
           g_bnep_native_netif_rx,
           g_l2cap_socket_registers,
           g_l2cap_socket_unregisters,
           g_l2cap_socket_binds,
           g_l2cap_socket_connects,
           g_l2cap_socket_listens,
           g_l2cap_socket_sends,
           g_l2cap_socket_recvs,
           g_l2cap_socket_native_recvs,
           g_l2cap_socket_native_attach_fails,
           g_l2cap_socket_native_recv_fails,
           g_rfcomm_socket_registers,
           g_rfcomm_socket_unregisters,
           g_rfcomm_socket_binds,
           g_rfcomm_socket_connects,
           g_rfcomm_socket_listens,
           g_rfcomm_socket_sends,
           g_rfcomm_socket_recvs,
           g_rfcomm_socket_getnames,
           g_rfcomm_socket_setopt,
           g_rfcomm_socket_getopt,
           g_iso_socket_registers,
           g_iso_socket_unregisters,
           g_iso_socket_binds,
           g_iso_socket_connects,
           g_iso_socket_sends, g_iso_socket_recvs,
           g_iso_socket_native_recvs,
           g_hci_user_ctrl_state.iso_cis_count,
           g_hci_user_ctrl_state.iso_cis_handle,
           g_hci_user_ctrl_state.iso_create_cis,
           g_hci_user_ctrl_state.iso_create_cis_status,
           g_hci_user_ctrl_state.iso_cis_established_events,
           g_hci_user_ctrl_state.iso_read_tx_syncs,
           g_hci_user_ctrl_state.iso_create_big,
           g_hci_user_ctrl_state.iso_create_big_status,
           g_hci_user_ctrl_state.iso_create_big_complete_events,
           g_hci_user_ctrl_state.iso_term_big,
           g_hci_user_ctrl_state.iso_term_big_status,
           g_hci_user_ctrl_state.iso_term_big_complete_events,
           g_hci_user_ctrl_state.iso_setup_path,
           g_bnep_native_sock_ioctl_getconnlists,
           g_bnep_native_sock_ioctl_getconninfos,
           g_bnep_native_sock_ioctl_getsuppfeats,
           g_bnep_native_sock_ioctl_connadds,
           g_bnep_native_sock_ioctl_conndels,
           g_bnep_native_sock_ioctl_connadds,
           g_bnep_native_sock_ioctl_conndels,
           g_bnep_native_session_creates,
           g_bnep_native_session_starts,
           g_bnep_native_session_stops,
           g_bnep_native_session_terminates,
           g_bnep_native_netdev_registers,
           g_bnep_native_netdev_unregisters,
           g_sock_registers, g_sock_unregisters, g_sock_create_probes,
           g_proto_registers, g_proto_unregisters,
           g_hci_mgmt_socket_cmds, g_hci_mgmt_socket_events,
           g_hci_mgmt_socket_recvs, g_hci_mgmt_chan_registers,
           g_hci_mgmt_chan_unregisters, g_hci_monitor_registers,
           g_hci_monitor_unregisters, g_hci_monitor_events,
           g_hci_data_socket_registers, g_hci_data_socket_unregisters,
           g_hci_data_socket_rx, g_hci_filter_sets, g_hci_filter_gets,
           g_hci_getname_calls, g_hci_ioctl_devlist_calls,
           g_hci_ioctl_devinfo_calls, g_hci_ioctl_devup_calls,
           g_hci_ioctl_devdown_calls, g_hci_ioctl_devreset_calls,
           g_hci_ioctl_devrestat_calls, g_hci_ioctl_devcmd_calls,
           g_hci_ioctl_connlist_calls, g_hci_ioctl_conninfo_calls,
           g_hci_ioctl_authinfo_calls, g_hci_ioctl_blockaddr_calls,
           g_hci_ioctl_unblockaddr_calls,
           g_hci_user_ctrl_state.random_addr_valid ? 1 : 0,
           user_random_addr,
           g_hci_user_ctrl_state.adv_enable,
           g_hci_user_ctrl_state.adv_type,
           g_hci_user_ctrl_state.adv_own_addr_type,
           g_hci_user_ctrl_state.adv_filter_policy,
           g_hci_user_ctrl_state.adv_interval_min,
           g_hci_user_ctrl_state.adv_interval_max,
           g_hci_user_ctrl_state.adv_data_len,
           g_hci_user_ctrl_state.scan_rsp_len,
           g_hci_user_ctrl_state.scan_enable,
           g_hci_user_ctrl_state.scan_filter_dup,
           g_hci_user_ctrl_state.scan_type,
           g_hci_user_ctrl_state.scan_own_addr_type,
           g_hci_user_ctrl_state.scan_filter_policy,
           g_hci_user_ctrl_state.scan_interval,
           g_hci_user_ctrl_state.scan_window,
           g_hci_user_ctrl_state.accept_list_count,
           g_hci_user_ctrl_state.resolv_list_count,
           g_hci_user_ctrl_state.addr_resolv_enable,
           g_hci_user_ctrl_state.rpa_timeout,
           g_hci_user_ctrl_state.event_mask_page2_sets,
           g_hci_user_ctrl_state.random_addr_sets,
           g_hci_user_ctrl_state.adv_param_sets,
           g_hci_user_ctrl_state.adv_data_sets,
           g_hci_user_ctrl_state.scan_rsp_sets,
           g_hci_user_ctrl_state.adv_enable_sets,
           g_hci_user_ctrl_state.scan_param_sets,
           g_hci_user_ctrl_state.scan_enable_sets,
           g_hci_user_ctrl_state.accept_list_clears,
           g_hci_user_ctrl_state.accept_list_adds,
           g_hci_user_ctrl_state.accept_list_dels,
           g_hci_user_ctrl_state.resolv_list_clears,
           g_hci_user_ctrl_state.addr_resolv_sets,
           g_hci_user_ctrl_state.rpa_timeout_sets,
           g_hci_user_ctrl_state.adv_hwsim_tx,
           g_hci_user_ctrl_state.scan_hwsim_polls,
           g_hci_user_ctrl_state.scan_hwsim_reports,
           g_hci_user_ctrl_state.iso_host_feature_bit,
           g_hci_user_ctrl_state.iso_host_feature_value,
           g_hci_user_ctrl_state.iso_cig_id,
           g_hci_user_ctrl_state.iso_cis_count,
           g_hci_user_ctrl_state.iso_cis_handle,
           g_hci_user_ctrl_state.iso_acl_handle,
           g_hci_user_ctrl_state.iso_big_id,
           g_hci_user_ctrl_state.iso_bis_count,
           g_hci_user_ctrl_state.iso_path_handle,
           g_hci_user_ctrl_state.iso_path_dir,
           g_hci_user_ctrl_state.iso_cis_established,
           g_hci_user_ctrl_state.iso_big_established,
           g_hci_user_ctrl_state.iso_big_terminated,
           g_hci_user_ctrl_state.iso_read_buffer_size_v2s,
           g_hci_user_ctrl_state.iso_read_tx_syncs,
           g_hci_user_ctrl_state.iso_host_feature_sets,
           g_hci_user_ctrl_state.iso_cig_param_sets,
           g_hci_user_ctrl_state.iso_create_cis,
           g_hci_user_ctrl_state.iso_remove_cig,
           g_hci_user_ctrl_state.iso_create_big,
           g_hci_user_ctrl_state.iso_term_big,
           g_hci_user_ctrl_state.iso_setup_path,
           g_hci_user_ctrl_state.iso_create_cis_status,
           g_hci_user_ctrl_state.iso_create_big_status,
           g_hci_user_ctrl_state.iso_term_big_status,
           g_hci_user_ctrl_state.iso_cis_established_events,
           g_hci_user_ctrl_state.iso_create_big_complete_events,
           g_hci_user_ctrl_state.iso_term_big_complete_events,
           g_l2cap_socket_registers,
           g_l2cap_socket_unregisters, g_l2cap_socket_binds,
           g_l2cap_socket_connects, g_l2cap_socket_listens,
           g_l2cap_socket_sends,
           g_l2cap_socket_recvs,
           g_l2cap_socket_native_recvs,
           g_l2cap_socket_native_attach_fails,
           g_l2cap_socket_native_recv_fails,
           g_rfcomm_socket_registers,
           g_rfcomm_socket_unregisters,
           g_rfcomm_socket_binds,
           g_rfcomm_socket_connects,
           g_rfcomm_socket_listens,
           g_rfcomm_socket_sends,
           g_rfcomm_socket_recvs,
           g_rfcomm_socket_getnames,
           g_rfcomm_socket_setopt,
           g_rfcomm_socket_getopt,
           g_bnep_socket_registers, g_bnep_socket_unregisters,
           g_bnep_socket_ioctl_suppfeat,
           g_bnep_socket_ioctl_connlist,
           g_bnep_socket_ioctl_conninfo,
           g_bnep_socket_ioctl_connadd,
           g_bnep_socket_ioctl_conndel,
           linux_bt_bnep_core_active_count(),
           g_bnep_core_session_adds,
           g_bnep_core_session_dels,
#ifdef CONFIG_NET_LINUX_BLUETOOTH_BNEP_LEGACY_DIAGNOSTIC
           1,
#else
           0,
#endif
           g_bnep_core_netdev_registers,
           g_bnep_core_netdev_unregisters,
           g_bnep_native_active,
           g_bnep_native_netdev_registers,
           g_bnep_native_netdev_unregisters,
           g_bnep_native_netdev_xmit,
           g_bnep_native_netdev_xmit_bytes,
           g_bnep_native_tx_frame,
           g_bnep_native_tx_frame_bytes,
           g_bnep_native_tx_frame_ok,
           g_bnep_native_l2cap_rx,
           g_bnep_native_l2cap_rx_bytes,
           g_bnep_native_l2cap_delivered,
           g_bnep_native_rx_frame,
           g_bnep_native_rx_frame_bytes,
           g_bnep_native_rx_frame_ok,
           g_bnep_native_netif_rx,
           g_bnep_native_netif_rx_bytes,
           g_bnep_native_session_creates,
           g_bnep_native_session_starts,
           g_bnep_native_session_stops,
           g_bnep_native_session_terminates,
           g_bnep_native_netdev_setups,
           g_bnep_native_ndo_start_xmits,
           g_bnep_native_session_links,
           g_bnep_native_session_unlinks,
           g_bnep_native_session_threads,
           g_bnep_native_kthread_runs,
           g_bnep_native_session_rx_dequeues,
           g_bnep_native_session_tx_dequeues,
           g_bnep_native_fd_handoffs,
           g_bnep_native_fd_active,
           g_bnep_native_fd_handoff_cleanups,
           g_bnep_native_fd_last,
           g_bnep_native_fd_psm,
           g_bnep_native_fd_cid,
           g_bnep_native_fd_handle,
           g_bnep_native_fd_role,
           g_bnep_native_fd_source,
           g_bnep_native_fd_handoff_rejects,
           g_bnep_native_fd_lookup_kept_probe,
           g_bnep_native_fd_lookup_rebind_probe,
           g_bnep_native_sock_ioctl_getconnlists,
           g_bnep_native_sock_ioctl_getconninfos,
           g_bnep_native_sock_ioctl_getsuppfeats,
           g_bnep_native_sock_ioctl_connadds,
           g_bnep_native_sock_ioctl_conndels,
           g_iso_socket_registers, g_iso_socket_unregisters,
           g_iso_socket_binds, g_iso_socket_connects,
           g_iso_socket_sends, g_iso_socket_recvs,
           g_iso_socket_native_recvs);

  return 0;
}

int linux_bt_upstream_hidp_socket_session_probe(const char *role,
                                                uint16_t handle,
                                                char *out,
                                                size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct linux_bt_hidp_connadd_req add_req;
  struct linux_bt_hidp_conndel_req del_req;
  struct linux_bt_hidp_conninfo info[1];
  struct linux_bt_hidp_conninfo get_info;
  struct linux_bt_hidp_connlist_req list_req;
  struct linux_bt_hidp_core_session *core_session;
  struct socket sock;
  bdaddr_t bdaddr;
  unsigned int active;
  unsigned int core_control = 0;
  unsigned int core_input = 0;
  unsigned int core_interrupt = 0;
  unsigned int core_linked = 0;
  unsigned int core_thread = 0;
  unsigned int core_uhid = 0;
  unsigned int control_messages = 0;
  unsigned int input_events = 0;
  unsigned int output_reports = 0;
  uint32_t info_state = 0;
  uint16_t info_vendor = 0;
  uint16_t info_product = 0;
  int add_ret = -EOPNOTSUPP;
  int accept_ret = -EOPNOTSUPP;
  int bind_ret = -EOPNOTSUPP;
  int connect_ret = -EOPNOTSUPP;
  int dup_add_ret = -EOPNOTSUPP;
  int getname_ret = -EOPNOTSUPP;
  int listen_ret = -EOPNOTSUPP;
  int list_ret = -EOPNOTSUPP;
  int info_ret = -EOPNOTSUPP;
  int mmap_ret = -EOPNOTSUPP;
  int poll_op_null = 0;
  int recvmsg_ret = -EOPNOTSUPP;
  int sendmsg_ret = -EOPNOTSUPP;
  int shutdown_ret = -EOPNOTSUPP;
  int socketpair_ret = -EOPNOTSUPP;
  int del_ret = -EOPNOTSUPP;
  int post_del_info_ret = -EOPNOTSUPP;
  int dup_del_ret = -EOPNOTSUPP;
  int create_ret;
  int release_ret = 0;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  memset(&sock, 0, sizeof(sock));
  sock.type = SOCK_RAW;
  linux_bt_hidp_fill_bdaddr(handle, &bdaddr);

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-hidp-socket: family=PF_BLUETOOTH invalid "
               "proto=BTPROTO_HIDP role=%s\n",
               role != NULL ? role : "unknown");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      snprintf(out, out_len,
               "upstream-hidp-socket: family=PF_BLUETOOTH unregistered "
               "proto=BTPROTO_HIDP role=%s\n",
               role != NULL ? role : "unknown");
      return -EAFNOSUPPORT;
    }

  create_ret = family->create(&init_net, &sock, BTPROTO_HIDP, 0);
  if (create_ret == 0 && sock.ops != NULL)
    {
      struct sockaddr addr;
      struct socket pair;
      struct msghdr msg;
      struct proto_accept_arg arg;

      memset(&addr, 0, sizeof(addr));
      addr.sa_family = AF_BLUETOOTH;
      memset(&pair, 0, sizeof(pair));
      memset(&msg, 0, sizeof(msg));
      memset(&arg, 0, sizeof(arg));
      poll_op_null = sock.ops->poll == NULL ? 1 : 0;

      if (sock.ops->bind != NULL)
        {
          bind_ret = sock.ops->bind(&sock, &addr, sizeof(addr));
        }

      if (sock.ops->getname != NULL)
        {
          getname_ret = sock.ops->getname(&sock, &addr, 0);
        }

      if (sock.ops->sendmsg != NULL)
        {
          sendmsg_ret = sock.ops->sendmsg(&sock, &msg, 0);
        }

      if (sock.ops->recvmsg != NULL)
        {
          recvmsg_ret = sock.ops->recvmsg(&sock, &msg, 0, 0);
        }

      if (sock.ops->listen != NULL)
        {
          listen_ret = sock.ops->listen(&sock, 1);
        }

      if (sock.ops->shutdown != NULL)
        {
          shutdown_ret = sock.ops->shutdown(&sock, SHUT_RDWR);
        }

      if (sock.ops->connect != NULL)
        {
          connect_ret = sock.ops->connect(&sock, &addr, sizeof(addr), 0);
        }

      if (sock.ops->socketpair != NULL)
        {
          socketpair_ret = sock.ops->socketpair(&sock, &pair);
        }

      if (sock.ops->accept != NULL)
        {
          accept_ret = sock.ops->accept(&sock, &pair, &arg);
        }

      if (sock.ops->mmap != NULL)
        {
          mmap_ret = sock.ops->mmap(NULL, &sock, NULL);
        }
    }

  if (create_ret == 0 && sock.ops != NULL && sock.ops->ioctl != NULL)
    {
      memset(&add_req, 0, sizeof(add_req));
      add_req.ctrl_sock = handle;
      add_req.intr_sock = (int)handle + 1;
      add_req.parser = 1;
      add_req.subclass = 0x40;
      add_req.vendor = 0x05ac;
      add_req.product = 0x024f;
      add_req.version = 0x0111;
      snprintf(add_req.name, sizeof(add_req.name),
               "Feather HIDP hwsim %s", role != NULL ? role : "device");

      add_ret = sock.ops->ioctl(&sock, LINUX_BT_NATIVE_HIDPCONNADD,
                                (unsigned long)&add_req);
      dup_add_ret = sock.ops->ioctl(&sock, LINUX_BT_NATIVE_HIDPCONNADD,
                                    (unsigned long)&add_req);

      memset(info, 0, sizeof(info));
      list_req.cnum = 1;
      list_req.ci = info;
      list_ret = sock.ops->ioctl(&sock, LINUX_BT_NATIVE_HIDPGETCONNLIST,
                                 (unsigned long)&list_req);

      memset(&get_info, 0, sizeof(get_info));
      get_info.bdaddr = bdaddr;
      info_ret = sock.ops->ioctl(&sock, LINUX_BT_NATIVE_HIDPGETCONNINFO,
                                 (unsigned long)&get_info);
      if (info_ret == 0)
        {
          info_state = get_info.state;
          info_vendor = get_info.vendor;
          info_product = get_info.product;
        }

      core_session = linux_bt_hidp_find_session_by_bdaddr(&bdaddr);
      if (core_session != NULL)
        {
          core_linked = core_session->session_linked ? 1 : 0;
          core_thread = core_session->session_thread ? 1 : 0;
          core_control = core_session->control_ready ? 1 : 0;
          core_interrupt = core_session->interrupt_ready ? 1 : 0;
          core_input = core_session->input_registered ? 1 : 0;
          core_uhid = core_session->uhid_registered ? 1 : 0;
          control_messages = core_session->control_messages;
          input_events = core_session->input_events;
          output_reports = core_session->output_reports;
        }

      memset(&del_req, 0, sizeof(del_req));
      del_req.bdaddr = bdaddr;
      del_ret = sock.ops->ioctl(&sock, LINUX_BT_NATIVE_HIDPCONNDEL,
                                (unsigned long)&del_req);

      memset(&get_info, 0, sizeof(get_info));
      get_info.bdaddr = bdaddr;
      post_del_info_ret =
        sock.ops->ioctl(&sock, LINUX_BT_NATIVE_HIDPGETCONNINFO,
                        (unsigned long)&get_info);
      dup_del_ret = sock.ops->ioctl(&sock, LINUX_BT_NATIVE_HIDPCONNDEL,
                                    (unsigned long)&del_req);
    }

  active = linux_bt_hidp_core_active_count();

  if (sock.ops != NULL && sock.ops->release != NULL)
    {
      release_ret = sock.ops->release(&sock);
    }

  snprintf(out, out_len,
           "upstream-hidp-socket: proto=BTPROTO_HIDP role=%s "
           "create-ret=%d ioctl=HIDPCONNADD ret=%d "
           "duplicate-ret=%d "
           "ioctl=HIDPGETCONNLIST ret=%d cnum=%u "
           "ioctl=HIDPGETCONNINFO ret=%d state=%u "
           "vendor=0x%04x product=0x%04x "
           "ioctl=HIDPCONNDEL ret=%d post-del-info-ret=%d "
           "missing-del-ret=%d final-active=%u "
           "core-session=linked:%u,thread:%u,control:%u,"
           "interrupt:%u,input:%u,uhid:%u "
           "core-traffic=control:%u,input:%u,output:%u "
           "no-data=sock_no bind-ret=%d getname-ret=%d "
           "sendmsg-ret=%d recvmsg-ret=%d listen-ret=%d "
           "shutdown-ret=%d connect-ret=%d socketpair-ret=%d "
           "accept-ret=%d mmap-ret=%d "
           "poll-op-null=%d release-ret=%d "
           "proto-name=%s "
           "source=third/linux-7.0.10/net/bluetooth/"
           "hidp/sock.c link=hwsim-l2cap-control-interrupt\n",
           role != NULL ? role : "unknown", create_ret, add_ret,
           dup_add_ret,
           list_ret, list_ret == 0 ? list_req.cnum : 0, info_ret,
           info_ret == 0 ? info_state : 0,
           info_ret == 0 ? info_vendor : 0,
           info_ret == 0 ? info_product : 0,
           del_ret, post_del_info_ret, dup_del_ret, active,
           core_linked, core_thread, core_control, core_interrupt,
           core_input, core_uhid, control_messages, input_events,
           output_reports,
           bind_ret, getname_ret, sendmsg_ret, recvmsg_ret,
           listen_ret, shutdown_ret, connect_ret, socketpair_ret,
           accept_ret, mmap_ret, poll_op_null, release_ret,
           g_linux_bt_hidp_sock_proto.name);

  if (create_ret < 0)
    {
      return create_ret;
    }

  if (add_ret < 0)
    {
      return add_ret;
    }

  if (list_ret < 0)
    {
      return list_ret;
    }

  if (info_ret < 0)
    {
      return info_ret;
    }

  if (del_ret < 0)
    {
      return del_ret;
    }

  if (dup_add_ret != -EALREADY)
    {
      return dup_add_ret < 0 ? dup_add_ret : -EINVAL;
    }

  if (post_del_info_ret != -ENOENT)
    {
      return post_del_info_ret < 0 ? post_del_info_ret : -EINVAL;
    }

  if (dup_del_ret != -ENOENT)
    {
      return dup_del_ret < 0 ? dup_del_ret : -EINVAL;
    }

  if (bind_ret != -EOPNOTSUPP ||
      getname_ret != -EOPNOTSUPP ||
      sendmsg_ret != -EOPNOTSUPP ||
      recvmsg_ret != -EOPNOTSUPP ||
      listen_ret != -EOPNOTSUPP ||
      shutdown_ret != -EOPNOTSUPP ||
      connect_ret != -EOPNOTSUPP ||
      socketpair_ret != -EOPNOTSUPP ||
      accept_ret != -EOPNOTSUPP ||
      mmap_ret != -EOPNOTSUPP)
    {
      return -EINVAL;
    }

  return release_ret;
}

int linux_bt_upstream_cmtp_socket_session_probe(uint16_t handle,
                                                char *out,
                                                size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct linux_bt_cmtp_connadd_req add_req;
  struct linux_bt_cmtp_conndel_req del_req;
  struct linux_bt_cmtp_conninfo info[1];
  struct linux_bt_cmtp_conninfo get_info;
  struct linux_bt_cmtp_connlist_req list_req;
  struct linux_bt_cmtp_core_session *core_session;
  struct socket sock;
  bdaddr_t bdaddr;
  unsigned int active;
  unsigned int blockids = 0;
  unsigned int capi_messages = 0;
  unsigned int core_capi = 0;
  unsigned int core_datapath = 0;
  unsigned int core_linked = 0;
  unsigned int core_reassembly = 0;
  unsigned int core_worker = 0;
  unsigned int rx_frames = 0;
  unsigned int tx_frames = 0;
  uint32_t info_flags = 0;
  uint32_t info_state = 0;
  int info_num = 0;
  int add_ret = -EOPNOTSUPP;
  int accept_ret = -EOPNOTSUPP;
  int bind_ret = -EOPNOTSUPP;
  int connect_ret = -EOPNOTSUPP;
  int dup_add_ret = -EOPNOTSUPP;
  int getname_ret = -EOPNOTSUPP;
  int listen_ret = -EOPNOTSUPP;
  int list_ret = -EOPNOTSUPP;
  int info_ret = -EOPNOTSUPP;
  int mmap_ret = -EOPNOTSUPP;
  int poll_op_null = 0;
  int recvmsg_ret = -EOPNOTSUPP;
  int sendmsg_ret = -EOPNOTSUPP;
  int shutdown_ret = -EOPNOTSUPP;
  int socketpair_ret = -EOPNOTSUPP;
  int del_ret = -EOPNOTSUPP;
  int post_del_info_ret = -EOPNOTSUPP;
  int dup_del_ret = -EOPNOTSUPP;
  int create_ret;
  int release_ret = 0;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  memset(&sock, 0, sizeof(sock));
  sock.type = SOCK_RAW;
  linux_bt_cmtp_fill_bdaddr(handle, &bdaddr);

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-cmtp-socket: family=PF_BLUETOOTH invalid "
               "proto=BTPROTO_CMTP\n");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      snprintf(out, out_len,
               "upstream-cmtp-socket: family=PF_BLUETOOTH unregistered "
               "proto=BTPROTO_CMTP\n");
      return -EAFNOSUPPORT;
    }

  create_ret = family->create(&init_net, &sock, BTPROTO_CMTP, 0);
  if (create_ret == 0 && sock.ops != NULL)
    {
      struct sockaddr addr;
      struct socket pair;
      struct msghdr msg;
      struct proto_accept_arg arg;

      memset(&addr, 0, sizeof(addr));
      addr.sa_family = AF_BLUETOOTH;
      memset(&pair, 0, sizeof(pair));
      memset(&msg, 0, sizeof(msg));
      memset(&arg, 0, sizeof(arg));
      poll_op_null = sock.ops->poll == NULL ? 1 : 0;

      if (sock.ops->bind != NULL)
        {
          bind_ret = sock.ops->bind(&sock, &addr, sizeof(addr));
        }

      if (sock.ops->getname != NULL)
        {
          getname_ret = sock.ops->getname(&sock, &addr, 0);
        }

      if (sock.ops->sendmsg != NULL)
        {
          sendmsg_ret = sock.ops->sendmsg(&sock, &msg, 0);
        }

      if (sock.ops->recvmsg != NULL)
        {
          recvmsg_ret = sock.ops->recvmsg(&sock, &msg, 0, 0);
        }

      if (sock.ops->listen != NULL)
        {
          listen_ret = sock.ops->listen(&sock, 1);
        }

      if (sock.ops->shutdown != NULL)
        {
          shutdown_ret = sock.ops->shutdown(&sock, SHUT_RDWR);
        }

      if (sock.ops->connect != NULL)
        {
          connect_ret = sock.ops->connect(&sock, &addr, sizeof(addr), 0);
        }

      if (sock.ops->socketpair != NULL)
        {
          socketpair_ret = sock.ops->socketpair(&sock, &pair);
        }

      if (sock.ops->accept != NULL)
        {
          accept_ret = sock.ops->accept(&sock, &pair, &arg);
        }

      if (sock.ops->mmap != NULL)
        {
          mmap_ret = sock.ops->mmap(NULL, &sock, NULL);
        }
    }

  if (create_ret == 0 && sock.ops != NULL && sock.ops->ioctl != NULL)
    {
      memset(&add_req, 0, sizeof(add_req));
      add_req.sock = handle;
      add_req.flags = BIT(LINUX_BT_CMTP_LOOPBACK);
      add_ret = sock.ops->ioctl(&sock, LINUX_BT_NATIVE_CMTPCONNADD,
                                (unsigned long)&add_req);
      dup_add_ret = sock.ops->ioctl(&sock, LINUX_BT_NATIVE_CMTPCONNADD,
                                    (unsigned long)&add_req);

      memset(info, 0, sizeof(info));
      list_req.cnum = 1;
      list_req.ci = info;
      list_ret = sock.ops->ioctl(&sock, LINUX_BT_NATIVE_CMTPGETCONNLIST,
                                 (unsigned long)&list_req);

      memset(&get_info, 0, sizeof(get_info));
      get_info.bdaddr = bdaddr;
      info_ret = sock.ops->ioctl(&sock, LINUX_BT_NATIVE_CMTPGETCONNINFO,
                                 (unsigned long)&get_info);
      if (info_ret == 0)
        {
          info_state = get_info.state;
          info_num = get_info.num;
          info_flags = get_info.flags;
        }

      core_session = linux_bt_cmtp_find_session_by_bdaddr(&bdaddr);
      if (core_session != NULL)
        {
          core_linked = core_session->session_linked ? 1 : 0;
          core_worker = core_session->worker_running ? 1 : 0;
          core_capi = core_session->capi_registered ? 1 : 0;
          core_reassembly = core_session->reassembly_ready ? 1 : 0;
          core_datapath = core_session->datapath_ready ? 1 : 0;
          blockids = core_session->blockids;
          tx_frames = core_session->tx_frames;
          rx_frames = core_session->rx_frames;
          capi_messages = core_session->capi_messages;
        }

      memset(&del_req, 0, sizeof(del_req));
      del_req.bdaddr = bdaddr;
      del_ret = sock.ops->ioctl(&sock, LINUX_BT_NATIVE_CMTPCONNDEL,
                                (unsigned long)&del_req);

      memset(&get_info, 0, sizeof(get_info));
      get_info.bdaddr = bdaddr;
      post_del_info_ret =
        sock.ops->ioctl(&sock, LINUX_BT_NATIVE_CMTPGETCONNINFO,
                        (unsigned long)&get_info);
      dup_del_ret = sock.ops->ioctl(&sock, LINUX_BT_NATIVE_CMTPCONNDEL,
                                    (unsigned long)&del_req);
    }

  active = linux_bt_cmtp_core_active_count();

  if (sock.ops != NULL && sock.ops->release != NULL)
    {
      release_ret = sock.ops->release(&sock);
    }

  snprintf(out, out_len,
           "upstream-cmtp-socket: proto=BTPROTO_CMTP create-ret=%d "
           "ioctl=CMTPCONNADD ret=%d ioctl=CMTPGETCONNLIST ret=%d "
           "cnum=%u ioctl=CMTPGETCONNINFO ret=%d state=%u num=%d "
           "flags=0x%08" PRIx32 " ioctl=CMTPCONNDEL ret=%d "
           "duplicate-ret=%d post-del-info-ret=%d missing-del-ret=%d "
           "final-active=%u "
           "core-session=linked:%u,worker:%u,capi:%u,"
           "reassembly:%u,datapath:%u "
           "core-traffic=blockids:%u,tx:%u,rx:%u,capimsg:%u "
           "no-data=sock_no bind-ret=%d getname-ret=%d "
           "sendmsg-ret=%d recvmsg-ret=%d listen-ret=%d "
           "shutdown-ret=%d connect-ret=%d socketpair-ret=%d "
           "accept-ret=%d mmap-ret=%d poll-op-null=%d release-ret=%d "
           "proto-name=%s "
           "source=third/linux-7.0.10/"
           "net/bluetooth/cmtp/sock.c link=hwsim-l2cap-capi-session\n",
           create_ret, add_ret, list_ret,
           list_ret == 0 ? list_req.cnum : 0, info_ret,
           info_ret == 0 ? info_state : 0,
           info_ret == 0 ? info_num : 0,
           info_ret == 0 ? info_flags : 0,
           del_ret, dup_add_ret, post_del_info_ret, dup_del_ret,
           active, core_linked, core_worker, core_capi, core_reassembly,
           core_datapath, blockids, tx_frames, rx_frames, capi_messages,
           bind_ret, getname_ret, sendmsg_ret, recvmsg_ret,
           listen_ret, shutdown_ret, connect_ret, socketpair_ret,
           accept_ret, mmap_ret, poll_op_null, release_ret,
           g_linux_bt_cmtp_sock_proto.name);

  if (create_ret < 0)
    {
      return create_ret;
    }

  if (add_ret < 0)
    {
      return add_ret;
    }

  if (list_ret < 0)
    {
      return list_ret;
    }

  if (info_ret < 0)
    {
      return info_ret;
    }

  if (del_ret < 0)
    {
      return del_ret;
    }

  if (dup_add_ret != -EALREADY)
    {
      return dup_add_ret < 0 ? dup_add_ret : -EINVAL;
    }

  if (post_del_info_ret != -ENOENT)
    {
      return post_del_info_ret < 0 ? post_del_info_ret : -EINVAL;
    }

  if (dup_del_ret != -ENOENT)
    {
      return dup_del_ret < 0 ? dup_del_ret : -EINVAL;
    }

  if (bind_ret != -EOPNOTSUPP ||
      getname_ret != -EOPNOTSUPP ||
      sendmsg_ret != -EOPNOTSUPP ||
      recvmsg_ret != -EOPNOTSUPP ||
      listen_ret != -EOPNOTSUPP ||
      shutdown_ret != -EOPNOTSUPP ||
      connect_ret != -EOPNOTSUPP ||
      socketpair_ret != -EOPNOTSUPP ||
      accept_ret != -EOPNOTSUPP ||
      mmap_ret != -EOPNOTSUPP)
    {
      return -EINVAL;
    }

  return release_ret;
}

int linux_bt_upstream_socket_probe_bind(int proto, int channel, int dev,
                                        char *out, size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct socket sock;
  struct sockaddr_hci haddr;
  struct sockaddr_hci nameaddr;
  struct sockaddr_l2 laddr;
  struct sockaddr_iso iaddr;
  const char *bind_kind = "none";
  int bind_ret = 0;
  int getname_ret = 0;
  int ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  memset(&sock, 0, sizeof(sock));
  memset(&nameaddr, 0, sizeof(nameaddr));
  sock.type = SOCK_RAW;
  if (proto == BTPROTO_L2CAP || proto == BTPROTO_ISO)
    {
      sock.type = SOCK_SEQPACKET;
    }
  g_sock_create_probes++;

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-socket: family=PF_BLUETOOTH invalid proto=%d\n",
               proto);
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      snprintf(out, out_len,
               "upstream-socket: family=PF_BLUETOOTH unregistered "
               "proto=%d\n",
               proto);
      return -EAFNOSUPPORT;
    }

  ret = family->create(&init_net, &sock, proto, 0);
  if (ret == 0 && channel >= 0)
    {
      if (sock.ops == NULL || sock.ops->bind == NULL)
        {
          bind_ret = -EOPNOTSUPP;
        }
      else if (proto == BTPROTO_L2CAP)
        {
          memset(&laddr, 0, sizeof(laddr));
          laddr.l2_family = AF_BLUETOOTH;
          laddr.l2_psm = cpu_to_le16((uint16_t)channel);
          laddr.l2_cid = cpu_to_le16(dev < 0 ? 0 : (uint16_t)dev);
          bacpy(&laddr.l2_bdaddr, BDADDR_ANY);
          bind_kind = "l2cap";
          bind_ret = sock.ops->bind(&sock, (struct sockaddr *)&laddr,
                                    sizeof(laddr));
        }
      else if (proto == BTPROTO_ISO)
        {
          memset(&iaddr, 0, sizeof(iaddr));
          iaddr.iso_family = AF_BLUETOOTH;
          iaddr.iso_bdaddr_type = channel < 0 ? 0 : (uint8_t)channel;
          bacpy(&iaddr.iso_bdaddr, BDADDR_ANY);
          bind_kind = "iso";
          bind_ret = sock.ops->bind(&sock, (struct sockaddr *)&iaddr,
                                    sizeof(iaddr));
        }
      else
        {
          memset(&haddr, 0, sizeof(haddr));
          haddr.hci_family = AF_BLUETOOTH;
          haddr.hci_channel = (unsigned short)channel;
          haddr.hci_dev = dev < 0 ? HCI_DEV_NONE : (unsigned short)dev;
          bind_kind = "hci";
          bind_ret = sock.ops->bind(&sock, (struct sockaddr *)&haddr,
                                    sizeof(haddr));
        }
    }

  if (ret == 0 && bind_ret == 0 && proto == BTPROTO_HCI &&
      sock.ops != NULL && sock.ops->getname != NULL)
    {
      getname_ret = sock.ops->getname(&sock,
                                      (struct sockaddr *)&nameaddr, 0);
    }

  snprintf(out, out_len,
           "upstream-socket: family=PF_BLUETOOTH proto=%d ret=%d "
           "sock-type=%d bind-kind=%s bind-channel=%d bind-dev=%d "
           "bind-ret=%d getname-ret=%d getname-dev=%u "
           "getname-channel=%u "
           "state=%d ops=%p sk=%p sk-state=%d sk-proto=%d\n",
           proto, ret, sock.type, bind_kind, channel, dev, bind_ret,
           getname_ret, nameaddr.hci_dev, nameaddr.hci_channel,
           sock.state, sock.ops, sock.sk,
           sock.sk != NULL ? sock.sk->sk_state : -1,
           sock.sk != NULL ? sock.sk->sk_protocol : -1);

  if (sock.ops != NULL && sock.ops->release != NULL)
    {
      sock.ops->release(&sock);
    }
  else if (sock.sk != NULL)
    {
      sock_put(sock.sk);
    }

  return ret == 0 && bind_ret < 0 ? bind_ret : ret;
}

int linux_bt_upstream_socket_probe(int proto, char *out, size_t out_len)
{
  return linux_bt_upstream_socket_probe_bind(proto, -1, -1, out, out_len);
}

int linux_bt_upstream_socket_send_probe(int channel, int dev,
                                        uint8_t pkt_type,
                                        const void *payload,
                                        size_t payload_len,
                                        char *out, size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct socket sock;
  struct sockaddr_hci haddr;
  struct msghdr msg;
  uint8_t h4[260];
  int create_ret;
  int bind_ret;
  int send_ret;

  if (out == NULL || out_len == 0 || payload_len + 1 > sizeof(h4))
    {
      return -EINVAL;
    }

  memset(&sock, 0, sizeof(sock));
  memset(&haddr, 0, sizeof(haddr));
  memset(&msg, 0, sizeof(msg));
  memset(h4, 0, sizeof(h4));

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-socket-send: family=PF_BLUETOOTH invalid\n");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      snprintf(out, out_len,
               "upstream-socket-send: family=PF_BLUETOOTH unregistered\n");
      return -EAFNOSUPPORT;
    }

  sock.type = SOCK_RAW;
  create_ret = family->create(&init_net, &sock, BTPROTO_HCI, 0);
  if (create_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-socket-send: create-ret=%d\n", create_ret);
      return create_ret;
    }

  haddr.hci_family = AF_BLUETOOTH;
  haddr.hci_channel = (unsigned short)channel;
  haddr.hci_dev = dev < 0 ? HCI_DEV_NONE : (unsigned short)dev;

  bind_ret = sock.ops != NULL && sock.ops->bind != NULL ?
             sock.ops->bind(&sock, (struct sockaddr *)&haddr,
                            sizeof(haddr)) : -EOPNOTSUPP;
  if (bind_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-socket-send: channel=%d dev=%d "
               "create-ret=%d bind-ret=%d\n",
               channel, dev, create_ret, bind_ret);
      goto out;
    }

  h4[0] = pkt_type;
  if (payload_len > 0)
    {
      memcpy(&h4[1], payload, payload_len);
    }

  msg.msg_iter.data = (const char *)h4;
  msg.msg_iter.count = payload_len + 1;
  send_ret = sock.ops != NULL && sock.ops->sendmsg != NULL ?
             sock.ops->sendmsg(&sock, &msg, payload_len + 1) :
             -EOPNOTSUPP;

  snprintf(out, out_len,
           "upstream-socket-send: channel=%d dev=%d pkt=0x%02x "
           "payload-len=%u create-ret=%d bind-ret=%d send-ret=%d\n",
           channel, dev, pkt_type, (unsigned int)payload_len, create_ret,
           bind_ret, send_ret);

out:
  if (sock.ops != NULL && sock.ops->release != NULL)
    {
      sock.ops->release(&sock);
    }

  return bind_ret < 0 ? bind_ret : send_ret;
}

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP
static unsigned int linux_bt_bnep_native_ioctl_cmd(unsigned int cmd)
{
  switch (cmd)
    {
      case BNEPCONNADD:
        return LINUX_BT_NATIVE_BNEPCONNADD;

      case BNEPCONNDEL:
        return LINUX_BT_NATIVE_BNEPCONNDEL;

      case BNEPGETCONNLIST:
        return LINUX_BT_NATIVE_BNEPGETCONNLIST;

      case BNEPGETCONNINFO:
        return LINUX_BT_NATIVE_BNEPGETCONNINFO;

      case BNEPGETSUPPFEAT:
        return LINUX_BT_NATIVE_BNEPGETSUPPFEAT;

      default:
        return cmd;
    }
}
#else
static unsigned int linux_bt_bnep_native_ioctl_cmd(unsigned int cmd)
{
  return cmd;
}
#endif

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP
static void linux_bt_bnep_fill_synthetic_dst(uint32_t seed,
                                             uint8_t dst[ETH_ALEN])
{
  dst[0] = (uint8_t)(seed & 0xff);
  dst[1] = (uint8_t)((seed >> 8) & 0xff);
  dst[2] = (uint8_t)((seed >> 16) & 0xff);
  dst[3] = (uint8_t)((seed >> 24) & 0xff);
  dst[4] = 0xbe;
  dst[5] = 0xef;
}

static bool linux_bt_bnep_bdaddr_is_any(const bdaddr_t *bdaddr)
{
  unsigned int i;

  if (bdaddr == NULL)
    {
      return true;
    }

  for (i = 0; i < ETH_ALEN; i++)
    {
      if (bdaddr->b[i] != 0)
        {
          return false;
        }
    }

  return true;
}

static void linux_bt_bnep_fill_l2cap_dst(
  const struct linux_bt_l2cap_bind_state *state, uint8_t dst[ETH_ALEN])
{
  unsigned int i;

  if (state == NULL)
    {
      linux_bt_bnep_fill_synthetic_dst(0, dst);
      return;
    }

  if (!linux_bt_bnep_bdaddr_is_any(&state->bdaddr))
    {
      for (i = 0; i < ETH_ALEN; i++)
        {
          dst[i] = state->bdaddr.b[ETH_ALEN - 1 - i];
        }

      return;
    }

  dst[0] = (uint8_t)(state->handle & 0xff);
  dst[1] = (uint8_t)(state->handle >> 8);
  dst[2] = (uint8_t)(state->cid & 0xff);
  dst[3] = (uint8_t)(state->psm & 0xff);
  dst[4] = 0xb1;
  dst[5] = 0x2c;
}

static struct linux_bt_l2cap_bind_state *
linux_bt_bnep_kept_l2cap_state(void)
{
  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.sk == NULL)
    {
      return NULL;
    }

  return linux_bt_l2cap_bound_state(g_l2cap_probe_socket.sk);
}
#endif

struct socket *linux_bt_compat_sockfd_lookup(int fd, int *err)
{
  struct linux_bt_sockif_l2cap_fd_state fd_state;
  struct socket *sock;
  void *l2cap_handle;
  int ret;

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP
  if (fd == (int)LINUX_BT_BNEP_CONNADD_KEPT_L2CAP &&
      g_l2cap_probe_bound && g_l2cap_probe_socket.sk != NULL &&
      g_l2cap_probe_socket.sk->sk_state == BT_CONNECTED)
    {
      g_bnep_native_fd_lookup_kept_probe++;
      g_l2cap_probe_bnep_refs++;
      if (err != NULL)
        {
          *err = 0;
        }

      return &g_l2cap_probe_socket;
    }
#endif

  l2cap_handle = NULL;
  ret = linux_bt_sockif_l2cap_upstream_from_fd(fd, &l2cap_handle);
  if (ret >= 0)
    {
      sock = linux_bt_upstream_l2cap_socket_kernel_socket(l2cap_handle);
      if (sock != NULL)
        {
#ifdef CONFIG_SIM_BTHWSIM
          struct linux_bt_l2cap_bind_state *state;

          state = linux_bt_l2cap_bound_state(sock->sk);
          if (state != NULL)
            {
              ret = linux_bt_l2cap_attach_send_chan(sock, state->handle,
                                                    state->cid);
              if (ret < 0)
                {
                  if (err != NULL)
                    {
                      *err = ret;
                    }

                  return NULL;
                }
            }
#endif

          if (err != NULL)
            {
              *err = 0;
            }

          return sock;
        }
    }

  ret = linux_bt_sockif_l2cap_state_from_fd(fd, &fd_state);
  if (ret < 0)
    {
      if (err != NULL)
        {
          *err = ret;
        }

      return NULL;
    }

  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.sk == NULL ||
      g_l2cap_probe_socket.sk->sk_state != BT_CONNECTED)
    {
      char out[128];

      ret = linux_bt_upstream_l2cap_socket_bind_probe(fd_state.psm,
                                                      fd_state.cid,
                                                      fd_state.handle,
                                                      out, sizeof(out));
      if (ret >= 0)
        {
          ret = linux_bt_upstream_l2cap_socket_connect_probe(fd_state.psm,
                                                             fd_state.cid,
                                                             out,
                                                             sizeof(out));
        }

      if (ret < 0)
        {
          if (err != NULL)
            {
              *err = ret;
            }

          return NULL;
        }

      g_bnep_native_fd_lookup_rebind_probe++;
    }

  g_l2cap_probe_bnep_refs++;

  if (err != NULL)
    {
      *err = 0;
    }

  return &g_l2cap_probe_socket;
}

void linux_bt_compat_sockfd_put(struct socket *sock)
{
  if (sock == &g_l2cap_probe_socket && g_l2cap_probe_bnep_refs > 0)
    {
      g_l2cap_probe_bnep_refs--;
      if (g_l2cap_probe_bnep_refs == 0 && g_l2cap_probe_close_pending)
        {
          linux_bt_l2cap_probe_release();
        }
    }
}

void linux_bt_compat_fput(struct file *file)
{
  if (linux_bt_upstream_l2cap_socket_close_bnep_file(file) == 0)
    {
      return;
    }

  if (file == &g_l2cap_probe_file ||
      (g_l2cap_probe_bound && g_l2cap_probe_socket.sk != NULL &&
       g_l2cap_probe_bnep_refs > 0))
    {
      linux_bt_compat_sockfd_put(&g_l2cap_probe_socket);
      if (g_l2cap_probe_bnep_refs == 0 && g_l2cap_probe_bound)
        {
          linux_bt_l2cap_probe_release();
        }
    }
}

int linux_bt_upstream_bnep_ioctl_raw(unsigned int cmd, unsigned long arg)
{
  const struct net_proto_family *family = NULL;
  struct socket sock;
  unsigned int native_cmd = linux_bt_bnep_native_ioctl_cmd(cmd);
  int create_ret;
  int ioctl_ret = -EOPNOTSUPP;

  memset(&sock, 0, sizeof(sock));
  sock.type = SOCK_RAW;
  g_sock_create_probes++;

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  create_ret = family->create(&init_net, &sock, BTPROTO_BNEP, 0);
  if (create_ret < 0)
    {
      return create_ret;
    }

  if (sock.ops != NULL && sock.ops->ioctl != NULL)
    {
      ioctl_ret = sock.ops->ioctl(&sock, native_cmd, arg);
    }

  if (sock.ops != NULL && sock.ops->release != NULL)
    {
      sock.ops->release(&sock);
    }
  else if (sock.sk != NULL)
    {
      sock_put(sock.sk);
    }

  return ioctl_ret;
}

void linux_bt_upstream_bnep_note_native_ioctl(unsigned int cmd, int ret)
{
  if (ret < 0)
    {
      return;
    }

  switch (cmd)
    {
      case LINUX_BT_NATIVE_BNEPGETSUPPFEAT:
        g_bnep_socket_ioctl_suppfeat++;
        break;

      case LINUX_BT_NATIVE_BNEPGETCONNLIST:
        g_bnep_socket_ioctl_connlist++;
        break;

      case LINUX_BT_NATIVE_BNEPGETCONNINFO:
        g_bnep_socket_ioctl_conninfo++;
        break;

      case LINUX_BT_NATIVE_BNEPCONNADD:
        g_bnep_socket_ioctl_connadd++;
        break;

      case LINUX_BT_NATIVE_BNEPCONNDEL:
        g_bnep_socket_ioctl_conndel++;
        break;

      default:
        break;
    }
}

void linux_bt_upstream_bnep_note_native_netdev_register(void)
{
  g_bnep_native_netdev_registers++;
  g_bnep_native_active++;
}

void linux_bt_upstream_bnep_note_native_netdev_unregister(void)
{
  g_bnep_native_netdev_unregisters++;
  if (g_bnep_native_active > 0)
    {
      g_bnep_native_active--;
    }
}

void linux_bt_upstream_bnep_note_native_netdev_xmit(size_t len)
{
  g_bnep_native_netdev_xmit++;
  g_bnep_native_netdev_xmit_bytes += len;
}

void linux_bt_upstream_bnep_note_native_tx_frame(size_t len, int ret)
{
  g_bnep_native_tx_frame++;
  g_bnep_native_tx_frame_bytes += len;
  if (ret == 0)
    {
      g_bnep_native_tx_frame_ok++;
    }
}

void linux_bt_upstream_bnep_note_native_l2cap_rx(size_t len, int delivered)
{
  g_bnep_native_l2cap_rx++;
  g_bnep_native_l2cap_rx_bytes += len;
  if (delivered > 0)
    {
      g_bnep_native_l2cap_delivered += delivered;
    }
}

void linux_bt_upstream_bnep_note_native_rx_frame(size_t len, int ret)
{
  g_bnep_native_rx_frame++;
  g_bnep_native_rx_frame_bytes += len;
  if (ret == 0)
    {
      g_bnep_native_rx_frame_ok++;
    }
}

void linux_bt_upstream_bnep_note_native_netif_rx(size_t len)
{
  g_bnep_native_netif_rx++;
  g_bnep_native_netif_rx_bytes += len;
}

void linux_bt_upstream_bnep_note_native_session_create(void)
{
  g_bnep_native_session_creates++;
}

void linux_bt_upstream_bnep_note_native_session_start(void)
{
  g_bnep_native_session_starts++;
}

void linux_bt_upstream_bnep_note_native_session_stop(void)
{
  g_bnep_native_session_stops++;
}

void linux_bt_upstream_bnep_note_native_session_terminate(void)
{
  g_bnep_native_session_terminates++;
}

void linux_bt_upstream_bnep_note_native_netdev_setup(void)
{
  g_bnep_native_netdev_setups++;
}

void linux_bt_upstream_bnep_note_native_ndo_start_xmit(void)
{
  g_bnep_native_ndo_start_xmits++;
}

void linux_bt_upstream_bnep_note_native_session_link(void)
{
  g_bnep_native_session_links++;
}

void linux_bt_upstream_bnep_note_native_session_unlink(void)
{
  g_bnep_native_session_unlinks++;
}

void linux_bt_upstream_bnep_note_native_session_thread(void)
{
  g_bnep_native_session_threads++;
}

void linux_bt_upstream_bnep_note_native_kthread_run(void)
{
  g_bnep_native_kthread_runs++;
}

void linux_bt_upstream_bnep_note_native_session_rx_dequeue(void)
{
  g_bnep_native_session_rx_dequeues++;
}

void linux_bt_upstream_bnep_note_native_session_tx_dequeue(void)
{
  g_bnep_native_session_tx_dequeues++;
}

void linux_bt_upstream_bnep_note_native_fd_handoff(int fd, uint16_t role,
                                                   int err)
{
  struct linux_bt_sockif_l2cap_fd_state state;
  void *l2cap_handle;

  if (err < 0)
    {
      return;
    }

  memset(&state, 0, sizeof(state));
  if (linux_bt_sockif_l2cap_state_from_fd(fd, &state) < 0)
    {
      return;
    }

  if (linux_bt_sockif_l2cap_upstream_from_fd(fd, &l2cap_handle) >= 0)
    {
      (void)linux_bt_upstream_l2cap_socket_mark_bnep_owner(l2cap_handle);
    }

  if (g_bnep_native_fd_active)
    {
      g_bnep_native_fd_handoff_rejects++;
      return;
    }

  g_bnep_native_fd_handoffs++;
  g_bnep_native_fd_active = 1;
  g_bnep_native_fd_last = fd;
  g_bnep_native_fd_psm = state.psm;
  g_bnep_native_fd_cid = state.cid;
  g_bnep_native_fd_handle = state.handle;
  g_bnep_native_fd_role = role;
  g_bnep_native_fd_source =
    fd == (int)LINUX_BT_BNEP_CONNADD_KEPT_L2CAP ?
    "kept-probe-l2cap" : "socket-fd";
}

int linux_bt_upstream_bnep_drain_pending_l2cap(uint16_t cid,
                                               unsigned int max_packets)
{
  uint8_t payload[LINUX_BT_L2CAP_ACL_MTU];
  unsigned int drained = 0;

  if (cid == 0 || max_packets == 0)
    {
      return 0;
    }

  while (drained < max_packets)
    {
      size_t payload_len = 0;
      int delivered;

      if (!linux_bt_l2cap_pending_pop(0, cid, payload, sizeof(payload),
                                      &payload_len))
        {
          break;
        }

      delivered = bnep_recv_l2cap_payload(cid, payload, payload_len);
      if (delivered <= 0)
        {
          break;
        }

      drained += (unsigned int)delivered;
    }

  return (int)drained;
}

void linux_bt_upstream_bnep_note_native_sock_ioctl_connadd(int err)
{
  (void)err;
  g_bnep_native_sock_ioctl_connadds++;
}

void linux_bt_upstream_bnep_note_native_sock_ioctl_conndel(int err)
{
  g_bnep_native_sock_ioctl_conndels++;

  if (err == 0)
    {
      if (g_bnep_native_fd_active)
        {
          g_bnep_native_fd_active = 0;
          g_bnep_native_fd_handoff_cleanups++;
          g_bnep_native_fd_last = -1;
          g_bnep_native_fd_psm = 0;
          g_bnep_native_fd_cid = 0;
          g_bnep_native_fd_handle = 0;
          g_bnep_native_fd_role = 0;
          g_bnep_native_fd_source = "none";
        }
    }
}

void linux_bt_upstream_bnep_note_native_sock_ioctl_getconnlist(int err)
{
  if (err == 0)
    {
      g_bnep_native_sock_ioctl_getconnlists++;
    }
}

void linux_bt_upstream_bnep_note_native_sock_ioctl_getconninfo(int err)
{
  if (err == 0)
    {
      g_bnep_native_sock_ioctl_getconninfos++;
    }
}

void linux_bt_upstream_bnep_note_native_sock_ioctl_getsuppfeat(int err)
{
  if (err == 0)
    {
      g_bnep_native_sock_ioctl_getsuppfeats++;
    }
}

int linux_bt_upstream_bnep_ioctl_probe(int action, uint32_t param,
                                       char *out, size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct socket sock;
  struct linux_bt_bnep_conninfo ci;
  struct linux_bt_bnep_conninfo list_ci[4];
  struct linux_bt_bnep_connlist_req cl;
  struct linux_bt_bnep_connadd_req ca;
  struct linux_bt_bnep_conndel_req cd;
  struct linux_bt_l2cap_bind_state *l2_state;
  struct linux_bt_bnep_core_session *active_session;
  uint32_t supp_feat = 0;
  unsigned long arg = 0;
  const char *name = "unknown";
  unsigned int cmd = 0;
  int create_ret;
  int ioctl_ret = -EINVAL;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  memset(&sock, 0, sizeof(sock));
  memset(&ci, 0, sizeof(ci));
  memset(list_ci, 0, sizeof(list_ci));
  memset(&cl, 0, sizeof(cl));
  memset(&ca, 0, sizeof(ca));
  memset(&cd, 0, sizeof(cd));
  sock.type = SOCK_RAW;
  g_sock_create_probes++;

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-bnep-ioctl: family=PF_BLUETOOTH invalid\n");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      snprintf(out, out_len,
               "upstream-bnep-ioctl: family=PF_BLUETOOTH unregistered\n");
      return -EAFNOSUPPORT;
    }

  create_ret = family->create(&init_net, &sock, BTPROTO_BNEP, 0);
  if (create_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-bnep-ioctl: create-ret=%d proto=%d\n",
               create_ret, BTPROTO_BNEP);
      return create_ret;
    }

  switch (action)
    {
      case LINUX_BT_BNEP_IOCTL_GETSUPPFEAT:
        name = "BNEPGETSUPPFEAT";
        cmd = BNEPGETSUPPFEAT;
        arg = (unsigned long)&supp_feat;
        break;

      case LINUX_BT_BNEP_IOCTL_GETCONNLIST:
        name = "BNEPGETCONNLIST";
        cmd = BNEPGETCONNLIST;
        cl.cnum = sizeof(list_ci) / sizeof(list_ci[0]);
        cl.ci = list_ci;
        arg = (unsigned long)&cl;
        break;

      case LINUX_BT_BNEP_IOCTL_GETCONNINFO:
        name = "BNEPGETCONNINFO";
        cmd = BNEPGETCONNINFO;
        l2_state = param == LINUX_BT_BNEP_CONNADD_KEPT_L2CAP ?
                   linux_bt_bnep_kept_l2cap_state() : NULL;
        if (l2_state != NULL)
          {
            linux_bt_bnep_fill_l2cap_dst(l2_state, ci.dst);
          }
        else
          {
            linux_bt_bnep_fill_synthetic_dst(param, ci.dst);
          }

        arg = (unsigned long)&ci;
        break;

      case LINUX_BT_BNEP_IOCTL_CONNADD:
        name = "BNEPCONNADD";
        cmd = BNEPCONNADD;
        ca.sock = (int)param;
        ca.role = LINUX_BT_BNEP_SVC_PANU;
        snprintf(ca.device, sizeof(ca.device), "btn0");

        arg = (unsigned long)&ca;
        break;

      case LINUX_BT_BNEP_IOCTL_CONNDEL:
        name = "BNEPCONNDEL";
        cmd = BNEPCONNDEL;
        l2_state = param == LINUX_BT_BNEP_CONNADD_KEPT_L2CAP ?
                   linux_bt_bnep_kept_l2cap_state() : NULL;
        if (l2_state != NULL)
          {
            linux_bt_bnep_fill_l2cap_dst(l2_state, cd.dst);
          }
        else
          {
            linux_bt_bnep_fill_synthetic_dst(param, cd.dst);
          }

        arg = (unsigned long)&cd;
        break;

      default:
        name = "invalid";
        ioctl_ret = -EINVAL;
        break;
    }

  if ((action == LINUX_BT_BNEP_IOCTL_GETCONNINFO ||
       action == LINUX_BT_BNEP_IOCTL_CONNDEL) &&
      param == LINUX_BT_BNEP_CONNADD_KEPT_L2CAP &&
      cmd != 0 && sock.ops != NULL && sock.ops->ioctl != NULL)
    {
      struct linux_bt_bnep_connlist_req lookup_cl;
      int lookup_ret;

      memset(&lookup_cl, 0, sizeof(lookup_cl));
      memset(list_ci, 0, sizeof(list_ci));
      lookup_cl.cnum = sizeof(list_ci) / sizeof(list_ci[0]);
      lookup_cl.ci = list_ci;
      lookup_ret = sock.ops->ioctl(&sock,
                                   linux_bt_bnep_native_ioctl_cmd(
                                     BNEPGETCONNLIST),
                                   (unsigned long)&lookup_cl);
      if (lookup_ret == 0 && lookup_cl.cnum > 0)
        {
          if (action == LINUX_BT_BNEP_IOCTL_GETCONNINFO)
            {
              memcpy(ci.dst, list_ci[0].dst, ETH_ALEN);
            }
          else
            {
              memcpy(cd.dst, list_ci[0].dst, ETH_ALEN);
            }
        }
    }

  if (cmd != 0 && sock.ops != NULL && sock.ops->ioctl != NULL)
    {
      ioctl_ret = sock.ops->ioctl(&sock,
                                  linux_bt_bnep_native_ioctl_cmd(cmd),
                                  arg);
    }

  active_session = linux_bt_bnep_first_active_session();
  snprintf(out, out_len,
           "upstream-bnep-ioctl: action=%d name=%s proto=%d "
           "create-ret=%d ioctl-ret=%d supp-feat=0x%08" PRIx32 " "
           "connlist-cnum=%" PRIu32 " conninfo-role=0x%04x "
           "conninfo-state=0x%04x conninfo-device=%s "
           "connlist0-state=0x%04x connlist0-device=%s "
           "connadd-device=%s bnep-core-active=%u "
           "bnep-netdev-registered=%u bnep-netdev-name=%s "
           "bnep-netdev-mtu=%u\n",
           action, name, BTPROTO_BNEP, create_ret, ioctl_ret,
           supp_feat, cl.cnum, ci.role, ci.state, ci.device,
           cl.cnum > 0 ? list_ci[0].state : 0,
           cl.cnum > 0 ? list_ci[0].device : "", ca.device,
           linux_bt_bnep_core_active_count(),
           active_session != NULL && active_session->netdev_registered ?
           1 : 0,
           active_session != NULL ? active_session->netdev_name : "",
           active_session != NULL ? active_session->netdev_mtu : 0);

  if (sock.ops != NULL && sock.ops->release != NULL)
    {
      sock.ops->release(&sock);
    }
  else if (sock.sk != NULL)
    {
      sock_put(sock.sk);
    }

  return ioctl_ret < 0 ? ioctl_ret : 0;
}

int linux_bt_upstream_hci_filter_probe(int dev, uint32_t type_mask,
                                       uint32_t event_mask0,
                                       uint32_t event_mask1,
                                       uint16_t opcode,
                                       char *out, size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct socket sock;
  struct sockaddr_hci haddr;
  struct hci_ufilter filter;
  struct hci_ufilter readback;
  int readback_len;
  int create_ret;
  int bind_ret;
  int set_ret = -EOPNOTSUPP;
  int get_ret = -EOPNOTSUPP;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  memset(&sock, 0, sizeof(sock));
  memset(&haddr, 0, sizeof(haddr));
  memset(&filter, 0, sizeof(filter));
  memset(&readback, 0, sizeof(readback));

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-hci-filter: family=PF_BLUETOOTH invalid\n");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      snprintf(out, out_len,
               "upstream-hci-filter: family=PF_BLUETOOTH unregistered\n");
      return -EAFNOSUPPORT;
    }

  sock.type = SOCK_RAW;
  create_ret = family->create(&init_net, &sock, BTPROTO_HCI, 0);
  if (create_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-hci-filter: create-ret=%d\n", create_ret);
      return create_ret;
    }

  haddr.hci_family = AF_BLUETOOTH;
  haddr.hci_channel = HCI_CHANNEL_RAW;
  haddr.hci_dev = dev < 0 ? HCI_DEV_NONE : (unsigned short)dev;
  bind_ret = sock.ops != NULL && sock.ops->bind != NULL ?
             sock.ops->bind(&sock, (struct sockaddr *)&haddr,
                            sizeof(haddr)) : -EOPNOTSUPP;
  if (bind_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-hci-filter: dev=%d create-ret=%d bind-ret=%d\n",
               dev, create_ret, bind_ret);
      goto out;
    }

  filter.type_mask = type_mask;
  filter.event_mask[0] = event_mask0;
  filter.event_mask[1] = event_mask1;
  filter.opcode = cpu_to_le16(opcode);
  set_ret = sock.ops != NULL && sock.ops->setsockopt != NULL ?
            sock.ops->setsockopt(&sock, SOL_HCI, HCI_FILTER,
                                 &filter, sizeof(filter)) : -EOPNOTSUPP;

  readback_len = sizeof(readback);
  get_ret = sock.ops != NULL && sock.ops->getsockopt != NULL ?
            sock.ops->getsockopt(&sock, SOL_HCI, HCI_FILTER,
                                 (char *)&readback, &readback_len) :
            -EOPNOTSUPP;

  snprintf(out, out_len,
           "upstream-hci-filter: dev=%d create-ret=%d bind-ret=%d "
           "set-ret=%d get-ret=%d get-len=%d type-mask=0x%08" PRIx32
           " event-mask0=0x%08" PRIx32 " event-mask1=0x%08" PRIx32
           " opcode=0x%04x\n",
           dev, create_ret, bind_ret, set_ret, get_ret, readback_len,
           readback.type_mask, readback.event_mask[0],
           readback.event_mask[1], le16_to_cpu(readback.opcode));

out:
  if (sock.ops != NULL && sock.ops->release != NULL)
    {
      sock.ops->release(&sock);
    }

  return bind_ret < 0 ? bind_ret : set_ret < 0 ? set_ret : get_ret;
}

int linux_bt_upstream_hci_ioctl_probe(uint16_t dev, int action,
                                      uint32_t dev_opt,
                                      char *out, size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct socket sock;
  struct sockaddr_hci haddr;
  struct hci_dev_info info;
  struct hci_dev_req dr;
  struct hci_auth_info_req auth_req;
  bdaddr_t reject_addr;
  uint8_t list_buf[sizeof(struct hci_dev_list_req) +
                   8 * sizeof(struct hci_dev_req)];
  uint8_t conn_list_buf[sizeof(struct hci_conn_list_req) +
                        8 * sizeof(struct hci_conn_info)];
  uint8_t conn_info_buf[sizeof(struct hci_conn_info_req) +
                        sizeof(struct hci_conn_info)];
  struct hci_dev_list_req *list;
  struct hci_conn_list_req *conn_list;
  struct hci_conn_info_req *conn_req;
  size_t off = 0;
  int create_ret;
  int list_ret = -EOPNOTSUPP;
  int action_ret = 0;
  int info_ret = -EOPNOTSUPP;
  uint8_t *reject_bytes = (uint8_t *)&reject_addr;
  unsigned int i;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  memset(&sock, 0, sizeof(sock));
  memset(&haddr, 0, sizeof(haddr));
  memset(&info, 0, sizeof(info));
  memset(&dr, 0, sizeof(dr));
  memset(&auth_req, 0, sizeof(auth_req));
  memset(&reject_addr, 0, sizeof(reject_addr));
  memset(list_buf, 0, sizeof(list_buf));
  memset(conn_list_buf, 0, sizeof(conn_list_buf));
  memset(conn_info_buf, 0, sizeof(conn_info_buf));

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-hci-ioctl: family=PF_BLUETOOTH invalid\n");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      snprintf(out, out_len,
               "upstream-hci-ioctl: family=PF_BLUETOOTH unregistered\n");
      return -EAFNOSUPPORT;
    }

  sock.type = SOCK_RAW;
  create_ret = family->create(&init_net, &sock, BTPROTO_HCI, 0);
  if (create_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-hci-ioctl: create-ret=%d\n", create_ret);
      return create_ret;
    }

  list = (struct hci_dev_list_req *)list_buf;
  conn_list = (struct hci_conn_list_req *)conn_list_buf;
  conn_req = (struct hci_conn_info_req *)conn_info_buf;
  bacpy(&auth_req.bdaddr, BDADDR_ANY);
  reject_bytes[0] = (uint8_t)(dev_opt == 0 ? 1 : dev_opt);
  reject_bytes[1] = (uint8_t)(dev_opt >> 8);
  reject_bytes[2] = (uint8_t)(dev_opt >> 16);
  reject_bytes[3] = (uint8_t)(dev_opt >> 24);
  list->dev_num = 8;
  if (sock.ops != NULL && sock.ops->ioctl != NULL)
    {
      list_ret = sock.ops->ioctl(&sock, HCIGETDEVLIST,
                                 (unsigned long)(uintptr_t)list);
      switch (action)
        {
          case 1:
            action_ret = sock.ops->ioctl(&sock, HCIDEVUP, dev);
            break;

          case 2:
            action_ret = sock.ops->ioctl(&sock, HCIDEVDOWN, dev);
            break;

          case 3:
            action_ret = sock.ops->ioctl(&sock, HCIDEVRESET, dev);
            break;

          case 4:
            action_ret = sock.ops->ioctl(&sock, HCIDEVRESTAT, dev);
            break;

          case 5:
            dr.dev_id = dev;
            dr.dev_opt = dev_opt;
            action_ret = sock.ops->ioctl(&sock, HCISETSCAN,
                                         (unsigned long)(uintptr_t)&dr);
            break;

          case 6:
            dr.dev_id = dev;
            dr.dev_opt = dev_opt;
            action_ret = sock.ops->ioctl(&sock, HCISETAUTH,
                                         (unsigned long)(uintptr_t)&dr);
            break;

          case 7:
            dr.dev_id = dev;
            dr.dev_opt = dev_opt;
            action_ret = sock.ops->ioctl(&sock, HCISETENCRYPT,
                                         (unsigned long)(uintptr_t)&dr);
            break;

          case 8:
            dr.dev_id = dev;
            dr.dev_opt = dev_opt;
            action_ret = sock.ops->ioctl(&sock, HCISETPTYPE,
                                         (unsigned long)(uintptr_t)&dr);
            break;

          case 9:
            dr.dev_id = dev;
            dr.dev_opt = dev_opt;
            action_ret = sock.ops->ioctl(&sock, HCISETLINKPOL,
                                         (unsigned long)(uintptr_t)&dr);
            break;

          case 10:
            dr.dev_id = dev;
            dr.dev_opt = dev_opt;
            action_ret = sock.ops->ioctl(&sock, HCISETLINKMODE,
                                         (unsigned long)(uintptr_t)&dr);
            break;

          case 11:
            dr.dev_id = dev;
            dr.dev_opt = dev_opt;
            action_ret = sock.ops->ioctl(&sock, HCISETACLMTU,
                                         (unsigned long)(uintptr_t)&dr);
            break;

          case 12:
            dr.dev_id = dev;
            dr.dev_opt = dev_opt;
            action_ret = sock.ops->ioctl(&sock, HCISETSCOMTU,
                                         (unsigned long)(uintptr_t)&dr);
            break;

          case 13:
            conn_list->dev_id = dev;
            conn_list->conn_num = 8;
            action_ret = sock.ops->ioctl(&sock, HCIGETCONNLIST,
                                         (unsigned long)(uintptr_t)conn_list);
            break;

          case 14:
            haddr.hci_family = AF_BLUETOOTH;
            haddr.hci_channel = HCI_CHANNEL_RAW;
            haddr.hci_dev = dev;
            action_ret = sock.ops->bind != NULL ?
                         sock.ops->bind(&sock, (struct sockaddr *)&haddr,
                                        sizeof(haddr)) : -EOPNOTSUPP;
            if (action_ret >= 0)
              {
                conn_req->type = dev_opt == 0 ? ACL_LINK : (uint8_t)dev_opt;
                bacpy(&conn_req->bdaddr, BDADDR_ANY);
                action_ret = sock.ops->ioctl(&sock, HCIGETCONNINFO,
                                             (unsigned long)(uintptr_t)
                                             conn_req);
              }
            break;

          case 15:
            haddr.hci_family = AF_BLUETOOTH;
            haddr.hci_channel = HCI_CHANNEL_RAW;
            haddr.hci_dev = dev;
            action_ret = sock.ops->bind != NULL ?
                         sock.ops->bind(&sock, (struct sockaddr *)&haddr,
                                        sizeof(haddr)) : -EOPNOTSUPP;
            if (action_ret >= 0)
              {
                action_ret = sock.ops->ioctl(&sock, HCIGETAUTHINFO,
                                             (unsigned long)(uintptr_t)
                                             &auth_req);
              }
            break;

          case 16:
            haddr.hci_family = AF_BLUETOOTH;
            haddr.hci_channel = HCI_CHANNEL_RAW;
            haddr.hci_dev = dev;
            action_ret = sock.ops->bind != NULL ?
                         sock.ops->bind(&sock, (struct sockaddr *)&haddr,
                                        sizeof(haddr)) : -EOPNOTSUPP;
            if (action_ret >= 0)
              {
                action_ret = sock.ops->ioctl(&sock, HCIBLOCKADDR,
                                             (unsigned long)(uintptr_t)
                                             &reject_addr);
              }
            break;

          case 17:
            haddr.hci_family = AF_BLUETOOTH;
            haddr.hci_channel = HCI_CHANNEL_RAW;
            haddr.hci_dev = dev;
            action_ret = sock.ops->bind != NULL ?
                         sock.ops->bind(&sock, (struct sockaddr *)&haddr,
                                        sizeof(haddr)) : -EOPNOTSUPP;
            if (action_ret >= 0)
              {
                action_ret = sock.ops->ioctl(&sock, HCIUNBLOCKADDR,
                                             (unsigned long)(uintptr_t)
                                             &reject_addr);
              }
            break;

          default:
            action_ret = 0;
            break;
        }

      info.dev_id = dev;
      info_ret = sock.ops->ioctl(&sock, HCIGETDEVINFO,
                                 (unsigned long)(uintptr_t)&info);
    }

  {
    int n = snprintf(out + off, out_len - off,
                     "upstream-hci-ioctl: create-ret=%d "
                     "devlist-ret=%d action=%d dev-opt=0x%08" PRIx32
                     " action-ret=%d "
                     "dev-num=%u",
                     create_ret, list_ret, action, dev_opt, action_ret,
                     list->dev_num);
    if (n < 0 || (size_t)n >= out_len - off)
      {
        out[out_len - 1] = '\0';
        goto out_release;
      }

    off += (size_t)n;
  }

  for (i = 0; i < list->dev_num && i < 8 && off < out_len; i++)
    {
      int n = snprintf(out + off, out_len - off,
                       " dev%u={id=%u,opt=0x%08" PRIx32 "}",
                       i, list->dev_req[i].dev_id,
                       list->dev_req[i].dev_opt);
      if (n < 0 || (size_t)n >= out_len - off)
        {
          out[out_len - 1] = '\0';
          goto out_release;
        }

      off += (size_t)n;
    }

  if (action == 13 && off < out_len)
    {
      int n = snprintf(out + off, out_len - off,
                       " conn-num=%u", conn_list->conn_num);
      if (n < 0 || (size_t)n >= out_len - off)
        {
          out[out_len - 1] = '\0';
          goto out_release;
        }

      off += (size_t)n;
      for (i = 0; i < conn_list->conn_num && i < 8 && off < out_len; i++)
        {
          struct hci_conn_info *ci = &conn_list->conn_info[i];

          n = snprintf(out + off, out_len - off,
                       " conn%u={handle=0x%04x,type=0x%02x,out=%u,"
                       "state=%u,link-mode=0x%08" PRIx32 "}",
                       i, ci->handle, ci->type, ci->out, ci->state,
                       ci->link_mode);
          if (n < 0 || (size_t)n >= out_len - off)
            {
              out[out_len - 1] = '\0';
              goto out_release;
            }

          off += (size_t)n;
        }
    }

  if (action == 14 && off < out_len)
    {
      struct hci_conn_info *ci = &conn_req->conn_info[0];
      int n = snprintf(out + off, out_len - off,
                       " conninfo={type=0x%02x,handle=0x%04x,out=%u,"
                       "state=%u,link-mode=0x%08" PRIx32 "}",
                       conn_req->type, ci->handle, ci->out, ci->state,
                       ci->link_mode);
      if (n < 0 || (size_t)n >= out_len - off)
        {
          out[out_len - 1] = '\0';
          goto out_release;
        }

      off += (size_t)n;
    }

  if (action == 15 && off < out_len)
    {
      int n = snprintf(out + off, out_len - off,
                       " authinfo={type=0x%02x}", auth_req.type);
      if (n < 0 || (size_t)n >= out_len - off)
        {
          out[out_len - 1] = '\0';
          goto out_release;
        }

      off += (size_t)n;
    }

  if ((action == 16 || action == 17) && off < out_len)
    {
      int n = snprintf(out + off, out_len - off,
                       " reject-addr=%02x:%02x:%02x:%02x:%02x:%02x",
                       reject_bytes[5], reject_bytes[4], reject_bytes[3],
                       reject_bytes[2], reject_bytes[1], reject_bytes[0]);
      if (n < 0 || (size_t)n >= out_len - off)
        {
          out[out_len - 1] = '\0';
          goto out_release;
        }

      off += (size_t)n;
    }

  if (off < out_len)
    {
      snprintf(out + off, out_len - off,
               " devinfo-ret=%d info={id=%u,name=%s,flags=0x%08" PRIx32
               ",type=%u,pkt-type=0x%08" PRIx32
               ",link-policy=0x%08" PRIx32 ",link-mode=0x%08" PRIx32
               ",acl-mtu=%u,acl-pkts=%u,sco-mtu=%u,sco-pkts=%u"
               ",acl-tx=%" PRIu32 ",acl-rx=%" PRIu32
               ",byte-tx=%" PRIu32 ",byte-rx=%" PRIu32 "}\n",
               info_ret, info.dev_id, info.name, info.flags, info.type,
               info.pkt_type, info.link_policy, info.link_mode,
               info.acl_mtu, info.acl_pkts, info.sco_mtu, info.sco_pkts,
               info.stat.acl_tx, info.stat.acl_rx, info.stat.byte_tx,
               info.stat.byte_rx);
    }

out_release:
  if (sock.ops != NULL && sock.ops->release != NULL)
    {
      sock.ops->release(&sock);
    }

  return list_ret < 0 ? list_ret : action_ret < 0 ? action_ret : info_ret;
}

int linux_bt_upstream_hci_ioctl_raw(unsigned int cmd, unsigned long arg)
{
  const struct net_proto_family *family = NULL;
  struct socket sock;
  int create_ret;
  int ioctl_ret;

  memset(&sock, 0, sizeof(sock));

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  sock.type = SOCK_RAW;
  create_ret = family->create(&init_net, &sock, BTPROTO_HCI, 0);
  if (create_ret < 0)
    {
      return create_ret;
    }

  ioctl_ret = sock.ops != NULL && sock.ops->ioctl != NULL ?
              sock.ops->ioctl(&sock, cmd, arg) : -EOPNOTSUPP;
  if (sock.ops != NULL && sock.ops->release != NULL)
    {
      sock.ops->release(&sock);
    }

  return ioctl_ret;
}

int linux_bt_upstream_hci_raw_open_probe(uint16_t dev, void **handle)
{
  const struct net_proto_family *family = NULL;
  struct linux_bt_upstream_hci_raw_handle *raw;
  struct sockaddr_hci haddr;
  int ret;

  if (handle == NULL)
    {
      return -EINVAL;
    }

  *handle = NULL;
  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  raw = kmm_zalloc(sizeof(*raw));
  if (raw == NULL)
    {
      return -ENOMEM;
    }

  raw->sock.type = SOCK_RAW;
  ret = family->create(&init_net, &raw->sock, BTPROTO_HCI, 0);
  if (ret < 0)
    {
      kmm_free(raw);
      return ret;
    }

  memset(&haddr, 0, sizeof(haddr));
  haddr.hci_family = AF_BLUETOOTH;
  haddr.hci_dev = dev;
  haddr.hci_channel = HCI_CHANNEL_RAW;
  ret = raw->sock.ops != NULL && raw->sock.ops->bind != NULL ?
        raw->sock.ops->bind(&raw->sock, (struct sockaddr *)&haddr,
                            sizeof(haddr)) : -EOPNOTSUPP;
  if (ret < 0)
    {
      if (raw->sock.ops != NULL && raw->sock.ops->release != NULL)
        {
          raw->sock.ops->release(&raw->sock);
        }

      kmm_free(raw);
      return ret;
    }

  raw->bound = true;
  raw->dev = dev;
  *handle = raw;
  return 0;
}

int linux_bt_upstream_hci_raw_send_probe(void *handle, const void *payload,
                                         size_t payload_len)
{
  struct linux_bt_upstream_hci_raw_handle *raw = handle;
  struct msghdr msg;
  const uint8_t *bytes = payload;
  int ret;

  if (raw == NULL || !raw->bound || payload == NULL || payload_len == 0)
    {
      return -EINVAL;
    }

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = payload;
  msg.msg_iter.count = payload_len;
  ret = raw->sock.ops != NULL && raw->sock.ops->sendmsg != NULL ?
        raw->sock.ops->sendmsg(&raw->sock, &msg, payload_len) :
        -EOPNOTSUPP;
  if (ret >= 0 && payload_len >= 4 && bytes[0] == HCI_COMMAND_PKT)
    {
      struct hci_dev *hdev = hci_dev_get(raw->dev);
      uint8_t event[6];

      if (hdev != NULL)
        {
          event[0] = HCI_EV_CMD_COMPLETE;
          event[1] = 4;
          event[2] = 1;
          event[3] = bytes[1];
          event[4] = bytes[2];
          event[5] = 0;
          (void)linux_bt_upstream_hci_sock_recv(hdev, HCI_EVENT_PKT,
                                                event, sizeof(event));

          hci_dev_put(hdev);
        }
    }

  return ret < 0 ? ret : (int)payload_len;
}

int linux_bt_upstream_hci_raw_recv_probe(void *handle, void *payload,
                                         size_t payload_len, int *flags)
{
  struct linux_bt_upstream_hci_raw_handle *raw = handle;
  struct msghdr msg;
  int ret;

  if (raw == NULL || !raw->bound || payload == NULL || payload_len == 0)
    {
      return -EINVAL;
    }

  if (flags != NULL)
    {
      *flags = 0;
    }

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = payload;
  msg.msg_iter.count = payload_len;
  ret = raw->sock.ops != NULL && raw->sock.ops->recvmsg != NULL ?
        raw->sock.ops->recvmsg(&raw->sock, &msg, payload_len, 0) :
        -EOPNOTSUPP;
  if (ret >= 0 && flags != NULL)
    {
      *flags = msg.msg_flags;
    }

  return ret;
}

int linux_bt_upstream_hci_raw_setsockopt_probe(void *handle, int level,
                                               int option,
                                               const void *value,
                                               unsigned int value_len)
{
  struct linux_bt_upstream_hci_raw_handle *raw = handle;

  if (raw == NULL || !raw->bound)
    {
      return -EINVAL;
    }

  return raw->sock.ops != NULL && raw->sock.ops->setsockopt != NULL ?
         raw->sock.ops->setsockopt(&raw->sock, level, option, value,
                                   value_len) : -EOPNOTSUPP;
}

int linux_bt_upstream_hci_raw_getsockopt_probe(void *handle, int level,
                                               int option, void *value,
                                               int *value_len)
{
  struct linux_bt_upstream_hci_raw_handle *raw = handle;

  if (raw == NULL || !raw->bound)
    {
      return -EINVAL;
    }

  return raw->sock.ops != NULL && raw->sock.ops->getsockopt != NULL ?
         raw->sock.ops->getsockopt(&raw->sock, level, option, value,
                                   value_len) : -EOPNOTSUPP;
}

int linux_bt_upstream_hci_raw_close_probe(void *handle)
{
  struct linux_bt_upstream_hci_raw_handle *raw = handle;

  if (raw == NULL)
    {
      return -EINVAL;
    }

  if (raw->sock.ops != NULL && raw->sock.ops->release != NULL)
    {
      raw->sock.ops->release(&raw->sock);
    }

  kmm_free(raw);
  return 0;
}

static void linux_bt_upstream_format_hex(const uint8_t *data, size_t len,
                                         char *out, size_t out_len)
{
  size_t off = 0;
  size_t i;

  if (out == NULL || out_len == 0)
    {
      return;
    }

  out[0] = '\0';
  for (i = 0; i < len && off < out_len; i++)
    {
      int n = snprintf(out + off, out_len - off, "%s%02x",
                       i == 0 ? "" : " ", data[i]);
      if (n < 0 || (size_t)n >= out_len - off)
        {
          out[out_len - 1] = '\0';
          return;
        }

      off += (size_t)n;
    }
}

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
static void linux_bt_noaf_queue_copy(uint8_t *dst, size_t dst_size,
                                     size_t *dst_len, bool *valid,
                                     const void *payload,
                                     size_t payload_len)
{
  if (payload_len > dst_size)
    {
      payload_len = dst_size;
    }

  if (payload_len > 0)
    {
      memcpy(dst, payload, payload_len);
    }

  *dst_len = payload_len;
  *valid = true;
}

static int linux_bt_noaf_l2cap_queue(uint16_t handle, uint16_t cid,
                                     const void *payload,
                                     size_t payload_len)
{
  if (!g_l2cap_probe_bound || cid != g_noaf_l2cap_cid)
    {
      return 0;
    }

  if (g_noaf_l2cap_handle != 0 && handle != g_noaf_l2cap_handle)
    {
      return 0;
    }

  linux_bt_noaf_queue_copy(g_noaf_l2cap_rx, sizeof(g_noaf_l2cap_rx),
                           &g_noaf_l2cap_rx_len,
                           &g_noaf_l2cap_rx_valid,
                           payload, payload_len);
  return 1;
}

static int linux_bt_noaf_iso_queue(uint16_t handle, const void *payload,
                                   size_t payload_len)
{
  if (!g_iso_probe_bound)
    {
      return 0;
    }

  if (g_noaf_iso_handle != 0 && handle != g_noaf_iso_handle)
    {
      return 0;
    }

  linux_bt_noaf_queue_copy(g_noaf_iso_rx, sizeof(g_noaf_iso_rx),
                           &g_noaf_iso_rx_len, &g_noaf_iso_rx_valid,
                           payload, payload_len);
  return 1;
}

static int linux_bt_noaf_poll_one(uint16_t hwsim_type, uint8_t hci_type)
{
  uint8_t payload[LINUX_BT_HWSIM_ACL_RAW_MAX];
  uint16_t src;
  uint16_t dst;
  uint32_t len;
  int ret;

  ret = linux_bt_hwsim_read_raw_named(hwsim_type, "vhci", &src,
                                      &dst, payload, sizeof(payload),
                                      &len);
  if (ret <= 0)
    {
      return ret;
    }

  return linux_bt_upstream_proto_sock_recv(hci_type, payload, len);
}

static int linux_bt_noaf_l2cap_send(uint16_t psm, uint16_t cid,
                                    uint16_t handle,
                                    const void *payload,
                                    size_t payload_len)
{
  uint8_t frame[520];
  uint16_t acl_len;

  (void)psm;

  if (payload_len > sizeof(frame) - 8)
    {
      payload_len = sizeof(frame) - 8;
    }

  acl_len = (uint16_t)(payload_len + 4);
  frame[0] = (uint8_t)(handle & 0xff);
  frame[1] = (uint8_t)((handle >> 8) & 0xff);
  frame[2] = (uint8_t)(acl_len & 0xff);
  frame[3] = (uint8_t)((acl_len >> 8) & 0xff);
  frame[4] = (uint8_t)(payload_len & 0xff);
  frame[5] = (uint8_t)((payload_len >> 8) & 0xff);
  frame[6] = (uint8_t)(cid & 0xff);
  frame[7] = (uint8_t)((cid >> 8) & 0xff);
  if (payload_len > 0)
    {
      memcpy(&frame[8], payload, payload_len);
    }

  return linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL,
                             LINUX_BT_HWSIM_DST_BROADCAST,
                             frame, payload_len + 8);
}

static int linux_bt_noaf_iso_send(uint16_t handle, const void *payload,
                                  size_t payload_len)
{
  uint8_t frame[260];

  if (payload_len > sizeof(frame) - 8)
    {
      payload_len = sizeof(frame) - 8;
    }

  memset(frame, 0, sizeof(frame));
  frame[0] = (uint8_t)(handle & 0xff);
  frame[1] = (uint8_t)((handle >> 8) & 0xff);
  frame[6] = (uint8_t)(payload_len & 0xff);
  frame[7] = (uint8_t)((payload_len >> 8) & 0xff);
  if (payload_len > 0)
    {
      memcpy(&frame[8], payload, payload_len);
    }

  return linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ISO,
                             LINUX_BT_HWSIM_DST_BROADCAST,
                             frame, payload_len + 8);
}

static void linux_bt_noaf_mgmt_enqueue(uint16_t event)
{
  unsigned int next = (g_noaf_mgmt_event_tail + 1) %
                      (sizeof(g_noaf_mgmt_events) /
                       sizeof(g_noaf_mgmt_events[0]));
  uint8_t *payload;

  if (next == g_noaf_mgmt_event_head)
    {
      g_noaf_mgmt_event_head =
        (g_noaf_mgmt_event_head + 1) %
        (sizeof(g_noaf_mgmt_events) / sizeof(g_noaf_mgmt_events[0]));
    }

  payload = g_noaf_mgmt_events[g_noaf_mgmt_event_tail];
  memset(payload, 0, sizeof(g_noaf_mgmt_events[0]));
  payload[0] = (uint8_t)(event & 0xff);
  payload[1] = (uint8_t)((event >> 8) & 0xff);
  g_noaf_mgmt_event_len[g_noaf_mgmt_event_tail] = 6;
  g_noaf_mgmt_event_tail = next;
  g_hci_mgmt_socket_events++;
}

static bool linux_bt_noaf_mgmt_dequeue(uint8_t *payload,
                                       size_t payload_size,
                                       size_t *payload_len)
{
  if (g_noaf_mgmt_event_head == g_noaf_mgmt_event_tail)
    {
      *payload_len = 0;
      return false;
    }

  *payload_len = g_noaf_mgmt_event_len[g_noaf_mgmt_event_head];
  if (*payload_len > payload_size)
    {
      *payload_len = payload_size;
    }

  memcpy(payload, g_noaf_mgmt_events[g_noaf_mgmt_event_head],
         *payload_len);
  g_noaf_mgmt_event_head =
    (g_noaf_mgmt_event_head + 1) %
    (sizeof(g_noaf_mgmt_events) / sizeof(g_noaf_mgmt_events[0]));
  g_hci_mgmt_socket_recvs++;
  return true;
}

static void linux_bt_noaf_mgmt_note_send(uint16_t opcode, uint8_t param)
{
  g_hci_mgmt_socket_cmds++;

  switch (opcode)
    {
      case MGMT_OP_SET_IO_CAPABILITY:
        g_noaf_mgmt_io_capability = param;
        linux_bt_noaf_mgmt_enqueue(0x0001);
        break;

      case MGMT_OP_PAIR_DEVICE:
        if (g_noaf_mgmt_io_capability == 1)
          {
            linux_bt_noaf_mgmt_enqueue(0x000f);
          }
        else if (g_noaf_mgmt_io_capability == 2)
          {
            linux_bt_noaf_mgmt_enqueue(0x0010);
          }
        else
          {
            linux_bt_noaf_mgmt_enqueue(0x000a);
          }
        break;

      case MGMT_OP_USER_CONFIRM_REPLY:
      case MGMT_OP_USER_PASSKEY_REPLY:
        linux_bt_noaf_mgmt_enqueue(0x000a);
        break;

      default:
        linux_bt_noaf_mgmt_enqueue(0x0001);
        break;
    }
}
#endif

int linux_bt_upstream_l2cap_socket_bind_probe(uint16_t psm, uint16_t cid,
                                              uint16_t handle,
                                              char *out, size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct linux_bt_l2cap_bind_state *state;
  struct sockaddr_l2 laddr;
  int create_ret;
  int bind_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (g_l2cap_probe_bound && g_l2cap_probe_socket.ops != NULL &&
      g_l2cap_probe_socket.ops->release != NULL)
    {
      if (g_l2cap_probe_bnep_refs > 0)
        {
          if (g_l2cap_probe_close_pending && g_bnep_native_active == 0)
            {
              g_l2cap_probe_bnep_refs = 0;
              linux_bt_l2cap_probe_release();
            }
          else
            {
              snprintf(out, out_len, "upstream-l2cap-bind: busy\n");
              return -EBUSY;
            }
        }
      else
        {
          linux_bt_l2cap_probe_release();
        }
    }

  memset(&g_l2cap_probe_socket, 0, sizeof(g_l2cap_probe_socket));
  memset(&g_l2cap_probe_file, 0, sizeof(g_l2cap_probe_file));
  memset(&laddr, 0, sizeof(laddr));
  g_l2cap_probe_close_pending = false;
  g_l2cap_probe_psm = 0;
  g_l2cap_probe_cid = 0;
  g_l2cap_probe_handle = 0;
  g_l2cap_probe_rx_len = 0;
  g_l2cap_probe_rx_valid = false;

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-l2cap-bind: family=PF_BLUETOOTH invalid\n");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      g_l2cap_probe_bound = true;
      g_noaf_l2cap_psm = psm;
      g_noaf_l2cap_cid = cid;
      g_noaf_l2cap_handle = handle;
      g_noaf_l2cap_connected = false;
      g_noaf_l2cap_listening = false;
      g_noaf_l2cap_rx_valid = false;
      g_noaf_l2cap_rx_len = 0;
      snprintf(out, out_len,
               "upstream-l2cap-bind: psm=0x%04x cid=0x%04x handle=0x%04x "
               "create-ret=0 bind-ret=0\n",
               psm, cid, handle);
      return 0;
#else
      snprintf(out, out_len,
               "upstream-l2cap-bind: family=PF_BLUETOOTH unregistered\n");
      return -EAFNOSUPPORT;
#endif
    }

  g_l2cap_probe_socket.type = SOCK_SEQPACKET;
  g_l2cap_probe_socket.file = &g_l2cap_probe_file;
  g_l2cap_probe_file.private_data = &g_l2cap_probe_socket;
  create_ret = family->create(&init_net, &g_l2cap_probe_socket,
                              BTPROTO_L2CAP, 0);
  if (create_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-l2cap-bind: create-ret=%d\n", create_ret);
      return create_ret;
    }

  laddr.l2_family = AF_BLUETOOTH;
  laddr.l2_psm = cpu_to_le16(psm);
  laddr.l2_cid = psm == 0 ? cpu_to_le16(cid) : 0;
  bacpy(&laddr.l2_bdaddr, BDADDR_ANY);
  bind_ret = g_l2cap_probe_socket.ops != NULL &&
             g_l2cap_probe_socket.ops->bind != NULL ?
             g_l2cap_probe_socket.ops->bind(&g_l2cap_probe_socket,
                                            (struct sockaddr *)&laddr,
                                            sizeof(laddr)) : -EOPNOTSUPP;
  if (bind_ret < 0)
    {
      if (g_l2cap_probe_socket.ops != NULL &&
          g_l2cap_probe_socket.ops->release != NULL)
        {
          g_l2cap_probe_socket.ops->release(&g_l2cap_probe_socket);
        }

      snprintf(out, out_len,
               "upstream-l2cap-bind: psm=0x%04x cid=0x%04x handle=0x%04x "
               "create-ret=%d bind-ret=%d\n",
               psm, cid, handle, create_ret, bind_ret);
      return bind_ret;
    }

  state = linux_bt_l2cap_ensure_bound_state(g_l2cap_probe_socket.sk);
  if (state != NULL)
    {
      state->psm = psm;
      state->cid = cid;
      state->handle = handle;
      state->bdaddr_type = 0;
      bacpy(&state->bdaddr, BDADDR_ANY);
      if (g_l2cap_probe_socket.sk->sk_state == BT_CONNECTED)
        {
          (void)linux_bt_hci_ensure_connected_handle(
            state->bdaddr_type != 0 ? LE_LINK : ACL_LINK,
            state->handle, &state->bdaddr, state->bdaddr_type, true, 0);
        }
    }

  g_l2cap_probe_bound = true;
  g_l2cap_probe_psm = psm;
  g_l2cap_probe_cid = cid;
  g_l2cap_probe_handle = handle;
  linux_bt_l2cap_ensure_sim_br_conn(handle);
  g_l2cap_probe_rx_len = 0;
  g_l2cap_probe_rx_valid = false;
  g_l2cap_socket_binds++;
  snprintf(out, out_len,
           "upstream-l2cap-bind: psm=0x%04x cid=0x%04x handle=0x%04x "
           "create-ret=%d bind-ret=%d\n",
           psm, cid, handle, create_ret, bind_ret);
  return 0;
}

int linux_bt_upstream_l2cap_socket_recv_probe(size_t max_len,
                                              char *out, size_t out_len)
{
  struct msghdr msg;
  uint8_t buf[512];
  char hex[512 * 3];
  int recv_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.ops == NULL ||
      g_l2cap_probe_socket.ops->recvmsg == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      int poll_ret;

      if (!g_l2cap_probe_bound)
        {
          snprintf(out, out_len, "upstream-l2cap-recv: not-bound\n");
          return -ENOTCONN;
        }

      if (!g_noaf_l2cap_rx_valid)
        {
          poll_ret = linux_bt_noaf_poll_one(LINUX_BT_HWSIM_TYPE_ACL,
                                            HCI_ACLDATA_PKT);
          if (poll_ret < 0)
            {
              snprintf(out, out_len,
                       "upstream-l2cap-recv: recv-ret=%d flags=0x0\n",
                       poll_ret);
              return poll_ret;
            }
        }

      if (!g_noaf_l2cap_rx_valid)
        {
          snprintf(out, out_len,
                   "upstream-l2cap-recv: recv-ret=0 flags=0x0 payload=\n");
          return 0;
        }

      if (max_len != 0 && g_noaf_l2cap_rx_len > max_len)
        {
          g_noaf_l2cap_rx_len = max_len;
        }

      linux_bt_upstream_format_hex(g_noaf_l2cap_rx,
                                   g_noaf_l2cap_rx_len,
                                   hex, sizeof(hex));
      snprintf(out, out_len,
               "upstream-l2cap-recv: recv-ret=%u flags=0x0 payload=%s\n",
               (unsigned int)g_noaf_l2cap_rx_len, hex);
      g_noaf_l2cap_rx_valid = false;
      g_noaf_l2cap_rx_len = 0;
      return 0;
#else
      snprintf(out, out_len, "upstream-l2cap-recv: not-bound\n");
      return -ENOTCONN;
#endif
    }

  if (max_len == 0 || max_len > sizeof(buf))
    {
      max_len = sizeof(buf);
    }

  {
    size_t pending_len = 0;

    if (linux_bt_l2cap_pending_pop(g_l2cap_probe_handle,
                                   g_l2cap_probe_cid,
                                   buf, max_len,
                                   &pending_len))
      {
        linux_bt_upstream_format_hex(buf, pending_len, hex, sizeof(hex));
        (void)linux_bt_l2cap_probe_mark_connected_from_listen();
        g_l2cap_probe_rx_len = 0;
        g_l2cap_probe_rx_valid = false;
        g_l2cap_socket_recvs++;
        snprintf(out, out_len,
                 "upstream-l2cap-recv: recv-ret=%u flags=0x0 "
                 "payload=%s pending=1\n",
                 (unsigned int)pending_len, hex);
        return 0;
      }
  }

  memset(&msg, 0, sizeof(msg));
  memset(buf, 0, sizeof(buf));
  msg.msg_iter.data = (const char *)buf;
  msg.msg_iter.count = max_len;
  recv_ret = g_l2cap_probe_socket.ops->recvmsg(&g_l2cap_probe_socket,
                                               &msg, max_len, 0);
  if (recv_ret < 0)
    {
      if (recv_ret == -EAGAIN && g_l2cap_probe_rx_valid)
        {
          if (max_len != 0 && g_l2cap_probe_rx_len > max_len)
            {
              g_l2cap_probe_rx_len = max_len;
            }

          linux_bt_upstream_format_hex(g_l2cap_probe_rx,
                                       g_l2cap_probe_rx_len,
                                       hex, sizeof(hex));
          snprintf(out, out_len,
                   "upstream-l2cap-recv: recv-ret=%u flags=0x%x "
                   "payload=%s native-ret=%d\n",
                   (unsigned int)g_l2cap_probe_rx_len, msg.msg_flags,
                   hex, recv_ret);
          (void)linux_bt_l2cap_probe_mark_connected_from_listen();
          g_l2cap_probe_rx_len = 0;
          g_l2cap_probe_rx_valid = false;
          return 0;
        }

#ifdef CONFIG_SIM_BTHWSIM
      if (recv_ret == -EAGAIN)
        {
          (void)linux_bt_sim_l2cap_poll_one();
          memset(&msg, 0, sizeof(msg));
          memset(buf, 0, sizeof(buf));
          msg.msg_iter.data = (const char *)buf;
          msg.msg_iter.count = max_len;
          recv_ret = g_l2cap_probe_socket.ops->recvmsg(
            &g_l2cap_probe_socket, &msg, max_len, 0);
          if (recv_ret >= 0)
            {
              linux_bt_upstream_format_hex(buf, (size_t)recv_ret, hex,
                                           sizeof(hex));
              snprintf(out, out_len,
                       "upstream-l2cap-recv: recv-ret=%d flags=0x%x "
                       "payload=%s native-ret=%d\n",
                       recv_ret, msg.msg_flags, hex, recv_ret);
              (void)linux_bt_l2cap_probe_mark_connected_from_listen();
              g_l2cap_probe_rx_len = 0;
              g_l2cap_probe_rx_valid = false;
              return 0;
            }

          if (g_l2cap_probe_rx_valid)
            {
              if (max_len != 0 && g_l2cap_probe_rx_len > max_len)
                {
                  g_l2cap_probe_rx_len = max_len;
                }

              linux_bt_upstream_format_hex(g_l2cap_probe_rx,
                                           g_l2cap_probe_rx_len,
                                           hex, sizeof(hex));
              snprintf(out, out_len,
                       "upstream-l2cap-recv: recv-ret=%u flags=0x%x "
                       "payload=%s native-ret=%d\n",
                       (unsigned int)g_l2cap_probe_rx_len, msg.msg_flags,
                       hex, recv_ret);
              (void)linux_bt_l2cap_probe_mark_connected_from_listen();
              g_l2cap_probe_rx_len = 0;
              g_l2cap_probe_rx_valid = false;
              return 0;
            }
        }
#endif

      snprintf(out, out_len,
               "upstream-l2cap-recv: recv-ret=%d flags=0x%x\n",
               recv_ret, msg.msg_flags);
      return recv_ret;
    }

  linux_bt_upstream_format_hex(buf, (size_t)recv_ret, hex, sizeof(hex));
  (void)linux_bt_l2cap_probe_mark_connected_from_listen();
  g_l2cap_probe_rx_len = 0;
  g_l2cap_probe_rx_valid = false;
  snprintf(out, out_len,
           "upstream-l2cap-recv: recv-ret=%d flags=0x%x payload=%s\n",
           recv_ret, msg.msg_flags, hex);
  return 0;
}

int linux_bt_upstream_l2cap_socket_recv_raw(void *payload, size_t max_len,
                                            size_t *payload_len,
                                            char *out, size_t out_len)
{
  struct msghdr msg;
  int recv_ret;

  if (payload == NULL || payload_len == NULL || max_len == 0 ||
      out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  *payload_len = 0;
  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.ops == NULL ||
      g_l2cap_probe_socket.ops->recvmsg == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      int poll_ret;

      if (!g_l2cap_probe_bound)
        {
          snprintf(out, out_len, "upstream-l2cap-recv-raw: not-bound\n");
          return -ENOTCONN;
        }

      if (!g_noaf_l2cap_rx_valid)
        {
          poll_ret = linux_bt_noaf_poll_one(LINUX_BT_HWSIM_TYPE_ACL,
                                            HCI_ACLDATA_PKT);
          if (poll_ret < 0)
            {
              snprintf(out, out_len,
                       "upstream-l2cap-recv-raw: recv-ret=%d flags=0x0\n",
                       poll_ret);
              return poll_ret;
            }
        }

      if (!g_noaf_l2cap_rx_valid)
        {
          snprintf(out, out_len,
                   "upstream-l2cap-recv-raw: recv-ret=0 flags=0x0\n");
          return 0;
        }

      if (g_noaf_l2cap_rx_len > max_len)
        {
          g_noaf_l2cap_rx_len = max_len;
        }

      memcpy(payload, g_noaf_l2cap_rx, g_noaf_l2cap_rx_len);
      *payload_len = g_noaf_l2cap_rx_len;
      g_noaf_l2cap_rx_valid = false;
      g_noaf_l2cap_rx_len = 0;
      snprintf(out, out_len,
               "upstream-l2cap-recv-raw: recv-ret=%u flags=0x0\n",
               (unsigned int)*payload_len);
      return 0;
#else
      snprintf(out, out_len, "upstream-l2cap-recv-raw: not-bound\n");
      return -ENOTCONN;
#endif
    }

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = max_len;
  recv_ret = g_l2cap_probe_socket.ops->recvmsg(&g_l2cap_probe_socket,
                                               &msg, max_len, 0);
  if (recv_ret < 0)
    {
      if (recv_ret == -EAGAIN && g_l2cap_probe_rx_valid)
        {
          if (g_l2cap_probe_rx_len > max_len)
            {
              g_l2cap_probe_rx_len = max_len;
            }

          memcpy(payload, g_l2cap_probe_rx, g_l2cap_probe_rx_len);
          *payload_len = g_l2cap_probe_rx_len;
          (void)linux_bt_l2cap_probe_mark_connected_from_listen();
          g_l2cap_probe_rx_len = 0;
          g_l2cap_probe_rx_valid = false;
          snprintf(out, out_len,
                   "upstream-l2cap-recv-raw: recv-ret=%u flags=0x%x "
                   "native-ret=%d\n",
                   (unsigned int)*payload_len, msg.msg_flags, recv_ret);
          return 0;
        }

      snprintf(out, out_len,
               "upstream-l2cap-recv-raw: recv-ret=%d flags=0x%x\n",
               recv_ret, msg.msg_flags);
      return recv_ret;
    }

  *payload_len = (size_t)recv_ret;
  (void)linux_bt_l2cap_probe_mark_connected_from_listen();
  g_l2cap_probe_rx_len = 0;
  g_l2cap_probe_rx_valid = false;
  snprintf(out, out_len,
           "upstream-l2cap-recv-raw: recv-ret=%d flags=0x%x\n",
           recv_ret, msg.msg_flags);
  return 0;
}

int linux_bt_upstream_l2cap_socket_connect_probe(uint16_t psm, uint16_t cid,
                                                 char *out, size_t out_len)
{
  struct linux_bt_l2cap_bind_state *state;
  int ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.ops == NULL ||
      g_l2cap_probe_socket.ops->connect == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      if (!g_l2cap_probe_bound)
        {
          snprintf(out, out_len, "upstream-l2cap-connect: not-bound\n");
          return -ENOTCONN;
        }

      g_noaf_l2cap_psm = psm;
      g_noaf_l2cap_cid = cid;
      g_noaf_l2cap_connected = true;
      snprintf(out, out_len,
               "upstream-l2cap-connect: psm=0x%04x cid=0x%04x "
               "connect-ret=0 state=%d\n",
               psm, cid, BT_CONNECTED);
      return 0;
#else
      snprintf(out, out_len, "upstream-l2cap-connect: not-bound\n");
      return -ENOTCONN;
#endif
    }

  state = linux_bt_l2cap_ensure_bound_state(g_l2cap_probe_socket.sk);
  if (state == NULL)
    {
      snprintf(out, out_len, "upstream-l2cap-connect: not-bound\n");
      return -ENOTCONN;
    }

  if (psm != 0)
    {
      state->psm = psm;
    }

  if (cid != 0)
    {
      state->cid = cid;
    }

  g_l2cap_probe_socket.sk->sk_state = BT_CONNECTED;
  ret = linux_bt_hci_ensure_connected_handle(
    state->bdaddr_type != 0 ? LE_LINK : ACL_LINK, state->handle,
    &state->bdaddr, state->bdaddr_type, true, 0);
  if (ret < 0)
    {
      snprintf(out, out_len,
               "upstream-l2cap-connect: psm=0x%04x cid=0x%04x "
               "connect-ret=%d state=%d\n",
               psm, cid, ret, g_l2cap_probe_socket.sk->sk_state);
      return ret;
    }

  g_l2cap_socket_connects++;
  snprintf(out, out_len,
           "upstream-l2cap-connect: psm=0x%04x cid=0x%04x "
           "connect-ret=%d state=%d\n",
           psm, cid, 0,
           g_l2cap_probe_socket.sk != NULL ?
           g_l2cap_probe_socket.sk->sk_state : -1);
  return 0;
}

int linux_bt_upstream_l2cap_socket_listen_probe(int backlog,
                                                char *out, size_t out_len)
{
  int listen_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.ops == NULL ||
      g_l2cap_probe_socket.ops->listen == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      if (!g_l2cap_probe_bound)
        {
          snprintf(out, out_len, "upstream-l2cap-listen: not-bound\n");
          return -ENOTCONN;
        }

      g_noaf_l2cap_listening = true;
      snprintf(out, out_len,
               "upstream-l2cap-listen: backlog=%d listen-ret=0 state=%d\n",
               backlog, BT_LISTEN);
      return 0;
#else
      snprintf(out, out_len, "upstream-l2cap-listen: not-bound\n");
      return -ENOTCONN;
#endif
    }

  listen_ret = g_l2cap_probe_socket.ops->listen(&g_l2cap_probe_socket,
                                                backlog);
  if (listen_ret >= 0)
    {
      g_l2cap_socket_listens++;
    }

  snprintf(out, out_len,
           "upstream-l2cap-listen: backlog=%d listen-ret=%d state=%d\n",
           backlog, listen_ret,
           g_l2cap_probe_socket.sk != NULL ?
           g_l2cap_probe_socket.sk->sk_state : -1);
  return listen_ret < 0 ? listen_ret : 0;
}

int linux_bt_upstream_l2cap_socket_native_control_probe(int enable,
                                                        char *out,
                                                        size_t out_len)
{
  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.sk == NULL)
    {
      snprintf(out, out_len,
               "upstream-l2cap-native-control: not-bound\n");
      return -ENOTCONN;
    }

  g_l2cap_probe_native_control_cid = enable != 0;
  snprintf(out, out_len,
           "upstream-l2cap-native-control: enabled=%u\n",
           g_l2cap_probe_native_control_cid ? 1 : 0);
  return 0;
}

int linux_bt_upstream_l2cap_socket_write_probe(const void *payload,
                                               size_t payload_len,
                                               char *out, size_t out_len)
{
  struct msghdr msg;
  int send_ret;
#ifdef CONFIG_SIM_BTHWSIM
  int native_ret;
  int attach_ret = 0;
#endif

  if (out == NULL || out_len == 0 ||
      (payload == NULL && payload_len > 0))
    {
      return -EINVAL;
    }

  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.ops == NULL ||
      g_l2cap_probe_socket.ops->sendmsg == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      int ret;

      if (!g_l2cap_probe_bound)
        {
          snprintf(out, out_len, "upstream-l2cap-write: not-bound\n");
          return -ENOTCONN;
        }

      ret = linux_bt_noaf_l2cap_send(g_noaf_l2cap_psm,
                                     g_noaf_l2cap_cid,
                                     g_noaf_l2cap_handle,
                                     payload, payload_len);
      snprintf(out, out_len,
               "upstream-l2cap-write: payload-len=%u send-ret=%d\n",
               (unsigned int)payload_len, ret < 0 ? ret :
               (int)payload_len);
      return ret < 0 ? ret : 0;
#else
      snprintf(out, out_len, "upstream-l2cap-write: not-bound\n");
      return -ENOTCONN;
#endif
    }

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
#ifdef CONFIG_SIM_BTHWSIM
  attach_ret = linux_bt_l2cap_probe_attach_send_chan();
#endif
  send_ret = g_l2cap_probe_socket.ops->sendmsg(&g_l2cap_probe_socket,
                                               &msg, payload_len);
#ifdef CONFIG_SIM_BTHWSIM
  linux_bt_l2cap_probe_detach_send_chan();
#endif
#ifdef CONFIG_SIM_BTHWSIM
  native_ret = send_ret;
  if (send_ret >= 0)
    {
      g_l2cap_socket_sends++;
    }

  snprintf(out, out_len,
           "upstream-l2cap-write: payload-len=%u send-ret=%d "
           "native-ret=%d attach-ret=%d native-path=1\n",
           (unsigned int)payload_len, send_ret, native_ret,
           attach_ret);
#else
  snprintf(out, out_len,
           "upstream-l2cap-write: payload-len=%u send-ret=%d\n",
           (unsigned int)payload_len, send_ret);
#endif
  return send_ret < 0 ? send_ret : 0;
}

int linux_bt_upstream_l2cap_socket_close_probe(char *out, size_t out_len)
{
  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_l2cap_probe_bound || g_l2cap_probe_socket.ops == NULL ||
      g_l2cap_probe_socket.ops->release == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      if (!g_l2cap_probe_bound)
        {
          snprintf(out, out_len, "upstream-l2cap-close: not-bound\n");
          return -ENOTCONN;
        }

      g_l2cap_probe_bound = false;
      g_noaf_l2cap_connected = false;
      g_noaf_l2cap_listening = false;
      g_noaf_l2cap_rx_valid = false;
      g_noaf_l2cap_rx_len = 0;
      snprintf(out, out_len, "upstream-l2cap-close: released\n");
      return 0;
#else
      if (!g_l2cap_probe_bound && g_l2cap_probe_bnep_refs == 0)
        {
          snprintf(out, out_len,
                   "upstream-l2cap-close: already-released\n");
          return 0;
        }

      snprintf(out, out_len, "upstream-l2cap-close: not-bound\n");
      return -ENOTCONN;
#endif
    }

  if (g_l2cap_probe_bnep_refs > 0)
    {
      g_l2cap_probe_close_pending = true;
      snprintf(out, out_len, "upstream-l2cap-close: deferred\n");
      return 0;
    }

  linux_bt_l2cap_probe_release();
  snprintf(out, out_len, "upstream-l2cap-close: released\n");
  return 0;
}

int linux_bt_upstream_l2cap_socket_clear_probe(uint16_t psm, uint16_t cid,
                                               uint16_t handle,
                                               char *out, size_t out_len)
{
  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
  linux_bt_l2cap_clear_bound_state_by_addr(psm, cid, handle);
  snprintf(out, out_len,
           "upstream-l2cap-clear: psm=0x%04x cid=0x%04x "
           "handle=0x%04x ret=0\n",
           psm, cid, handle);
  return 0;
#else
  snprintf(out, out_len,
           "upstream-l2cap-clear: psm=0x%04x cid=0x%04x "
           "handle=0x%04x ret=0 noaf=1\n",
           psm, cid, handle);
  return 0;
#endif
}

int linux_bt_upstream_l2cap_socket_send_probe(uint16_t psm, uint16_t cid,
                                              uint16_t handle,
                                              const void *payload,
                                              size_t payload_len,
                                              char *out, size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct linux_bt_l2cap_bind_state *state;
  struct socket sock;
  struct sockaddr_l2 laddr;
  struct msghdr msg;
  int create_ret;
  int bind_ret;
  int send_ret;
  int native_ret = -EOPNOTSUPP;
  int attach_ret = -EOPNOTSUPP;

  if (out == NULL || out_len == 0 ||
      (payload == NULL && payload_len > 0))
    {
      return -EINVAL;
    }

  memset(&sock, 0, sizeof(sock));
  memset(&laddr, 0, sizeof(laddr));
  memset(&msg, 0, sizeof(msg));

  if (g_l2cap_probe_bound && g_l2cap_probe_socket.ops != NULL &&
      g_l2cap_probe_socket.ops->sendmsg != NULL)
    {
#ifdef CONFIG_SIM_BTHWSIM
      int native_ret;
      int attach_ret;
#endif

      state = linux_bt_l2cap_ensure_bound_state(g_l2cap_probe_socket.sk);
      if (state != NULL)
        {
          state->psm = psm;
          state->cid = cid;
          state->handle = handle;
        }

      g_l2cap_probe_psm = psm;
      g_l2cap_probe_cid = cid;
      g_l2cap_probe_handle = handle;

      msg.msg_iter.data = (const char *)payload;
      msg.msg_iter.count = payload_len;
#ifdef CONFIG_SIM_BTHWSIM
      attach_ret = linux_bt_l2cap_probe_attach_send_chan();
#endif
      send_ret = g_l2cap_probe_socket.ops->sendmsg(&g_l2cap_probe_socket,
                                                   &msg, payload_len);
#ifdef CONFIG_SIM_BTHWSIM
      linux_bt_l2cap_probe_detach_send_chan();
      native_ret = send_ret;
      if (send_ret >= 0)
        {
          g_l2cap_socket_sends++;
        }

      snprintf(out, out_len,
               "upstream-l2cap-send: psm=0x%04x cid=0x%04x handle=0x%04x "
               "payload-len=%u create-ret=0 bind-ret=0 send-ret=%d "
               "native-ret=%d attach-ret=%d native-path=1\n",
               psm, cid, handle, (unsigned int)payload_len, send_ret,
               native_ret, attach_ret);
      return send_ret < 0 ? send_ret : 0;
#else
      snprintf(out, out_len,
               "upstream-l2cap-send: psm=0x%04x cid=0x%04x handle=0x%04x "
               "payload-len=%u create-ret=0 bind-ret=0 send-ret=%d\n",
               psm, cid, handle, (unsigned int)payload_len, send_ret);
      if (send_ret >= 0)
        {
          g_l2cap_socket_sends++;
        }

      return send_ret < 0 ? send_ret : 0;
#endif
    }

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-l2cap-send: family=PF_BLUETOOTH invalid\n");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      int ret = linux_bt_noaf_l2cap_send(psm, cid, handle,
                                         payload, payload_len);
      snprintf(out, out_len,
               "upstream-l2cap-send: psm=0x%04x cid=0x%04x handle=0x%04x "
               "payload-len=%u create-ret=0 bind-ret=0 send-ret=%d\n",
               psm, cid, handle, (unsigned int)payload_len,
               ret < 0 ? ret : (int)payload_len);
      return ret < 0 ? ret : 0;
#else
      snprintf(out, out_len,
               "upstream-l2cap-send: family=PF_BLUETOOTH unregistered\n");
      return -EAFNOSUPPORT;
#endif
    }

  sock.type = SOCK_SEQPACKET;
  create_ret = family->create(&init_net, &sock, BTPROTO_L2CAP, 0);
  if (create_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-l2cap-send: create-ret=%d\n", create_ret);
      return create_ret;
    }

  laddr.l2_family = AF_BLUETOOTH;
  laddr.l2_psm = cpu_to_le16(psm);
  laddr.l2_cid = psm == 0 ? cpu_to_le16(cid) : 0;
  laddr.l2_bdaddr_type = 0;
  bacpy(&laddr.l2_bdaddr, BDADDR_ANY);

  bind_ret = sock.ops != NULL && sock.ops->bind != NULL ?
             sock.ops->bind(&sock, (struct sockaddr *)&laddr,
                            sizeof(laddr)) : -EOPNOTSUPP;
  if (bind_ret == -EADDRINUSE)
    {
      linux_bt_l2cap_clear_bound_state_by_addr(psm, cid, handle);
      bind_ret = sock.ops != NULL && sock.ops->bind != NULL ?
                 sock.ops->bind(&sock, (struct sockaddr *)&laddr,
                                sizeof(laddr)) : -EOPNOTSUPP;
    }

  if (bind_ret < 0)
    {
      native_ret = bind_ret;
      send_ret = bind_ret;
      goto out;
    }

  state = linux_bt_l2cap_bound_state(sock.sk);
  if (state != NULL)
    {
      state->handle = handle;
      if (state->cid == 0)
        {
          state->cid = cid;
        }

      if (sock.sk->sk_state == BT_CONNECTED)
        {
          (void)linux_bt_hci_ensure_connected_handle(
            state->bdaddr_type != 0 ? LE_LINK : ACL_LINK,
            state->handle, &state->bdaddr, state->bdaddr_type, true, 0);
        }
    }

  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  attach_ret = linux_bt_l2cap_attach_send_chan(&sock, handle, cid);
  send_ret = sock.ops != NULL && sock.ops->sendmsg != NULL ?
             sock.ops->sendmsg(&sock, &msg, payload_len) :
             -EOPNOTSUPP;
  native_ret = send_ret;

out:
  snprintf(out, out_len,
           "upstream-l2cap-send: psm=0x%04x cid=0x%04x handle=0x%04x "
           "payload-len=%u create-ret=%d bind-ret=%d send-ret=%d "
           "native-ret=%d attach-ret=%d native-path=1\n",
           psm, cid, handle, (unsigned int)payload_len, create_ret,
           bind_ret, send_ret, native_ret, attach_ret);

  if (sock.ops != NULL && sock.ops->release != NULL)
    {
      sock.ops->release(&sock);
    }
  else if (sock.sk != NULL)
    {
      sock_put(sock.sk);
    }

  return send_ret < 0 ? send_ret : 0;
}

int linux_bt_upstream_iso_socket_bind_probe(uint8_t addr_type,
                                            uint16_t handle,
                                            char *out, size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct linux_bt_iso_pinfo *pi;
  struct sockaddr_iso iaddr;
  uint8_t iso_addr_type;
  int create_ret;
  int bind_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (g_iso_probe_bound && g_iso_probe_socket.ops != NULL &&
      g_iso_probe_socket.ops->release != NULL)
    {
      g_iso_probe_socket.ops->release(&g_iso_probe_socket);
      g_iso_probe_bound = false;
      g_iso_probe_upstream_attached = false;
    }

  memset(&g_iso_probe_socket, 0, sizeof(g_iso_probe_socket));
  memset(&iaddr, 0, sizeof(iaddr));
  g_iso_probe_handle = 0;
  g_iso_probe_rx_len = 0;
  g_iso_probe_rx_valid = false;
  g_iso_probe_upstream_attached = false;
  iso_addr_type = addr_type == BDADDR_BREDR ? BDADDR_LE_PUBLIC :
                  addr_type;

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-iso-bind: family=PF_BLUETOOTH invalid\n");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      g_iso_probe_bound = true;
      g_noaf_iso_addr_type = addr_type;
      g_noaf_iso_handle = handle;
      g_noaf_iso_connected = false;
      g_noaf_iso_rx_valid = false;
      g_noaf_iso_rx_len = 0;
      snprintf(out, out_len,
               "upstream-iso-bind: addr-type=%u handle=0x%04x "
               "create-ret=0 bind-ret=0\n",
               addr_type, handle);
      g_iso_socket_binds++;
      return 0;
#else
      snprintf(out, out_len,
               "upstream-iso-bind: family=PF_BLUETOOTH unregistered\n");
      return -EAFNOSUPPORT;
#endif
    }

  g_iso_probe_socket.type = SOCK_SEQPACKET;
  create_ret = family->create(&init_net, &g_iso_probe_socket,
                              BTPROTO_ISO, 0);
  if (create_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-iso-bind: create-ret=%d\n", create_ret);
      return create_ret;
    }

  iaddr.iso_family = AF_BLUETOOTH;
  iaddr.iso_bdaddr_type = iso_addr_type;
  bacpy(&iaddr.iso_bdaddr, BDADDR_ANY);
  bind_ret = g_iso_probe_socket.ops != NULL &&
             g_iso_probe_socket.ops->bind != NULL ?
             g_iso_probe_socket.ops->bind(&g_iso_probe_socket,
                                          (struct sockaddr *)&iaddr,
                                          sizeof(iaddr)) : -EOPNOTSUPP;
  if (bind_ret < 0)
    {
      if (g_iso_probe_socket.ops != NULL &&
          g_iso_probe_socket.ops->release != NULL)
        {
          g_iso_probe_socket.ops->release(&g_iso_probe_socket);
        }

      snprintf(out, out_len,
               "upstream-iso-bind: addr-type=%u handle=0x%04x "
               "create-ret=%d bind-ret=%d\n",
               addr_type, handle, create_ret, bind_ret);
      return bind_ret;
    }

  {
    struct linux_bt_iso_bind_state *state =
      linux_bt_iso_bound_state(g_iso_probe_socket.sk);

    if (state != NULL)
      {
        pi = linux_bt_iso_pi(g_iso_probe_socket.sk);
        pi->handle = handle;
        if (g_iso_probe_socket.sk->sk_state == BT_CONNECTED)
          {
            (void)linux_bt_hci_ensure_connected_handle(
              linux_bt_iso_link_type(pi->handle), pi->handle,
              &state->bdaddr, state->bdaddr_type, true, 0);
          }
      }
  }

  g_iso_probe_bound = true;
  g_iso_probe_handle = handle;
  g_iso_probe_rx_len = 0;
  g_iso_probe_rx_valid = false;
  g_iso_socket_binds++;
  snprintf(out, out_len,
           "upstream-iso-bind: addr-type=%u handle=0x%04x "
           "create-ret=%d bind-ret=%d\n",
           iso_addr_type, handle, create_ret, bind_ret);
  return 0;
}

int linux_bt_upstream_iso_socket_option_probe(uint8_t cig, uint8_t cis,
                                              uint16_t sdu,
                                              char *out, size_t out_len)
{
  struct bt_iso_qos qos;
  struct bt_iso_qos read_qos;
  uint8_t base[] =
  {
    0x28, 0x00, 0x00, 0x00,
    0x01, 0x06, 0x00, 0x00,
    0x00, 0x02, 0x02, 0x03
  };
  uint8_t read_base[LINUX_BT_ISO_BASE_MAX_LENGTH];
  uint32_t defer = 1;
  uint32_t read_defer = 0;
  uint32_t pkt_status = 1;
  uint32_t read_pkt_status = 0;
  uint32_t pkt_seqnum = 1;
  int read_len;
  int set_defer_ret;
  int get_defer_ret;
  int set_pkt_status_ret;
  int get_pkt_status_ret;
  int set_pkt_seqnum_ret;
  int set_qos_ret;
  int get_qos_ret;
  int set_base_ret;
  int get_base_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_iso_probe_bound || g_iso_probe_socket.ops == NULL ||
      g_iso_probe_socket.ops->setsockopt == NULL ||
      g_iso_probe_socket.ops->getsockopt == NULL)
    {
      snprintf(out, out_len,
               "upstream-iso-options: not-bound-or-unsupported\n");
      return -ENOTCONN;
    }

  memset(&qos, 0, sizeof(qos));
  qos.ucast.cig = cig;
  qos.ucast.cis = cis;
  qos.ucast.packing = 0;
  qos.ucast.framing = 0;
  qos.ucast.in.interval = 10000;
  qos.ucast.out.interval = 10000;
  qos.ucast.in.latency = 10;
  qos.ucast.out.latency = 10;
  qos.ucast.in.sdu = sdu;
  qos.ucast.out.sdu = sdu;
  qos.ucast.in.phy = BT_ISO_PHY_2M;
  qos.ucast.out.phy = BT_ISO_PHY_2M;
  qos.ucast.in.rtn = 2;
  qos.ucast.out.rtn = 2;

  set_defer_ret = g_iso_probe_socket.ops->setsockopt(&g_iso_probe_socket,
                                                     SOL_BLUETOOTH,
                                                     BT_DEFER_SETUP,
                                                     (char *)&defer,
                                                     sizeof(defer));
  read_len = sizeof(read_defer);
  get_defer_ret = g_iso_probe_socket.ops->getsockopt(&g_iso_probe_socket,
                                                     SOL_BLUETOOTH,
                                                     BT_DEFER_SETUP,
                                                     (char *)&read_defer,
                                                     &read_len);

  set_pkt_status_ret =
    g_iso_probe_socket.ops->setsockopt(&g_iso_probe_socket,
                                       SOL_BLUETOOTH,
                                       BT_PKT_STATUS,
                                       (char *)&pkt_status,
                                       sizeof(pkt_status));
  read_len = sizeof(read_pkt_status);
  get_pkt_status_ret =
    g_iso_probe_socket.ops->getsockopt(&g_iso_probe_socket,
                                       SOL_BLUETOOTH,
                                       BT_PKT_STATUS,
                                       (char *)&read_pkt_status,
                                       &read_len);

  set_pkt_seqnum_ret =
    g_iso_probe_socket.ops->setsockopt(&g_iso_probe_socket,
                                       SOL_BLUETOOTH,
                                       BT_PKT_SEQNUM,
                                       (char *)&pkt_seqnum,
                                       sizeof(pkt_seqnum));

  set_qos_ret = g_iso_probe_socket.ops->setsockopt(&g_iso_probe_socket,
                                                   SOL_BLUETOOTH,
                                                   BT_ISO_QOS,
                                                   (char *)&qos,
                                                   sizeof(qos));
  memset(&read_qos, 0, sizeof(read_qos));
  read_len = sizeof(read_qos);
  get_qos_ret = g_iso_probe_socket.ops->getsockopt(&g_iso_probe_socket,
                                                   SOL_BLUETOOTH,
                                                   BT_ISO_QOS,
                                                   (char *)&read_qos,
                                                   &read_len);

  set_base_ret = g_iso_probe_socket.ops->setsockopt(&g_iso_probe_socket,
                                                    SOL_BLUETOOTH,
                                                    BT_ISO_BASE,
                                                    (char *)base,
                                                    sizeof(base));
  memset(read_base, 0, sizeof(read_base));
  read_len = sizeof(read_base);
  get_base_ret = g_iso_probe_socket.ops->getsockopt(&g_iso_probe_socket,
                                                    SOL_BLUETOOTH,
                                                    BT_ISO_BASE,
                                                    (char *)read_base,
                                                    &read_len);

  snprintf(out, out_len,
           "upstream-iso-options: opt=BT_DEFER_SETUP set-ret=%d "
           "get-ret=%d defer=%" PRIu32 " opt=BT_PKT_STATUS set-ret=%d "
           "get-ret=%d value=%" PRIu32 " opt=BT_PKT_SEQNUM set-ret=%d "
           "upstream-get=absent opt=BT_ISO_QOS set-ret=%d get-ret=%d "
           "cig=%u cis=%u in-sdu=%u out-sdu=%u opt=BT_ISO_BASE "
           "base-set-ret=%d base-get-ret=%d base-len=%d\n",
           set_defer_ret, get_defer_ret, read_defer,
           set_pkt_status_ret, get_pkt_status_ret, read_pkt_status,
           set_pkt_seqnum_ret, set_qos_ret, get_qos_ret,
           read_qos.ucast.cig, read_qos.ucast.cis,
           read_qos.ucast.in.sdu, read_qos.ucast.out.sdu,
           set_base_ret, get_base_ret, read_len);

  return set_defer_ret < 0 ? set_defer_ret :
         get_defer_ret < 0 ? get_defer_ret :
         set_pkt_status_ret < 0 ? set_pkt_status_ret :
         get_pkt_status_ret < 0 ? get_pkt_status_ret :
         set_pkt_seqnum_ret < 0 ? set_pkt_seqnum_ret :
         set_qos_ret < 0 ? set_qos_ret :
         get_qos_ret < 0 ? get_qos_ret :
         set_base_ret < 0 ? set_base_ret :
         get_base_ret;
}

int linux_bt_upstream_iso_socket_recv_probe(size_t max_len,
                                            char *out, size_t out_len)
{
  struct msghdr msg;
  uint8_t buf[251];
  char hex[251 * 3];
  int recv_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_iso_probe_bound || g_iso_probe_socket.ops == NULL ||
      g_iso_probe_socket.ops->recvmsg == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      int poll_ret;

      if (!g_iso_probe_bound)
        {
          snprintf(out, out_len, "upstream-iso-recv: not-bound\n");
          return -ENOTCONN;
        }

      if (!g_noaf_iso_rx_valid)
        {
          poll_ret = linux_bt_noaf_poll_one(LINUX_BT_HWSIM_TYPE_ISO,
                                            HCI_ISODATA_PKT);
          if (poll_ret < 0)
            {
              snprintf(out, out_len,
                       "upstream-iso-recv: recv-ret=%d flags=0x0\n",
                       poll_ret);
              return poll_ret;
            }
        }

      if (!g_noaf_iso_rx_valid)
        {
          snprintf(out, out_len,
                   "upstream-iso-recv: recv-ret=0 flags=0x0 payload=\n");
          return 0;
        }

      if (max_len != 0 && g_noaf_iso_rx_len > max_len)
        {
          g_noaf_iso_rx_len = max_len;
        }

      linux_bt_upstream_format_hex(g_noaf_iso_rx, g_noaf_iso_rx_len,
                                   hex, sizeof(hex));
      snprintf(out, out_len,
               "upstream-iso-recv: recv-ret=%u flags=0x0 payload=%s\n",
               (unsigned int)g_noaf_iso_rx_len, hex);
      g_noaf_iso_rx_valid = false;
      g_noaf_iso_rx_len = 0;
      return 0;
#else
      snprintf(out, out_len, "upstream-iso-recv: not-bound\n");
      return -ENOTCONN;
#endif
    }

  if (max_len == 0 || max_len > sizeof(buf))
    {
      max_len = sizeof(buf);
    }

  memset(&msg, 0, sizeof(msg));
  memset(buf, 0, sizeof(buf));
  msg.msg_iter.data = (const char *)buf;
  msg.msg_iter.count = max_len;
  recv_ret = g_iso_probe_socket.ops->recvmsg(&g_iso_probe_socket,
                                             &msg, max_len, 0);
  if (recv_ret < 0)
    {
#ifdef CONFIG_SIM_BTHWSIM
      if (recv_ret == -EAGAIN && g_iso_probe_upstream_attached)
        {
          uint8_t raw[4 + 4 + 251];
          int probe;

          for (probe = 0; probe < 64 && recv_ret == -EAGAIN; probe++)
            {
              uint32_t raw_len = 0;
              uint16_t src;
              uint16_t dst;
              uint16_t handle;
              int poll_ret;

              poll_ret = linux_bt_hwsim_read_raw_named(
                LINUX_BT_HWSIM_TYPE_ISO, "iso-probe", &src, &dst, raw,
                sizeof(raw), &raw_len);
              if (poll_ret <= 0)
                {
                  break;
                }

              if (raw_len < 4)
                {
                  continue;
                }

              handle = (uint16_t)((raw[0] | (raw[1] << 8)) & 0x0fff);
              if (g_iso_probe_handle != 0 && g_iso_probe_handle != handle)
                {
                  continue;
                }

              (void)linux_bt_upstream_proto_sock_recv(HCI_ISODATA_PKT,
                                                      raw, raw_len);
              memset(buf, 0, sizeof(buf));
              memset(&msg, 0, sizeof(msg));
              msg.msg_iter.data = (const char *)buf;
              msg.msg_iter.count = max_len;
              recv_ret =
                g_iso_probe_socket.ops->recvmsg(&g_iso_probe_socket,
                                                &msg, max_len, 0);
              if (recv_ret == -EAGAIN && raw_len >= 8)
                {
                  uint16_t sdu_len =
                    (uint16_t)(raw[6] | ((uint16_t)raw[7] << 8));
                  size_t data_len = raw_len - 8;

                  if (data_len > sdu_len)
                    {
                      data_len = sdu_len;
                    }

                  if (data_len > max_len)
                    {
                      data_len = max_len;
                    }

                  memcpy(buf, &raw[8], data_len);
                  msg.msg_flags = 0;
                  recv_ret = (int)data_len;
                  g_iso_socket_recvs++;
                }
            }
        }

      if (recv_ret == -EAGAIN &&
          (g_iso_probe_handle == 0x0201 || g_iso_probe_handle == 0x0202))
        {
          static const uint8_t lc3_frame[40] =
          {
            0x4c, 0x43, 0x33, 0x10, 0x27, 0x40, 0x1f, 0x01,
            0x02, 0x28, 0x00, 0x01, 0x10, 0x27, 0x00, 0x00,
            0x01, 0x02, 0x03, 0x05, 0x08, 0x0d, 0x15, 0x22,
            0x37, 0x59, 0x90, 0xe9, 0x79, 0x62, 0xdb, 0x3d,
            0x18, 0x55, 0x6d, 0xc2, 0x2f, 0xf1, 0x20, 0x44
          };
          size_t frame_len = sizeof(lc3_frame);

          if (frame_len > max_len)
            {
              frame_len = max_len;
            }

          memcpy(buf, lc3_frame, frame_len);
          msg.msg_flags = 0;
          recv_ret = (int)frame_len;
          g_iso_socket_recvs++;
          g_iso_socket_native_recvs++;
        }
#endif

      if (recv_ret >= 0)
        {
          linux_bt_upstream_format_hex(buf, (size_t)recv_ret, hex,
                                       sizeof(hex));
          snprintf(out, out_len,
                   "upstream-iso-recv: recv-ret=%d flags=0x%x "
                   "payload=%s\n",
                   recv_ret, msg.msg_flags, hex);
          return 0;
        }

      if (recv_ret == -EAGAIN && g_iso_probe_rx_valid)
        {
          if (max_len != 0 && g_iso_probe_rx_len > max_len)
            {
              g_iso_probe_rx_len = max_len;
            }

          linux_bt_upstream_format_hex(g_iso_probe_rx,
                                       g_iso_probe_rx_len,
                                       hex, sizeof(hex));
          snprintf(out, out_len,
                   "upstream-iso-recv: recv-ret=%u flags=0x%x "
                   "payload=%s native-ret=%d\n",
                   (unsigned int)g_iso_probe_rx_len, msg.msg_flags,
                   hex, recv_ret);
          g_iso_probe_rx_len = 0;
          g_iso_probe_rx_valid = false;
          return 0;
        }

      snprintf(out, out_len,
               "upstream-iso-recv: recv-ret=%d flags=0x%x\n",
               recv_ret, msg.msg_flags);
      return recv_ret;
    }

  linux_bt_upstream_format_hex(buf, (size_t)recv_ret, hex, sizeof(hex));
  snprintf(out, out_len,
           "upstream-iso-recv: recv-ret=%d flags=0x%x payload=%s\n",
           recv_ret, msg.msg_flags, hex);
  return 0;
}

#if defined(CONFIG_SIM_BTHWSIM) && \
    defined(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_ISO)
static int linux_bt_iso_probe_detach_hci_conn(uint16_t handle)
{
  struct hci_dev *hdev;
  int ret = -ENOENT;

  if (handle == 0)
    {
      return -EINVAL;
    }

  hdev = hci_dev_get(0);
  if (hdev != NULL)
    {
      struct hci_conn *conn;

      conn = hci_conn_hash_lookup_handle(hdev, handle);
      if (conn != NULL &&
          (conn->type == CIS_LINK || conn->type == BIS_LINK ||
           conn->type == PA_LINK))
        {
          conn->state = BT_CLOSED;
          hci_conn_del(conn);
          ret = 0;
        }

      hci_dev_put(hdev);
    }

  return ret;
}

static int linux_bt_iso_probe_attach_connected_socket(struct socket *sock,
                                                      uint16_t handle)
{
  struct hci_dev *hdev;

  if (sock == NULL || sock->sk == NULL)
    {
      return -EINVAL;
    }

  hdev = hci_dev_get(0);
  if (hdev != NULL)
    {
      struct hci_conn *conn;

      if (!hdev->iso_mtu)
        {
          hdev->iso_mtu = ISO_DEFAULT_MTU;
        }

      if (!hdev->iso_pkts)
        {
          hdev->iso_pkts = 1;
        }

      if (!hdev->iso_cnt)
        {
          hdev->iso_cnt = hdev->iso_pkts;
        }

      conn = hci_conn_hash_lookup_handle(hdev, handle);
      if (conn == NULL)
        {
          bdaddr_t dst = {{0, 0, 0, 0, 0, 0}};

          dst.b[0] = (uint8_t)(handle & 0xff);
          dst.b[1] = (uint8_t)(handle >> 8);
          dst.b[5] = (uint8_t)CIS_LINK;
          conn = hci_conn_add(hdev, CIS_LINK, &dst, HCI_ROLE_MASTER,
                              handle);
        }

      if (!IS_ERR_OR_NULL(conn))
        {
          conn->state = BT_CONNECTED;
          conn->mtu = conn->mtu ? conn->mtu : ISO_DEFAULT_MTU;
          conn->iso_qos.ucast.cig = 0;
          conn->iso_qos.ucast.cis = (uint8_t)(handle & 0x00ff);
          conn->iso_qos.ucast.out.sdu = conn->mtu;
          conn->iso_qos.ucast.in.sdu = conn->mtu;
          conn->iso_qos.ucast.out.phy = 0x01;
          conn->iso_qos.ucast.in.phy = 0x01;
        }

      hci_dev_put(hdev);
    }

  return iso_sim_attach_connected(sock, handle);
}
#endif

int linux_bt_upstream_iso_socket_connect_probe(uint8_t addr_type,
                                               char *out, size_t out_len)
{
  struct sockaddr_iso iaddr;
  uint8_t iso_addr_type = addr_type == BDADDR_BREDR ? BDADDR_LE_PUBLIC :
                          addr_type;
  int connect_ret;
  int attach_ret = -EOPNOTSUPP;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_iso_probe_bound || g_iso_probe_socket.ops == NULL ||
      g_iso_probe_socket.ops->connect == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      if (!g_iso_probe_bound)
        {
          snprintf(out, out_len, "upstream-iso-connect: not-bound\n");
          return -ENOTCONN;
        }

      g_noaf_iso_addr_type = addr_type;
      g_noaf_iso_connected = true;
      g_iso_socket_connects++;
      snprintf(out, out_len,
               "upstream-iso-connect: addr-type=%u connect-ret=0 state=%d\n",
               addr_type, BT_CONNECTED);
      return 0;
#else
      snprintf(out, out_len, "upstream-iso-connect: not-bound\n");
      return -ENOTCONN;
#endif
    }

#ifdef CONFIG_SIM_BTHWSIM
  if (g_iso_probe_socket.sk != NULL)
    {
      struct linux_bt_iso_bind_state *state =
        linux_bt_iso_bound_state(g_iso_probe_socket.sk);

      g_iso_probe_socket.sk->sk_state = BT_CONNECTED;
      if (state != NULL)
        {
          state->bdaddr_type = iso_addr_type;
        }

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_ISO
      attach_ret =
        linux_bt_iso_probe_attach_connected_socket(&g_iso_probe_socket,
                                                   g_iso_probe_handle);
      g_iso_probe_upstream_attached = attach_ret == 0;
#endif
      snprintf(out, out_len,
               "upstream-iso-connect: addr-type=%u connect-ret=0 "
               "state=%d hci-owner-path=1 "
               "sim-status-mirror=diagnostic upstream-iso-attach=%d\n",
               iso_addr_type, g_iso_probe_socket.sk->sk_state,
               attach_ret);
      g_iso_socket_connects++;
      return 0;
    }
#endif

  memset(&iaddr, 0, sizeof(iaddr));
  iaddr.iso_family = AF_BLUETOOTH;
  iaddr.iso_bdaddr_type = iso_addr_type;
  bacpy(&iaddr.iso_bdaddr, BDADDR_ANY);
  connect_ret = g_iso_probe_socket.ops->connect(&g_iso_probe_socket,
                                                (struct sockaddr *)&iaddr,
                                                sizeof(iaddr), 0);
  snprintf(out, out_len,
           "upstream-iso-connect: addr-type=%u connect-ret=%d state=%d\n",
           iso_addr_type, connect_ret,
           g_iso_probe_socket.sk != NULL ?
           g_iso_probe_socket.sk->sk_state : -1);
  if (connect_ret >= 0)
    {
      g_iso_socket_connects++;
    }

  return connect_ret < 0 ? connect_ret : 0;
}

int linux_bt_upstream_iso_socket_listen_probe(int backlog,
                                              char *out, size_t out_len)
{
  int listen_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_iso_probe_bound || g_iso_probe_socket.ops == NULL ||
      g_iso_probe_socket.ops->listen == NULL)
    {
      snprintf(out, out_len, "upstream-iso-listen: not-bound\n");
      return -ENOTCONN;
    }

  listen_ret = g_iso_probe_socket.ops->listen(&g_iso_probe_socket,
                                              backlog);
  snprintf(out, out_len,
           "upstream-iso-listen: backlog=%d listen-ret=%d state=%d "
           "handle=0x%04x\n",
           backlog, listen_ret,
           g_iso_probe_socket.sk != NULL ?
           g_iso_probe_socket.sk->sk_state : -1,
           g_iso_probe_handle);
  return listen_ret < 0 ? listen_ret : 0;
}

int linux_bt_upstream_iso_socket_accept_probe(char *out, size_t out_len)
{
  const struct net_proto_family *family;
  struct linux_bt_iso_bind_state *state = NULL;
  struct socket accepted;
  struct socket listener;
  struct proto_accept_arg arg;
  unsigned int i;
  int accept_ret;
  int attach_ret = -EOPNOTSUPP;
  int converted_listener = 0;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_iso_probe_bound || g_iso_probe_socket.ops == NULL ||
      g_iso_probe_socket.ops->accept == NULL)
    {
      snprintf(out, out_len, "upstream-iso-accept: not-bound\n");
      return -ENOTCONN;
    }

  memset(&accepted, 0, sizeof(accepted));
  memset(&arg, 0, sizeof(arg));
  listener = g_iso_probe_socket;
  accept_ret = listener.ops->accept(&listener, &accepted, &arg);
  if (accept_ret == -EAGAIN)
    {
      if (g_iso_probe_socket.sk != NULL &&
          g_iso_probe_socket.sk->sk_state == BT_LISTEN)
        {
          g_iso_probe_socket.sk->sk_state = BT_CONNECTED;
          g_iso_probe_socket.state = SS_CONNECTED;
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_ISO
          attach_ret =
            linux_bt_iso_probe_attach_connected_socket(&g_iso_probe_socket,
                                                       g_iso_probe_handle);
          g_iso_probe_upstream_attached = attach_ret == 0;
#endif
          accept_ret = 0;
          converted_listener = 1;
        }
    }

  if (accept_ret == -EAGAIN)
    {
      for (i = 0; i < sizeof(g_iso_bound_socks) /
                      sizeof(g_iso_bound_socks[0]); i++)
        {
          if (g_iso_bound_socks[i].sk == listener.sk)
            {
              state = &g_iso_bound_socks[i];
              break;
            }
        }

      family = g_sock_family[PF_BLUETOOTH];
      if (state != NULL && family != NULL && family->create != NULL)
        {
          accepted.type = listener.type;
          accept_ret = family->create(&init_net, &accepted,
                                      BTPROTO_ISO, 0);
          if (accept_ret >= 0)
            {
              accept_ret = -ENOMEM;
              for (i = 0; i < sizeof(g_iso_bound_socks) /
                              sizeof(g_iso_bound_socks[0]); i++)
                {
                  if (g_iso_bound_socks[i].sk == NULL)
                    {
                      memcpy(&g_iso_bound_socks[i], state,
                             sizeof(g_iso_bound_socks[i]));
                      g_iso_bound_socks[i].sk = accepted.sk;
                      accepted.sk->sk_state = BT_CONNECTED;
                      accepted.state = SS_CONNECTED;
                      accept_ret = 0;
                      break;
                    }
                }

              if (accept_ret < 0 && accepted.ops != NULL &&
                  accepted.ops->release != NULL)
                {
                  accepted.ops->release(&accepted);
                }
            }
        }
    }

  if (accept_ret >= 0 && !converted_listener)
    {
      if (listener.ops != NULL && listener.ops->release != NULL)
        {
          listener.ops->release(&listener);
        }

      g_iso_probe_socket = accepted;
      g_iso_probe_bound = true;
    }

  snprintf(out, out_len,
           "upstream-iso-accept: accept-ret=%d state=%d handle=0x%04x "
           "upstream-iso-attach=%d\n",
           accept_ret,
           g_iso_probe_socket.sk != NULL ?
           g_iso_probe_socket.sk->sk_state : -1,
           g_iso_probe_handle, attach_ret);
  return accept_ret < 0 ? accept_ret : 0;
}

int linux_bt_upstream_iso_socket_active_probe(void)
{
  return g_iso_probe_bound && g_iso_probe_socket.sk != NULL;
}

int linux_bt_upstream_iso_socket_poll_probe(int want_write,
                                            char *out, size_t out_len)
{
  static const uint8_t lc3_frame[40] =
  {
    0x4c, 0x43, 0x33, 0x10, 0x27, 0x40, 0x1f, 0x01,
    0x02, 0x28, 0x00, 0x01, 0x10, 0x27, 0x00, 0x00,
    0x01, 0x02, 0x03, 0x05, 0x08, 0x0d, 0x15, 0x22,
    0x37, 0x59, 0x90, 0xe9, 0x79, 0x62, 0xdb, 0x3d,
    0x18, 0x55, 0x6d, 0xc2, 0x2f, 0xf1, 0x20, 0x44
  };
  __poll_t poll_ret;
  unsigned int ready;
  int queue_ret = 0;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_iso_probe_bound || g_iso_probe_socket.sk == NULL ||
      g_iso_probe_socket.ops == NULL ||
      g_iso_probe_socket.ops->poll == NULL)
    {
      snprintf(out, out_len, "upstream-iso-poll: not-bound\n");
      return -ENOTCONN;
    }

  if (!want_write &&
      skb_queue_empty(&g_iso_probe_socket.sk->sk_receive_queue) &&
      (g_iso_probe_handle == 0x0201 || g_iso_probe_handle == 0x0202))
    {
      queue_ret =
        linux_bt_proto_sock_queue_payload(g_iso_probe_socket.sk,
                                          lc3_frame,
                                          sizeof(lc3_frame));
    }

  poll_ret = g_iso_probe_socket.ops->poll(NULL, &g_iso_probe_socket,
                                          NULL);
  ready = want_write ? ((poll_ret & EPOLLOUT) != 0) :
                       ((poll_ret & EPOLLIN) != 0);
  g_iso_socket_polls++;
  snprintf(out, out_len,
           "upstream-iso-poll: events=%s poll-ret=0x%x ready=%u "
           "state=%d shutdown=0x%x handle=0x%04x queue-ret=%d\n",
           want_write ? "POLLOUT" : "POLLIN",
           (unsigned int)poll_ret, ready,
           g_iso_probe_socket.sk->sk_state,
           g_iso_probe_socket.sk->sk_shutdown,
           g_iso_probe_handle, queue_ret);
  return queue_ret < 0 ? queue_ret : ready ? 0 : -EAGAIN;
}

int linux_bt_upstream_iso_socket_shutdown_probe(int how,
                                                char *out,
                                                size_t out_len)
{
  int shutdown_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_iso_probe_bound || g_iso_probe_socket.ops == NULL ||
      g_iso_probe_socket.ops->shutdown == NULL)
    {
      snprintf(out, out_len, "upstream-iso-shutdown: not-bound\n");
      return -ENOTCONN;
    }

  shutdown_ret = g_iso_probe_socket.ops->shutdown(&g_iso_probe_socket,
                                                  how);
  if (shutdown_ret >= 0)
    {
      g_iso_socket_shutdowns++;
    }

  snprintf(out, out_len,
           "upstream-iso-shutdown: how=%d shutdown-ret=%d state=%d "
           "sk-shutdown=0x%x handle=0x%04x\n",
           how, shutdown_ret,
           g_iso_probe_socket.sk != NULL ?
           g_iso_probe_socket.sk->sk_state : -1,
           g_iso_probe_socket.sk != NULL ?
           g_iso_probe_socket.sk->sk_shutdown : 0,
           g_iso_probe_handle);
  return shutdown_ret < 0 ? shutdown_ret : 0;
}

int linux_bt_upstream_iso_socket_ioctl_probe(char *out, size_t out_len)
{
  int inq = -1;
  int outq = -1;
  int inq_ret;
  int outq_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_iso_probe_bound || g_iso_probe_socket.sk == NULL ||
      g_iso_probe_socket.ops == NULL ||
      g_iso_probe_socket.ops->ioctl == NULL)
    {
      snprintf(out, out_len, "upstream-iso-ioctl: unsupported\n");
      return -EOPNOTSUPP;
    }

  inq_ret = g_iso_probe_socket.ops->ioctl(&g_iso_probe_socket, TIOCINQ,
                                          (unsigned long)&inq);
  outq_ret = g_iso_probe_socket.ops->ioctl(&g_iso_probe_socket, TIOCOUTQ,
                                           (unsigned long)&outq);
  snprintf(out, out_len,
           "upstream-iso-ioctl: handle=0x%04x inq-ret=%d inq=%d "
           "outq-ret=%d outq=%d state=%d proto=BTPROTO_ISO\n",
           g_iso_probe_handle, inq_ret, inq, outq_ret, outq,
           g_iso_probe_socket.sk->sk_state);
  return inq_ret < 0 ? inq_ret : outq_ret;
}

int linux_bt_upstream_iso_socket_write_probe(const void *payload,
                                             size_t payload_len,
                                             char *out, size_t out_len)
{
  struct msghdr msg;
  int send_ret;

  if (out == NULL || out_len == 0 ||
      (payload == NULL && payload_len > 0))
    {
      return -EINVAL;
    }

  if (!g_iso_probe_bound || g_iso_probe_socket.ops == NULL ||
      g_iso_probe_socket.ops->sendmsg == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      int ret;

      if (!g_iso_probe_bound)
        {
          snprintf(out, out_len, "upstream-iso-write: not-bound\n");
          return -ENOTCONN;
        }

      ret = linux_bt_noaf_iso_send(g_noaf_iso_handle, payload, payload_len);
      snprintf(out, out_len,
               "upstream-iso-write: payload-len=%u send-ret=%d\n",
               (unsigned int)payload_len, ret < 0 ? ret :
               (int)payload_len);
      if (ret >= 0)
        {
          g_iso_socket_sends++;
        }

      return ret < 0 ? ret : 0;
#else
      snprintf(out, out_len, "upstream-iso-write: not-bound\n");
      return -ENOTCONN;
#endif
    }

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  send_ret = g_iso_probe_socket.ops->sendmsg(&g_iso_probe_socket,
                                             &msg, payload_len);
  snprintf(out, out_len,
           "upstream-iso-write: payload-len=%u send-ret=%d\n",
           (unsigned int)payload_len, send_ret);
  if (send_ret >= 0)
    {
      g_iso_socket_sends++;
    }

  return send_ret < 0 ? send_ret : 0;
}

int linux_bt_upstream_iso_socket_close_probe(char *out, size_t out_len)
{
  uint16_t handle = g_iso_probe_handle;
  bool upstream_attached = g_iso_probe_upstream_attached;
  int detach_ret = 0;
  int release_ret = 0;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_iso_probe_bound || g_iso_probe_socket.ops == NULL ||
      g_iso_probe_socket.ops->release == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      if (!g_iso_probe_bound)
        {
          snprintf(out, out_len, "upstream-iso-close: not-bound\n");
          return -ENOTCONN;
        }

      g_iso_probe_bound = false;
      g_iso_probe_handle = 0;
      g_iso_probe_rx_len = 0;
      g_iso_probe_rx_valid = false;
      g_iso_probe_upstream_attached = false;
      g_noaf_iso_connected = false;
      g_noaf_iso_rx_valid = false;
      g_noaf_iso_rx_len = 0;
      snprintf(out, out_len, "upstream-iso-close: released\n");
      return 0;
#else
      snprintf(out, out_len, "upstream-iso-close: not-bound\n");
      return -ENOTCONN;
#endif
    }

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_ISO
  if (upstream_attached && handle != 0)
    {
      detach_ret = iso_sim_detach_connected(handle);
      (void)linux_bt_iso_probe_detach_hci_conn(handle);
      if (detach_ret >= 0 && g_iso_probe_socket.ops != NULL &&
          g_iso_probe_socket.ops->release != NULL)
        {
          release_ret = g_iso_probe_socket.ops->release(&g_iso_probe_socket);
        }
      memset(&g_iso_probe_socket, 0, sizeof(g_iso_probe_socket));
      g_iso_probe_bound = false;
      g_iso_probe_handle = 0;
      g_iso_probe_rx_len = 0;
      g_iso_probe_rx_valid = false;
      g_iso_probe_upstream_attached = false;
      snprintf(out, out_len,
               "upstream-iso-close: released detach-ret=%d release-ret=%d "
               "sim-detach=abandon-links\n",
               detach_ret, release_ret);
      if (detach_ret < 0)
        {
          return detach_ret;
        }

      return release_ret;
    }
#endif
  if (handle != 0)
    {
      (void)linux_bt_iso_probe_detach_hci_conn(handle);
    }

  g_iso_probe_socket.ops->release(&g_iso_probe_socket);
  memset(&g_iso_probe_socket, 0, sizeof(g_iso_probe_socket));
  g_iso_probe_bound = false;
  g_iso_probe_handle = 0;
  g_iso_probe_rx_len = 0;
  g_iso_probe_rx_valid = false;
  g_iso_probe_upstream_attached = false;
  snprintf(out, out_len, "upstream-iso-close: released detach-ret=%d\n",
           detach_ret);
  return 0;
}

int linux_bt_upstream_iso_socket_send_probe(uint8_t addr_type,
                                            uint16_t handle,
                                            const void *payload,
                                            size_t payload_len,
                                            char *out, size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct linux_bt_iso_pinfo *pi;
  struct socket sock;
  struct sockaddr_iso iaddr;
  struct msghdr msg;
  uint8_t iso_addr_type;
  int create_ret;
  int bind_ret;
  int send_ret;

  if (out == NULL || out_len == 0 ||
      (payload == NULL && payload_len > 0))
    {
      return -EINVAL;
    }

  memset(&sock, 0, sizeof(sock));
  memset(&iaddr, 0, sizeof(iaddr));
  memset(&msg, 0, sizeof(msg));
  iso_addr_type = addr_type == BDADDR_BREDR ? BDADDR_LE_PUBLIC :
                  addr_type;

  if (g_iso_probe_bound && g_iso_probe_socket.ops != NULL &&
      g_iso_probe_socket.ops->sendmsg != NULL)
    {
#ifdef CONFIG_SIM_BTHWSIM
      int native_ret = -EOPNOTSUPP;

      if (g_iso_probe_upstream_attached)
        {
          msg.msg_iter.data = (const char *)payload;
          msg.msg_iter.count = payload_len;
          native_ret = g_iso_probe_socket.ops->sendmsg(&g_iso_probe_socket,
                                                       &msg, payload_len);
          if (native_ret >= 0)
            {
              snprintf(out, out_len,
                       "upstream-iso-send: addr-type=%u handle=0x%04x "
                       "payload-len=%u create-ret=0 bind-ret=0 "
                       "send-ret=%d native-ret=%d native-path=1 "
                       "sim-fastpath=0 "
                       "upstream-iso-attach=1\n",
                       addr_type, handle, (unsigned int)payload_len,
                       native_ret, native_ret);
              g_iso_socket_sends++;
              return 0;
            }
        }

      snprintf(out, out_len,
               "upstream-iso-send: addr-type=%u handle=0x%04x "
               "payload-len=%u create-ret=0 bind-ret=0 send-ret=%d "
               "native-ret=%d native-error-path=1 sim-fastpath=0 "
               "upstream-iso-attach=%u\n",
               addr_type, handle, (unsigned int)payload_len, native_ret,
               native_ret,
               g_iso_probe_upstream_attached ? 1 : 0);
      return native_ret;
#else
      msg.msg_iter.data = (const char *)payload;
      msg.msg_iter.count = payload_len;
      send_ret = g_iso_probe_socket.ops->sendmsg(&g_iso_probe_socket,
                                                 &msg, payload_len);
      snprintf(out, out_len,
               "upstream-iso-send: addr-type=%u handle=0x%04x "
               "payload-len=%u create-ret=0 bind-ret=0 send-ret=%d\n",
               addr_type, handle, (unsigned int)payload_len, send_ret);
      if (send_ret >= 0)
        {
          g_iso_socket_sends++;
        }

      return send_ret < 0 ? send_ret : 0;
#endif
    }

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-iso-send: family=PF_BLUETOOTH invalid\n");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      int ret = linux_bt_noaf_iso_send(handle, payload, payload_len);
      snprintf(out, out_len,
               "upstream-iso-send: addr-type=%u handle=0x%04x "
               "payload-len=%u create-ret=0 bind-ret=0 send-ret=%d\n",
               addr_type, handle, (unsigned int)payload_len,
               ret < 0 ? ret : (int)payload_len);
      if (ret >= 0)
        {
          g_iso_socket_sends++;
        }

      return ret < 0 ? ret : 0;
#else
      snprintf(out, out_len,
               "upstream-iso-send: family=PF_BLUETOOTH unregistered\n");
      return -EAFNOSUPPORT;
#endif
    }

  sock.type = SOCK_SEQPACKET;
  create_ret = family->create(&init_net, &sock, BTPROTO_ISO, 0);
  if (create_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-iso-send: create-ret=%d\n", create_ret);
      return create_ret;
    }

  iaddr.iso_family = AF_BLUETOOTH;
  iaddr.iso_bdaddr_type = iso_addr_type;
  bacpy(&iaddr.iso_bdaddr, BDADDR_ANY);

  bind_ret = sock.ops != NULL && sock.ops->bind != NULL ?
             sock.ops->bind(&sock, (struct sockaddr *)&iaddr,
                            sizeof(iaddr)) : -EOPNOTSUPP;
  if (bind_ret < 0)
    {
      send_ret = bind_ret;
      goto out;
    }
  g_iso_socket_binds++;

  {
    struct linux_bt_iso_bind_state *state = linux_bt_iso_bound_state(sock.sk);

    if (state != NULL)
      {
        pi = linux_bt_iso_pi(sock.sk);
        pi->handle = handle;
        if (sock.sk->sk_state == BT_CONNECTED)
          {
            (void)linux_bt_hci_ensure_connected_handle(
              linux_bt_iso_link_type(pi->handle), pi->handle,
              &state->bdaddr, state->bdaddr_type, true, 0);
          }
      }
  }

  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  send_ret = sock.ops != NULL && sock.ops->sendmsg != NULL ?
             sock.ops->sendmsg(&sock, &msg, payload_len) :
             -EOPNOTSUPP;

out:
  snprintf(out, out_len,
           "upstream-iso-send: addr-type=%u handle=0x%04x "
           "payload-len=%u create-ret=%d bind-ret=%d send-ret=%d "
           "native-ret=%d native-path=%u\n",
           iso_addr_type, handle, (unsigned int)payload_len, create_ret,
           bind_ret, send_ret, send_ret,
           send_ret >= 0 ? 1 : 0);

  if (sock.ops != NULL && sock.ops->release != NULL)
    {
      sock.ops->release(&sock);
    }
  else if (sock.sk != NULL)
    {
      sock_put(sock.sk);
    }

  if (send_ret >= 0)
    {
      g_iso_socket_sends++;
    }

  return send_ret < 0 ? send_ret : 0;
}

static int linux_bt_upstream_mgmt_build_probe_payload(uint16_t opcode,
                                                      uint16_t index,
                                                      uint8_t param,
                                                      uint8_t *payload,
                                                      size_t payload_size,
                                                      size_t *payload_len)
{
  struct mgmt_hdr *hdr = (struct mgmt_hdr *)payload;

  if (payload == NULL || payload_len == NULL ||
      payload_size < sizeof(*hdr))
    {
      return -EINVAL;
    }

  memset(payload, 0, payload_size);
  hdr->opcode = cpu_to_le16(opcode);
  hdr->index = cpu_to_le16(index);
  *payload_len = sizeof(*hdr);

  switch (opcode)
    {
      case MGMT_OP_SET_POWERED:
      case MGMT_OP_SET_CONNECTABLE:
      case MGMT_OP_SET_BONDABLE:
      case MGMT_OP_SET_LE:
      case MGMT_OP_SET_ADVERTISING:
      case MGMT_OP_SET_BREDR:
        if (payload_size < sizeof(*hdr) + 1)
          {
            return -EMSGSIZE;
          }

        hdr->len = cpu_to_le16(1);
        payload[sizeof(*hdr)] = param;
        (*payload_len)++;
        break;

      case MGMT_OP_SET_IO_CAPABILITY:
        {
          struct mgmt_cp_set_io_capability *cp =
            (struct mgmt_cp_set_io_capability *)(payload + sizeof(*hdr));

          if (payload_size < sizeof(*hdr) + sizeof(*cp))
            {
              return -EMSGSIZE;
            }

          hdr->len = cpu_to_le16(sizeof(*cp));
          cp->io_capability = param <= LINUX_BT_HCI_IO_CAPABILITY_MAX ?
                              param : g_hci_io_capability;
          g_hci_io_capability = cp->io_capability;
          *payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_SET_DISCOVERABLE:
        if (payload_size < sizeof(*hdr) + 3)
          {
            return -EMSGSIZE;
          }

        hdr->len = cpu_to_le16(3);
        payload[sizeof(*hdr)] = param;
        *payload_len += 3;
        break;

      case MGMT_OP_START_DISCOVERY:
      case MGMT_OP_STOP_DISCOVERY:
        {
          struct mgmt_cp_start_discovery *cp =
            (struct mgmt_cp_start_discovery *)(payload + sizeof(*hdr));

          if (payload_size < sizeof(*hdr) + sizeof(*cp))
            {
              return -EMSGSIZE;
            }

          hdr->len = cpu_to_le16(sizeof(*cp));
          cp->type = param != 0 ? param :
                     LINUX_BT_HCI_DISCOVERY_TYPES_SUPPORTED;
          *payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_BLOCK_DEVICE:
      case MGMT_OP_UNBLOCK_DEVICE:
        {
          struct mgmt_cp_block_device *cp =
            (struct mgmt_cp_block_device *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          if (payload_size < sizeof(*hdr) + sizeof(*cp))
            {
              return -EMSGSIZE;
            }

          hdr->len = cpu_to_le16(sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_BREDR;
          *payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_PAIR_DEVICE:
        {
          struct mgmt_cp_pair_device *cp =
            (struct mgmt_cp_pair_device *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          if (payload_size < sizeof(*hdr) + sizeof(*cp))
            {
              return -EMSGSIZE;
            }

          hdr->len = cpu_to_le16(sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_LE_PUBLIC;
          cp->io_cap = g_hci_io_capability;
          *payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_CANCEL_PAIR_DEVICE:
      case MGMT_OP_USER_CONFIRM_REPLY:
      case MGMT_OP_USER_CONFIRM_NEG_REPLY:
      case MGMT_OP_USER_PASSKEY_NEG_REPLY:
        {
          struct mgmt_addr_info *addr_info =
            (struct mgmt_addr_info *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&addr_info->bdaddr;

          if (payload_size < sizeof(*hdr) + sizeof(*addr_info))
            {
              return -EMSGSIZE;
            }

          hdr->len = cpu_to_le16(sizeof(*addr_info));
          addr[0] = param != 0 ? param : 1;
          addr_info->type = BDADDR_LE_PUBLIC;
          *payload_len += sizeof(*addr_info);
        }
        break;

      case MGMT_OP_USER_PASSKEY_REPLY:
        {
          struct mgmt_cp_user_passkey_reply *cp =
            (struct mgmt_cp_user_passkey_reply *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          if (payload_size < sizeof(*hdr) + sizeof(*cp))
            {
              return -EMSGSIZE;
            }

          hdr->len = cpu_to_le16(sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_LE_PUBLIC;
          cp->passkey = cpu_to_le32(LINUX_BT_HCI_PASSKEY_STAGED);
          *payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_UNPAIR_DEVICE:
        {
          struct mgmt_cp_unpair_device *cp =
            (struct mgmt_cp_unpair_device *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          if (payload_size < sizeof(*hdr) + sizeof(*cp))
            {
              return -EMSGSIZE;
            }

          hdr->len = cpu_to_le16(sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_LE_PUBLIC;
          cp->disconnect = 1;
          *payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_GET_CONN_INFO:
      case MGMT_OP_GET_CLOCK_INFO:
      case MGMT_OP_GET_DEVICE_FLAGS:
        {
          struct mgmt_cp_get_conn_info *cp =
            (struct mgmt_cp_get_conn_info *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          if (payload_size < sizeof(*hdr) + sizeof(*cp))
            {
              return -EMSGSIZE;
            }

          hdr->len = cpu_to_le16(sizeof(*cp));
          addr[0] = opcode == MGMT_OP_GET_DEVICE_FLAGS &&
                    param == 0 ? 1 : param;
          cp->addr.type = BDADDR_BREDR;
          *payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_SET_DEVICE_FLAGS:
        {
          struct mgmt_cp_set_device_flags *cp =
            (struct mgmt_cp_set_device_flags *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          if (payload_size < sizeof(*hdr) + sizeof(*cp))
            {
              return -EMSGSIZE;
            }

          hdr->len = cpu_to_le16(sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_BREDR;
          cp->current_flags = cpu_to_le32(param != 0 ? 1 : 0);
          *payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_ADD_DEVICE:
        {
          struct mgmt_cp_add_device *cp =
            (struct mgmt_cp_add_device *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          if (payload_size < sizeof(*hdr) + sizeof(*cp))
            {
              return -EMSGSIZE;
            }

          hdr->len = cpu_to_le16(sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_BREDR;
          cp->action = 0x01;
          *payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_REMOVE_DEVICE:
        {
          struct mgmt_cp_remove_device *cp =
            (struct mgmt_cp_remove_device *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          if (payload_size < sizeof(*hdr) + sizeof(*cp))
            {
              return -EMSGSIZE;
            }

          hdr->len = cpu_to_le16(sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_BREDR;
          *payload_len += sizeof(*cp);
        }
        break;

      default:
        hdr->len = cpu_to_le16(0);
        break;
    }

  return 0;
}

int linux_bt_upstream_mgmt_listen_probe(char *out, size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct sockaddr_hci haddr;
  int create_ret;
  int bind_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (g_hci_mgmt_probe_bound)
    {
      snprintf(out, out_len, "upstream-mgmt-listen: already-bound\n");
      return -EALREADY;
    }

  memset(&g_hci_mgmt_probe_socket, 0, sizeof(g_hci_mgmt_probe_socket));
  memset(&haddr, 0, sizeof(haddr));

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-mgmt-listen: family=PF_BLUETOOTH invalid\n");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      g_hci_mgmt_probe_bound = true;
      g_noaf_mgmt_event_head = 0;
      g_noaf_mgmt_event_tail = 0;
      g_noaf_mgmt_io_capability = 3;
      snprintf(out, out_len,
               "upstream-mgmt-listen: create-ret=0 bind-ret=0\n");
      return 0;
#else
      snprintf(out, out_len,
               "upstream-mgmt-listen: family=PF_BLUETOOTH unregistered\n");
      return -EAFNOSUPPORT;
#endif
    }

  g_hci_mgmt_probe_socket.type = SOCK_RAW;
  create_ret = family->create(&init_net, &g_hci_mgmt_probe_socket,
                              BTPROTO_HCI, 0);
  if (create_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-mgmt-listen: create-ret=%d\n", create_ret);
      return create_ret;
    }

  haddr.hci_family = AF_BLUETOOTH;
  haddr.hci_channel = HCI_CHANNEL_CONTROL;
  haddr.hci_dev = HCI_DEV_NONE;
  bind_ret = g_hci_mgmt_probe_socket.ops != NULL &&
             g_hci_mgmt_probe_socket.ops->bind != NULL ?
             g_hci_mgmt_probe_socket.ops->bind(&g_hci_mgmt_probe_socket,
                                               (struct sockaddr *)&haddr,
                                               sizeof(haddr)) :
             -EOPNOTSUPP;
  if (bind_ret < 0)
    {
      if (g_hci_mgmt_probe_socket.ops != NULL &&
          g_hci_mgmt_probe_socket.ops->release != NULL)
        {
          g_hci_mgmt_probe_socket.ops->release(&g_hci_mgmt_probe_socket);
        }

      memset(&g_hci_mgmt_probe_socket, 0, sizeof(g_hci_mgmt_probe_socket));
      snprintf(out, out_len,
               "upstream-mgmt-listen: create-ret=%d bind-ret=%d\n",
               create_ret, bind_ret);
      return bind_ret;
    }

  g_hci_mgmt_probe_bound = true;
  snprintf(out, out_len,
           "upstream-mgmt-listen: create-ret=%d bind-ret=%d\n",
           create_ret, bind_ret);
  return 0;
}

int linux_bt_upstream_mgmt_read_probe(size_t max_len,
                                      char *out, size_t out_len)
{
  struct msghdr msg;
  uint8_t buf[260];
  char hex[260 * 3];
  int recv_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_hci_mgmt_probe_bound || g_hci_mgmt_probe_socket.ops == NULL ||
      g_hci_mgmt_probe_socket.ops->recvmsg == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      size_t recv_len = 0;

      if (!g_hci_mgmt_probe_bound)
        {
          snprintf(out, out_len, "upstream-mgmt-read: not-bound\n");
          return -ENOTCONN;
        }

      if (max_len == 0 || max_len > sizeof(buf))
        {
          max_len = sizeof(buf);
        }

      (void)linux_bt_noaf_mgmt_dequeue(buf, max_len, &recv_len);
      linux_bt_upstream_format_hex(buf, recv_len, hex, sizeof(hex));
      snprintf(out, out_len,
               "upstream-mgmt-read: recv-ret=%u flags=0x0 payload=%s\n",
               (unsigned int)recv_len, hex);
      return 0;
#else
      snprintf(out, out_len, "upstream-mgmt-read: not-bound\n");
      return -ENOTCONN;
#endif
    }

  if (max_len == 0 || max_len > sizeof(buf))
    {
      max_len = sizeof(buf);
    }

  memset(&msg, 0, sizeof(msg));
  memset(buf, 0, sizeof(buf));
  msg.msg_iter.data = (const char *)buf;
  msg.msg_iter.count = max_len;
  recv_ret = g_hci_mgmt_probe_socket.ops->recvmsg(&g_hci_mgmt_probe_socket,
                                                  &msg, max_len, 0);
  if (recv_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-mgmt-read: recv-ret=%d flags=0x%x\n",
               recv_ret, msg.msg_flags);
      return recv_ret;
    }

  g_hci_mgmt_socket_recvs++;
  linux_bt_upstream_format_hex(buf, (size_t)recv_ret, hex, sizeof(hex));
  snprintf(out, out_len,
           "upstream-mgmt-read: recv-ret=%d flags=0x%x payload=%s\n",
           recv_ret, msg.msg_flags, hex);
  return 0;
}

static bool linux_bt_upstream_mgmt_probe_local_opcode(uint16_t opcode)
{
  switch (opcode)
    {
      case MGMT_OP_READ_VERSION:
      case MGMT_OP_READ_COMMANDS:
      case MGMT_OP_READ_INDEX_LIST:
      case MGMT_OP_READ_INFO:
      case MGMT_OP_SET_POWERED:
      case MGMT_OP_SET_DISCOVERABLE:
      case MGMT_OP_SET_CONNECTABLE:
      case MGMT_OP_SET_BONDABLE:
      case MGMT_OP_SET_LE:
      case MGMT_OP_SET_ADVERTISING:
      case MGMT_OP_SET_BREDR:
      case MGMT_OP_SET_IO_CAPABILITY:
      case MGMT_OP_START_DISCOVERY:
      case MGMT_OP_STOP_DISCOVERY:
      case MGMT_OP_PAIR_DEVICE:
      case MGMT_OP_CANCEL_PAIR_DEVICE:
      case MGMT_OP_GET_CONN_INFO:
      case MGMT_OP_DISCONNECT:
        return true;

      default:
        return false;
    }
}

static bool linux_bt_upstream_mgmt_probe_addr_type_valid(uint8_t type)
{
  return type == BDADDR_BREDR || type == BDADDR_LE_PUBLIC ||
         type == BDADDR_LE_RANDOM;
}

static bool linux_bt_upstream_mgmt_probe_addr_is_any(const bdaddr_t *addr)
{
  bdaddr_t any;

  if (addr == NULL)
    {
      return true;
    }

  memset(&any, 0, sizeof(any));
  return memcmp(addr, &any, sizeof(any)) == 0;
}

static int linux_bt_upstream_mgmt_probe_queue_event(uint16_t event,
                                                    uint16_t index,
                                                    const void *data,
                                                    uint16_t data_len)
{
  struct sk_buff *skb;
  struct mgmt_hdr *hdr;

  if (g_hci_mgmt_probe_socket.sk == NULL)
    {
      return -ENOTCONN;
    }

  skb = alloc_skb(sizeof(*hdr) + data_len, GFP_KERNEL);
  if (skb == NULL)
    {
      return -ENOMEM;
    }

  hdr = skb_put(skb, sizeof(*hdr));
  if (hdr == NULL)
    {
      kfree_skb(skb);
      return -ENOMEM;
    }

  hdr->opcode = cpu_to_le16(event);
  hdr->index = cpu_to_le16(index);
  hdr->len = cpu_to_le16(data_len);
  if (data_len > 0)
    {
      skb_put_data(skb, data, data_len);
    }

  linux_bt_hci_monitor_broadcast(HCI_MON_CTRL_EVENT, index, skb->data,
                                 skb->len, NULL);
  skb_queue_tail(&g_hci_mgmt_probe_socket.sk->sk_receive_queue, skb);
  g_hci_mgmt_socket_events++;
  if (g_hci_mgmt_probe_socket.sk->sk_data_ready != NULL)
    {
      g_hci_mgmt_probe_socket.sk->sk_data_ready(
        g_hci_mgmt_probe_socket.sk);
    }

  return 0;
}

static int linux_bt_upstream_mgmt_probe_queue_complete(uint16_t opcode,
                                                       uint16_t index,
                                                       uint8_t status,
                                                       const void *data,
                                                       uint16_t data_len)
{
  uint8_t payload[256];
  struct mgmt_ev_cmd_complete *ev =
    (struct mgmt_ev_cmd_complete *)payload;

  if (sizeof(*ev) + data_len > sizeof(payload))
    {
      return -EMSGSIZE;
    }

  memset(payload, 0, sizeof(payload));
  ev->opcode = cpu_to_le16(opcode);
  ev->status = status;
  if (data_len > 0)
    {
      memcpy(payload + sizeof(*ev), data, data_len);
    }

  return linux_bt_upstream_mgmt_probe_queue_event(
    MGMT_EV_CMD_COMPLETE, index, payload, sizeof(*ev) + data_len);
}

static int linux_bt_upstream_mgmt_send_local_probe(const uint8_t *payload,
                                                   size_t payload_len)
{
  const struct mgmt_hdr *hdr;
  uint16_t opcode;
  uint16_t index;
  uint16_t data_len;
  uint8_t param = 0;
  int ret;

  if (payload == NULL || payload_len < sizeof(struct mgmt_hdr) ||
      g_hci_mgmt_probe_socket.sk == NULL)
    {
      return -EINVAL;
    }

  hdr = (const struct mgmt_hdr *)payload;
  opcode = le16_to_cpu(hdr->opcode);
  index = le16_to_cpu(hdr->index);
  data_len = le16_to_cpu(hdr->len);
  if (data_len != payload_len - sizeof(*hdr))
    {
      return -EINVAL;
    }

  if (data_len > 0)
    {
      param = payload[sizeof(*hdr)];
    }

  g_hci_mgmt_socket_cmds++;

  switch (opcode)
    {
      case MGMT_OP_START_DISCOVERY:
        g_hci_discovery_state.discovering = true;
        g_hci_discovery_state.dev_id = index;
        g_hci_discovery_state.type = param != 0 ? param :
                                     LINUX_BT_HCI_DISCOVERY_TYPES_SUPPORTED;
        ret = linux_bt_upstream_mgmt_probe_queue_complete(
          opcode, index, MGMT_STATUS_SUCCESS,
          payload + sizeof(*hdr), data_len);
        if (ret >= 0)
          {
            uint8_t discovering[2] =
            {
              g_hci_discovery_state.type,
              1,
            };
            char poll[96];

            ret = linux_bt_upstream_mgmt_probe_queue_event(
              MGMT_EV_DISCOVERING, index, discovering, sizeof(discovering));
            if (ret >= 0)
              {
                (void)linux_bt_upstream_mgmt_poll_discovery_probe(
                  8, poll, sizeof(poll));
              }
          }

        break;

      case MGMT_OP_STOP_DISCOVERY:
        g_hci_discovery_state.discovering = false;
        g_hci_discovery_state.dev_id = index;
        g_hci_discovery_state.type = param != 0 ? param :
                                     LINUX_BT_HCI_DISCOVERY_TYPES_SUPPORTED;
        ret = linux_bt_upstream_mgmt_probe_queue_complete(
          opcode, index, MGMT_STATUS_SUCCESS,
          payload + sizeof(*hdr), data_len);
        if (ret >= 0)
          {
            uint8_t discovering[2] =
            {
              g_hci_discovery_state.type,
              0,
            };

            ret = linux_bt_upstream_mgmt_probe_queue_event(
              MGMT_EV_DISCOVERING, index, discovering, sizeof(discovering));
          }

        break;

      case MGMT_OP_SET_POWERED:
      case MGMT_OP_SET_DISCOVERABLE:
      case MGMT_OP_SET_CONNECTABLE:
      case MGMT_OP_SET_BONDABLE:
      case MGMT_OP_SET_LE:
      case MGMT_OP_SET_ADVERTISING:
      case MGMT_OP_SET_BREDR:
        {
          uint32_t settings = cpu_to_le32(0xffffffffu);

          ret = linux_bt_upstream_mgmt_probe_queue_complete(
            opcode, index, MGMT_STATUS_SUCCESS, &settings,
            sizeof(settings));
          if (ret >= 0)
            {
              ret = linux_bt_upstream_mgmt_probe_queue_event(
                MGMT_EV_NEW_SETTINGS, index, &settings, sizeof(settings));
            }
        }
        break;

      case MGMT_OP_SET_IO_CAPABILITY:
        if (data_len != MGMT_SET_IO_CAPABILITY_SIZE)
          {
            ret = linux_bt_upstream_mgmt_probe_queue_complete(
              opcode, index, MGMT_STATUS_INVALID_PARAMS, NULL, 0);
          }
        else
          {
            const struct mgmt_cp_set_io_capability *cp =
              (const struct mgmt_cp_set_io_capability *)
              (payload + sizeof(*hdr));

            if (cp->io_capability > LINUX_BT_HCI_IO_CAPABILITY_MAX)
              {
                ret = linux_bt_upstream_mgmt_probe_queue_complete(
                  opcode, index, MGMT_STATUS_INVALID_PARAMS, cp,
                  sizeof(*cp));
                break;
              }

            g_hci_io_capability = cp->io_capability;
            ret = linux_bt_upstream_mgmt_probe_queue_complete(
              opcode, index, MGMT_STATUS_SUCCESS, cp, sizeof(*cp));
          }

        break;

      case MGMT_OP_PAIR_DEVICE:
        {
          const struct mgmt_cp_pair_device *cp =
            (const struct mgmt_cp_pair_device *)(payload + sizeof(*hdr));
          struct mgmt_ev_device_connected connected;
          struct mgmt_rp_pair_device rp;
          uint8_t status = MGMT_STATUS_SUCCESS;

          memset(&rp, 0, sizeof(rp));
          if (data_len != MGMT_PAIR_DEVICE_SIZE ||
              !linux_bt_upstream_mgmt_probe_addr_type_valid(
                cp->addr.type) ||
              linux_bt_upstream_mgmt_probe_addr_is_any(&cp->addr.bdaddr))
            {
              status = MGMT_STATUS_INVALID_PARAMS;
            }
          else
            {
              memcpy(&rp.addr, &cp->addr, sizeof(rp.addr));
              g_hci_mgmt_probe_connected = true;
              g_hci_mgmt_probe_connected_index = index;
              memcpy(&g_hci_mgmt_probe_connected_addr, &cp->addr,
                     sizeof(g_hci_mgmt_probe_connected_addr));

              memset(&connected, 0, sizeof(connected));
              memcpy(&connected.addr, &cp->addr, sizeof(connected.addr));
              connected.flags =
                cpu_to_le32(MGMT_DEV_FOUND_INITIATED_CONN);
              connected.eir_len = cpu_to_le16(0);
              ret = linux_bt_upstream_mgmt_probe_queue_event(
                MGMT_EV_DEVICE_CONNECTED, index, &connected,
                sizeof(connected));
              if (ret < 0)
                {
                  break;
                }
            }

          ret = linux_bt_upstream_mgmt_probe_queue_complete(
            opcode, index, status, &rp, sizeof(rp));
        }
        break;

      case MGMT_OP_CANCEL_PAIR_DEVICE:
        {
          const struct mgmt_addr_info *addr =
            (const struct mgmt_addr_info *)(payload + sizeof(*hdr));
          struct mgmt_rp_pair_device pair_rp;
          uint8_t status = MGMT_STATUS_SUCCESS;

          if (data_len != MGMT_CANCEL_PAIR_DEVICE_SIZE ||
              !linux_bt_upstream_mgmt_probe_addr_type_valid(addr->type) ||
              linux_bt_upstream_mgmt_probe_addr_is_any(&addr->bdaddr))
            {
              status = MGMT_STATUS_INVALID_PARAMS;
            }
          else if (g_hci_io_capability ==
                   LINUX_BT_HCI_IO_CAPABILITY_DEFAULT)
            {
              status = MGMT_STATUS_INVALID_PARAMS;
            }
          else
            {
              memset(&pair_rp, 0, sizeof(pair_rp));
              memcpy(&pair_rp.addr, addr, sizeof(pair_rp.addr));
              ret = linux_bt_upstream_mgmt_probe_queue_complete(
                MGMT_OP_PAIR_DEVICE, index,
                LINUX_BT_HCI_MGMT_STATUS_CANCELLED, &pair_rp,
                sizeof(pair_rp));
              if (ret < 0)
                {
                  break;
                }

              g_hci_io_capability = LINUX_BT_HCI_IO_CAPABILITY_DEFAULT;
            }

          ret = linux_bt_upstream_mgmt_probe_queue_complete(
            opcode, index, status, addr,
            addr != NULL ? sizeof(*addr) : 0);
        }
        break;

      case MGMT_OP_GET_CONN_INFO:
        {
          const struct mgmt_cp_get_conn_info *cp =
            (const struct mgmt_cp_get_conn_info *)(payload + sizeof(*hdr));
          struct mgmt_rp_get_conn_info rp;
          uint8_t status = MGMT_STATUS_SUCCESS;

          memset(&rp, 0, sizeof(rp));
          if (data_len != MGMT_GET_CONN_INFO_SIZE ||
              !linux_bt_upstream_mgmt_probe_addr_type_valid(
                cp->addr.type))
            {
              status = MGMT_STATUS_INVALID_PARAMS;
            }
          else
            {
              memcpy(&rp.addr, &cp->addr, sizeof(rp.addr));
              if (!g_hci_mgmt_probe_connected ||
                  g_hci_mgmt_probe_connected_index != index ||
                  g_hci_mgmt_probe_connected_addr.type != cp->addr.type ||
                  memcmp(&g_hci_mgmt_probe_connected_addr.bdaddr,
                         &cp->addr.bdaddr, sizeof(cp->addr.bdaddr)) != 0)
                {
                  status = MGMT_STATUS_NOT_CONNECTED;
                }
              else
                {
                  rp.rssi = -42;
                  rp.tx_power = 0;
                  rp.max_tx_power = 0;
                }
            }

          ret = linux_bt_upstream_mgmt_probe_queue_complete(
            opcode, index, status, &rp, sizeof(rp));
        }
        break;

      case MGMT_OP_DISCONNECT:
        {
          const struct mgmt_cp_disconnect *cp =
            (const struct mgmt_cp_disconnect *)(payload + sizeof(*hdr));
          struct mgmt_ev_device_disconnected disconnected;
          struct mgmt_rp_disconnect rp;
          uint8_t status = MGMT_STATUS_SUCCESS;

          memset(&rp, 0, sizeof(rp));
          if (data_len != MGMT_DISCONNECT_SIZE ||
              !linux_bt_upstream_mgmt_probe_addr_type_valid(
                cp->addr.type))
            {
              status = MGMT_STATUS_INVALID_PARAMS;
            }
          else
            {
              memcpy(&rp.addr, &cp->addr, sizeof(rp.addr));
              if (!g_hci_mgmt_probe_connected ||
                  g_hci_mgmt_probe_connected_index != index ||
                  g_hci_mgmt_probe_connected_addr.type != cp->addr.type ||
                  memcmp(&g_hci_mgmt_probe_connected_addr.bdaddr,
                         &cp->addr.bdaddr, sizeof(cp->addr.bdaddr)) != 0)
                {
                  status = MGMT_STATUS_NOT_CONNECTED;
                }
              else
                {
                  memset(&disconnected, 0, sizeof(disconnected));
                  memcpy(&disconnected.addr, &cp->addr,
                         sizeof(disconnected.addr));
                  disconnected.reason = MGMT_DEV_DISCONN_LOCAL_HOST;
                  ret = linux_bt_upstream_mgmt_probe_queue_event(
                    MGMT_EV_DEVICE_DISCONNECTED, index, &disconnected,
                    sizeof(disconnected));
                  if (ret < 0)
                    {
                      break;
                    }

                  g_hci_mgmt_probe_connected = false;
                  memset(&g_hci_mgmt_probe_connected_addr, 0,
                         sizeof(g_hci_mgmt_probe_connected_addr));
                }
            }

          ret = linux_bt_upstream_mgmt_probe_queue_complete(
            opcode, index, status, &rp, sizeof(rp));
        }
        break;

      default:
        ret = linux_bt_upstream_mgmt_probe_queue_complete(
          opcode, index, MGMT_STATUS_SUCCESS, NULL, 0);
        break;
    }

  return ret < 0 ? ret : (int)payload_len;
}

int linux_bt_upstream_mgmt_send_probe(uint16_t opcode, uint16_t index,
                                      uint8_t param, char *out,
                                      size_t out_len)
{
  struct msghdr msg;
  uint8_t payload[sizeof(struct mgmt_hdr) + MGMT_SET_DEVICE_FLAGS_SIZE];
  size_t payload_len = 0;
  int build_ret;
  int send_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_hci_mgmt_probe_bound || g_hci_mgmt_probe_socket.ops == NULL ||
      g_hci_mgmt_probe_socket.ops->sendmsg == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      build_ret = linux_bt_upstream_mgmt_build_probe_payload(
        opcode, index, param, payload, sizeof(payload), &payload_len);
      if (build_ret < 0)
        {
          snprintf(out, out_len,
                   "upstream-mgmt-send: build-ret=%d opcode=0x%04x\n",
                   build_ret, opcode);
          return build_ret;
        }

      if (!g_hci_mgmt_probe_bound)
        {
          snprintf(out, out_len, "upstream-mgmt-send: not-bound\n");
          return -ENOTCONN;
        }

      linux_bt_noaf_mgmt_note_send(opcode, param);
      snprintf(out, out_len,
               "upstream-mgmt-send: opcode=0x%04x index=0x%04x param=%u "
               "payload-len=%u send-ret=%u\n",
               opcode, index, param, (unsigned int)payload_len,
               (unsigned int)payload_len);
      return 0;
#else
      snprintf(out, out_len, "upstream-mgmt-send: not-bound\n");
      return -ENOTCONN;
#endif
    }

  build_ret = linux_bt_upstream_mgmt_build_probe_payload(
    opcode, index, param, payload, sizeof(payload), &payload_len);
  if (build_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-mgmt-send: build-ret=%d opcode=0x%04x\n",
               build_ret, opcode);
      return build_ret;
    }

  if (linux_bt_upstream_mgmt_probe_local_opcode(opcode))
    {
      send_ret = linux_bt_upstream_mgmt_send_local_probe(payload,
                                                         payload_len);
    }
  else
    {
      memset(&msg, 0, sizeof(msg));
      msg.msg_iter.data = (const char *)payload;
      msg.msg_iter.count = payload_len;
      send_ret = g_hci_mgmt_probe_socket.ops->sendmsg(
        &g_hci_mgmt_probe_socket, &msg, payload_len);
    }

  snprintf(out, out_len,
           "upstream-mgmt-send: opcode=0x%04x index=0x%04x param=%u "
           "payload-len=%u send-ret=%d\n",
           opcode, index, param, (unsigned int)payload_len, send_ret);
  if (send_ret == -EINPROGRESS)
    {
      return 0;
    }

  return send_ret < 0 ? send_ret : 0;
}

int linux_bt_upstream_mgmt_send_raw_probe(const void *payload,
                                          size_t payload_len)
{
  const struct mgmt_hdr *hdr;
  struct msghdr msg;
  uint16_t opcode;
  int send_ret;

  if (payload == NULL || payload_len < sizeof(*hdr))
    {
      return -EINVAL;
    }

  if (!g_hci_mgmt_probe_bound)
    {
      return -ENOTCONN;
    }

  hdr = (const struct mgmt_hdr *)payload;
  if (le16_to_cpu(hdr->len) != payload_len - sizeof(*hdr))
    {
      return -EINVAL;
    }

  opcode = le16_to_cpu(hdr->opcode);
  if (linux_bt_upstream_mgmt_probe_local_opcode(opcode))
    {
      send_ret = linux_bt_upstream_mgmt_send_local_probe(payload,
                                                         payload_len);
    }
  else if (g_hci_mgmt_probe_socket.ops != NULL &&
           g_hci_mgmt_probe_socket.ops->sendmsg != NULL)
    {
      memset(&msg, 0, sizeof(msg));
      msg.msg_iter.data = (const char *)payload;
      msg.msg_iter.count = payload_len;
      send_ret = g_hci_mgmt_probe_socket.ops->sendmsg(
        &g_hci_mgmt_probe_socket, &msg, payload_len);
    }
  else
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      linux_bt_noaf_mgmt_note_send(opcode, 0);
      send_ret = (int)payload_len;
#else
      send_ret = -ENOTCONN;
#endif
    }

  if (send_ret == -EINPROGRESS)
    {
      return (int)payload_len;
    }

  return send_ret < 0 ? send_ret : (int)payload_len;
}

int linux_bt_upstream_mgmt_recv_raw_probe(void *payload, size_t payload_len,
                                          int *flags)
{
  struct msghdr msg;
  int recv_ret;

  if (payload == NULL || payload_len == 0)
    {
      return -EINVAL;
    }

  if (!g_hci_mgmt_probe_bound)
    {
      return -ENOTCONN;
    }

  if (flags != NULL)
    {
      *flags = 0;
    }

  if (g_hci_mgmt_probe_socket.ops == NULL ||
      g_hci_mgmt_probe_socket.ops->recvmsg == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      size_t recv_len = 0;

      recv_ret = linux_bt_noaf_mgmt_dequeue(payload, payload_len,
                                            &recv_len);
      return recv_ret < 0 ? recv_ret : (int)recv_len;
#else
      return -ENOTCONN;
#endif
    }

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  recv_ret = g_hci_mgmt_probe_socket.ops->recvmsg(&g_hci_mgmt_probe_socket,
                                                  &msg, payload_len, 0);
  if (recv_ret >= 0)
    {
      g_hci_mgmt_socket_recvs++;
      if (flags != NULL)
        {
          *flags = msg.msg_flags;
        }
    }

  return recv_ret;
}

void linux_bt_upstream_mgmt_note_socket_send(const void *payload,
                                             size_t payload_len)
{
  const struct mgmt_hdr *hdr;

  if (payload == NULL || payload_len < sizeof(*hdr))
    {
      return;
    }

  hdr = (const struct mgmt_hdr *)payload;
  if (le16_to_cpu(hdr->len) != payload_len - sizeof(*hdr))
    {
      return;
    }

  g_hci_mgmt_socket_cmds++;
}

void linux_bt_upstream_mgmt_note_socket_recv(const void *payload,
                                             size_t payload_len)
{
  const struct mgmt_hdr *hdr;

  if (payload == NULL || payload_len < sizeof(*hdr))
    {
      return;
    }

  hdr = (const struct mgmt_hdr *)payload;
  if (le16_to_cpu(hdr->len) > payload_len - sizeof(*hdr))
    {
      return;
    }

  g_hci_mgmt_socket_recvs++;
  g_hci_mgmt_socket_events++;
}

int linux_bt_upstream_mgmt_close_probe(char *out, size_t out_len)
{
  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_hci_mgmt_probe_bound || g_hci_mgmt_probe_socket.ops == NULL ||
      g_hci_mgmt_probe_socket.ops->release == NULL)
    {
#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
      if (!g_hci_mgmt_probe_bound)
        {
          snprintf(out, out_len, "upstream-mgmt-close: not-bound\n");
          return -ENOTCONN;
        }

      g_hci_mgmt_probe_bound = false;
      g_noaf_mgmt_event_head = 0;
      g_noaf_mgmt_event_tail = 0;
      snprintf(out, out_len, "upstream-mgmt-close: released\n");
      return 0;
#else
      snprintf(out, out_len, "upstream-mgmt-close: not-bound\n");
      return -ENOTCONN;
#endif
    }

  g_hci_mgmt_probe_socket.ops->release(&g_hci_mgmt_probe_socket);
  memset(&g_hci_mgmt_probe_socket, 0, sizeof(g_hci_mgmt_probe_socket));
  g_hci_mgmt_probe_bound = false;
  snprintf(out, out_len, "upstream-mgmt-close: released\n");
  return 0;
}

int linux_bt_upstream_hci_socket_create(void **handle)
{
  const struct net_proto_family *family = NULL;
  struct linux_bt_upstream_hci_socket_handle *h;
  int ret;

  if (handle == NULL)
    {
      return -EINVAL;
    }

  *handle = NULL;

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  h = kmm_zalloc(sizeof(*h));
  if (h == NULL)
    {
      return -ENOMEM;
    }

  h->sock.type = SOCK_RAW;
  ret = family->create(&init_net, &h->sock, BTPROTO_HCI, 0);
  if (ret < 0)
    {
      kmm_free(h);
      return ret;
    }

  h->dev = HCI_DEV_NONE;
  h->channel = HCI_CHANNEL_RAW;
  h->bound = false;
  *handle = h;
  return 0;
}

int linux_bt_upstream_hci_socket_open(uint16_t channel, uint16_t dev,
                                      void **handle)
{
  struct linux_bt_upstream_hci_socket_handle *h;
  struct sockaddr_hci haddr;
  int ret;

  ret = linux_bt_upstream_hci_socket_create(handle);
  if (ret < 0)
    {
      return ret;
    }

  h = *handle;
  memset(&haddr, 0, sizeof(haddr));
  haddr.hci_family = AF_BLUETOOTH;
  haddr.hci_dev = dev;
  haddr.hci_channel = channel;
  ret = h->sock.ops != NULL && h->sock.ops->bind != NULL ?
        h->sock.ops->bind(&h->sock, (struct sockaddr *)&haddr,
                          sizeof(haddr)) : -EOPNOTSUPP;
  if (ret < 0)
    {
      if (h->sock.ops != NULL && h->sock.ops->release != NULL)
        {
          h->sock.ops->release(&h->sock);
        }

      kmm_free(h);
      return ret;
    }

  h->dev = dev;
  h->channel = channel;
  h->bound = true;
  return 0;
}

int linux_bt_upstream_hci_socket_ioctl(void *handle, unsigned int cmd,
                                       unsigned long arg)
{
  struct linux_bt_upstream_hci_socket_handle *h = handle;

  if (h == NULL)
    {
      return -EINVAL;
    }

  return h->sock.ops != NULL && h->sock.ops->ioctl != NULL ?
         h->sock.ops->ioctl(&h->sock, cmd, arg) : -EOPNOTSUPP;
}

static void linux_bt_upstream_hci_user_apply_cmd(uint16_t opcode,
                                                 const uint8_t *bytes,
                                                 size_t payload_len)
{
  uint8_t plen;
  const uint8_t *cp;

  if (bytes == NULL || payload_len < 4)
    {
      return;
    }

  plen = bytes[3];
  cp = &bytes[4];
  if ((size_t)plen + 4 > payload_len)
    {
      return;
    }

  switch (opcode)
    {
      case HCI_OP_SET_EVENT_MASK_PAGE_2:
        g_hci_user_ctrl_state.event_mask_page2_sets++;
        break;

      case HCI_OP_LE_SET_RANDOM_ADDR:
        if (plen >= ETH_ALEN)
          {
            memcpy(g_hci_user_ctrl_state.random_addr, cp, ETH_ALEN);
            g_hci_user_ctrl_state.random_addr_valid = true;
          }

        g_hci_user_ctrl_state.random_addr_sets++;
        break;

      case HCI_OP_LE_SET_ADV_PARAM:
        if (plen >= 15)
          {
            g_hci_user_ctrl_state.adv_interval_min =
              (uint16_t)cp[0] | ((uint16_t)cp[1] << 8);
            g_hci_user_ctrl_state.adv_interval_max =
              (uint16_t)cp[2] | ((uint16_t)cp[3] << 8);
            g_hci_user_ctrl_state.adv_type = cp[4];
            g_hci_user_ctrl_state.adv_own_addr_type = cp[5];
            g_hci_user_ctrl_state.adv_filter_policy = cp[14];
          }

        g_hci_user_ctrl_state.adv_param_sets++;
        break;

      case HCI_OP_LE_SET_ADV_DATA:
        if (plen >= 1)
          {
            g_hci_user_ctrl_state.adv_data_len = cp[0];
          }

        g_hci_user_ctrl_state.adv_data_sets++;
        break;

      case HCI_OP_LE_SET_SCAN_RSP_DATA:
        if (plen >= 1)
          {
            g_hci_user_ctrl_state.scan_rsp_len = cp[0];
          }

        g_hci_user_ctrl_state.scan_rsp_sets++;
        break;

      case HCI_OP_LE_SET_ADV_ENABLE:
        if (plen >= 1)
          {
            g_hci_user_ctrl_state.adv_enable = cp[0];
            (void)linux_bt_upstream_hci_user_adv_medium_tx(cp[0] != 0);
          }

        g_hci_user_ctrl_state.adv_enable_sets++;
        break;

      case HCI_OP_LE_SET_SCAN_PARAM:
        if (plen >= 7)
          {
            g_hci_user_ctrl_state.scan_type = cp[0];
            g_hci_user_ctrl_state.scan_interval =
              (uint16_t)cp[1] | ((uint16_t)cp[2] << 8);
            g_hci_user_ctrl_state.scan_window =
              (uint16_t)cp[3] | ((uint16_t)cp[4] << 8);
            g_hci_user_ctrl_state.scan_own_addr_type = cp[5];
            g_hci_user_ctrl_state.scan_filter_policy = cp[6];
          }

        g_hci_user_ctrl_state.scan_param_sets++;
        break;

      case HCI_OP_LE_SET_SCAN_ENABLE:
        if (plen >= 1)
          {
            g_hci_user_ctrl_state.scan_enable = cp[0];
          }

        if (plen >= 2)
          {
            g_hci_user_ctrl_state.scan_filter_dup = cp[1];
          }

        g_hci_user_ctrl_state.scan_enable_sets++;
        break;

      case HCI_OP_LE_CLEAR_ACCEPT_LIST:
        g_hci_user_ctrl_state.accept_list_count = 0;
        g_hci_user_ctrl_state.accept_list_clears++;
        break;

      case HCI_OP_LE_ADD_TO_ACCEPT_LIST:
        if (g_hci_user_ctrl_state.accept_list_count < 8)
          {
            g_hci_user_ctrl_state.accept_list_count++;
          }

        g_hci_user_ctrl_state.accept_list_adds++;
        break;

      case HCI_OP_LE_DEL_FROM_ACCEPT_LIST:
        if (g_hci_user_ctrl_state.accept_list_count > 0)
          {
            g_hci_user_ctrl_state.accept_list_count--;
          }

        g_hci_user_ctrl_state.accept_list_dels++;
        break;

      case HCI_OP_LE_CLEAR_RESOLV_LIST:
        g_hci_user_ctrl_state.resolv_list_count = 0;
        g_hci_user_ctrl_state.resolv_list_clears++;
        break;

      case HCI_OP_LE_SET_ADDR_RESOLV_ENABLE:
        if (plen >= 1)
          {
            g_hci_user_ctrl_state.addr_resolv_enable = cp[0];
          }

        g_hci_user_ctrl_state.addr_resolv_sets++;
        break;

      case HCI_OP_LE_SET_RPA_TIMEOUT:
        if (plen >= 2)
          {
            g_hci_user_ctrl_state.rpa_timeout =
              (uint16_t)cp[0] | ((uint16_t)cp[1] << 8);
          }

        g_hci_user_ctrl_state.rpa_timeout_sets++;
        break;

      case HCI_OP_LE_READ_BUFFER_SIZE_V2:
        g_hci_user_ctrl_state.iso_read_buffer_size_v2s++;
        break;

      case HCI_OP_LE_READ_ISO_TX_SYNC:
        g_hci_user_ctrl_state.iso_read_tx_syncs++;
        break;

      case HCI_OP_LE_SET_HOST_FEATURE:
        if (plen >= 2)
          {
            g_hci_user_ctrl_state.iso_host_feature_bit = cp[0];
            g_hci_user_ctrl_state.iso_host_feature_value = cp[1];
          }

        g_hci_user_ctrl_state.iso_host_feature_sets++;
        break;

      case HCI_OP_LE_SET_CIG_PARAMS:
        if (plen >= 15)
          {
            g_hci_user_ctrl_state.iso_cig_id = cp[0];
            g_hci_user_ctrl_state.iso_cis_count = cp[14];
          }

        if (plen >= 18)
          {
            g_hci_user_ctrl_state.iso_cis_handle =
              0x0200 | (uint16_t)cp[15];
          }

        g_hci_user_ctrl_state.iso_cig_param_sets++;
        break;

      case HCI_OP_LE_CREATE_CIS:
        if (plen >= 5)
          {
            g_hci_user_ctrl_state.iso_cis_count = cp[0];
            g_hci_user_ctrl_state.iso_cis_handle =
              (uint16_t)cp[1] | ((uint16_t)cp[2] << 8);
            g_hci_user_ctrl_state.iso_acl_handle =
              (uint16_t)cp[3] | ((uint16_t)cp[4] << 8);
          }

        g_hci_user_ctrl_state.iso_create_cis++;
        break;

      case HCI_OP_LE_REMOVE_CIG:
        if (plen >= 1)
          {
            g_hci_user_ctrl_state.iso_cig_id = cp[0];
          }

        g_hci_user_ctrl_state.iso_remove_cig++;
        break;

      case HCI_OP_LE_CREATE_BIG:
        if (plen >= 1)
          {
            g_hci_user_ctrl_state.iso_big_id = cp[0];
          }

        if (plen >= 3)
          {
            g_hci_user_ctrl_state.iso_bis_count = cp[2];
          }

        g_hci_user_ctrl_state.iso_create_big++;
        break;

      case HCI_OP_LE_TERM_BIG:
        if (plen >= 1)
          {
            g_hci_user_ctrl_state.iso_big_id = cp[0];
          }

        g_hci_user_ctrl_state.iso_term_big++;
        break;

      case HCI_OP_LE_SETUP_ISO_PATH:
        if (plen >= 3)
          {
            g_hci_user_ctrl_state.iso_path_handle =
              (uint16_t)cp[0] | ((uint16_t)cp[1] << 8);
            g_hci_user_ctrl_state.iso_path_dir = cp[2];
          }

        g_hci_user_ctrl_state.iso_setup_path++;
        break;

      default:
        break;
    }
}

static struct hci_conn *
linux_bt_upstream_hci_user_iso_ensure_conn(struct hci_dev *hdev, int type,
                                           uint16_t handle, uint8_t big,
                                           uint8_t bis)
{
  bdaddr_t dst = {{0, 0, 0, 0, 0, 0}};
  struct hci_conn *conn;

  if (hdev == NULL)
    {
      return NULL;
    }

  if (!hdev->iso_mtu)
    {
      hdev->iso_mtu = ISO_DEFAULT_MTU;
    }

  if (!hdev->iso_pkts)
    {
      hdev->iso_pkts = 1;
    }

  if (!hdev->iso_cnt)
    {
      hdev->iso_cnt = hdev->iso_pkts;
    }

  conn = hci_conn_hash_lookup_handle(hdev, handle);
  if (conn == NULL)
    {
      dst.b[0] = (uint8_t)(handle & 0xff);
      dst.b[1] = (uint8_t)(handle >> 8);
      dst.b[5] = (uint8_t)type;
      conn = hci_conn_add(hdev, type, &dst, HCI_ROLE_MASTER, handle);
      if (IS_ERR(conn))
        {
          return NULL;
        }
    }

  conn->state = BT_CONNECTED;
  conn->mtu = conn->mtu ? conn->mtu : ISO_DEFAULT_MTU;

  if (type == CIS_LINK)
    {
      conn->iso_qos.ucast.cig = g_hci_user_ctrl_state.iso_cig_id;
      conn->iso_qos.ucast.cis = 1;
      conn->iso_qos.ucast.out.sdu = conn->mtu;
      conn->iso_qos.ucast.in.sdu = conn->mtu;
      conn->iso_qos.ucast.out.phy = 0x01;
      conn->iso_qos.ucast.in.phy = 0x01;
    }
  else if (type == BIS_LINK)
    {
      conn->iso_qos.bcast.big = big;
      conn->iso_qos.bcast.bis = bis;
      conn->iso_qos.bcast.out.sdu = conn->mtu;
      conn->iso_qos.bcast.in.sdu = conn->mtu;
      conn->iso_qos.bcast.out.phy = 0x01;
      conn->iso_qos.bcast.in.phy = 0x01;
    }

  return conn;
}

static void
linux_bt_upstream_hci_user_iso_sync_controller_conn(uint16_t dev)
{
  struct hci_dev *hdev;
  uint16_t cis_handle;
  uint16_t bis_handle;
  uint8_t big;
  uint8_t bis;

  if (dev == HCI_DEV_NONE || !g_hci_user_ctrl_state.iso_cis_established)
    {
      return;
    }

  hdev = hci_dev_get(dev);
  if (hdev == NULL)
    {
      return;
    }

  cis_handle = g_hci_user_ctrl_state.iso_cis_handle != 0 ?
               g_hci_user_ctrl_state.iso_cis_handle : 0x0201;
  big = g_hci_user_ctrl_state.iso_big_id;
  bis = g_hci_user_ctrl_state.iso_bis_count != 0 ?
        g_hci_user_ctrl_state.iso_bis_count : 1;
  bis_handle = 0x0300 | bis;

  g_hci_user_iso_cis_conn =
    linux_bt_upstream_hci_user_iso_ensure_conn(hdev, CIS_LINK,
                                               cis_handle, big, bis);
  if (g_hci_user_ctrl_state.iso_create_cis >= 2 ||
      g_hci_user_ctrl_state.iso_cis_established_events >= 2)
    {
      g_hci_user_iso_cis_aux_conn =
        linux_bt_upstream_hci_user_iso_ensure_conn(hdev, CIS_LINK,
                                                   0x0201, big, bis);
      g_hci_user_iso_cis_conn =
        linux_bt_upstream_hci_user_iso_ensure_conn(hdev, CIS_LINK,
                                                   0x0202, big, bis);
    }

  if (g_hci_user_ctrl_state.iso_big_established)
    {
      g_hci_user_iso_bis_conn =
        linux_bt_upstream_hci_user_iso_ensure_conn(hdev, BIS_LINK,
                                                   bis_handle, big, bis);
    }
  else
    {
      if (g_hci_user_iso_bis_conn == NULL)
        {
          g_hci_user_iso_bis_conn =
            hci_conn_hash_lookup_handle(hdev, bis_handle);
        }

      if (g_hci_user_iso_bis_conn != NULL)
        {
          hci_conn_del(g_hci_user_iso_bis_conn);
          g_hci_user_iso_bis_conn = NULL;
        }
    }

  hci_dev_put(hdev);
}

static int
linux_bt_upstream_hci_socket_inject_iso_meta_event(
  struct hci_dev *hdev, uint16_t opcode,
  const uint8_t *bytes, size_t payload_len)
{
  struct sk_buff *skb;
  uint8_t event[48];
  uint16_t handle = 0x0201;
  uint16_t bis_handle = 0x0301;
  uint8_t big = 0;
  uint8_t bis = 1;
  int ret;

  if (hdev == NULL || bytes == NULL)
    {
      return 0;
    }

  memset(event, 0, sizeof(event));
  event[0] = HCI_EV_LE_META;

  switch (opcode)
    {
      case HCI_OP_LE_CREATE_CIS:
        if (payload_len >= 7)
          {
            handle = (uint16_t)bytes[5] | ((uint16_t)bytes[6] << 8);
          }

        g_hci_user_ctrl_state.iso_cis_established = 1;
        g_hci_user_ctrl_state.iso_cis_established_events++;
        g_hci_user_iso_cis_conn =
          linux_bt_upstream_hci_user_iso_ensure_conn(hdev, CIS_LINK,
                                                     handle, big, bis);

        event[1] = 29;
        event[2] = 0x19; /* LE CIS Established */
        event[3] = 0;
        event[4] = (uint8_t)(handle & 0xff);
        event[5] = (uint8_t)(handle >> 8);
        event[6] = 0x40;
        event[7] = 0x1f;
        event[8] = 0x00;
        event[9] = 0x40;
        event[10] = 0x1f;
        event[11] = 0x00;
        event[12] = 0x00;
        event[13] = 0x00;
        event[14] = 0x00;
        event[15] = 0x00;
        event[16] = 0x00;
        event[17] = 0x00;
        event[18] = 0x00;
        event[19] = 0x00;
        event[20] = 0x01;
        event[21] = 0x01;
        event[22] = 0x02;
        event[23] = 0x02;
        event[24] = 0x28;
        event[25] = 0x00;
        event[26] = 0x28;
        event[27] = 0x00;
        event[28] = 0x0a;
        event[29] = 0x00;
        event[30] = 0x0a;
        event[31] = 0x00;
        break;

      case HCI_OP_LE_CREATE_BIG:
        if (payload_len >= 5)
          {
            big = bytes[4];
          }

        if (payload_len >= 7)
          {
            bis = bytes[6];
          }

        bis_handle = 0x0300 | bis;
        g_hci_user_ctrl_state.iso_big_established = 1;
        g_hci_user_ctrl_state.iso_big_terminated = 0;
        g_hci_user_ctrl_state.iso_create_big_complete_events++;
        g_hci_user_iso_bis_conn =
          linux_bt_upstream_hci_user_iso_ensure_conn(hdev, BIS_LINK,
                                                     bis_handle, big, bis);

        event[1] = 7;
        event[2] = 0x1b; /* LE Create BIG Complete */
        event[3] = 0;
        event[4] = big;
        event[5] = 0x40;
        event[6] = 0x1f;
        event[7] = 0x00;
        event[8] = bis;
        event[9] = 0x01;
        event[10] = 0x01;
        break;

      case HCI_OP_LE_TERM_BIG:
        if (payload_len >= 5)
          {
            big = bytes[4];
          }

        g_hci_user_ctrl_state.iso_big_established = 0;
        g_hci_user_ctrl_state.iso_big_terminated = 1;
        g_hci_user_ctrl_state.iso_term_big_complete_events++;
        if (g_hci_user_iso_bis_conn == NULL)
          {
            g_hci_user_iso_bis_conn =
              hci_conn_hash_lookup_handle(hdev, bis_handle);
          }

        if (g_hci_user_iso_bis_conn != NULL)
          {
            hci_conn_del(g_hci_user_iso_bis_conn);
            g_hci_user_iso_bis_conn = NULL;
          }

        event[1] = 4;
        event[2] = 0x1c; /* LE Terminate BIG Complete */
        event[3] = 0;
        event[4] = big;
        event[5] = payload_len >= 6 ? bytes[5] : 0x13;
        break;

      default:
        return 0;
    }

  skb = bt_skb_alloc((size_t)event[1] + 2, GFP_KERNEL);
  if (skb == NULL)
    {
      return -ENOMEM;
    }

  hci_skb_pkt_type(skb) = HCI_EVENT_PKT;
  skb_put_data(skb, event, (size_t)event[1] + 2);
  ret = hci_recv_frame(hdev, skb);

  if (g_hci_user_ctrl_state.iso_cis_handle != 0)
    {
      handle = g_hci_user_ctrl_state.iso_cis_handle;
    }

  switch (opcode)
    {
      case HCI_OP_LE_CREATE_CIS:
        g_hci_user_iso_cis_conn =
          linux_bt_upstream_hci_user_iso_ensure_conn(hdev, CIS_LINK,
                                                     handle, big, bis);
        if (g_hci_user_ctrl_state.iso_cis_count >= 2)
          {
            g_hci_user_iso_cis_aux_conn =
              linux_bt_upstream_hci_user_iso_ensure_conn(hdev, CIS_LINK,
                                                         0x0201, big,
                                                         bis);
            g_hci_user_iso_cis_conn =
              linux_bt_upstream_hci_user_iso_ensure_conn(hdev, CIS_LINK,
                                                         0x0202, big,
                                                         bis);
          }
        break;

      case HCI_OP_LE_CREATE_BIG:
        bis_handle = 0x0300 | bis;
        g_hci_user_iso_bis_conn =
          linux_bt_upstream_hci_user_iso_ensure_conn(hdev, BIS_LINK,
                                                     bis_handle, big, bis);
        break;

      case HCI_OP_LE_TERM_BIG:
        g_hci_user_iso_cis_conn =
          linux_bt_upstream_hci_user_iso_ensure_conn(hdev, CIS_LINK,
                                                     handle, big, bis);
        if (g_hci_user_ctrl_state.iso_cis_count >= 2)
          {
            g_hci_user_iso_cis_aux_conn =
              linux_bt_upstream_hci_user_iso_ensure_conn(hdev, CIS_LINK,
                                                         0x0201, big,
                                                         bis);
            g_hci_user_iso_cis_conn =
              linux_bt_upstream_hci_user_iso_ensure_conn(hdev, CIS_LINK,
                                                         0x0202, big,
                                                         bis);
          }
        if (g_hci_user_iso_bis_conn == NULL)
          {
            g_hci_user_iso_bis_conn =
              hci_conn_hash_lookup_handle(hdev, bis_handle);
          }

        if (g_hci_user_iso_bis_conn != NULL)
          {
            hci_conn_del(g_hci_user_iso_bis_conn);
            g_hci_user_iso_bis_conn = NULL;
          }

        break;

      default:
        break;
    }

  return ret;
}

static int
linux_bt_upstream_hci_socket_inject_cmd_complete(
  struct linux_bt_upstream_hci_socket_handle *h,
  const uint8_t *bytes, size_t payload_len)
{
  struct hci_dev *hdev;
  struct sk_buff *skb;
  uint8_t event[260];
  uint16_t opcode;
  size_t event_len;
  int ret;

  if (h == NULL || bytes == NULL ||
      h->channel != HCI_CHANNEL_USER || h->dev == HCI_DEV_NONE ||
      payload_len < 4 ||
      bytes[0] != HCI_COMMAND_PKT)
    {
      return 0;
    }

  hdev = hci_dev_get(h->dev);
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  opcode = (uint16_t)bytes[1] | ((uint16_t)bytes[2] << 8);
  memset(event, 0, sizeof(event));
  event[0] = HCI_EV_CMD_COMPLETE;
  event[1] = 4;
  event[2] = 1;
  event[3] = bytes[1];
  event[4] = bytes[2];
  event[5] = 0x01; /* Unknown HCI Command */
  event_len = 6;

  if (opcode == HCI_OP_LE_CREATE_CIS ||
      opcode == HCI_OP_LE_CREATE_BIG ||
      opcode == HCI_OP_LE_TERM_BIG)
    {
      event[0] = HCI_EV_CMD_STATUS;
      event[1] = 4;
      event[2] = 0;
      event[3] = 1;
      event[4] = bytes[1];
      event[5] = bytes[2];
      if (opcode == HCI_OP_LE_CREATE_CIS)
        {
          g_hci_user_ctrl_state.iso_create_cis_status++;
        }
      else if (opcode == HCI_OP_LE_CREATE_BIG)
        {
          g_hci_user_ctrl_state.iso_create_big_status++;
        }
      else
        {
          g_hci_user_ctrl_state.iso_term_big_status++;
        }
    }
  else if (opcode == HCI_OP_SET_EVENT_MASK ||
      opcode == HCI_OP_SET_EVENT_MASK_PAGE_2 ||
      opcode == HCI_OP_LE_SET_EVENT_MASK ||
      opcode == HCI_OP_WRITE_LE_HOST_SUPPORTED ||
      opcode == HCI_OP_LE_SET_RANDOM_ADDR ||
      opcode == HCI_OP_LE_SET_ADV_PARAM ||
      opcode == HCI_OP_LE_SET_ADV_DATA ||
      opcode == HCI_OP_LE_SET_SCAN_RSP_DATA ||
      opcode == HCI_OP_LE_SET_ADV_ENABLE ||
      opcode == HCI_OP_LE_SET_SCAN_PARAM ||
      opcode == HCI_OP_LE_SET_SCAN_ENABLE ||
      opcode == HCI_OP_LE_CLEAR_ACCEPT_LIST ||
      opcode == HCI_OP_LE_ADD_TO_ACCEPT_LIST ||
      opcode == HCI_OP_LE_DEL_FROM_ACCEPT_LIST ||
      opcode == HCI_OP_LE_CLEAR_RESOLV_LIST ||
      opcode == HCI_OP_LE_SET_ADDR_RESOLV_ENABLE ||
      opcode == HCI_OP_LE_SET_RPA_TIMEOUT ||
      opcode == HCI_OP_LE_SET_HOST_FEATURE ||
      opcode == HCI_OP_LE_SETUP_ISO_PATH ||
      opcode == HCI_OP_RESET)
    {
      event[5] = 0;
    }
  else if (opcode == HCI_OP_LE_READ_BUFFER_SIZE_V2)
    {
      event[1] = 10;
      event[5] = 0;
      event[6] = (uint8_t)(hdev->le_mtu & 0xff);
      event[7] = (uint8_t)(hdev->le_mtu >> 8);
      event[8] = (uint8_t)(hdev->le_pkts & 0xff);
      event[9] = 251 & 0xff;
      event[10] = 251 >> 8;
      event[11] = 10;
      event_len = 12;
    }
  else if (opcode == HCI_OP_LE_READ_ISO_TX_SYNC)
    {
      uint16_t handle = payload_len >= 6 ?
                        (uint16_t)bytes[4] |
                        ((uint16_t)bytes[5] << 8) : 0x0201;

      event[1] = 15;
      event[5] = 0;
      event[6] = (uint8_t)(handle & 0xff);
      event[7] = (uint8_t)(handle >> 8);
      event[8] = 1;
      event[9] = 0;
      event[10] = 0;
      event[11] = 0;
      event[12] = 0;
      event[13] = 0;
      event[14] = 0;
      event[15] = 0;
      event[16] = 0;
      event_len = 17;
    }
  else if (opcode == HCI_OP_LE_SET_CIG_PARAMS)
    {
      uint8_t cig = payload_len >= 5 ? bytes[4] : 0;
      uint8_t cis = payload_len >= 20 ? bytes[19] : 1;
      uint16_t handle = 0x0200 | cis;

      event[1] = 8;
      event[5] = 0;
      event[6] = cig;
      event[7] = 1;
      event[8] = (uint8_t)(handle & 0xff);
      event[9] = (uint8_t)(handle >> 8);
      event_len = 10;
    }
  else if (opcode == HCI_OP_LE_REMOVE_CIG)
    {
      event[1] = 5;
      event[5] = 0;
      event[6] = payload_len >= 5 ? bytes[4] : 0;
      event_len = 7;
    }
  else if (opcode == HCI_OP_READ_LOCAL_VERSION)
    {
      event[1] = 12;
      event[5] = 0;
      event[6] = 0x09; /* Bluetooth 5.0 */
      event[7] = 0x01;
      event[8] = 0x00;
      event[9] = 0x09; /* LMP 5.0 */
      event[10] = 0xff;
      event[11] = 0xff;
      event[12] = 0x01;
      event[13] = 0x00;
      event_len = 14;
    }
  else if (opcode == HCI_OP_READ_BD_ADDR)
    {
      event[1] = 10;
      event[5] = 0;
      event[6] = hdev->bdaddr.b[0];
      event[7] = hdev->bdaddr.b[1];
      event[8] = hdev->bdaddr.b[2];
      event[9] = hdev->bdaddr.b[3];
      event[10] = hdev->bdaddr.b[4];
      event[11] = hdev->bdaddr.b[5];
      event_len = 12;
    }
  else if (opcode == HCI_OP_READ_LOCAL_NAME)
    {
      event[1] = 252;
      event[5] = 0;
      memcpy(&event[6], "NuttX Linux BT hwsim", 20);
      event_len = 254;
    }
  else if (opcode == HCI_OP_READ_LOCAL_COMMANDS)
    {
      event[1] = 68;
      event[5] = 0;
      memcpy(&event[6], hdev->commands, 64);
      event_len = 70;
    }
  else if (opcode == HCI_OP_READ_LOCAL_FEATURES)
    {
      event[1] = 12;
      event[5] = 0;
      memcpy(&event[6], hdev->features[0], 8);
      event_len = 14;
    }
  else if (opcode == HCI_OP_READ_LOCAL_EXT_FEATURES)
    {
      event[1] = 14;
      event[5] = 0;
      event[6] = payload_len >= 5 ? bytes[4] : 0;
      event[7] = 0;
      event_len = 16;
    }
  else if (opcode == HCI_OP_READ_BUFFER_SIZE)
    {
      event[1] = 8;
      event[5] = 0;
      event[6] = (uint8_t)(hdev->acl_mtu & 0xff);
      event[7] = (uint8_t)(hdev->acl_mtu >> 8);
      event[8] = (uint8_t)(hdev->sco_mtu & 0xff);
      event[9] = (uint8_t)(hdev->acl_pkts & 0xff);
      event[10] = (uint8_t)(hdev->acl_pkts >> 8);
      event[11] = (uint8_t)(hdev->sco_pkts & 0xff);
      event[12] = (uint8_t)(hdev->sco_pkts >> 8);
      event_len = 13;
    }
  else if (opcode == HCI_OP_LE_READ_BUFFER_SIZE)
    {
      event[1] = 7;
      event[5] = 0;
      event[6] = (uint8_t)(hdev->le_mtu & 0xff);
      event[7] = (uint8_t)(hdev->le_mtu >> 8);
      event[8] = (uint8_t)(hdev->le_pkts & 0xff);
      event_len = 9;
    }
  else if (opcode == HCI_OP_LE_READ_ADV_TX_POWER)
    {
      event[1] = 5;
      event[5] = 0;
      event[6] = 0;
      event_len = 7;
    }
  else if (opcode == HCI_OP_LE_READ_LOCAL_FEATURES)
    {
      event[1] = 12;
      event[5] = 0;
      memcpy(&event[6], hdev->le_features, 8);
      event_len = 14;
    }
  else if (opcode == HCI_OP_LE_READ_SUPPORTED_STATES)
    {
      static const uint8_t default_le_states[8] =
      {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
      };
      const uint8_t *states = hdev->le_states;
      uint8_t empty = 0;
      unsigned int i;

      for (i = 0; i < 8; i++)
        {
          empty |= hdev->le_states[i];
        }

      if (empty == 0)
        {
          states = default_le_states;
        }

      event[1] = 12;
      event[5] = 0;
      memcpy(&event[6], states, 8);
      event_len = 14;
    }
  else if (opcode == HCI_OP_LE_READ_DEF_DATA_LEN)
    {
      event[1] = 8;
      event[5] = 0;
      event[6] = 251 & 0xff;
      event[7] = 251 >> 8;
      event[8] = 0x48;
      event[9] = 0x08;
      event_len = 10;
    }
  else if (opcode == HCI_OP_LE_READ_ACCEPT_LIST_SIZE)
    {
      event[1] = 3;
      event[5] = 0;
      event[6] = hdev->le_accept_list_size != 0 ?
                 hdev->le_accept_list_size : 8;
      event_len = 7;
    }
  else if (opcode == HCI_OP_LE_READ_RESOLV_LIST_SIZE)
    {
      event[1] = 3;
      event[5] = 0;
      event[6] = hdev->le_resolv_list_size != 0 ?
                 hdev->le_resolv_list_size : 8;
      event_len = 7;
    }
  else if (opcode == HCI_OP_LE_READ_MAX_DATA_LEN)
    {
      event[1] = 12;
      event[5] = 0;
      event[6] = 251 & 0xff;
      event[7] = 251 >> 8;
      event[8] = 0x48;
      event[9] = 0x08;
      event[10] = 251 & 0xff;
      event[11] = 251 >> 8;
      event[12] = 0x48;
      event[13] = 0x08;
      event_len = 14;
    }
  else if (opcode == HCI_OP_LE_READ_NUM_SUPPORTED_ADV_SETS)
    {
      event[1] = 3;
      event[5] = 0;
      event[6] = hdev->le_num_of_adv_sets != 0 ?
                 hdev->le_num_of_adv_sets : 1;
      event_len = 7;
    }
  else if (opcode == HCI_OP_LE_READ_TRANSMIT_POWER)
    {
      event[1] = 6;
      event[5] = 0;
      event[6] = (uint8_t)-20;
      event[7] = 10;
      event_len = 8;
    }

  if ((event[0] == HCI_EV_CMD_COMPLETE && event[5] == 0) ||
      (event[0] == HCI_EV_CMD_STATUS && event[2] == 0))
    {
      linux_bt_upstream_hci_user_apply_cmd(opcode, bytes, payload_len);
    }

  skb = bt_skb_alloc(event_len, GFP_KERNEL);
  if (skb == NULL)
    {
      hci_dev_put(hdev);
      return -ENOMEM;
    }

  hci_skb_pkt_type(skb) = HCI_EVENT_PKT;
  skb_put_data(skb, event, event_len);
  ret = hci_recv_frame(hdev, skb);
  if (ret >= 0 &&
      ((event[0] == HCI_EV_CMD_COMPLETE && event[5] == 0) ||
       (event[0] == HCI_EV_CMD_STATUS && event[2] == 0)) &&
      (opcode == HCI_OP_LE_CREATE_CIS ||
       opcode == HCI_OP_LE_CREATE_BIG ||
       opcode == HCI_OP_LE_TERM_BIG))
    {
      (void)linux_bt_upstream_hci_socket_inject_iso_meta_event(hdev,
                                                               opcode,
                                                               bytes,
                                                               payload_len);
    }

  hci_dev_put(hdev);
  return ret < 0 ? ret : 1;
}

int linux_bt_upstream_hci_socket_send(void *handle, const void *payload,
                                      size_t payload_len)
{
  struct linux_bt_upstream_hci_socket_handle *h = handle;
  struct msghdr msg;
  const uint8_t *bytes = payload;
  uint16_t opcode = 0;
  int ret;

  if (h == NULL || payload == NULL || payload_len == 0)
    {
      return -EINVAL;
    }

  if (h->sock.ops == NULL || h->sock.ops->sendmsg == NULL)
    {
      return -EOPNOTSUPP;
    }

  if (payload_len >= 4 && bytes[0] == HCI_COMMAND_PKT)
    {
      opcode = (uint16_t)bytes[1] | ((uint16_t)bytes[2] << 8);
    }

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  ret = h->sock.ops->sendmsg(&h->sock, &msg, payload_len);
  if (ret >= 0 || ret == -EINPROGRESS)
    {
      linux_bt_upstream_hci_socket_inject_cmd_complete(h, bytes,
                                                       payload_len);
      if (h->channel == HCI_CHANNEL_USER &&
          h->dev != HCI_DEV_NONE &&
          opcode == HCI_OP_LE_SET_SCAN_ENABLE &&
          payload_len >= 5 &&
          bytes[4] != 0)
        {
          struct hci_dev *hdev = hci_dev_get(h->dev);

          if (hdev != NULL)
            {
              (void)linux_bt_upstream_hci_user_scan_medium_poll(hdev);
              hci_dev_put(hdev);
            }
        }
    }

  return ret == -EINPROGRESS ? (int)payload_len : ret;
}

int linux_bt_upstream_hci_socket_recv(void *handle, void *payload,
                                      size_t payload_len, int *flags)
{
  struct linux_bt_upstream_hci_socket_handle *h = handle;
  struct msghdr msg;
  int ret;

  if (h == NULL || payload == NULL || payload_len == 0)
    {
      return -EINVAL;
    }

  if (h->sock.ops == NULL || h->sock.ops->recvmsg == NULL)
    {
      return -EOPNOTSUPP;
    }

  if (flags != NULL)
    {
      *flags = 0;
    }

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  ret = h->sock.ops->recvmsg(&h->sock, &msg, payload_len, 0);
  if (ret >= 0 && flags != NULL)
    {
      *flags = msg.msg_flags;
    }

  return ret;
}

int linux_bt_upstream_hci_socket_setsockopt(void *handle, int level,
                                            int option,
                                            const void *value,
                                            unsigned int value_len)
{
  struct linux_bt_upstream_hci_socket_handle *h = handle;

  if (h == NULL || value == NULL)
    {
      return -EINVAL;
    }

  return h->sock.ops != NULL && h->sock.ops->setsockopt != NULL ?
         h->sock.ops->setsockopt(&h->sock, level, option, value,
                                 value_len) : -EOPNOTSUPP;
}

int linux_bt_upstream_hci_socket_getsockopt(void *handle, int level,
                                            int option, void *value,
                                            int *value_len)
{
  struct linux_bt_upstream_hci_socket_handle *h = handle;

  if (h == NULL || value == NULL || value_len == NULL)
    {
      return -EINVAL;
    }

  return h->sock.ops != NULL && h->sock.ops->getsockopt != NULL ?
         h->sock.ops->getsockopt(&h->sock, level, option, value,
                                 value_len) : -EOPNOTSUPP;
}

int linux_bt_upstream_hci_socket_close(void *handle)
{
  struct linux_bt_upstream_hci_socket_handle *h = handle;
  uint16_t channel;
  uint16_t dev;

  if (h == NULL)
    {
      return -EINVAL;
    }

  channel = h->channel;
  dev = h->dev;

  if (h->sock.ops != NULL && h->sock.ops->release != NULL)
    {
      h->sock.ops->release(&h->sock);
    }

  if (channel == HCI_CHANNEL_USER)
    {
      linux_bt_upstream_hci_user_iso_sync_controller_conn(dev);
    }

  kmm_free(h);
  return 0;
}

int linux_bt_upstream_l2cap_socket_open(uint16_t psm, uint16_t cid,
                                        uint16_t handle, void **out_handle)
{
  const struct net_proto_family *family = NULL;
  struct linux_bt_upstream_l2cap_socket_handle *h;
  struct linux_bt_l2cap_bind_state *state;
  struct sockaddr_l2 laddr;
  int ret;

  if (out_handle == NULL)
    {
      return -EINVAL;
    }

  *out_handle = NULL;

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  h = kmm_zalloc(sizeof(*h));
  if (h == NULL)
    {
      return -ENOMEM;
    }

  h->sock.type = SOCK_SEQPACKET;
  h->sock.file = &h->file;
  h->file.private_data = h;
  ret = family->create(&init_net, &h->sock, BTPROTO_L2CAP, 0);
  if (ret < 0)
    {
      kmm_free(h);
      return ret;
    }

  memset(&laddr, 0, sizeof(laddr));
  laddr.l2_family = AF_BLUETOOTH;
  laddr.l2_psm = cpu_to_le16(psm);
  laddr.l2_cid = psm == 0 ? cpu_to_le16(cid) : 0;
  laddr.l2_bdaddr_type = 0;
  bacpy(&laddr.l2_bdaddr, BDADDR_ANY);

  ret = h->sock.ops != NULL && h->sock.ops->bind != NULL ?
        h->sock.ops->bind(&h->sock, (struct sockaddr *)&laddr,
                          sizeof(laddr)) : -EOPNOTSUPP;
  if (ret < 0)
    {
#ifdef CONFIG_SIM_BTHWSIM
      if (ret == -EADDRINUSE && cid != 0)
        {
          ret = 0;
        }
      else
#endif
      {
      if (h->sock.ops != NULL && h->sock.ops->release != NULL)
        {
          h->sock.ops->release(&h->sock);
        }

      kmm_free(h);
      return ret;
      }
    }

  state = linux_bt_l2cap_ensure_bound_state(h->sock.sk);
  if (state != NULL)
    {
      state->psm = psm;
      state->cid = cid;
      state->handle = handle;
      state->bdaddr_type = 0;
      bacpy(&state->bdaddr, BDADDR_ANY);
      if (h->sock.sk->sk_state == BT_CONNECTED)
        {
          (void)linux_bt_hci_ensure_connected_handle(
            ACL_LINK, state->handle, &state->bdaddr, state->bdaddr_type,
            true, 0);
        }
    }

  h->psm = psm;
  h->cid = cid;
  h->handle = handle;
  h->bound = true;
  *out_handle = h;
  g_l2cap_socket_binds++;
  return 0;
}

int linux_bt_upstream_l2cap_socket_connect_handle(void *handle,
                                                  uint16_t psm,
                                                  uint16_t cid)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  struct linux_bt_l2cap_bind_state *state;

  if (h == NULL)
    {
      return -EINVAL;
    }

  if (psm != 0)
    {
      h->psm = psm;
    }

  if (cid != 0)
    {
      h->cid = cid;
    }

  state = linux_bt_l2cap_ensure_bound_state(h->sock.sk);
  if (state != NULL)
    {
      state->psm = h->psm;
      state->cid = h->cid;
      state->handle = h->handle;
      state->bdaddr_type = 0;
      bacpy(&state->bdaddr, BDADDR_ANY);
    }

  if (h->sock.sk != NULL)
    {
      bdaddr_t any;

      bacpy(&any, BDADDR_ANY);
      h->sock.sk->sk_state = BT_CONNECTED;
      (void)linux_bt_hci_ensure_connected_handle(
        ACL_LINK, h->handle, state != NULL ? &state->bdaddr : &any,
        0, true, 0);
    }

  h->connected = true;
  linux_bt_upstream_l2cap_note_pending_accept(h);
  g_l2cap_socket_connects++;
  return 0;
}

int linux_bt_upstream_l2cap_socket_listen_handle(void *handle, int backlog)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  int ret;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->listen == NULL)
    {
      return -EOPNOTSUPP;
    }

  ret = h->sock.ops->listen(&h->sock, backlog);
  if (ret >= 0)
    {
      h->bound = true;
      h->listening = true;
      (void)linux_bt_upstream_l2cap_register_listener(h);
      g_l2cap_socket_listens++;
    }

  return ret;
}

int linux_bt_upstream_l2cap_socket_accept_handle(void *handle,
                                                 void **out_handle)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  struct linux_bt_upstream_l2cap_socket_handle *accepted;
  struct proto_accept_arg arg;
  int ret;

  if (out_handle == NULL)
    {
      return -EINVAL;
    }

  *out_handle = NULL;
  if (h == NULL || h->sock.ops == NULL || h->sock.ops->accept == NULL)
    {
      return -EOPNOTSUPP;
    }

  accepted = kmm_zalloc(sizeof(*accepted));
  if (accepted == NULL)
    {
      return -ENOMEM;
    }

  accepted->sock.type = h->sock.type;
  accepted->sock.file = &accepted->file;
  accepted->file.private_data = accepted;
  memset(&arg, 0, sizeof(arg));
  ret = h->sock.ops->accept(&h->sock, &accepted->sock, &arg);
  if (ret < 0)
    {
      if (ret != -EAGAIN || h->pending_accepts == 0)
        {
          kmm_free(accepted);
          return ret;
        }

      accepted->sock.state = SS_CONNECTED;
    }

  accepted->psm = h->psm;
  accepted->cid = h->cid;
  accepted->handle = h->handle;
  accepted->bound = true;
  accepted->connected = true;
  if (h->pending_accepts > 0)
    {
      h->pending_accepts--;
    }

  *out_handle = accepted;
  return OK;
}

int linux_bt_upstream_l2cap_socket_getsockopt_handle(void *handle,
                                                     int level,
                                                     int optname,
                                                     void *optval,
                                                     int *optlen)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  struct l2cap_conninfo cinfo;
  size_t copy_len;

  if (h == NULL || optval == NULL || optlen == NULL ||
      h->sock.ops == NULL || h->sock.ops->getsockopt == NULL)
    {
      return -EINVAL;
    }

  if (level == SOL_L2CAP && optname == L2CAP_CONNINFO)
    {
      if (!h->connected)
        {
          return -ENOTCONN;
        }

      if (*optlen < 0)
        {
          return -EINVAL;
        }

      memset(&cinfo, 0, sizeof(cinfo));
      cinfo.hci_handle = h->handle;
      copy_len = (size_t)*optlen < sizeof(cinfo) ?
                 (size_t)*optlen : sizeof(cinfo);
      memcpy(optval, &cinfo, copy_len);
      *optlen = (int)copy_len;
      return 0;
    }

  return h->sock.ops->getsockopt(&h->sock, level, optname,
                                 (char *)optval, optlen);
}

int linux_bt_upstream_l2cap_socket_setsockopt_handle(void *handle,
                                                     int level,
                                                     int optname,
                                                     const void *optval,
                                                     unsigned int optlen)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;

  if (h == NULL || optval == NULL ||
      h->sock.ops == NULL || h->sock.ops->setsockopt == NULL)
    {
      return -EINVAL;
    }

  return h->sock.ops->setsockopt(&h->sock, level, optname,
                                 (char *)optval, optlen);
}

int linux_bt_upstream_l2cap_socket_option_probe(void *handle,
                                                char *out,
                                                size_t out_len)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  struct l2cap_options opts;
  struct l2cap_options read_opts;
  struct bt_power pwr;
  struct bt_power read_pwr;
  uint32_t flushable = BT_FLUSHABLE_ON;
  uint32_t policy = BT_CHANNEL_POLICY_BREDR_PREFERRED;
  uint32_t read_flushable = 0;
  uint32_t read_policy = 0xffffffff;
  uint32_t lm;
  uint32_t read_lm = 0;
  int read_len;
  int set_flushable_ret;
  int get_flushable_ret;
  int set_power_ret;
  int get_power_ret;
  int set_policy_ret;
  int get_policy_ret;
  int set_options_ret;
  int get_options_ret;
  int set_lm_ret;
  int get_lm_ret;

  if (h == NULL || out == NULL || out_len == 0 || h->sock.ops == NULL)
    {
      return -EINVAL;
    }

  if (h->sock.ops->setsockopt == NULL ||
      h->sock.ops->getsockopt == NULL)
    {
      snprintf(out, out_len,
               "upstream-l2cap-options: unsupported\n");
      return -EOPNOTSUPP;
    }

  memset(&opts, 0, sizeof(opts));
  opts.omtu = 247;
  opts.imtu = 247;
  opts.flush_to = L2CAP_DEFAULT_FLUSH_TO;
  opts.mode = L2CAP_MODE_BASIC;
  opts.fcs = L2CAP_FCS_CRC16;
  opts.max_tx = L2CAP_DEFAULT_MAX_TX;
  opts.txwin_size = L2CAP_DEFAULT_TX_WINDOW;

  set_options_ret = h->sock.ops->setsockopt(&h->sock, SOL_L2CAP,
                                            L2CAP_OPTIONS,
                                            (char *)&opts,
                                            sizeof(opts));

  memset(&read_opts, 0, sizeof(read_opts));
  read_len = sizeof(read_opts);
  get_options_ret = h->sock.ops->getsockopt(&h->sock, SOL_L2CAP,
                                            L2CAP_OPTIONS,
                                            (char *)&read_opts,
                                            &read_len);

  lm = L2CAP_LM_AUTH | L2CAP_LM_ENCRYPT;
  set_lm_ret = h->sock.ops->setsockopt(&h->sock, SOL_L2CAP,
                                       L2CAP_LM, (char *)&lm,
                                       sizeof(lm));
  read_len = sizeof(read_lm);
  get_lm_ret = h->sock.ops->getsockopt(&h->sock, SOL_L2CAP,
                                       L2CAP_LM, (char *)&read_lm,
                                       &read_len);

  set_flushable_ret = h->sock.ops->setsockopt(&h->sock, SOL_BLUETOOTH,
                                              BT_FLUSHABLE,
                                              (char *)&flushable,
                                              sizeof(flushable));
  read_len = sizeof(read_flushable);
  get_flushable_ret = h->sock.ops->getsockopt(&h->sock, SOL_BLUETOOTH,
                                              BT_FLUSHABLE,
                                              (char *)&read_flushable,
                                              &read_len);

  memset(&pwr, 0, sizeof(pwr));
  pwr.force_active = BT_POWER_FORCE_ACTIVE_ON;
  set_power_ret = h->sock.ops->setsockopt(&h->sock, SOL_BLUETOOTH,
                                          BT_POWER, (char *)&pwr,
                                          sizeof(pwr));
  memset(&read_pwr, 0, sizeof(read_pwr));
  read_len = sizeof(read_pwr);
  get_power_ret = h->sock.ops->getsockopt(&h->sock, SOL_BLUETOOTH,
                                          BT_POWER, (char *)&read_pwr,
                                          &read_len);

  set_policy_ret = h->sock.ops->setsockopt(&h->sock, SOL_BLUETOOTH,
                                           BT_CHANNEL_POLICY,
                                           (char *)&policy,
                                           sizeof(policy));
  read_len = sizeof(read_policy);
  get_policy_ret = h->sock.ops->getsockopt(&h->sock, SOL_BLUETOOTH,
                                           BT_CHANNEL_POLICY,
                                           (char *)&read_policy,
                                           &read_len);

  snprintf(out, out_len,
           "upstream-l2cap-options: opt=L2CAP_OPTIONS set-ret=%d "
           "get-ret=%d imtu=%u omtu=%u mode=%u fcs=%u opt=L2CAP_LM "
           "lm-set-ret=%d lm-get-ret=%d lm=0x%08" PRIx32 " "
           "opt=BT_FLUSHABLE set-ret=%d get-ret=%d value=%" PRIu32 " "
           "opt=BT_POWER set-ret=%d get-ret=%d force-active=%u "
           "opt=BT_CHANNEL_POLICY set-ret=%d get-ret=%d policy=%" PRIu32
           "\n",
           set_options_ret, get_options_ret, read_opts.imtu,
           read_opts.omtu, read_opts.mode, read_opts.fcs, set_lm_ret,
           get_lm_ret, read_lm, set_flushable_ret, get_flushable_ret,
           read_flushable, set_power_ret, get_power_ret,
           read_pwr.force_active, set_policy_ret, get_policy_ret,
           read_policy);

  return set_options_ret < 0 ? set_options_ret :
         get_options_ret < 0 ? get_options_ret :
         set_lm_ret < 0 ? set_lm_ret :
         get_lm_ret < 0 ? get_lm_ret :
         set_flushable_ret < 0 ? set_flushable_ret :
         get_flushable_ret < 0 ? get_flushable_ret :
         set_power_ret < 0 ? set_power_ret :
         get_power_ret < 0 ? get_power_ret :
         set_policy_ret == -EOPNOTSUPP && get_policy_ret == 0 ? 0 :
         set_policy_ret < 0 ? set_policy_ret :
         get_policy_ret;
}

int linux_bt_upstream_l2cap_socket_connected_option_probe(void *handle,
                                                          char *out,
                                                          size_t out_len)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  struct l2cap_options opts;
  struct l2cap_conninfo cinfo;
  uint32_t bredr_phy = BT_PHY_BR_1M_1SLOT | BT_PHY_EDR_2M_1SLOT;
  uint32_t current_phy = 0;
  uint32_t invalid_le_phy = BT_PHY_LE_1M_TX | BT_PHY_LE_1M_RX;
  uint32_t read_phy = 0;
  int get_phy_ret;
  int invalid_le_ret;
  int read_len;
  int cinfo_len;
  int cinfo_ret;
  int set_bredr_phy_ret;
  uint8_t mode = BT_MODE_BASIC;
  const char *mode_gate;
  int set_options_ret;
  int set_mode_ret;

  if (h == NULL || out == NULL || out_len == 0 || h->sock.ops == NULL)
    {
      return -EINVAL;
    }

  if (h->sock.ops->setsockopt == NULL ||
      h->sock.ops->getsockopt == NULL)
    {
      snprintf(out, out_len,
               "upstream-l2cap-connected-options: unsupported\n");
      return -EOPNOTSUPP;
    }

  memset(&opts, 0, sizeof(opts));
  opts.omtu = 247;
  opts.imtu = 247;
  opts.flush_to = L2CAP_DEFAULT_FLUSH_TO;
  opts.mode = L2CAP_MODE_BASIC;
  opts.fcs = L2CAP_FCS_CRC16;
  opts.max_tx = L2CAP_DEFAULT_MAX_TX;
  opts.txwin_size = L2CAP_DEFAULT_TX_WINDOW;

  set_options_ret = h->sock.ops->setsockopt(&h->sock, SOL_L2CAP,
                                            L2CAP_OPTIONS,
                                            (char *)&opts,
                                            sizeof(opts));
  set_mode_ret = h->sock.ops->setsockopt(&h->sock, SOL_BLUETOOTH,
                                         BT_MODE, (char *)&mode,
                                         sizeof(mode));
  mode_gate = set_mode_ret == -ENOPROTOOPT ? "ecred-disabled" :
              set_mode_ret == -EINVAL ? "state-checked" :
              "unexpected";
  memset(&cinfo, 0, sizeof(cinfo));
  cinfo_len = sizeof(cinfo);
  cinfo_ret = linux_bt_upstream_l2cap_socket_getsockopt_handle(
                h, SOL_L2CAP, L2CAP_CONNINFO, &cinfo, &cinfo_len);
  read_len = sizeof(current_phy);
  get_phy_ret = h->sock.ops->getsockopt(&h->sock, SOL_BLUETOOTH,
                                        BT_PHY, (char *)&current_phy,
                                        &read_len);
  set_bredr_phy_ret = h->sock.ops->setsockopt(&h->sock, SOL_BLUETOOTH,
                                              BT_PHY,
                                              (char *)&bredr_phy,
                                              sizeof(bredr_phy));
  read_len = sizeof(read_phy);
  if (h->sock.ops->getsockopt(&h->sock, SOL_BLUETOOTH, BT_PHY,
                              (char *)&read_phy, &read_len) < 0)
    {
      read_phy = 0;
    }

  invalid_le_ret = h->sock.ops->setsockopt(&h->sock, SOL_BLUETOOTH,
                                           BT_PHY,
                                           (char *)&invalid_le_phy,
                                           sizeof(invalid_le_phy));

  snprintf(out, out_len,
           "upstream-l2cap-connected-options: opt=L2CAP_OPTIONS "
           "set-connected-ret=%d opt=BT_MODE set-connected-ret=%d "
           "bt-mode-gate=%s conninfo-ret=%d conninfo-handle=0x%04x "
           "conninfo-dev-class=%02x:%02x:%02x "
           "opt=BT_PHY get-ret=%d current=0x%08" PRIx32 " "
           "set-bredr-ret=%d after=0x%08" PRIx32 " "
           "invalid-le-ret=%d "
           "gate=hci-conn-set-phy-type-check\n",
           set_options_ret, set_mode_ret, mode_gate, cinfo_ret,
           cinfo.hci_handle, cinfo.dev_class[0], cinfo.dev_class[1],
           cinfo.dev_class[2], get_phy_ret, current_phy,
           set_bredr_phy_ret, read_phy, invalid_le_ret);

  return set_options_ret == -EINVAL &&
         (set_mode_ret == -EINVAL ||
          set_mode_ret == -ENOPROTOOPT) &&
         cinfo_ret == 0 &&
         get_phy_ret == 0 &&
         set_bredr_phy_ret == 0 &&
         invalid_le_ret == -EINVAL ? 0 : -EINVAL;
}

int linux_bt_upstream_l2cap_socket_write_handle(void *handle,
                                                const void *payload,
                                                size_t payload_len,
                                                char *out,
                                                size_t out_len)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  struct linux_bt_l2cap_bind_state *state;
  bdaddr_t any;
  struct msghdr msg;
  int send_ret;
#ifdef CONFIG_SIM_BTHWSIM
  int native_ret;
  int attach_ret;
#endif

  if (out == NULL || out_len == 0 ||
      (payload == NULL && payload_len > 0))
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->sendmsg == NULL)
    {
      snprintf(out, out_len, "upstream-l2cap-write-handle: not-bound\n");
      return -ENOTCONN;
    }

  state = linux_bt_l2cap_ensure_bound_state(h->sock.sk);
  if (state != NULL)
    {
      state->psm = h->psm;
      state->cid = h->cid;
      state->handle = h->handle;
      if (bacmp(&state->bdaddr, BDADDR_ANY) != 0)
        {
          (void)linux_bt_hci_ensure_connected_handle(
            state->bdaddr_type != 0 ? LE_LINK : ACL_LINK,
            h->handle, &state->bdaddr, state->bdaddr_type, true, 0);
        }
    }

  bacpy(&any, BDADDR_ANY);
  (void)linux_bt_hci_ensure_connected_handle(
    ACL_LINK, h->handle, state != NULL ? &state->bdaddr : &any,
    state != NULL ? state->bdaddr_type : 0, true, 0);

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
#ifdef CONFIG_SIM_BTHWSIM
  attach_ret = linux_bt_l2cap_attach_send_chan(&h->sock, h->handle, h->cid);
#endif
  send_ret = h->sock.ops->sendmsg(&h->sock, &msg, payload_len);
#ifdef CONFIG_SIM_BTHWSIM
  native_ret = send_ret;
  if (send_ret >= 0)
    {
      g_l2cap_socket_sends++;
      linux_bt_l2cap_pending_store(h->handle, h->cid, payload,
                                   payload_len);
    }

  snprintf(out, out_len,
           "upstream-l2cap-write-handle: psm=0x%04x cid=0x%04x "
           "handle=0x%04x payload-len=%u send-ret=%d native-ret=%d "
           "attach-ret=%d native-path=1\n",
           h->psm, h->cid, h->handle, (unsigned int)payload_len,
           send_ret, native_ret, attach_ret);
#else
  if (send_ret >= 0)
    {
      g_l2cap_socket_sends++;
    }

  snprintf(out, out_len,
           "upstream-l2cap-write-handle: psm=0x%04x cid=0x%04x "
           "handle=0x%04x payload-len=%u send-ret=%d\n",
           h->psm, h->cid, h->handle, (unsigned int)payload_len,
           send_ret);
#endif
  return send_ret < 0 ? send_ret : 0;
}

int linux_bt_upstream_l2cap_socket_recv_handle(void *handle, size_t max_len,
                                               char *out, size_t out_len)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  struct linux_bt_l2cap_bind_state *state;
  bdaddr_t any;
  struct msghdr msg;
  uint8_t buf[512];
  char hex[512 * 3];
  int recv_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->recvmsg == NULL)
    {
      snprintf(out, out_len, "upstream-l2cap-recv-handle: not-bound\n");
      return -ENOTCONN;
    }

  state = linux_bt_l2cap_ensure_bound_state(h->sock.sk);
  if (state != NULL)
    {
      state->psm = h->psm;
      state->cid = h->cid;
      state->handle = h->handle;
    }

  bacpy(&any, BDADDR_ANY);
  (void)linux_bt_hci_ensure_connected_handle(
    ACL_LINK, h->handle, state != NULL ? &state->bdaddr : &any,
    state != NULL ? state->bdaddr_type : 0, false, 0);

#ifdef CONFIG_SIM_BTHWSIM
  if (h->cid != 0)
    {
      (void)linux_bt_l2cap_attach_recv_chan(&h->sock, h->handle, h->cid);
    }
#endif

  if (max_len == 0 || max_len > sizeof(buf))
    {
      max_len = sizeof(buf);
    }

  memset(&msg, 0, sizeof(msg));
  memset(buf, 0, sizeof(buf));
  msg.msg_iter.data = (const char *)buf;
  msg.msg_iter.count = max_len;
  recv_ret = h->sock.ops->recvmsg(&h->sock, &msg, max_len, 0);
#ifdef CONFIG_SIM_BTHWSIM
  if (recv_ret == -EAGAIN)
    {
      unsigned int poll_iter;

      for (poll_iter = 0; poll_iter < 16 && recv_ret == -EAGAIN;
           poll_iter++)
        {
          size_t pending_len = 0;
          int poll_ret;

          poll_ret = linux_bt_sim_l2cap_poll_one();
          if (linux_bt_l2cap_pending_pop(h->handle, h->cid, buf, max_len,
                                         &pending_len))
            {
              linux_bt_upstream_format_hex(buf, pending_len, hex,
                                           sizeof(hex));
              g_l2cap_socket_recvs++;
              snprintf(out, out_len,
                       "upstream-l2cap-recv-handle: psm=0x%04x "
                       "cid=0x%04x handle=0x%04x recv-ret=%u "
                       "flags=0x0 payload=%s pending=1 poll=%u\n",
                       h->psm, h->cid, h->handle,
                       (unsigned int)pending_len, hex, poll_iter + 1);
              return 0;
            }

          memset(&msg, 0, sizeof(msg));
          memset(buf, 0, sizeof(buf));
          msg.msg_iter.data = (const char *)buf;
          msg.msg_iter.count = max_len;
          recv_ret = h->sock.ops->recvmsg(&h->sock, &msg, max_len, 0);
          if (poll_ret <= 0)
            {
              break;
            }
        }
    }
#endif
  if (recv_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-l2cap-recv-handle: psm=0x%04x cid=0x%04x "
               "handle=0x%04x recv-ret=%d flags=0x%x\n",
               h->psm, h->cid, h->handle, recv_ret, msg.msg_flags);
      return recv_ret;
    }

  linux_bt_l2cap_pending_drop_match(h->handle, h->cid, buf,
                                    (size_t)recv_ret);
  linux_bt_upstream_format_hex(buf, (size_t)recv_ret, hex, sizeof(hex));
  g_l2cap_socket_recvs++;
  snprintf(out, out_len,
           "upstream-l2cap-recv-handle: psm=0x%04x cid=0x%04x "
           "handle=0x%04x recv-ret=%d flags=0x%x payload=%s\n",
           h->psm, h->cid, h->handle, recv_ret, msg.msg_flags, hex);
  return 0;
}

int linux_bt_upstream_l2cap_socket_poll_handle(void *handle, short events,
                                               short *revents)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;

  if (h == NULL)
    {
      return -EINVAL;
    }

  return linux_bt_upstream_socket_poll_handle(&h->sock, events, revents);
}

int linux_bt_upstream_l2cap_socket_shutdown_handle(void *handle, int how)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->shutdown == NULL)
    {
      return -EOPNOTSUPP;
    }

  return h->sock.ops->shutdown(&h->sock, how);
}

int linux_bt_upstream_l2cap_socket_ioctl_handle(void *handle,
                                                unsigned int cmd,
                                                unsigned long arg)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->ioctl == NULL)
    {
      return -EOPNOTSUPP;
    }

  return h->sock.ops->ioctl(&h->sock, cmd, arg);
}

int linux_bt_upstream_l2cap_socket_ioctl_probe(void *handle,
                                               char *out,
                                               size_t out_len)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  int inq = -1;
  int outq = -1;
  int inq_ret;
  int outq_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.sk == NULL || h->sock.ops == NULL ||
      h->sock.ops->ioctl == NULL)
    {
      snprintf(out, out_len, "upstream-l2cap-ioctl: unsupported\n");
      return -EOPNOTSUPP;
    }

  inq_ret = h->sock.ops->ioctl(&h->sock, TIOCINQ,
                               (unsigned long)&inq);
  outq_ret = h->sock.ops->ioctl(&h->sock, TIOCOUTQ,
                                (unsigned long)&outq);
  snprintf(out, out_len,
           "upstream-l2cap-ioctl: psm=0x%04x cid=0x%04x "
           "handle=0x%04x inq-ret=%d inq=%d outq-ret=%d outq=%d "
           "state=%d proto=BTPROTO_L2CAP\n",
           h->psm, h->cid, h->handle, inq_ret, inq, outq_ret, outq,
           h->sock.sk->sk_state);
  return inq_ret < 0 ? inq_ret : outq_ret;
}

int linux_bt_upstream_l2cap_socket_poll_probe(void *handle,
                                              int want_write,
                                              char *out,
                                              size_t out_len)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  __poll_t poll_ret;
  unsigned int ready;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.sk == NULL || h->sock.ops == NULL ||
      h->sock.ops->poll == NULL)
    {
      snprintf(out, out_len, "upstream-l2cap-poll: unsupported\n");
      return -EOPNOTSUPP;
    }

  poll_ret = h->sock.ops->poll(NULL, &h->sock, NULL);
  ready = want_write ? ((poll_ret & EPOLLOUT) != 0) :
                       ((poll_ret & EPOLLIN) != 0);
  snprintf(out, out_len,
           "upstream-l2cap-poll: psm=0x%04x cid=0x%04x "
           "handle=0x%04x events=%s poll-ret=0x%x ready=%u "
           "state=%d shutdown=0x%x proto=BTPROTO_L2CAP\n",
           h->psm, h->cid, h->handle,
           want_write ? "POLLOUT" : "POLLIN",
           (unsigned int)poll_ret, ready,
           h->sock.sk->sk_state, h->sock.sk->sk_shutdown);
  return ready ? 0 : -EAGAIN;
}

int linux_bt_upstream_l2cap_socket_timestamp_probe(void *handle,
                                                   char *out,
                                                   size_t out_len)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  struct __kernel_old_timeval tv;
  struct linux_bt_compat_timespec
  {
    long tv_sec;
    long tv_nsec;
  } ts;
  int tv_ret;
  int ts_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.sk == NULL || h->sock.ops == NULL ||
      h->sock.ops->gettstamp == NULL)
    {
      snprintf(out, out_len, "upstream-l2cap-timestamp: unsupported\n");
      return -EOPNOTSUPP;
    }

  memset(&tv, 0, sizeof(tv));
  memset(&ts, 0, sizeof(ts));
  tv_ret = h->sock.ops->gettstamp(&h->sock, &tv, true, false);
  ts_ret = h->sock.ops->gettstamp(&h->sock, &ts, false, false);
  snprintf(out, out_len,
           "upstream-l2cap-timestamp: psm=0x%04x cid=0x%04x "
           "handle=0x%04x timeval-ret=%d tv-sec=%ld tv-usec=%ld "
           "timespec-ret=%d ts-sec=%ld ts-nsec=%ld "
           "proto=BTPROTO_L2CAP\n",
           h->psm, h->cid, h->handle, tv_ret, tv.tv_sec, tv.tv_usec,
           ts_ret, ts.tv_sec, ts.tv_nsec);
  return tv_ret < 0 ? tv_ret : ts_ret;
}

static int linux_bt_upstream_l2cap_socket_release_handle(
  struct linux_bt_upstream_l2cap_socket_handle *h)
{
  int release_ret = 0;

  if (h == NULL)
    {
      return -EINVAL;
    }

  linux_bt_upstream_l2cap_unregister_listener(h);
  h->listening = false;
  linux_bt_l2cap_clear_bound_state_by_sk(h->sock.sk);
  if (h->cid == 0)
    {
      linux_bt_l2cap_clear_bound_state_by_addr(h->psm, h->cid, h->handle);
    }

  if (h->sock.ops != NULL && h->sock.ops->release != NULL)
    {
      release_ret = h->sock.ops->release(&h->sock);
    }

#ifdef CONFIG_SIM_BTHWSIM
  if (h->handle != 0 && h->cid != 0)
    {
      struct hci_dev *hdev;

      hdev = linux_bt_upstream_vhci_default_hdev();
      if (hdev != NULL)
        {
          (void)l2cap_sim_detach_cid(hdev, h->handle, h->cid);
        }
    }
#endif

  h->sock.sk = NULL;
  h->sock.ops = NULL;
  h->bound = false;
  h->connected = false;
  return release_ret;
}

int linux_bt_upstream_l2cap_socket_close_handle(void *handle)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  int pre_state;
  int pre_shutdown;
  int close_ret = 0;

  if (h == NULL)
    {
      return -EINVAL;
    }

  if (h->bnep_owned)
    {
      h->bnep_user_closed = true;
      return 0;
    }

  pre_state = h->sock.sk != NULL ? h->sock.sk->sk_state : -1;
  pre_shutdown = h->sock.sk != NULL ? h->sock.sk->sk_shutdown : 0;
  if (!h->bnep_released)
    {
      close_ret = linux_bt_upstream_l2cap_socket_release_handle(h);
    }

  printf("upstream-l2cap-shutdown-close: psm=0x%04x cid=0x%04x "
         "handle=0x%04x pre-state=%d pre-shutdown=0x%x close-ret=%d "
         "post-state=%d post-shutdown=0x%x proto=BTPROTO_L2CAP\n",
         h->psm, h->cid, h->handle, pre_state, pre_shutdown, close_ret,
         h->sock.sk != NULL ? h->sock.sk->sk_state : BT_CLOSED,
         h->sock.sk != NULL ? h->sock.sk->sk_shutdown : SHUTDOWN_MASK);
  h->file.private_data = NULL;
  kmm_free(h);
  return close_ret;
}

int linux_bt_upstream_iso_socket_open(uint8_t addr_type, uint16_t handle,
                                      void **out_handle)
{
  const struct net_proto_family *family = NULL;
  struct linux_bt_upstream_iso_socket_handle *h;
  struct linux_bt_iso_bind_state *state;
  struct linux_bt_iso_pinfo *pi;
  struct sockaddr_iso iaddr;
  uint8_t iso_addr_type;
  int ret;

  if (out_handle == NULL)
    {
      return -EINVAL;
    }

  *out_handle = NULL;

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  h = kmm_zalloc(sizeof(*h));
  if (h == NULL)
    {
      return -ENOMEM;
    }

  h->sock.type = SOCK_SEQPACKET;
  h->sock.file = &h->file;
  h->file.private_data = h;
  ret = family->create(&init_net, &h->sock, BTPROTO_ISO, 0);
  if (ret < 0)
    {
      kmm_free(h);
      return ret;
    }

  iso_addr_type = addr_type == BDADDR_BREDR ? BDADDR_LE_PUBLIC : addr_type;
  memset(&iaddr, 0, sizeof(iaddr));
  iaddr.iso_family = AF_BLUETOOTH;
  iaddr.iso_bdaddr_type = iso_addr_type;
  bacpy(&iaddr.iso_bdaddr, BDADDR_ANY);
  ret = h->sock.ops != NULL && h->sock.ops->bind != NULL ?
        h->sock.ops->bind(&h->sock, (struct sockaddr *)&iaddr,
                          sizeof(iaddr)) : -EOPNOTSUPP;
  if (ret < 0)
    {
      if (h->sock.ops != NULL && h->sock.ops->release != NULL)
        {
          h->sock.ops->release(&h->sock);
        }

      kmm_free(h);
      return ret;
    }

  state = linux_bt_iso_bound_state(h->sock.sk);
  if (state != NULL)
    {
      state->bdaddr_type = iso_addr_type;
      bacpy(&state->bdaddr, BDADDR_ANY);
    }

  pi = h->sock.sk != NULL ? linux_bt_iso_pi(h->sock.sk) : NULL;
  if (pi != NULL)
    {
      pi->handle = handle;
    }

  h->addr_type = iso_addr_type;
  h->handle = handle;
  h->bound = true;
  *out_handle = h;
  g_iso_socket_binds++;
  return 0;
}

int linux_bt_upstream_iso_socket_connect_handle(void *handle,
                                                uint8_t addr_type)
{
  struct linux_bt_upstream_iso_socket_handle *h = handle;
  struct linux_bt_iso_bind_state *state;
  struct sockaddr_iso iaddr;
  int ret = 0;

  if (h == NULL)
    {
      return -EINVAL;
    }

  h->addr_type = addr_type == BDADDR_BREDR ? BDADDR_LE_PUBLIC : addr_type;
  state = linux_bt_iso_bound_state(h->sock.sk);
  if (state != NULL)
    {
      state->bdaddr_type = h->addr_type;
      bacpy(&state->bdaddr, BDADDR_ANY);
    }

#ifdef CONFIG_SIM_BTHWSIM
  if (h->sock.sk != NULL)
    {
      h->sock.sk->sk_state = BT_CONNECTED;
      h->sock.state = SS_CONNECTED;
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_ISO
      ret = linux_bt_iso_probe_attach_connected_socket(&h->sock,
                                                       h->handle);
#else
      ret = 0;
#endif
    }
  else
#endif
    {
      memset(&iaddr, 0, sizeof(iaddr));
      iaddr.iso_family = AF_BLUETOOTH;
      iaddr.iso_bdaddr_type = h->addr_type;
      bacpy(&iaddr.iso_bdaddr, BDADDR_ANY);
      ret = h->sock.ops != NULL && h->sock.ops->connect != NULL ?
            h->sock.ops->connect(&h->sock, (struct sockaddr *)&iaddr,
                                 sizeof(iaddr), 0) : -EOPNOTSUPP;
    }

  if (ret < 0)
    {
      return ret;
    }

  h->connected = true;
  linux_bt_upstream_iso_note_pending_accept(h);
  g_iso_socket_connects++;
  return 0;
}

int linux_bt_upstream_iso_socket_listen_handle(void *handle, int backlog)
{
  struct linux_bt_upstream_iso_socket_handle *h = handle;
  int ret;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->listen == NULL)
    {
      return -EOPNOTSUPP;
    }

  ret = h->sock.ops->listen(&h->sock, backlog);
  if (ret >= 0)
    {
      h->bound = true;
      h->listening = true;
      (void)linux_bt_upstream_iso_register_listener(h);
    }

  return ret;
}

int linux_bt_upstream_iso_socket_accept_handle(void *handle,
                                               void **out_handle)
{
  struct linux_bt_upstream_iso_socket_handle *h = handle;
  struct linux_bt_upstream_iso_socket_handle *accepted;
  struct proto_accept_arg arg;
  int ret;

  if (out_handle == NULL)
    {
      return -EINVAL;
    }

  *out_handle = NULL;
  if (h == NULL || h->sock.ops == NULL || h->sock.ops->accept == NULL)
    {
      return -EOPNOTSUPP;
    }

  accepted = kmm_zalloc(sizeof(*accepted));
  if (accepted == NULL)
    {
      return -ENOMEM;
    }

  accepted->sock.type = h->sock.type;
  accepted->sock.file = &accepted->file;
  accepted->file.private_data = accepted;
  memset(&arg, 0, sizeof(arg));
  ret = h->sock.ops->accept(&h->sock, &accepted->sock, &arg);
  if (ret < 0)
    {
      if (ret != -EAGAIN || h->pending_accepts == 0)
        {
          kmm_free(accepted);
          return ret;
        }

      accepted->sock.state = SS_CONNECTED;
    }

  accepted->addr_type = h->addr_type;
  accepted->handle = h->handle;
  accepted->bound = true;
  accepted->connected = true;
  if (h->pending_accepts > 0)
    {
      h->pending_accepts--;
    }

  *out_handle = accepted;
  return OK;
}

int linux_bt_upstream_iso_socket_getsockopt_handle(void *handle,
                                                   int level,
                                                   int optname,
                                                   void *optval,
                                                   int *optlen)
{
  struct linux_bt_upstream_iso_socket_handle *h = handle;

  if (h == NULL || optval == NULL || optlen == NULL ||
      h->sock.ops == NULL || h->sock.ops->getsockopt == NULL)
    {
      return -EINVAL;
    }

  return h->sock.ops->getsockopt(&h->sock, level, optname,
                                 (char *)optval, optlen);
}

int linux_bt_upstream_iso_socket_setsockopt_handle(void *handle,
                                                   int level,
                                                   int optname,
                                                   const void *optval,
                                                   unsigned int optlen)
{
  struct linux_bt_upstream_iso_socket_handle *h = handle;

  if (h == NULL || optval == NULL ||
      h->sock.ops == NULL || h->sock.ops->setsockopt == NULL)
    {
      return -EINVAL;
    }

  return h->sock.ops->setsockopt(&h->sock, level, optname,
                                 (char *)optval, optlen);
}

int linux_bt_upstream_iso_socket_write_handle(void *handle,
                                              const void *payload,
                                              size_t payload_len,
                                              char *out,
                                              size_t out_len)
{
  struct linux_bt_upstream_iso_socket_handle *h = handle;
  struct msghdr msg;
  int send_ret;

  if (out == NULL || out_len == 0 ||
      (payload == NULL && payload_len > 0))
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->sendmsg == NULL)
    {
      snprintf(out, out_len, "upstream-iso-write-handle: not-bound\n");
      return -ENOTCONN;
    }

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  send_ret = h->sock.ops->sendmsg(&h->sock, &msg, payload_len);
  snprintf(out, out_len,
           "upstream-iso-write-handle: addr-type=%u handle=0x%04x "
           "payload-len=%u send-ret=%d native-path=1\n",
           h->addr_type, h->handle, (unsigned int)payload_len, send_ret);
  return send_ret < 0 ? send_ret : 0;
}

int linux_bt_upstream_iso_socket_recv_handle(void *handle, void *payload,
                                             size_t max_len, int *flags)
{
  struct linux_bt_upstream_iso_socket_handle *h = handle;
  struct msghdr msg;
  int recv_ret;

  if (payload == NULL && max_len > 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->recvmsg == NULL)
    {
      return -ENOTCONN;
    }

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = max_len;
  recv_ret = h->sock.ops->recvmsg(&h->sock, &msg, max_len, 0);
  if (flags != NULL)
    {
      *flags = msg.msg_flags;
    }

  return recv_ret;
}

int linux_bt_upstream_iso_socket_poll_handle(void *handle, short events,
                                             short *revents)
{
  struct linux_bt_upstream_iso_socket_handle *h = handle;

  if (h == NULL)
    {
      return -EINVAL;
    }

  return linux_bt_upstream_socket_poll_handle(&h->sock, events, revents);
}

int linux_bt_upstream_iso_socket_shutdown_handle(void *handle, int how)
{
  struct linux_bt_upstream_iso_socket_handle *h = handle;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->shutdown == NULL)
    {
      return -EOPNOTSUPP;
    }

  return h->sock.ops->shutdown(&h->sock, how);
}

int linux_bt_upstream_iso_socket_ioctl_handle(void *handle,
                                              unsigned int cmd,
                                              unsigned long arg)
{
  struct linux_bt_upstream_iso_socket_handle *h = handle;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->ioctl == NULL)
    {
      return -EOPNOTSUPP;
    }

  return h->sock.ops->ioctl(&h->sock, cmd, arg);
}

int linux_bt_upstream_iso_socket_close_handle(void *handle)
{
  struct linux_bt_upstream_iso_socket_handle *h = handle;
  int close_ret = 0;

  if (h == NULL)
    {
      return -EINVAL;
    }

  linux_bt_upstream_iso_unregister_listener(h);
  h->listening = false;
  if (h->handle != 0)
    {
      (void)linux_bt_iso_probe_detach_hci_conn(h->handle);
    }

  if (h->sock.ops != NULL && h->sock.ops->release != NULL)
    {
      close_ret = h->sock.ops->release(&h->sock);
    }
  else if (h->sock.sk != NULL)
    {
      sock_put(h->sock.sk);
    }

  h->file.private_data = NULL;
  kmm_free(h);
  return close_ret;
}

int linux_bt_upstream_rfcomm_socket_open(uint8_t channel, uint16_t handle,
                                         void **out_handle)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  const struct net_proto_family *family = NULL;
  struct linux_bt_upstream_rfcomm_socket_handle *h = NULL;
  struct linux_bt_sockaddr_rc laddr;
  struct linux_bt_sockaddr_rc name;
  uint16_t mtu = LINUX_BT_RFCOMM_DEFAULT_MTU;
  uint32_t lm = 0;
  int ret;

  if (out_handle == NULL)
    {
      return -EINVAL;
    }

  *out_handle = NULL;

  if (!linux_bt_rfcomm_valid_channel(channel))
    {
      return -EINVAL;
    }

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  h = kmm_zalloc(sizeof(*h));
  if (h == NULL)
    {
      return -ENOMEM;
    }

  h->sock.type = SOCK_STREAM;
  h->sock.file = &h->file;
  h->file.private_data = h;

  ret = family->create(&init_net, &h->sock, BTPROTO_RFCOMM, 0);
  if (ret < 0)
    {
      kmm_free(h);
      return ret;
    }

  memset(&laddr, 0, sizeof(laddr));
  laddr.rc_family = AF_BLUETOOTH;
  laddr.rc_channel = channel;
  bacpy(&laddr.rc_bdaddr, BDADDR_ANY);

  ret = h->sock.ops != NULL && h->sock.ops->bind != NULL ?
        h->sock.ops->bind(&h->sock, (struct sockaddr *)&laddr,
                          sizeof(laddr)) : -EOPNOTSUPP;
  if (ret < 0)
    {
      if (h->sock.ops != NULL && h->sock.ops->release != NULL)
        {
          h->sock.ops->release(&h->sock);
        }

      kmm_free(h);
      return ret;
    }

  if (h->sock.ops != NULL && h->sock.ops->setsockopt != NULL)
    {
      (void)h->sock.ops->setsockopt(&h->sock, SOL_RFCOMM, SO_RFCOMM_MTU,
                                    (char *)&mtu, sizeof(mtu));
      (void)h->sock.ops->setsockopt(&h->sock, SOL_RFCOMM, SO_RFCOMM_LM,
                                    (char *)&lm, sizeof(lm));
    }

  if (h->sock.ops != NULL && h->sock.ops->getsockopt != NULL)
    {
      int len = sizeof(mtu);

      (void)h->sock.ops->getsockopt(&h->sock, SOL_RFCOMM, SO_RFCOMM_MTU,
                                    (char *)&mtu, &len);
      len = sizeof(lm);
      (void)h->sock.ops->getsockopt(&h->sock, SOL_RFCOMM, SO_RFCOMM_LM,
                                    (char *)&lm, &len);
    }

  if (h->sock.ops != NULL && h->sock.ops->getname != NULL)
    {
      (void)h->sock.ops->getname(&h->sock, (struct sockaddr *)&name, 0);
    }

  h->channel = channel;
  h->cid = LINUX_BT_RFCOMM_DEFAULT_CID + channel;
  h->handle = handle != 0 ? handle : LINUX_BT_RFCOMM_DEFAULT_HANDLE;
  if (h->sock.sk != NULL)
    {
      linux_bt_rfcomm_pi(h->sock.sk)->handle = h->handle;
    }

  h->bound = true;
  *out_handle = h;
  return 0;
#else
  (void)channel;
  (void)handle;
  if (out_handle != NULL)
    {
      *out_handle = NULL;
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_connect_handle(void *handle,
                                                   uint8_t channel)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  struct linux_bt_sockaddr_rc raddr;
  int ret;

  if (h == NULL)
    {
      return -EINVAL;
    }

  if (channel != 0)
    {
      if (!linux_bt_rfcomm_valid_channel(channel))
        {
          return -EINVAL;
        }

      h->channel = channel;
      h->cid = LINUX_BT_RFCOMM_DEFAULT_CID + channel;
    }

  memset(&raddr, 0, sizeof(raddr));
  raddr.rc_family = AF_BLUETOOTH;
  raddr.rc_channel = h->channel;
  bacpy(&raddr.rc_bdaddr, BDADDR_ANY);

  ret = h->sock.ops != NULL && h->sock.ops->connect != NULL ?
        h->sock.ops->connect(&h->sock, (struct sockaddr *)&raddr,
                             sizeof(raddr), 0) : -EOPNOTSUPP;
  if (ret < 0)
    {
      return ret;
    }

  if (h->sock.sk != NULL)
    {
      linux_bt_rfcomm_pi(h->sock.sk)->handle = h->handle;
    }

  h->connected = true;
  return 0;
#else
  (void)handle;
  (void)channel;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_listen_handle(void *handle, int backlog)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->listen == NULL)
    {
      return -EOPNOTSUPP;
    }

  return h->sock.ops->listen(&h->sock, backlog);
#else
  (void)handle;
  (void)backlog;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_accept_handle(void *handle,
                                                  void **out_handle)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  struct linux_bt_upstream_rfcomm_socket_handle *accepted;
  struct proto_accept_arg arg;
  int ret;

  if (out_handle == NULL)
    {
      return -EINVAL;
    }

  *out_handle = NULL;
  if (h == NULL || h->sock.ops == NULL || h->sock.ops->accept == NULL)
    {
      return -EOPNOTSUPP;
    }

  accepted = kmm_zalloc(sizeof(*accepted));
  if (accepted == NULL)
    {
      return -ENOMEM;
    }

  accepted->sock.type = h->sock.type;
  accepted->sock.file = &accepted->file;
  accepted->file.private_data = accepted;
  memset(&arg, 0, sizeof(arg));
  ret = h->sock.ops->accept(&h->sock, &accepted->sock, &arg);
  if (ret < 0)
    {
      kmm_free(accepted);
      return ret;
    }

  accepted->channel = h->channel;
  accepted->cid = h->cid;
  accepted->handle = h->handle;
  accepted->bound = true;
  accepted->connected = true;
  *out_handle = accepted;
  return OK;
#else
  (void)handle;
  (void)out_handle;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_getsockopt_handle(void *handle,
                                                      int level,
                                                      int optname,
                                                      void *optval,
                                                      int *optlen)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;

  if (h == NULL || optval == NULL || optlen == NULL ||
      h->sock.ops == NULL || h->sock.ops->getsockopt == NULL)
    {
      return -EINVAL;
    }

  return h->sock.ops->getsockopt(&h->sock, level, optname,
                                 (char *)optval, optlen);
#else
  (void)handle;
  (void)level;
  (void)optname;
  (void)optval;
  (void)optlen;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_setsockopt_handle(void *handle,
                                                      int level,
                                                      int optname,
                                                      const void *optval,
                                                      unsigned int optlen)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;

  if (h == NULL || optval == NULL ||
      h->sock.ops == NULL || h->sock.ops->setsockopt == NULL)
    {
      return -EINVAL;
    }

  return h->sock.ops->setsockopt(&h->sock, level, optname,
                                 (char *)optval, optlen);
#else
  (void)handle;
  (void)level;
  (void)optname;
  (void)optval;
  (void)optlen;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_listen_accept_probe(uint8_t channel,
                                                        uint16_t handle,
                                                        char *out,
                                                        size_t out_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h;
  struct linux_bt_rfcomm_pinfo *pi = NULL;
  struct bt_security sec;
  struct socket accepted;
  struct file accepted_file;
  struct proto_accept_arg arg;
  struct linux_bt_sockaddr_rc accepted_name;
  struct linux_bt_sockaddr_rc accepted_peer_name;
  struct linux_bt_sockaddr_rc listen_name;
  struct linux_bt_sockaddr_rc listen_peer_name;
  uint32_t defer = 1;
  uint32_t defer_state = 0;
  int opt_len;
  int accepted_name_ret = -EOPNOTSUPP;
  int accepted_peer_name_ret = -EOPNOTSUPP;
  int listen_name_ret = -EOPNOTSUPP;
  int listen_peer_name_ret = -EOPNOTSUPP;
  int open_ret;
  int sec_ret = -EOPNOTSUPP;
  int sec_get_ret = -EOPNOTSUPP;
  int defer_ret = -EOPNOTSUPP;
  int defer_get_ret = -EOPNOTSUPP;
  uint8_t sec_level = 0;
  int listen_ret = -EOPNOTSUPP;
  int accept_ret = -EOPNOTSUPP;
  int release_ret = 0;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  memset(&accepted, 0, sizeof(accepted));
  memset(&accepted_file, 0, sizeof(accepted_file));
  memset(&arg, 0, sizeof(arg));
  memset(&accepted_name, 0, sizeof(accepted_name));
  memset(&accepted_peer_name, 0, sizeof(accepted_peer_name));
  memset(&listen_name, 0, sizeof(listen_name));
  memset(&listen_peer_name, 0, sizeof(listen_peer_name));

  open_ret = linux_bt_upstream_rfcomm_socket_open(channel, handle,
                                                  (void **)&h);
  if (open_ret >= 0 && h != NULL && h->sock.ops != NULL &&
      h->sock.ops->setsockopt != NULL)
    {
      memset(&sec, 0, sizeof(sec));
      sec.level = BT_SECURITY_MEDIUM;
      sec_ret = h->sock.ops->setsockopt(&h->sock, SOL_BLUETOOTH,
                                        BT_SECURITY, (char *)&sec,
                                        sizeof(sec));
      defer_ret = h->sock.ops->setsockopt(&h->sock, SOL_BLUETOOTH,
                                          BT_DEFER_SETUP, (char *)&defer,
                                          sizeof(defer));
    }

  if (open_ret >= 0 && h != NULL && h->sock.ops != NULL &&
      h->sock.ops->getsockopt != NULL)
    {
      memset(&sec, 0, sizeof(sec));
      opt_len = sizeof(sec);
      sec_get_ret = h->sock.ops->getsockopt(&h->sock, SOL_BLUETOOTH,
                                            BT_SECURITY, (char *)&sec,
                                            &opt_len);
      sec_level = sec.level;
      opt_len = sizeof(defer_state);
      defer_get_ret = h->sock.ops->getsockopt(&h->sock, SOL_BLUETOOTH,
                                              BT_DEFER_SETUP,
                                              (char *)&defer_state,
                                              &opt_len);
    }

  if (open_ret >= 0 && h != NULL && h->sock.ops != NULL &&
      h->sock.ops->listen != NULL)
    {
      listen_ret = h->sock.ops->listen(&h->sock, 1);
      if (h->sock.ops->getname != NULL)
        {
          listen_name_ret =
            h->sock.ops->getname(&h->sock,
                                 (struct sockaddr *)&listen_name, 0);
          listen_peer_name_ret =
            h->sock.ops->getname(&h->sock,
                                 (struct sockaddr *)&listen_peer_name, 1);
        }
    }

  if (listen_ret >= 0 && h->sock.ops != NULL &&
      h->sock.ops->accept != NULL)
    {
      accepted.type = h->sock.type;
      accepted.file = &accepted_file;
      accepted_file.private_data = h;
      accept_ret = h->sock.ops->accept(&h->sock, &accepted, &arg);
      if (accept_ret >= 0 && accepted.sk != NULL)
        {
          pi = linux_bt_rfcomm_pi(accepted.sk);
          if (accepted.ops != NULL && accepted.ops->getname != NULL)
            {
              accepted_name_ret =
                accepted.ops->getname(&accepted,
                                      (struct sockaddr *)&accepted_name,
                                      0);
              accepted_peer_name_ret =
                accepted.ops->getname(&accepted,
                                      (struct sockaddr *)&accepted_peer_name,
                                      1);
            }
        }
    }

  snprintf(out, out_len,
           "upstream-rfcomm-listen-accept: channel=%u cid=0x%04x "
           "handle=0x%04x open-ret=%d listen-ret=%d accept-ret=%d "
           "accepted-state=%d getname-ret=%d getname-channel=%u "
           "peer-getname-ret=%d accepted-getname-ret=%d "
           "accepted-channel=%u accepted-peer-getname-ret=%d "
           "accepted-peer-channel=%u sec-ret=%d sec-get-ret=%d "
           "sec-level=%u "
           "defer-ret=%d defer-get-ret=%d defer=%u accepted-sec-level=%u "
           "accepted-defer=%u proto=BTPROTO_RFCOMM proto-name=%s\n",
           channel,
           pi != NULL ? pi->cid :
           (uint16_t)(LINUX_BT_RFCOMM_DEFAULT_CID + channel),
           pi != NULL ? pi->handle :
           (handle != 0 ? handle : LINUX_BT_RFCOMM_DEFAULT_HANDLE),
           open_ret, listen_ret, accept_ret,
           accepted.sk != NULL ? accepted.sk->sk_state : -1,
           listen_name_ret, listen_name.rc_channel, listen_peer_name_ret,
           accepted_name_ret, accepted_name.rc_channel,
           accepted_peer_name_ret, accepted_peer_name.rc_channel,
           sec_ret, sec_get_ret, sec_level, defer_ret, defer_get_ret,
           defer_state, pi != NULL ? pi->sec_level : 0,
           pi != NULL && pi->defer_setup ? 1 : 0,
           g_linux_bt_rfcomm_sock_proto.name);

  if (accepted.ops != NULL && accepted.ops->release != NULL)
    {
      (void)accepted.ops->release(&accepted);
    }

  if (open_ret >= 0 && h != NULL)
    {
      release_ret = linux_bt_upstream_rfcomm_socket_close_handle(h);
      if (release_ret < 0 && accept_ret >= 0)
        {
          accept_ret = release_ret;
        }
    }

  if (open_ret < 0)
    {
      return open_ret;
    }

  if (listen_ret < 0)
    {
      return listen_ret;
    }

  return accept_ret < 0 ? accept_ret : 0;
#else
  (void)channel;
  (void)handle;
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len,
               "upstream-rfcomm-listen-accept: unsupported\n");
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_write_handle(void *handle,
                                                const void *payload,
                                                size_t payload_len,
                                                char *out,
                                                size_t out_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  struct msghdr msg;
  int send_ret;

  if (out == NULL || out_len == 0 ||
      (payload == NULL && payload_len > 0))
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->sendmsg == NULL)
    {
      snprintf(out, out_len, "upstream-rfcomm-write-handle: not-bound\n");
      return -ENOTCONN;
    }

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  send_ret = h->sock.ops->sendmsg(&h->sock, &msg, payload_len);

  snprintf(out, out_len,
           "upstream-rfcomm-write-handle: channel=%u cid=0x%04x "
           "handle=0x%04x payload-len=%u send-ret=%d native-path=1 "
           "proto=BTPROTO_RFCOMM\n",
           h->channel, h->cid, h->handle, (unsigned int)payload_len,
           send_ret);
  return send_ret < 0 ? send_ret : 0;
#else
  (void)handle;
  (void)payload;
  (void)payload_len;
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len,
               "upstream-rfcomm-write-handle: unsupported\n");
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_recv_handle(void *handle, void *payload,
                                               size_t max_len,
                                               int *msg_flags,
                                               char *out, size_t out_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  struct msghdr msg;
  uint8_t buf[512];
  char hex[512 * 3];
  int recv_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->recvmsg == NULL)
    {
      snprintf(out, out_len, "upstream-rfcomm-recv-handle: not-bound\n");
      return -ENOTCONN;
    }

  if (max_len == 0 || max_len > sizeof(buf))
    {
      max_len = sizeof(buf);
    }

  memset(&msg, 0, sizeof(msg));
  memset(buf, 0, sizeof(buf));
  msg.msg_iter.data = (const char *)buf;
  msg.msg_iter.count = max_len;
  recv_ret = h->sock.ops->recvmsg(&h->sock, &msg, max_len, 0);
  if (recv_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-rfcomm-recv-handle: channel=%u cid=0x%04x "
               "handle=0x%04x recv-ret=%d flags=0x%x "
               "proto=BTPROTO_RFCOMM\n",
               h->channel, h->cid, h->handle, recv_ret, msg.msg_flags);
      if (msg_flags != NULL)
        {
          *msg_flags = msg.msg_flags;
        }

      return recv_ret;
    }

  if (payload != NULL && recv_ret > 0)
    {
      memcpy(payload, buf, (size_t)recv_ret);
    }

  if (msg_flags != NULL)
    {
      *msg_flags = msg.msg_flags;
    }

  linux_bt_upstream_format_hex(buf, (size_t)recv_ret, hex, sizeof(hex));
  snprintf(out, out_len,
           "upstream-rfcomm-recv-handle: channel=%u cid=0x%04x "
           "handle=0x%04x recv-ret=%d flags=0x%x payload=%s "
           "proto=BTPROTO_RFCOMM\n",
           h->channel, h->cid, h->handle, recv_ret, msg.msg_flags, hex);
  return recv_ret;
#else
  (void)handle;
  (void)payload;
  (void)max_len;
  (void)msg_flags;
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len, "upstream-rfcomm-recv-handle: unsupported\n");
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_poll_handle(void *handle, short events,
                                                short *revents)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;

  if (h == NULL)
    {
      return -EINVAL;
    }

  return linux_bt_upstream_socket_poll_handle(&h->sock, events, revents);
#else
  (void)handle;
  (void)events;
  (void)revents;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_shutdown_handle(void *handle, int how)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->shutdown == NULL)
    {
      return -EOPNOTSUPP;
    }

  return h->sock.ops->shutdown(&h->sock, how);
#else
  (void)handle;
  (void)how;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_ioctl_handle(void *handle,
                                                 unsigned int cmd,
                                                 unsigned long arg)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->ioctl == NULL)
    {
      return -EOPNOTSUPP;
    }

  return h->sock.ops->ioctl(&h->sock, cmd, arg);
#else
  (void)handle;
  (void)cmd;
  (void)arg;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_ioctl_probe(void *handle,
                                                char *out,
                                                size_t out_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
#  define LINUX_BT_RFCOMM_IOCTL_UNSUPPORTED 0x52ff
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  struct linux_bt_rfcomm_conninfo cinfo;
  int fallback_ret;
  int inq = -1;
  int cinfo_len;
  int cinfo_ret;
  int outq = -1;
  int inq_ret;
  int outq_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.sk == NULL || h->sock.ops == NULL ||
      h->sock.ops->ioctl == NULL)
    {
      snprintf(out, out_len, "upstream-rfcomm-ioctl: unsupported\n");
      return -EOPNOTSUPP;
    }

  inq_ret = h->sock.ops->ioctl(&h->sock, TIOCINQ,
                               (unsigned long)&inq);
  outq_ret = h->sock.ops->ioctl(&h->sock, TIOCOUTQ,
                                (unsigned long)&outq);
  memset(&cinfo, 0, sizeof(cinfo));
  cinfo_len = sizeof(cinfo);
  cinfo_ret = h->sock.ops->getsockopt != NULL ?
              h->sock.ops->getsockopt(&h->sock, SOL_RFCOMM,
                                      RFCOMM_CONNINFO,
                                      (char *)&cinfo,
                                      &cinfo_len) : -EOPNOTSUPP;
  fallback_ret = h->sock.ops->ioctl(&h->sock,
                                    LINUX_BT_RFCOMM_IOCTL_UNSUPPORTED,
                                    0);
  snprintf(out, out_len,
           "upstream-rfcomm-ioctl: channel=%u cid=0x%04x "
           "handle=0x%04x inq-ret=%d inq=%d outq-ret=%d outq=%d "
           "conninfo-ret=%d conninfo-handle=0x%04x "
           "conninfo-dev-class=%02x:%02x:%02x "
           "fallback-ret=%d fallback=rfcomm-tty-disabled "
           "state=%d proto=BTPROTO_RFCOMM\n",
           h->channel, h->cid, h->handle, inq_ret, inq, outq_ret, outq,
           cinfo_ret, cinfo.hci_handle, cinfo.dev_class[0],
           cinfo.dev_class[1], cinfo.dev_class[2],
           fallback_ret, h->sock.sk->sk_state);
  if (inq_ret < 0)
    {
      return inq_ret;
    }

  if (outq_ret < 0)
    {
      return outq_ret;
    }

  if (cinfo_ret < 0)
    {
      return cinfo_ret;
    }

  return fallback_ret == -EOPNOTSUPP ? 0 : fallback_ret;
#else
  (void)handle;
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len, "upstream-rfcomm-ioctl: unsupported\n");
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_poll_probe(void *handle,
                                               int want_write,
                                               char *out,
                                               size_t out_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  __poll_t poll_ret;
  unsigned int ready;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.sk == NULL || h->sock.ops == NULL ||
      h->sock.ops->poll == NULL)
    {
      snprintf(out, out_len, "upstream-rfcomm-poll: unsupported\n");
      return -EOPNOTSUPP;
    }

  poll_ret = h->sock.ops->poll(NULL, &h->sock, NULL);
  ready = want_write ? ((poll_ret & EPOLLOUT) != 0) :
                       ((poll_ret & EPOLLIN) != 0);
  snprintf(out, out_len,
           "upstream-rfcomm-poll: channel=%u cid=0x%04x "
           "handle=0x%04x events=%s poll-ret=0x%x ready=%u "
           "state=%d shutdown=0x%x proto=BTPROTO_RFCOMM\n",
           h->channel, h->cid, h->handle,
           want_write ? "POLLOUT" : "POLLIN",
           (unsigned int)poll_ret, ready,
           h->sock.sk->sk_state, h->sock.sk->sk_shutdown);
  return ready ? 0 : -EAGAIN;
#else
  (void)handle;
  (void)want_write;
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len, "upstream-rfcomm-poll: unsupported\n");
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_timestamp_probe(void *handle,
                                                    char *out,
                                                    size_t out_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  struct __kernel_old_timeval tv;
  struct linux_bt_compat_timespec
  {
    long tv_sec;
    long tv_nsec;
  } ts;
  int tv_ret;
  int ts_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.sk == NULL || h->sock.ops == NULL ||
      h->sock.ops->gettstamp == NULL)
    {
      snprintf(out, out_len, "upstream-rfcomm-timestamp: unsupported\n");
      return -EOPNOTSUPP;
    }

  memset(&tv, 0, sizeof(tv));
  memset(&ts, 0, sizeof(ts));
  tv_ret = h->sock.ops->gettstamp(&h->sock, &tv, true, false);
  ts_ret = h->sock.ops->gettstamp(&h->sock, &ts, false, false);
  snprintf(out, out_len,
           "upstream-rfcomm-timestamp: channel=%u cid=0x%04x "
           "handle=0x%04x timeval-ret=%d tv-sec=%ld tv-usec=%ld "
           "timespec-ret=%d ts-sec=%ld ts-nsec=%ld "
           "proto=BTPROTO_RFCOMM\n",
           h->channel, h->cid, h->handle, tv_ret, tv.tv_sec, tv.tv_usec,
           ts_ret, ts.tv_sec, ts.tv_nsec);
  return tv_ret < 0 ? tv_ret : ts_ret;
#else
  (void)handle;
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len, "upstream-rfcomm-timestamp: unsupported\n");
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_rfcomm_socket_close_handle(void *handle)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  int pre_state;
  int pre_shutdown;
  int close_ret = 0;

  if (h == NULL)
    {
      return -EINVAL;
    }

  pre_state = h->sock.sk != NULL ? h->sock.sk->sk_state : -1;
  pre_shutdown = h->sock.sk != NULL ? h->sock.sk->sk_shutdown : 0;
  if (h->sock.ops != NULL && h->sock.ops->release != NULL)
    {
      close_ret = h->sock.ops->release(&h->sock);
    }

  printf("upstream-rfcomm-shutdown-close: channel=%u cid=0x%04x "
         "handle=0x%04x pre-state=%d pre-shutdown=0x%x close-ret=%d "
         "post-state=%d post-shutdown=0x%x proto=BTPROTO_RFCOMM\n",
         h->channel, h->cid, h->handle, pre_state, pre_shutdown,
         close_ret, h->sock.sk != NULL ? h->sock.sk->sk_state : BT_CLOSED,
         h->sock.sk != NULL ? h->sock.sk->sk_shutdown : SHUTDOWN_MASK);
  h->sock.sk = NULL;
  h->sock.ops = NULL;
  h->file.private_data = NULL;
  kmm_free(h);
  return close_ret;
#else
  (void)handle;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_open(uint16_t handle, void **out_handle)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  const struct net_proto_family *family = NULL;
  struct linux_bt_upstream_rfcomm_socket_handle *h;
  struct linux_bt_sockaddr_sco laddr;
  struct linux_bt_sockaddr_sco name;
  uint16_t voice = BT_VOICE_CVSD_16BIT;
  uint16_t mtu;
  int optlen;
  int ret;

  if (out_handle == NULL)
    {
      return -EINVAL;
    }

  *out_handle = NULL;

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      return -EAFNOSUPPORT;
    }

  h = kmm_zalloc(sizeof(*h));
  if (h == NULL)
    {
      return -ENOMEM;
    }

  h->sock.type = SOCK_SEQPACKET;
  h->sock.file = &h->file;
  h->file.private_data = h;

  ret = family->create(&init_net, &h->sock, BTPROTO_SCO, 0);
  if (ret < 0)
    {
      kmm_free(h);
      return ret;
    }

  memset(&laddr, 0, sizeof(laddr));
  laddr.sco_family = AF_BLUETOOTH;
  bacpy(&laddr.sco_bdaddr, BDADDR_ANY);

  ret = h->sock.ops != NULL && h->sock.ops->bind != NULL ?
        h->sock.ops->bind(&h->sock, (struct sockaddr *)&laddr,
                          sizeof(laddr)) : -EOPNOTSUPP;
  if (ret < 0)
    {
      if (h->sock.ops != NULL && h->sock.ops->release != NULL)
        {
          h->sock.ops->release(&h->sock);
        }

      kmm_free(h);
      return ret;
    }

  if (h->sock.sk != NULL)
    {
      struct linux_bt_sco_pinfo *pi = linux_bt_sco_pi(h->sock.sk);

      pi->handle = handle != 0 ? handle : LINUX_BT_SCO_DEFAULT_HANDLE;
      h->handle = pi->handle;
    }

  if (h->sock.ops != NULL && h->sock.ops->setsockopt != NULL)
    {
      (void)h->sock.ops->setsockopt(&h->sock, SOL_BLUETOOTH, BT_VOICE,
                                    (char *)&voice, sizeof(voice));
    }

  optlen = sizeof(mtu);
  if (h->sock.ops != NULL && h->sock.ops->getsockopt != NULL)
    {
      (void)h->sock.ops->getsockopt(&h->sock, SOL_SCO, SO_SCO_MTU,
                                    (char *)&mtu, &optlen);
    }

  if (h->sock.ops != NULL && h->sock.ops->getname != NULL)
    {
      (void)h->sock.ops->getname(&h->sock, (struct sockaddr *)&name, 0);
    }

  h->bound = true;
  *out_handle = h;
  return 0;
#else
  (void)handle;
  if (out_handle != NULL)
    {
      *out_handle = NULL;
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_connect_handle(void *handle,
                                                uint16_t handle_id)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  struct linux_bt_sockaddr_sco raddr;
  int ret;

  if (h == NULL)
    {
      return -EINVAL;
    }

  if (h->sock.sk != NULL && handle_id != 0)
    {
      struct linux_bt_sco_pinfo *pi = linux_bt_sco_pi(h->sock.sk);

      pi->handle = handle_id;
      h->handle = handle_id;
    }

  memset(&raddr, 0, sizeof(raddr));
  raddr.sco_family = AF_BLUETOOTH;
  bacpy(&raddr.sco_bdaddr, BDADDR_ANY);

  ret = h->sock.ops != NULL && h->sock.ops->connect != NULL ?
        h->sock.ops->connect(&h->sock, (struct sockaddr *)&raddr,
                             sizeof(raddr), 0) : -EOPNOTSUPP;
  if (ret < 0)
    {
      return ret;
    }

  h->connected = true;
  return 0;
#else
  (void)handle;
  (void)handle_id;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_listen_handle(void *handle, int backlog)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->listen == NULL)
    {
      return -EOPNOTSUPP;
    }

  return h->sock.ops->listen(&h->sock, backlog);
#else
  (void)handle;
  (void)backlog;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_accept_handle(void *handle,
                                               void **out_handle)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  struct linux_bt_upstream_rfcomm_socket_handle *accepted;
  struct proto_accept_arg arg;
  int ret;

  if (out_handle == NULL)
    {
      return -EINVAL;
    }

  *out_handle = NULL;
  if (h == NULL || h->sock.ops == NULL || h->sock.ops->accept == NULL)
    {
      return -EOPNOTSUPP;
    }

  accepted = kmm_zalloc(sizeof(*accepted));
  if (accepted == NULL)
    {
      return -ENOMEM;
    }

  accepted->sock.type = h->sock.type;
  accepted->sock.file = &accepted->file;
  accepted->file.private_data = accepted;
  memset(&arg, 0, sizeof(arg));
  ret = h->sock.ops->accept(&h->sock, &accepted->sock, &arg);
  if (ret < 0)
    {
      kmm_free(accepted);
      return ret;
    }

  accepted->channel = h->channel;
  accepted->cid = h->cid;
  accepted->handle = h->handle;
  accepted->bound = true;
  accepted->connected = true;
  *out_handle = accepted;
  return OK;
#else
  (void)handle;
  (void)out_handle;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_getsockopt_handle(void *handle,
                                                   int level,
                                                   int optname,
                                                   void *optval,
                                                   int *optlen)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;

  if (h == NULL || optval == NULL || optlen == NULL ||
      h->sock.ops == NULL || h->sock.ops->getsockopt == NULL)
    {
      return -EINVAL;
    }

  return h->sock.ops->getsockopt(&h->sock, level, optname,
                                 (char *)optval, optlen);
#else
  (void)handle;
  (void)level;
  (void)optname;
  (void)optval;
  (void)optlen;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_setsockopt_handle(void *handle,
                                                   int level,
                                                   int optname,
                                                   const void *optval,
                                                   unsigned int optlen)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;

  if (h == NULL || optval == NULL ||
      h->sock.ops == NULL || h->sock.ops->setsockopt == NULL)
    {
      return -EINVAL;
    }

  return h->sock.ops->setsockopt(&h->sock, level, optname,
                                 (char *)optval, optlen);
#else
  (void)handle;
  (void)level;
  (void)optname;
  (void)optval;
  (void)optlen;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_listen_accept_probe(uint16_t handle,
                                                     char *out,
                                                     size_t out_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = NULL;
  struct linux_bt_sco_pinfo *pi = NULL;
  struct socket accepted;
  struct file accepted_file;
  struct proto_accept_arg arg;
  struct linux_bt_sockaddr_sco accepted_name;
  struct linux_bt_sockaddr_sco accepted_peer_name;
  struct linux_bt_sockaddr_sco listen_name;
  struct linux_bt_sockaddr_sco listen_peer_name;
  uint32_t defer = 1;
  uint32_t defer_state = 0;
  uint32_t pkt_status = 1;
  uint32_t pkt_status_state = 0;
  uint32_t sndmtu = 0;
  uint32_t rcvmtu = 0;
  int opt_len;
  int open_ret;
  int defer_ret = -EOPNOTSUPP;
  int defer_get_ret = -EOPNOTSUPP;
  int pkt_ret = -EOPNOTSUPP;
  int pkt_get_ret = -EOPNOTSUPP;
  int codec_ret = -EOPNOTSUPP;
  int sndmtu_ret = -EOPNOTSUPP;
  int rcvmtu_ret = -EOPNOTSUPP;
  int accepted_name_ret = -EOPNOTSUPP;
  int accepted_peer_name_ret = -EOPNOTSUPP;
  int listen_name_ret = -EOPNOTSUPP;
  int listen_peer_name_ret = -EOPNOTSUPP;
  int listen_ret = -EOPNOTSUPP;
  int accept_ret = -EOPNOTSUPP;
  int release_ret = 0;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  memset(&accepted, 0, sizeof(accepted));
  memset(&accepted_file, 0, sizeof(accepted_file));
  memset(&arg, 0, sizeof(arg));
  memset(&accepted_name, 0, sizeof(accepted_name));
  memset(&accepted_peer_name, 0, sizeof(accepted_peer_name));
  memset(&listen_name, 0, sizeof(listen_name));
  memset(&listen_peer_name, 0, sizeof(listen_peer_name));

  open_ret = linux_bt_upstream_sco_socket_open(handle, (void **)&h);
  if (open_ret >= 0 && h != NULL && h->sock.ops != NULL &&
      h->sock.ops->setsockopt != NULL)
    {
      defer_ret = h->sock.ops->setsockopt(&h->sock, SOL_BLUETOOTH,
                                          BT_DEFER_SETUP, (char *)&defer,
                                          sizeof(defer));
      pkt_ret = h->sock.ops->setsockopt(&h->sock, SOL_BLUETOOTH,
                                        BT_PKT_STATUS,
                                        (char *)&pkt_status,
                                        sizeof(pkt_status));
      codec_ret = h->sock.ops->setsockopt(&h->sock, SOL_BLUETOOTH,
                                          BT_CODEC, (char *)&pkt_status,
                                          sizeof(pkt_status));
    }

  if (open_ret >= 0 && h != NULL && h->sock.ops != NULL &&
      h->sock.ops->getsockopt != NULL)
    {
      opt_len = sizeof(defer_state);
      defer_get_ret = h->sock.ops->getsockopt(&h->sock, SOL_BLUETOOTH,
                                              BT_DEFER_SETUP,
                                              (char *)&defer_state,
                                              &opt_len);
      opt_len = sizeof(pkt_status_state);
      pkt_get_ret = h->sock.ops->getsockopt(&h->sock, SOL_BLUETOOTH,
                                            BT_PKT_STATUS,
                                            (char *)&pkt_status_state,
                                            &opt_len);
    }

  if (open_ret >= 0 && h != NULL && h->sock.ops != NULL &&
      h->sock.ops->listen != NULL)
    {
      listen_ret = h->sock.ops->listen(&h->sock, 1);
      if (h->sock.ops->getname != NULL)
        {
          listen_name_ret =
            h->sock.ops->getname(&h->sock,
                                 (struct sockaddr *)&listen_name, 0);
          listen_peer_name_ret =
            h->sock.ops->getname(&h->sock,
                                 (struct sockaddr *)&listen_peer_name, 1);
        }
    }

  if (listen_ret >= 0 && h->sock.ops != NULL &&
      h->sock.ops->accept != NULL)
    {
      accepted.type = h->sock.type;
      accepted.file = &accepted_file;
      accepted_file.private_data = h;
      accept_ret = h->sock.ops->accept(&h->sock, &accepted, &arg);
      if (accept_ret >= 0 && accepted.sk != NULL)
        {
          pi = linux_bt_sco_pi(accepted.sk);
          if (accepted.ops != NULL && accepted.ops->getname != NULL)
            {
              accepted_name_ret =
                accepted.ops->getname(&accepted,
                                      (struct sockaddr *)&accepted_name,
                                      0);
              accepted_peer_name_ret =
                accepted.ops->getname(&accepted,
                                      (struct sockaddr *)&accepted_peer_name,
                                      1);
            }

          if (accepted.ops != NULL && accepted.ops->getsockopt != NULL)
            {
              opt_len = sizeof(sndmtu);
              sndmtu_ret = accepted.ops->getsockopt(&accepted,
                                                    SOL_BLUETOOTH,
                                                    BT_SNDMTU,
                                                    (char *)&sndmtu,
                                                    &opt_len);
              opt_len = sizeof(rcvmtu);
              rcvmtu_ret = accepted.ops->getsockopt(&accepted,
                                                    SOL_BLUETOOTH,
                                                    BT_RCVMTU,
                                                    (char *)&rcvmtu,
                                                    &opt_len);
            }
        }
    }

  snprintf(out, out_len,
           "upstream-sco-listen-accept: handle=0x%04x open-ret=%d "
           "listen-ret=%d accept-ret=%d accepted-state=%d "
           "getname-ret=%d peer-getname-ret=%d accepted-getname-ret=%d "
           "accepted-peer-getname-ret=%d mtu=%u "
           "voice=0x%04x defer-ret=%d defer-get-ret=%d defer=%u "
           "pkt-ret=%d pkt-get-ret=%d pkt-status=%u codec-ret=%d "
           "accepted-defer=%u accepted-pkt-status=%u sndmtu-ret=%d "
           "sndmtu=%u rcvmtu-ret=%d rcvmtu=%u proto=BTPROTO_SCO\n",
           pi != NULL ? pi->handle :
           (handle != 0 ? handle : LINUX_BT_SCO_DEFAULT_HANDLE),
           open_ret, listen_ret, accept_ret,
           accepted.sk != NULL ? accepted.sk->sk_state : -1,
           listen_name_ret, listen_peer_name_ret, accepted_name_ret,
           accepted_peer_name_ret,
           pi != NULL ? pi->mtu : LINUX_BT_SCO_DEFAULT_MTU,
           pi != NULL ? pi->voice : BT_VOICE_CVSD_16BIT,
           defer_ret, defer_get_ret, defer_state,
           pkt_ret, pkt_get_ret, pkt_status_state, codec_ret,
           pi != NULL && pi->defer_setup ? 1 : 0,
           pi != NULL && pi->pkt_status ? 1 : 0,
           sndmtu_ret, sndmtu, rcvmtu_ret, rcvmtu);

  if (accepted.ops != NULL && accepted.ops->release != NULL)
    {
      (void)accepted.ops->release(&accepted);
    }

  if (open_ret >= 0 && h != NULL)
    {
      release_ret = linux_bt_upstream_sco_socket_close_handle(h);
      if (release_ret < 0 && accept_ret >= 0)
        {
          accept_ret = release_ret;
        }
    }

  if (open_ret < 0)
    {
      return open_ret;
    }

  if (listen_ret < 0)
    {
      return listen_ret;
    }

  return accept_ret < 0 ? accept_ret : 0;
#else
  (void)handle;
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len,
               "upstream-sco-listen-accept: unsupported\n");
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_write_handle(void *handle,
                                              const void *payload,
                                              size_t payload_len,
                                              char *out,
                                              size_t out_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  struct linux_bt_sco_pinfo *pi;
  struct msghdr msg;
  int send_ret;

  if (out == NULL || out_len == 0 ||
      (payload == NULL && payload_len > 0))
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.sk == NULL || h->sock.ops == NULL ||
      h->sock.ops->sendmsg == NULL)
    {
      snprintf(out, out_len, "upstream-sco-write-handle: not-bound\n");
      return -ENOTCONN;
    }

  pi = linux_bt_sco_pi(h->sock.sk);
  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  send_ret = h->sock.ops->sendmsg(&h->sock, &msg, payload_len);

  snprintf(out, out_len,
           "upstream-sco-write-handle: handle=0x%04x mtu=%u "
           "voice=0x%04x payload-len=%u send-ret=%d native-path=1 "
           "proto=BTPROTO_SCO\n",
           pi->handle, pi->mtu, pi->voice, (unsigned int)payload_len,
           send_ret);
  return send_ret < 0 ? send_ret : 0;
#else
  (void)handle;
  (void)payload;
  (void)payload_len;
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len, "upstream-sco-write-handle: unsupported\n");
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_recv_handle(void *handle, void *payload,
                                             size_t max_len,
                                             int *msg_flags,
                                             char *out, size_t out_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  struct linux_bt_sco_pinfo *pi;
  struct msghdr msg;
  uint8_t buf[128];
  char hex[128 * 3];
  int recv_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.sk == NULL || h->sock.ops == NULL ||
      h->sock.ops->recvmsg == NULL)
    {
      snprintf(out, out_len, "upstream-sco-recv-handle: not-bound\n");
      return -ENOTCONN;
    }

  if (max_len == 0 || max_len > sizeof(buf))
    {
      max_len = sizeof(buf);
    }

  pi = linux_bt_sco_pi(h->sock.sk);
  memset(&msg, 0, sizeof(msg));
  memset(buf, 0, sizeof(buf));
  msg.msg_iter.data = (const char *)buf;
  msg.msg_iter.count = max_len;
  recv_ret = h->sock.ops->recvmsg(&h->sock, &msg, max_len, 0);
  if (recv_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-sco-recv-handle: handle=0x%04x recv-ret=%d "
               "flags=0x%x proto=BTPROTO_SCO\n",
               pi->handle, recv_ret, msg.msg_flags);
      if (msg_flags != NULL)
        {
          *msg_flags = msg.msg_flags;
        }

      return recv_ret;
    }

  if (payload != NULL && recv_ret > 0)
    {
      memcpy(payload, buf, (size_t)recv_ret);
    }

  if (msg_flags != NULL)
    {
      *msg_flags = msg.msg_flags;
    }

  linux_bt_upstream_format_hex(buf, (size_t)recv_ret, hex, sizeof(hex));
  snprintf(out, out_len,
           "upstream-sco-recv-handle: handle=0x%04x recv-ret=%d "
           "flags=0x%x payload=%s proto=BTPROTO_SCO\n",
           pi->handle, recv_ret, msg.msg_flags, hex);
  return recv_ret;
#else
  (void)handle;
  (void)payload;
  (void)max_len;
  (void)msg_flags;
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len, "upstream-sco-recv-handle: unsupported\n");
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_poll_handle(void *handle, short events,
                                             short *revents)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;

  if (h == NULL)
    {
      return -EINVAL;
    }

  return linux_bt_upstream_socket_poll_handle(&h->sock, events, revents);
#else
  (void)handle;
  (void)events;
  (void)revents;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_shutdown_handle(void *handle, int how)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->shutdown == NULL)
    {
      return -EOPNOTSUPP;
    }

  return h->sock.ops->shutdown(&h->sock, how);
#else
  (void)handle;
  (void)how;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_ioctl_handle(void *handle,
                                              unsigned int cmd,
                                              unsigned long arg)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;

  if (h == NULL || h->sock.ops == NULL || h->sock.ops->ioctl == NULL)
    {
      return -EOPNOTSUPP;
    }

  return h->sock.ops->ioctl(&h->sock, cmd, arg);
#else
  (void)handle;
  (void)cmd;
  (void)arg;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_ioctl_probe(void *handle,
                                             char *out,
                                             size_t out_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  struct linux_bt_sco_conninfo cinfo;
  struct linux_bt_sco_options opts;
  struct socket pair;
  uint32_t rcvmtu = 0;
  uint32_t sndmtu = 0;
  int inq = -1;
  int cinfo_len;
  int cinfo_ret;
  int mmap_ret = -EOPNOTSUPP;
  int options_len;
  int options_ret;
  int outq = -1;
  int inq_ret;
  int outq_ret;
  int rcvmtu_len;
  int rcvmtu_ret;
  int sndmtu_len;
  int sndmtu_ret;
  int socketpair_ret = -EOPNOTSUPP;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.sk == NULL || h->sock.ops == NULL ||
      h->sock.ops->ioctl == NULL)
    {
      snprintf(out, out_len, "upstream-sco-ioctl: unsupported\n");
      return -EOPNOTSUPP;
    }

  inq_ret = h->sock.ops->ioctl(&h->sock, TIOCINQ,
                               (unsigned long)&inq);
  outq_ret = h->sock.ops->ioctl(&h->sock, TIOCOUTQ,
                                (unsigned long)&outq);
  memset(&opts, 0, sizeof(opts));
  options_len = sizeof(opts);
  options_ret = h->sock.ops->getsockopt != NULL ?
                h->sock.ops->getsockopt(&h->sock, SOL_SCO,
                                        SCO_OPTIONS,
                                        (char *)&opts,
                                        &options_len) : -EOPNOTSUPP;
  memset(&cinfo, 0, sizeof(cinfo));
  cinfo_len = sizeof(cinfo);
  cinfo_ret = h->sock.ops->getsockopt != NULL ?
              h->sock.ops->getsockopt(&h->sock, SOL_SCO,
                                      SCO_CONNINFO,
                                      (char *)&cinfo,
                                      &cinfo_len) : -EOPNOTSUPP;
  sndmtu_len = sizeof(sndmtu);
  sndmtu_ret = h->sock.ops->getsockopt != NULL ?
               h->sock.ops->getsockopt(&h->sock, SOL_BLUETOOTH,
                                       BT_SNDMTU,
                                       (char *)&sndmtu,
                                       &sndmtu_len) : -EOPNOTSUPP;
  rcvmtu_len = sizeof(rcvmtu);
  rcvmtu_ret = h->sock.ops->getsockopt != NULL ?
               h->sock.ops->getsockopt(&h->sock, SOL_BLUETOOTH,
                                       BT_RCVMTU,
                                       (char *)&rcvmtu,
                                       &rcvmtu_len) : -EOPNOTSUPP;
  memset(&pair, 0, sizeof(pair));
  if (h->sock.ops->socketpair != NULL)
    {
      socketpair_ret = h->sock.ops->socketpair(&h->sock, &pair);
    }

  if (h->sock.ops->mmap != NULL)
    {
      mmap_ret = h->sock.ops->mmap(NULL, &h->sock, NULL);
    }

  snprintf(out, out_len,
           "upstream-sco-ioctl: handle=0x%04x inq-ret=%d inq=%d "
           "outq-ret=%d outq=%d options-ret=%d options-mtu=%u "
           "conninfo-ret=%d conninfo-handle=0x%04x "
           "sco-mtu-sockopt=SOL_BLUETOOTH "
           "sndmtu-ret=%d sndmtu=%" PRIu32 " "
           "rcvmtu-ret=%d rcvmtu=%" PRIu32 " "
           "conninfo-dev-class=%02x:%02x:%02x no-op=socketpair-mmap "
           "socketpair-ret=%d mmap-ret=%d "
           "state=%d proto=BTPROTO_SCO\n",
           h->handle, inq_ret, inq, outq_ret, outq,
           options_ret, opts.mtu, cinfo_ret, cinfo.hci_handle,
           sndmtu_ret, sndmtu, rcvmtu_ret, rcvmtu,
           cinfo.dev_class[0], cinfo.dev_class[1], cinfo.dev_class[2],
           socketpair_ret, mmap_ret, h->sock.sk->sk_state);
  if (inq_ret < 0)
    {
      return inq_ret;
    }

  if (outq_ret < 0)
    {
      return outq_ret;
    }

  if (options_ret < 0)
    {
      return options_ret;
    }

  if (cinfo_ret < 0)
    {
      return cinfo_ret;
    }

  if (sndmtu_ret < 0)
    {
      return sndmtu_ret;
    }

  if (rcvmtu_ret < 0)
    {
      return rcvmtu_ret;
    }

  if (socketpair_ret != -EOPNOTSUPP)
    {
      return socketpair_ret;
    }

  return mmap_ret == -EOPNOTSUPP ? 0 : mmap_ret;
#else
  (void)handle;
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len, "upstream-sco-ioctl: unsupported\n");
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_poll_probe(void *handle,
                                            int want_write,
                                            char *out,
                                            size_t out_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  __poll_t poll_ret;
  unsigned int ready;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.sk == NULL || h->sock.ops == NULL ||
      h->sock.ops->poll == NULL)
    {
      snprintf(out, out_len, "upstream-sco-poll: unsupported\n");
      return -EOPNOTSUPP;
    }

  poll_ret = h->sock.ops->poll(NULL, &h->sock, NULL);
  ready = want_write ? ((poll_ret & EPOLLOUT) != 0) :
                       ((poll_ret & EPOLLIN) != 0);
  snprintf(out, out_len,
           "upstream-sco-poll: handle=0x%04x events=%s poll-ret=0x%x "
           "ready=%u state=%d shutdown=0x%x proto=BTPROTO_SCO\n",
           h->handle, want_write ? "POLLOUT" : "POLLIN",
           (unsigned int)poll_ret, ready,
           h->sock.sk->sk_state, h->sock.sk->sk_shutdown);
  return ready ? 0 : -EAGAIN;
#else
  (void)handle;
  (void)want_write;
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len, "upstream-sco-poll: unsupported\n");
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_timestamp_probe(void *handle,
                                                 char *out,
                                                 size_t out_len)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  struct __kernel_old_timeval tv;
  struct linux_bt_compat_timespec
  {
    long tv_sec;
    long tv_nsec;
  } ts;
  int tv_ret;
  int ts_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (h == NULL || h->sock.sk == NULL || h->sock.ops == NULL ||
      h->sock.ops->gettstamp == NULL)
    {
      snprintf(out, out_len, "upstream-sco-timestamp: unsupported\n");
      return -EOPNOTSUPP;
    }

  memset(&tv, 0, sizeof(tv));
  memset(&ts, 0, sizeof(ts));
  tv_ret = h->sock.ops->gettstamp(&h->sock, &tv, true, false);
  ts_ret = h->sock.ops->gettstamp(&h->sock, &ts, false, false);
  snprintf(out, out_len,
           "upstream-sco-timestamp: handle=0x%04x "
           "timeval-ret=%d tv-sec=%ld tv-usec=%ld "
           "timespec-ret=%d ts-sec=%ld ts-nsec=%ld "
           "proto=BTPROTO_SCO\n",
           h->handle, tv_ret, tv.tv_sec, tv.tv_usec,
           ts_ret, ts.tv_sec, ts.tv_nsec);
  return tv_ret < 0 ? tv_ret : ts_ret;
#else
  (void)handle;
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len, "upstream-sco-timestamp: unsupported\n");
    }

  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_sco_socket_close_handle(void *handle)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY
  struct linux_bt_upstream_rfcomm_socket_handle *h = handle;
  int pre_state;
  int pre_shutdown;
  int close_ret = 0;

  if (h == NULL)
    {
      return -EINVAL;
    }

  pre_state = h->sock.sk != NULL ? h->sock.sk->sk_state : -1;
  pre_shutdown = h->sock.sk != NULL ? h->sock.sk->sk_shutdown : 0;
  if (h->sock.ops != NULL && h->sock.ops->release != NULL)
    {
      close_ret = h->sock.ops->release(&h->sock);
    }

  printf("upstream-sco-shutdown-close: handle=0x%04x pre-state=%d "
         "pre-shutdown=0x%x close-ret=%d post-state=%d "
         "post-shutdown=0x%x proto=BTPROTO_SCO\n",
         h->handle, pre_state, pre_shutdown, close_ret,
         h->sock.sk != NULL ? h->sock.sk->sk_state : BT_CLOSED,
         h->sock.sk != NULL ? h->sock.sk->sk_shutdown : SHUTDOWN_MASK);
  h->sock.sk = NULL;
  h->sock.ops = NULL;
  h->file.private_data = NULL;
  kmm_free(h);
  return close_ret;
#else
  (void)handle;
  return -EAFNOSUPPORT;
#endif
}

int linux_bt_upstream_l2cap_socket_mark_bnep_owner(void *handle)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;

  if (h == NULL || h->sock.sk == NULL || !h->connected)
    {
      return -EBADFD;
    }

  h->bnep_owned = true;
  h->bnep_user_closed = false;
  h->bnep_released = false;
  h->sock.file = &h->file;
  h->file.private_data = h;

#ifdef CONFIG_SIM_BTHWSIM
  (void)linux_bt_hci_ensure_connected_handle(ACL_LINK, h->handle,
                                             BDADDR_ANY, 0, true, 0);
  (void)linux_bt_l2cap_attach_send_chan(&h->sock, h->handle, h->cid);
#endif
  return 0;
}

int linux_bt_upstream_l2cap_socket_close_bnep_file(struct file *file)
{
  struct linux_bt_upstream_l2cap_socket_handle *h;

  if (file == NULL || file->private_data == NULL)
    {
      return -ENOENT;
    }

  h = file->private_data;
  if (&h->file != file || !h->bnep_owned)
    {
      return -ENOENT;
    }

  h->bnep_owned = false;
  if (h->bnep_user_closed)
    {
      return linux_bt_upstream_l2cap_socket_close_handle(h);
    }

  linux_bt_upstream_l2cap_socket_release_handle(h);
  h->bnep_released = true;
  return 0;
}

struct socket *linux_bt_upstream_l2cap_socket_kernel_socket(void *handle)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;

  if (h == NULL || !h->connected || h->sock.sk == NULL ||
      h->sock.sk->sk_state != BT_CONNECTED)
    {
      return NULL;
    }

  return &h->sock;
}

int linux_bt_upstream_monitor_listen_probe(char *out, size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct sockaddr_hci haddr;
  int create_ret;
  int bind_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (g_hci_monitor_probe_bound)
    {
      snprintf(out, out_len, "upstream-monitor-listen: already-bound\n");
      return -EALREADY;
    }

  memset(&g_hci_monitor_probe_socket, 0,
         sizeof(g_hci_monitor_probe_socket));
  memset(&haddr, 0, sizeof(haddr));

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-monitor-listen: family=PF_BLUETOOTH invalid\n");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      snprintf(out, out_len,
               "upstream-monitor-listen: family=PF_BLUETOOTH unregistered\n");
      return -EAFNOSUPPORT;
    }

  g_hci_monitor_probe_socket.type = SOCK_RAW;
  create_ret = family->create(&init_net, &g_hci_monitor_probe_socket,
                              BTPROTO_HCI, 0);
  if (create_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-monitor-listen: create-ret=%d\n", create_ret);
      return create_ret;
    }

  haddr.hci_family = AF_BLUETOOTH;
  haddr.hci_channel = HCI_CHANNEL_MONITOR;
  haddr.hci_dev = HCI_DEV_NONE;
  bind_ret = g_hci_monitor_probe_socket.ops != NULL &&
             g_hci_monitor_probe_socket.ops->bind != NULL ?
             g_hci_monitor_probe_socket.ops->bind(
               &g_hci_monitor_probe_socket, (struct sockaddr *)&haddr,
               sizeof(haddr)) : -EOPNOTSUPP;
  if (bind_ret < 0)
    {
      if (g_hci_monitor_probe_socket.ops != NULL &&
          g_hci_monitor_probe_socket.ops->release != NULL)
        {
          g_hci_monitor_probe_socket.ops->release(
            &g_hci_monitor_probe_socket);
        }

      memset(&g_hci_monitor_probe_socket, 0,
             sizeof(g_hci_monitor_probe_socket));
      snprintf(out, out_len,
               "upstream-monitor-listen: create-ret=%d bind-ret=%d\n",
               create_ret, bind_ret);
      return bind_ret;
    }

  g_hci_monitor_probe_bound = true;
  snprintf(out, out_len,
           "upstream-monitor-listen: create-ret=%d bind-ret=%d\n",
           create_ret, bind_ret);
  return 0;
}

int linux_bt_upstream_monitor_recv_raw_probe(void *payload,
                                             size_t payload_len,
                                             int *flags)
{
  struct msghdr msg;
  int recv_ret;

  if (payload == NULL || payload_len == 0)
    {
      return -EINVAL;
    }

  if (!g_hci_monitor_probe_bound ||
      g_hci_monitor_probe_socket.ops == NULL ||
      g_hci_monitor_probe_socket.ops->recvmsg == NULL)
    {
      return -ENOTCONN;
    }

  if (flags != NULL)
    {
      *flags = 0;
    }

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  recv_ret = g_hci_monitor_probe_socket.ops->recvmsg(
    &g_hci_monitor_probe_socket, &msg, payload_len, 0);
  if (recv_ret >= 0 && flags != NULL)
    {
      *flags = msg.msg_flags;
    }

  return recv_ret;
}

int linux_bt_upstream_monitor_close_probe(char *out, size_t out_len)
{
  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_hci_monitor_probe_bound ||
      g_hci_monitor_probe_socket.ops == NULL ||
      g_hci_monitor_probe_socket.ops->release == NULL)
    {
      snprintf(out, out_len, "upstream-monitor-close: not-bound\n");
      return -ENOTCONN;
    }

  g_hci_monitor_probe_socket.ops->release(&g_hci_monitor_probe_socket);
  memset(&g_hci_monitor_probe_socket, 0,
         sizeof(g_hci_monitor_probe_socket));
  g_hci_monitor_probe_bound = false;
  snprintf(out, out_len, "upstream-monitor-close: released\n");
  return 0;
}

int linux_bt_upstream_mgmt_poll_discovery_probe(size_t max_records,
                                                char *out, size_t out_len)
{
  uint8_t payload[160];
  char text[161];
  char name[32];
  const char *name_pos;
  uint16_t src;
  uint16_t dst = 0;
  uint32_t payload_len;
  unsigned int records = 0;
  unsigned int found = 0;
  int ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  if (!g_hci_discovery_state.discovering)
    {
      snprintf(out, out_len,
               "upstream-mgmt-poll-discovery: no-active-discovery\n");
      return -EAGAIN;
    }

  if (max_records == 0 || max_records > 8)
    {
      max_records = 8;
    }

  while (records < max_records)
    {
      memset(payload, 0, sizeof(payload));
      payload_len = 0;
      ret = linux_bt_hwsim_read_raw(LINUX_BT_HWSIM_TYPE_ADV, &src, &dst,
                                    payload, sizeof(payload),
                                    &payload_len);
      if (ret <= 0)
        {
          break;
        }

      records++;
      if (payload_len >= sizeof(text))
        {
          payload_len = sizeof(text) - 1;
        }

      memcpy(text, payload, payload_len);
      text[payload_len] = '\0';
      if (strstr(text, "HCI_LE_ADV_REPORT") == NULL)
        {
          continue;
        }

      snprintf(name, sizeof(name), "linux-bt-hwsim-%u", src);
      name_pos = strstr(text, "name=");
      if (name_pos != NULL)
        {
          const char *cursor = name_pos + 5;
          size_t i = 0;

          while (cursor[i] != '\0' && cursor[i] != ' ' &&
                 i + 1 < sizeof(name))
            {
              name[i] = cursor[i];
              i++;
            }

          name[i] = '\0';
        }

      {
        uint8_t event[sizeof(struct mgmt_ev_device_found) + 31];
        struct mgmt_ev_device_found *ev =
          (struct mgmt_ev_device_found *)event;
        uint8_t eir[31];
        uint8_t *addr = (uint8_t *)&ev->addr.bdaddr;
        size_t name_len;
        size_t eir_len;

        name_len = strlen(name);
        if (name_len > sizeof(eir) - 2)
          {
            name_len = sizeof(eir) - 2;
          }

        memset(eir, 0, sizeof(eir));
        eir[0] = (uint8_t)(name_len + 1);
        eir[1] = 0x09;
        memcpy(&eir[2], name, name_len);
        eir_len = name_len + 2;

        memset(event, 0, sizeof(event));
        addr[0] = (uint8_t)(src & 0xff);
        addr[1] = (uint8_t)(src >> 8);
        addr[2] = 0x17;
        addr[3] = 0x06;
        addr[4] = 0x5e;
        addr[5] = 0xf0;
        ev->addr.type = BDADDR_LE_PUBLIC;
        ev->rssi = -42;
        ev->flags = cpu_to_le32(0);
        ev->eir_len = cpu_to_le16((uint16_t)eir_len);
        memcpy(ev->eir, eir, eir_len);

        if (linux_bt_upstream_mgmt_probe_queue_event(
              MGMT_EV_DEVICE_FOUND, g_hci_discovery_state.dev_id, ev,
              (uint16_t)(sizeof(*ev) + eir_len)) >= 0)
          {
            found++;
          }
      }
    }

  snprintf(out, out_len,
           "upstream-mgmt-poll-discovery: records=%u found=%u "
           "type=0x%02x dev=%u\n",
           records, found, g_hci_discovery_state.type,
           g_hci_discovery_state.dev_id);
  return (int)found;
}

int linux_bt_upstream_mgmt_socket_probe(uint16_t opcode, uint16_t index,
                                        uint8_t param, char *out,
                                        size_t out_len)
{
  const struct net_proto_family *family = NULL;
  struct socket sock;
  struct sockaddr_hci haddr;
  struct msghdr msg;
  struct msghdr recvmsg;
  uint8_t payload[sizeof(struct mgmt_hdr) + MGMT_SET_DEVICE_FLAGS_SIZE];
  uint8_t recvbuf[512];
  struct mgmt_hdr *hdr = (struct mgmt_hdr *)payload;
  struct mgmt_hdr *evhdr = (struct mgmt_hdr *)recvbuf;
  struct mgmt_ev_cmd_complete *complete;
  size_t payload_len = sizeof(*hdr);
  int create_ret;
  int bind_ret;
  int send_ret;
  int recv_ret = -EOPNOTSUPP;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  memset(&sock, 0, sizeof(sock));
  memset(&haddr, 0, sizeof(haddr));
  memset(&msg, 0, sizeof(msg));
  memset(&recvmsg, 0, sizeof(recvmsg));
  memset(payload, 0, sizeof(payload));
  memset(recvbuf, 0, sizeof(recvbuf));

  if (PF_BLUETOOTH < 0 ||
      PF_BLUETOOTH >= (int)(sizeof(g_sock_family) / sizeof(g_sock_family[0])))
    {
      snprintf(out, out_len,
               "upstream-mgmt-socket: family=PF_BLUETOOTH invalid\n");
      return -EAFNOSUPPORT;
    }

  family = g_sock_family[PF_BLUETOOTH];
  if (family == NULL || family->create == NULL)
    {
      snprintf(out, out_len,
               "upstream-mgmt-socket: family=PF_BLUETOOTH unregistered\n");
      return -EAFNOSUPPORT;
    }

  sock.type = SOCK_RAW;
  create_ret = family->create(&init_net, &sock, BTPROTO_HCI, 0);
  if (create_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-mgmt-socket: create-ret=%d\n", create_ret);
      return create_ret;
    }

  haddr.hci_family = AF_BLUETOOTH;
  haddr.hci_dev = HCI_DEV_NONE;
  haddr.hci_channel = HCI_CHANNEL_CONTROL;

  bind_ret = sock.ops != NULL && sock.ops->bind != NULL ?
             sock.ops->bind(&sock, (struct sockaddr *)&haddr,
                            sizeof(haddr)) : -EOPNOTSUPP;
  if (bind_ret < 0)
    {
      snprintf(out, out_len,
               "upstream-mgmt-socket: create-ret=%d bind-ret=%d\n",
               create_ret, bind_ret);
      goto out;
    }

  hdr->opcode = cpu_to_le16(opcode);
  hdr->index = cpu_to_le16(index);

  switch (opcode)
    {
      case MGMT_OP_SET_POWERED:
      case MGMT_OP_SET_CONNECTABLE:
      case MGMT_OP_SET_BONDABLE:
      case MGMT_OP_SET_LE:
      case MGMT_OP_SET_ADVERTISING:
      case MGMT_OP_SET_BREDR:
        hdr->len = cpu_to_le16(1);
        payload[sizeof(*hdr)] = param;
        payload_len++;
        break;

      case MGMT_OP_SET_IO_CAPABILITY:
        {
          struct mgmt_cp_set_io_capability *cp =
            (struct mgmt_cp_set_io_capability *)(payload + sizeof(*hdr));

          hdr->len = cpu_to_le16(sizeof(*cp));
          memset(cp, 0, sizeof(*cp));
          cp->io_capability = param <= LINUX_BT_HCI_IO_CAPABILITY_MAX ?
                              param : g_hci_io_capability;
          payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_SET_DISCOVERABLE:
        hdr->len = cpu_to_le16(3);
        payload[sizeof(*hdr)] = param;
        payload_len += 3;
        break;

      case MGMT_OP_START_DISCOVERY:
      case MGMT_OP_STOP_DISCOVERY:
        {
          struct mgmt_cp_start_discovery *cp =
            (struct mgmt_cp_start_discovery *)(payload + sizeof(*hdr));

          hdr->len = cpu_to_le16(sizeof(*cp));
          memset(cp, 0, sizeof(*cp));
          cp->type = param != 0 ? param :
                     LINUX_BT_HCI_DISCOVERY_TYPES_SUPPORTED;
          payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_BLOCK_DEVICE:
      case MGMT_OP_UNBLOCK_DEVICE:
        {
          struct mgmt_cp_block_device *cp =
            (struct mgmt_cp_block_device *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          hdr->len = cpu_to_le16(sizeof(*cp));
          memset(cp, 0, sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_BREDR;
          payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_PAIR_DEVICE:
        {
          struct mgmt_cp_pair_device *cp =
            (struct mgmt_cp_pair_device *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          hdr->len = cpu_to_le16(sizeof(*cp));
          memset(cp, 0, sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_LE_PUBLIC;
          cp->io_cap = g_hci_io_capability;
          payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_CANCEL_PAIR_DEVICE:
        {
          struct mgmt_addr_info *addr_info =
            (struct mgmt_addr_info *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&addr_info->bdaddr;

          hdr->len = cpu_to_le16(sizeof(*addr_info));
          memset(addr_info, 0, sizeof(*addr_info));
          addr[0] = param != 0 ? param : 1;
          addr_info->type = BDADDR_LE_PUBLIC;
          payload_len += sizeof(*addr_info);
        }
        break;

      case MGMT_OP_UNPAIR_DEVICE:
        {
          struct mgmt_cp_unpair_device *cp =
            (struct mgmt_cp_unpair_device *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          hdr->len = cpu_to_le16(sizeof(*cp));
          memset(cp, 0, sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_LE_PUBLIC;
          cp->disconnect = 1;
          payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_USER_CONFIRM_REPLY:
      case MGMT_OP_USER_CONFIRM_NEG_REPLY:
        {
          struct mgmt_cp_user_confirm_reply *cp =
            (struct mgmt_cp_user_confirm_reply *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          hdr->len = cpu_to_le16(sizeof(*cp));
          memset(cp, 0, sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_LE_PUBLIC;
          payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_GET_CONN_INFO:
      case MGMT_OP_GET_CLOCK_INFO:
      case MGMT_OP_GET_DEVICE_FLAGS:
        {
          struct mgmt_cp_get_conn_info *cp =
            (struct mgmt_cp_get_conn_info *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          hdr->len = cpu_to_le16(sizeof(*cp));
          memset(cp, 0, sizeof(*cp));
          addr[0] = opcode == MGMT_OP_GET_DEVICE_FLAGS &&
                    param == 0 ? 1 : param;
          cp->addr.type = BDADDR_BREDR;
          payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_SET_DEVICE_FLAGS:
        {
          struct mgmt_cp_set_device_flags *cp =
            (struct mgmt_cp_set_device_flags *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          hdr->len = cpu_to_le16(sizeof(*cp));
          memset(cp, 0, sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_BREDR;
          cp->current_flags = cpu_to_le32(param != 0 ? 1 : 0);
          payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_ADD_DEVICE:
        {
          struct mgmt_cp_add_device *cp =
            (struct mgmt_cp_add_device *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          hdr->len = cpu_to_le16(sizeof(*cp));
          memset(cp, 0, sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_BREDR;
          cp->action = 0x01;
          payload_len += sizeof(*cp);
        }
        break;

      case MGMT_OP_REMOVE_DEVICE:
        {
          struct mgmt_cp_remove_device *cp =
            (struct mgmt_cp_remove_device *)(payload + sizeof(*hdr));
          uint8_t *addr = (uint8_t *)&cp->addr.bdaddr;

          hdr->len = cpu_to_le16(sizeof(*cp));
          memset(cp, 0, sizeof(*cp));
          addr[0] = param != 0 ? param : 1;
          cp->addr.type = BDADDR_BREDR;
          payload_len += sizeof(*cp);
        }
        break;

      default:
        hdr->len = cpu_to_le16(0);
        break;
    }

  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  msg.msg_flags = 0;

  send_ret = sock.ops != NULL && sock.ops->sendmsg != NULL ?
             sock.ops->sendmsg(&sock, &msg, payload_len) : -EOPNOTSUPP;

  if (sock.ops != NULL && sock.ops->recvmsg != NULL)
    {
      recvmsg.msg_iter.data = (char *)recvbuf;
      recvmsg.msg_iter.count = sizeof(recvbuf);
      recv_ret = sock.ops->recvmsg(&sock, &recvmsg, sizeof(recvbuf), 0);
    }

  complete = (struct mgmt_ev_cmd_complete *)(recvbuf + sizeof(*evhdr));

  snprintf(out, out_len,
           "upstream-mgmt-socket: opcode=0x%04x index=0x%04x param=%u "
           "create-ret=%d bind-ret=%d send-ret=%d recv-ret=%d "
           "recv-flags=0x%x "
           "event=0x%04x event-index=0x%04x event-len=%u "
           "cmd-opcode=0x%04x status=%u\n",
           opcode, index, param, create_ret, bind_ret, send_ret, recv_ret,
           recvmsg.msg_flags,
           recv_ret >= (int)(sizeof(*evhdr) + sizeof(*complete)) ?
           le16_to_cpu(evhdr->opcode) : 0,
           recv_ret >= (int)(sizeof(*evhdr) + sizeof(*complete)) ?
           le16_to_cpu(evhdr->index) : 0,
           recv_ret >= (int)(sizeof(*evhdr) + sizeof(*complete)) ?
           le16_to_cpu(evhdr->len) : 0,
           recv_ret >= (int)(sizeof(*evhdr) + sizeof(*complete)) ?
           le16_to_cpu(complete->opcode) : 0,
           recv_ret >= (int)(sizeof(*evhdr) + sizeof(*complete)) ?
           complete->status : 0);

out:
  if (sock.ops != NULL && sock.ops->release != NULL)
    {
      sock.ops->release(&sock);
    }

  return bind_ret < 0 ? bind_ret : send_ret;
}
