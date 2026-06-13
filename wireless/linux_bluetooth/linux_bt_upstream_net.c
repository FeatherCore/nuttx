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
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/kmalloc.h>
#include <nuttx/wireless/linux_bluetooth.h>

#include <linux/module.h>
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

#ifndef cpu_to_le64
#  define cpu_to_le64(v) htole64(v)
#endif

#ifndef le64_to_cpu
#  define le64_to_cpu(v) le64toh(v)
#endif

#ifndef ETH_ALEN
#  define ETH_ALEN 6
#endif

#ifndef BNEPCONNADD
#  define BNEPCONNADD      0x42c8
#  define BNEPCONNDEL      0x42c9
#  define BNEPGETCONNLIST  0x42d2
#  define BNEPGETCONNINFO  0x42d3
#  define BNEPGETSUPPFEAT  0x42d4
#endif

#define LINUX_BT_BNEP_SETUP_RESPONSE 0
#define LINUX_BT_BNEP_SVC_PANU       0x1115
#define LINUX_BT_BNEP_STAGING_STATE_CONNECTED 1
#define LINUX_BT_BNEP_STAGING_MAX_SESSIONS    4
#define LINUX_BT_BNEP_CONNADD_KEPT_L2CAP      UINT32_MAX
#define LINUX_BT_BNEP_STAGING_MTU             1500
#define LINUX_BT_NATIVE_BNEPCONNADD           _IOW('B', 200, int)
#define LINUX_BT_NATIVE_BNEPCONNDEL           _IOW('B', 201, int)
#define LINUX_BT_NATIVE_BNEPGETCONNLIST       _IOR('B', 210, int)
#define LINUX_BT_NATIVE_BNEPGETCONNINFO       _IOR('B', 211, int)
#define LINUX_BT_NATIVE_BNEPGETSUPPFEAT       _IOR('B', 212, int)

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

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP
extern int bnep_recv_l2cap_payload(uint16_t cid, const void *payload,
                                   size_t payload_len);
#endif
#ifdef CONFIG_NET_LINUX_BLUETOOTH_6LOWPAN_BRIDGE
extern int linux_bt_6lowpan_recv_l2cap_payload(uint16_t cid,
                                               const void *payload,
                                               size_t payload_len);
#endif

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
static unsigned long g_bnep_socket_registers;
static unsigned long g_bnep_socket_unregisters;
static unsigned long g_bnep_socket_ioctl_suppfeat;
static unsigned long g_bnep_socket_ioctl_connlist;
static unsigned long g_bnep_socket_ioctl_conninfo;
static unsigned long g_bnep_socket_ioctl_connadd;
static unsigned long g_bnep_socket_ioctl_conndel;
static unsigned long g_bnep_staging_session_adds;
static unsigned long g_bnep_staging_session_dels;
static unsigned long g_bnep_staging_netdev_registers;
static unsigned long g_bnep_staging_netdev_unregisters;
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
static unsigned long g_iso_socket_registers;
static unsigned long g_iso_socket_unregisters;
static unsigned long g_iso_socket_binds;
static unsigned long g_iso_socket_connects;
static unsigned long g_iso_socket_sends;
static unsigned long g_iso_socket_recvs;
static unsigned long g_iso_socket_native_recvs;
static struct hci_mgmt_chan *g_hci_mgmt_chan[8];
static struct sock *g_hci_monitor_socks[4];
static struct sock *g_hci_data_socks[8];
static struct sock *g_hci_control_socks[4];
static bool g_hci_sock_proto_registered;
static bool g_l2cap_sock_proto_registered;
static bool g_bnep_sock_proto_registered;
static bool g_iso_sock_proto_registered;
static bool g_linux_bt_mgmt_chan_registered;

struct linux_bt_bnep_staging_session
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

static struct linux_bt_bnep_staging_session
  g_bnep_staging_sessions[LINUX_BT_BNEP_STAGING_MAX_SESSIONS];

static unsigned int linux_bt_bnep_staging_active_count(void)
{
  unsigned int active = 0;
  unsigned int i;

  for (i = 0; i < LINUX_BT_BNEP_STAGING_MAX_SESSIONS; i++)
    {
      if (g_bnep_staging_sessions[i].used)
        {
          active++;
        }
    }

  return active;
}

static struct linux_bt_bnep_staging_session *
linux_bt_bnep_first_active_session(void)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_BNEP_STAGING_MAX_SESSIONS; i++)
    {
      if (g_bnep_staging_sessions[i].used)
        {
          return &g_bnep_staging_sessions[i];
        }
    }

  return NULL;
}

static void linux_bt_bnep_staging_netdev_register(
  struct linux_bt_bnep_staging_session *session)
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
  session->netdev_mtu = LINUX_BT_BNEP_STAGING_MTU;
  session->netdev_registered = true;
  g_bnep_staging_netdev_registers++;
}

static void linux_bt_bnep_staging_netdev_unregister(
  struct linux_bt_bnep_staging_session *session)
{
  if (session == NULL || !session->netdev_registered)
    {
      return;
    }

  session->netdev_registered = false;
  linux_bt_bnep_hwsim_netdev_unregister();
  g_bnep_staging_netdev_unregisters++;
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

struct linux_bt_hci_conn_snapshot
{
  bool used;
  struct sock *owner;
  uint16_t dev_id;
  uint8_t bdaddr_type;
  uint8_t auth_type;
  struct hci_conn_info info;
};

static struct linux_bt_hci_conn_snapshot g_hci_conn_snapshots[16];
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
  uint8_t fallback[16];
  size_t fallback_len;
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
  uint16_t psm;
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
};

static struct linux_bt_l2cap_bind_state g_l2cap_bound_socks[8];
static struct socket g_l2cap_probe_socket;
static struct file g_l2cap_probe_file;
static unsigned int g_l2cap_probe_bnep_refs;
static bool g_l2cap_probe_close_pending;
static bool g_l2cap_probe_bound;

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
          g_l2cap_bound_socks[i].sk = sk;
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
  uint8_t addr[6];
  uint16_t peer;

  if (handle < 0x0050)
    {
      return;
    }

  hdev = linux_bt_upstream_vhci_default_hdev();
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
  uint8_t data[512];
  size_t len;
};

static struct linux_bt_l2cap_pending_payload g_l2cap_pending_payloads[8];

static int linux_bt_hci_conn_snapshot_upsert(struct sock *owner,
                                             uint8_t type,
                                             uint16_t handle,
                                             const bdaddr_t *bdaddr,
                                             uint8_t bdaddr_type,
                                             uint8_t out,
                                             uint16_t state,
                                             uint32_t link_mode,
                                             uint8_t auth_type);

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
  struct hci_conn *hcon;
  struct l2cap_chan *chan;

  if (sock == NULL || sock->sk == NULL || handle == 0)
    {
      return -ENOTCONN;
    }

  linux_bt_l2cap_ensure_sim_br_conn(handle);

  hdev = linux_bt_upstream_vhci_default_hdev();
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  hcon = hci_conn_hash_lookup_handle(hdev, handle);
  if (hcon == NULL)
    {
      return -ENOTCONN;
    }

  chan = l2cap_pi(sock->sk)->chan;
  if (chan == NULL)
    {
      return -ENOTCONN;
    }

  sock->sk->sk_state = BT_CONNECTED;
  if (g_l2cap_probe_sim_hchan == NULL ||
      g_l2cap_probe_sim_hchan->conn != hcon)
    {
      g_l2cap_probe_sim_hchan = hci_chan_create(hcon);
      if (g_l2cap_probe_sim_hchan == NULL)
        {
          return -ENOMEM;
        }
    }

  memset(&g_l2cap_probe_sim_conn, 0, sizeof(g_l2cap_probe_sim_conn));
  g_l2cap_probe_sim_conn.hcon = hcon;
  g_l2cap_probe_sim_conn.hchan = g_l2cap_probe_sim_hchan;
  g_l2cap_probe_sim_conn.mtu = hcon->mtu != 0 ? hcon->mtu :
                               L2CAP_DEFAULT_MTU;
  g_l2cap_probe_sim_conn.feat_mask = 0;

  chan->conn = &g_l2cap_probe_sim_conn;
  chan->state = BT_CONNECTED;
  chan->mode = L2CAP_MODE_BASIC;
  chan->chan_type = L2CAP_CHAN_CONN_ORIENTED;
  chan->scid = cid != 0 ? cid : L2CAP_CID_DYN_START;
  chan->dcid = chan->scid;
  chan->omtu = L2CAP_DEFAULT_MTU;
  if (chan->imtu == 0)
    {
      chan->imtu = L2CAP_DEFAULT_MTU;
    }

  return 0;
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
  return linux_bt_hci_conn_snapshot_upsert(
    g_l2cap_probe_socket.sk,
    state->bdaddr_type != 0 ? LE_LINK : ACL_LINK, state->handle,
    &state->bdaddr, state->bdaddr_type, 1,
    g_l2cap_probe_socket.sk->sk_state, 0, 0);
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

  if (payload == NULL || payload_len == 0)
    {
      return;
    }

  for (i = 0; i < sizeof(g_l2cap_pending_payloads) /
                  sizeof(g_l2cap_pending_payloads[0]); i++)
    {
      if (g_l2cap_pending_payloads[i].valid &&
          g_l2cap_pending_payloads[i].handle == handle &&
          g_l2cap_pending_payloads[i].cid == cid)
        {
          break;
        }
    }

  if (i == sizeof(g_l2cap_pending_payloads) /
           sizeof(g_l2cap_pending_payloads[0]))
    {
      for (i = 0; i < sizeof(g_l2cap_pending_payloads) /
                      sizeof(g_l2cap_pending_payloads[0]); i++)
        {
          if (!g_l2cap_pending_payloads[i].valid)
            {
              break;
            }
        }
    }

  if (i == sizeof(g_l2cap_pending_payloads) /
           sizeof(g_l2cap_pending_payloads[0]))
    {
      i = 0;
    }

  if (payload_len > sizeof(g_l2cap_pending_payloads[i].data))
    {
      payload_len = sizeof(g_l2cap_pending_payloads[i].data);
    }

  g_l2cap_pending_payloads[i].valid = true;
  g_l2cap_pending_payloads[i].handle = handle;
  g_l2cap_pending_payloads[i].cid = cid;
  g_l2cap_pending_payloads[i].len = payload_len;
  memcpy(g_l2cap_pending_payloads[i].data, payload, payload_len);
}

static bool linux_bt_l2cap_pending_pop(uint16_t handle, uint16_t cid,
                                       void *payload, size_t max_len,
                                       size_t *payload_len)
{
  unsigned int i;

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

      *payload_len = g_l2cap_pending_payloads[i].len;
      if (*payload_len > max_len)
        {
          *payload_len = max_len;
        }

      if (*payload_len > 0)
        {
          memcpy(payload, g_l2cap_pending_payloads[i].data, *payload_len);
        }

      memset(&g_l2cap_pending_payloads[i], 0,
             sizeof(g_l2cap_pending_payloads[i]));
      return true;
    }

  return false;
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

static int linux_bt_hci_mgmt_broadcast_conn_event(
  const struct linux_bt_hci_conn_snapshot *snapshot,
  bool connected);
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
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
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
  .name = "HCI-staging",
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

static int linux_bt_hci_conn_snapshot_upsert(struct sock *owner,
                                             uint8_t type,
                                             uint16_t handle,
                                             const bdaddr_t *bdaddr,
                                             uint8_t bdaddr_type,
                                             uint8_t out,
                                             uint16_t state,
                                             uint32_t link_mode,
                                             uint8_t auth_type)
{
  struct linux_bt_hci_conn_snapshot *slot = NULL;
  struct linux_bt_hci_conn_snapshot old;
  bool had_old = false;
  unsigned int i;

  memset(&old, 0, sizeof(old));
  if (owner == NULL)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(g_hci_conn_snapshots) /
                  sizeof(g_hci_conn_snapshots[0]); i++)
    {
      if (g_hci_conn_snapshots[i].used &&
          g_hci_conn_snapshots[i].owner == owner &&
          g_hci_conn_snapshots[i].info.type == type)
        {
          old = g_hci_conn_snapshots[i];
          had_old = true;
          slot = &g_hci_conn_snapshots[i];
          break;
        }

      if (g_hci_conn_snapshots[i].used &&
          g_hci_conn_snapshots[i].owner == owner)
        {
          if (g_hci_conn_snapshots[i].info.state == BT_CONNECTED)
            {
              linux_bt_hci_mgmt_broadcast_conn_event(
                &g_hci_conn_snapshots[i], false);
            }

          memset(&g_hci_conn_snapshots[i], 0,
                 sizeof(g_hci_conn_snapshots[i]));
          if (slot == NULL)
            {
              slot = &g_hci_conn_snapshots[i];
            }

          continue;
        }

      if (!g_hci_conn_snapshots[i].used && slot == NULL)
        {
          slot = &g_hci_conn_snapshots[i];
        }
    }

  if (slot == NULL)
    {
      return -ENOMEM;
    }

  if (had_old && old.info.state == BT_CONNECTED && state != BT_CONNECTED)
    {
      linux_bt_hci_mgmt_broadcast_conn_event(&old, false);
    }

  memset(slot, 0, sizeof(*slot));
  slot->used = true;
  slot->owner = owner;
  slot->dev_id = 0;
  slot->bdaddr_type = bdaddr_type;
  slot->auth_type = auth_type;
  slot->info.handle = handle;
  slot->info.type = type;
  slot->info.out = out;
  slot->info.state = state;
  slot->info.link_mode = link_mode;
  if (bdaddr != NULL)
    {
      bacpy(&slot->info.bdaddr, bdaddr);
    }
  else
    {
      bacpy(&slot->info.bdaddr, BDADDR_ANY);
    }

  if (state == BT_CONNECTED &&
      (!had_old || old.info.state != BT_CONNECTED))
    {
      linux_bt_hci_mgmt_broadcast_conn_event(slot, true);
    }

  return 0;
}

static void linux_bt_hci_conn_snapshot_delete_owner(struct sock *owner)
{
  unsigned int i;

  if (owner == NULL)
    {
      return;
    }

  for (i = 0; i < sizeof(g_hci_conn_snapshots) /
                  sizeof(g_hci_conn_snapshots[0]); i++)
    {
      if (g_hci_conn_snapshots[i].used &&
          g_hci_conn_snapshots[i].owner == owner)
        {
          if (g_hci_conn_snapshots[i].info.state == BT_CONNECTED)
            {
              linux_bt_hci_mgmt_broadcast_conn_event(
                &g_hci_conn_snapshots[i], false);
            }

          memset(&g_hci_conn_snapshots[i], 0,
                 sizeof(g_hci_conn_snapshots[i]));
        }
    }
}

static bool linux_bt_hci_sock_conn_match(
  const struct hci_conn_info *info,
  const struct hci_conn_info_req *req)
{
  if (info == NULL || req == NULL)
    {
      return false;
    }

  return info->type == req->type &&
         memcmp(&info->bdaddr, &req->bdaddr, sizeof(info->bdaddr)) == 0;
}

static int linux_bt_hci_sock_get_conn_list(unsigned long arg)
{
  struct hci_conn_list_req *list =
    (struct hci_conn_list_req *)(uintptr_t)arg;
  struct hci_dev *hdev;
  uint16_t max;
  uint16_t count = 0;
  unsigned int i;

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

  for (i = 0; i < sizeof(g_hci_conn_snapshots) /
                  sizeof(g_hci_conn_snapshots[0]) &&
                  count < max; i++)
    {
      if (!g_hci_conn_snapshots[i].used ||
          g_hci_conn_snapshots[i].dev_id != hdev->id)
        {
          continue;
        }

      memcpy(&list->conn_info[count], &g_hci_conn_snapshots[i].info,
             sizeof(list->conn_info[count]));
      count++;
    }

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
  unsigned int i;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (arg == 0)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(g_hci_conn_snapshots) /
                  sizeof(g_hci_conn_snapshots[0]); i++)
    {
      if (!g_hci_conn_snapshots[i].used ||
          g_hci_conn_snapshots[i].dev_id != hdev->id)
        {
          continue;
        }

      if (linux_bt_hci_sock_conn_match(&g_hci_conn_snapshots[i].info, req))
        {
          memcpy(&req->conn_info[0], &g_hci_conn_snapshots[i].info,
                 sizeof(g_hci_conn_snapshots[i].info));
          g_hci_ioctl_conninfo_calls++;
          return 0;
        }
    }

  return -ENOENT;
}

static const struct linux_bt_hci_conn_snapshot *
linux_bt_hci_conn_snapshot_lookup(uint16_t dev_id,
                                  const struct mgmt_addr_info *addr)
{
  uint8_t type;
  unsigned int i;

  if (addr == NULL)
    {
      return NULL;
    }

  type = addr->type == BDADDR_BREDR ? ACL_LINK : LE_LINK;
  for (i = 0; i < sizeof(g_hci_conn_snapshots) /
                  sizeof(g_hci_conn_snapshots[0]); i++)
    {
      if (!g_hci_conn_snapshots[i].used ||
          g_hci_conn_snapshots[i].dev_id != dev_id ||
          g_hci_conn_snapshots[i].info.type != type ||
          g_hci_conn_snapshots[i].info.state != BT_CONNECTED ||
          memcmp(&g_hci_conn_snapshots[i].info.bdaddr, &addr->bdaddr,
                 sizeof(addr->bdaddr)) != 0)
        {
          continue;
        }

      return &g_hci_conn_snapshots[i];
    }

  return NULL;
}

static int linux_bt_hci_sock_get_auth_info(struct hci_dev *hdev,
                                           unsigned long arg)
{
  struct hci_auth_info_req *req =
    (struct hci_auth_info_req *)(uintptr_t)arg;
  unsigned int i;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (arg == 0)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(g_hci_conn_snapshots) /
                  sizeof(g_hci_conn_snapshots[0]); i++)
    {
      if (!g_hci_conn_snapshots[i].used ||
          g_hci_conn_snapshots[i].dev_id != hdev->id ||
          g_hci_conn_snapshots[i].info.type != ACL_LINK)
        {
          continue;
        }

      if (memcmp(&g_hci_conn_snapshots[i].info.bdaddr, &req->bdaddr,
                 sizeof(req->bdaddr)) == 0)
        {
          req->type = g_hci_conn_snapshots[i].auth_type != 0 ?
                      g_hci_conn_snapshots[i].auth_type :
                      hdev->auth_enable;
          g_hci_ioctl_authinfo_calls++;
          return 0;
        }
    }

  return -ENOENT;
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
  struct mgmt_ev_device_connected ev;
  uint8_t link_type;
  unsigned int i;
  int ret;
  int sent = 0;

  if (hdev == NULL || addr == NULL || owner == NULL ||
      !linux_bt_hci_mgmt_addr_type_valid(addr->type) ||
      linux_bt_hci_sock_addr_is_any(&addr->bdaddr))
    {
      return -EINVAL;
    }

  link_type = addr->type == BDADDR_BREDR ? ACL_LINK : LE_LINK;
  ret = linux_bt_hci_conn_snapshot_upsert(owner, link_type, 0x0040,
                                          &addr->bdaddr, addr->type, 1,
                                          BT_CONNECTED, 0, addr->type);
  if (ret < 0)
    {
      return ret;
    }

  memset(&ev, 0, sizeof(ev));
  memcpy(&ev.addr, addr, sizeof(ev.addr));
  ev.flags = cpu_to_le32(MGMT_DEV_FOUND_INITIATED_CONN);
  ev.eir_len = cpu_to_le16(0);

  for (i = 0; i < sizeof(g_hci_control_socks) /
                  sizeof(g_hci_control_socks[0]); i++)
    {
      if (g_hci_control_socks[i] == NULL ||
          g_hci_control_socks[i]->sk_state != BT_BOUND)
        {
          continue;
        }

      if (linux_bt_hci_sock_queue_mgmt_event(
            g_hci_control_socks[i], MGMT_EV_DEVICE_CONNECTED, hdev->id,
            &ev, sizeof(ev)) == 0)
        {
          sent++;
        }
    }

  return sent;
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
  uint8_t link_type;
  unsigned int i;

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
      link_type = cp->addr.type == BDADDR_BREDR ? ACL_LINK : LE_LINK;
      for (i = 0; i < sizeof(g_hci_conn_snapshots) /
                      sizeof(g_hci_conn_snapshots[0]); i++)
        {
          if (!g_hci_conn_snapshots[i].used ||
              g_hci_conn_snapshots[i].dev_id != hdev->id ||
              g_hci_conn_snapshots[i].info.type != link_type ||
              memcmp(&g_hci_conn_snapshots[i].info.bdaddr,
                     &cp->addr.bdaddr, sizeof(cp->addr.bdaddr)) != 0)
            {
              continue;
            }

          if (g_hci_conn_snapshots[i].info.state == BT_CONNECTED)
            {
              linux_bt_hci_mgmt_broadcast_conn_event(
                &g_hci_conn_snapshots[i], false);
            }

          memset(&g_hci_conn_snapshots[i], 0,
                 sizeof(g_hci_conn_snapshots[i]));
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

static uint8_t linux_bt_hci_mgmt_addr_type(
  const struct linux_bt_hci_conn_snapshot *snapshot)
{
  if (snapshot == NULL || snapshot->info.type == ACL_LINK)
    {
      return BDADDR_BREDR;
    }

  return snapshot->bdaddr_type != 0 ? snapshot->bdaddr_type :
                                      BDADDR_LE_PUBLIC;
}

static int linux_bt_hci_mgmt_broadcast_conn_event(
  const struct linux_bt_hci_conn_snapshot *snapshot,
  bool connected)
{
  unsigned int i;
  int sent = 0;

  if (snapshot == NULL)
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
          bacpy(&ev.addr.bdaddr, &snapshot->info.bdaddr);
          ev.addr.type = linux_bt_hci_mgmt_addr_type(snapshot);
          ev.flags = cpu_to_le32(snapshot->info.out != 0 ?
                                 MGMT_DEV_FOUND_INITIATED_CONN : 0);
          ev.eir_len = cpu_to_le16(0);
          if (linux_bt_hci_sock_queue_mgmt_event(
                g_hci_control_socks[i], MGMT_EV_DEVICE_CONNECTED,
                snapshot->dev_id, &ev, sizeof(ev)) == 0)
            {
              sent++;
            }
        }
      else
        {
          struct mgmt_ev_device_disconnected ev;

          memset(&ev, 0, sizeof(ev));
          bacpy(&ev.addr.bdaddr, &snapshot->info.bdaddr);
          ev.addr.type = linux_bt_hci_mgmt_addr_type(snapshot);
          ev.reason = MGMT_DEV_DISCONN_LOCAL_HOST;
          if (linux_bt_hci_sock_queue_mgmt_event(
                g_hci_control_socks[i], MGMT_EV_DEVICE_DISCONNECTED,
                snapshot->dev_id, &ev, sizeof(ev)) == 0)
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
          uint8_t link_type = 0;
          unsigned int i;
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
              link_type = cp->addr.type == BDADDR_BREDR ? ACL_LINK :
                          LE_LINK;
              for (i = 0; i < sizeof(g_hci_conn_snapshots) /
                              sizeof(g_hci_conn_snapshots[0]); i++)
                {
                  if (!g_hci_conn_snapshots[i].used ||
                      g_hci_conn_snapshots[i].dev_id != hdev->id ||
                      g_hci_conn_snapshots[i].info.type != link_type ||
                      memcmp(&g_hci_conn_snapshots[i].info.bdaddr,
                             &cp->addr.bdaddr,
                             sizeof(cp->addr.bdaddr)) != 0)
                    {
                      continue;
                    }

                  found = true;
                  if (g_hci_conn_snapshots[i].info.state == BT_CONNECTED)
                    {
                      struct mgmt_ev_device_disconnected ev;

                      linux_bt_hci_mgmt_broadcast_conn_event(
                        &g_hci_conn_snapshots[i], false);
                      memset(&ev, 0, sizeof(ev));
                      memcpy(&ev.addr, &cp->addr, sizeof(ev.addr));
                      ev.reason = MGMT_DEV_DISCONN_LOCAL_HOST;
                      linux_bt_hci_sock_queue_mgmt_event(
                        sk, MGMT_EV_DEVICE_DISCONNECTED, hdev->id, &ev,
                        sizeof(ev));
                    }

                  memset(&g_hci_conn_snapshots[i], 0,
                         sizeof(g_hci_conn_snapshots[i]));
                  break;
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
          const struct linux_bt_hci_conn_snapshot *snapshot = NULL;

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
              snapshot = linux_bt_hci_conn_snapshot_lookup(hdev->id,
                                                           &cp->addr);
              status = snapshot != NULL ? MGMT_STATUS_SUCCESS :
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
          const struct linux_bt_hci_conn_snapshot *snapshot = NULL;
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
              snapshot = linux_bt_hci_conn_snapshot_lookup(hdev->id,
                                                           &cp->addr);
              status = snapshot != NULL ? MGMT_STATUS_SUCCESS :
                                          MGMT_STATUS_NOT_CONNECTED;
            }

          if (status == MGMT_STATUS_SUCCESS)
            {
              rp.local_clock = cpu_to_le32(0);
              rp.piconet_clock = cpu_to_le32(
                snapshot != NULL ? snapshot->info.handle : 0);
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
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
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
#else
  return 0;
#endif
}

void hci_sock_cleanup(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
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
static int linux_bt_hci_conn_snapshot_upsert(struct sock *owner,
                                             uint8_t type,
                                             uint16_t handle,
                                             const bdaddr_t *bdaddr,
                                             uint8_t bdaddr_type,
                                             uint8_t out,
                                             uint16_t state,
                                             uint32_t link_mode,
                                             uint8_t auth_type)
{
  struct linux_bt_hci_conn_snapshot *slot = NULL;
  unsigned int i;

  if (owner == NULL)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(g_hci_conn_snapshots) /
                  sizeof(g_hci_conn_snapshots[0]); i++)
    {
      if (g_hci_conn_snapshots[i].used &&
          g_hci_conn_snapshots[i].owner == owner &&
          g_hci_conn_snapshots[i].info.type == type)
        {
          slot = &g_hci_conn_snapshots[i];
          break;
        }

      if (slot == NULL && !g_hci_conn_snapshots[i].used)
        {
          slot = &g_hci_conn_snapshots[i];
        }
    }

  if (slot == NULL)
    {
      return -ENOMEM;
    }

  memset(slot, 0, sizeof(*slot));
  slot->used = true;
  slot->owner = owner;
  slot->dev_id = 0;
  slot->bdaddr_type = bdaddr_type;
  slot->auth_type = auth_type;
  slot->info.handle = handle;
  slot->info.type = type;
  slot->info.out = out;
  slot->info.state = state;
  slot->info.link_mode = link_mode;
  if (bdaddr != NULL)
    {
      bacpy(&slot->info.bdaddr, bdaddr);
    }

  return 0;
}

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
static int linux_bt_hci_conn_snapshot_upsert(struct sock *owner,
                                             uint8_t type,
                                             uint16_t handle,
                                             const bdaddr_t *bdaddr,
                                             uint8_t bdaddr_type,
                                             uint8_t out,
                                             uint16_t state,
                                             uint32_t link_mode,
                                             uint8_t auth_type)
{
  struct linux_bt_hci_conn_snapshot *slot = NULL;
  unsigned int i;

  if (owner == NULL)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(g_hci_conn_snapshots) /
                  sizeof(g_hci_conn_snapshots[0]); i++)
    {
      if (g_hci_conn_snapshots[i].used &&
          g_hci_conn_snapshots[i].owner == owner &&
          g_hci_conn_snapshots[i].info.type == type)
        {
          slot = &g_hci_conn_snapshots[i];
          break;
        }

      if (slot == NULL && !g_hci_conn_snapshots[i].used)
        {
          slot = &g_hci_conn_snapshots[i];
        }
    }

  if (slot == NULL)
    {
      return -ENOMEM;
    }

  memset(slot, 0, sizeof(*slot));
  slot->used = true;
  slot->owner = owner;
  slot->dev_id = 0;
  slot->bdaddr_type = bdaddr_type;
  slot->auth_type = auth_type;
  slot->info.handle = handle;
  slot->info.type = type;
  slot->info.out = out;
  slot->info.state = state;
  slot->info.link_mode = link_mode;
  if (bdaddr != NULL)
    {
      bacpy(&slot->info.bdaddr, bdaddr);
    }

  return 0;
}

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
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
static struct proto g_linux_bt_l2cap_sock_proto =
{
  .name = "L2CAP-staging",
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
      linux_bt_hci_conn_snapshot_delete_owner(sock->sk);
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
  int ret;

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
          sock->sk->sk_state = BT_BOUND;
          ret = linux_bt_hci_conn_snapshot_upsert(
            sock->sk,
            g_l2cap_bound_socks[i].bdaddr_type != 0 ? LE_LINK : ACL_LINK,
            g_l2cap_bound_socks[i].handle, &g_l2cap_bound_socks[i].bdaddr,
            g_l2cap_bound_socks[i].bdaddr_type, 0, sock->sk->sk_state,
            0, 0);
          if (ret < 0)
            {
              memset(&g_l2cap_bound_socks[i], 0,
                     sizeof(g_l2cap_bound_socks[i]));
              return ret;
            }

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
  ret = linux_bt_hci_conn_snapshot_upsert(
    sock->sk, state->bdaddr_type != 0 ? LE_LINK : ACL_LINK, state->handle,
    &state->bdaddr, state->bdaddr_type, 1, sock->sk->sk_state, 0, 0);
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
  int ret;

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
  ret = linux_bt_hci_conn_snapshot_upsert(
    sock->sk, state->bdaddr_type != 0 ? LE_LINK : ACL_LINK, state->handle,
    &state->bdaddr, state->bdaddr_type, 0, sock->sk->sk_state, 0, 0);
  if (ret < 0)
    {
      return ret;
    }

  g_l2cap_socket_listens++;
  return 0;
}

static int linux_bt_l2cap_sock_sendmsg(struct socket *sock,
                                       struct msghdr *msg,
                                       size_t len)
{
  struct linux_bt_l2cap_bind_state *state;
  uint8_t acl[4 + 4 + 512];
  uint16_t acl_len;
  uint16_t handle_pb;
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

  if (len == 0 || len > sizeof(acl) - 8 || msg->msg_iter.count < len)
    {
      return -EMSGSIZE;
    }

  state = linux_bt_l2cap_bound_state(sock->sk);
  if (state == NULL || state->cid == 0)
    {
      return -ENOTCONN;
    }

  acl_len = (uint16_t)(4 + len);
  handle_pb = (uint16_t)(state->handle | (0x02 << 12));
  acl[0] = (uint8_t)(handle_pb & 0xff);
  acl[1] = (uint8_t)(handle_pb >> 8);
  acl[2] = (uint8_t)(acl_len & 0xff);
  acl[3] = (uint8_t)(acl_len >> 8);
  acl[4] = (uint8_t)(len & 0xff);
  acl[5] = (uint8_t)(len >> 8);
  acl[6] = (uint8_t)(state->cid & 0xff);
  acl[7] = (uint8_t)(state->cid >> 8);
  memcpy(&acl[8], msg->msg_iter.data, len);

  ret = linux_bt_upstream_hci_send(HCI_ACLDATA_PKT, acl, len + 8);
  if (ret < 0)
    {
      printf("linux-bt-l2cap-sendmsg: hci-send ret=%d state=%d "
             "payload-len=%u cid=0x%04x handle=0x%04x\n",
             ret, sock->sk->sk_state, (unsigned int)len,
             state->cid, state->handle);
    }
  if (ret >= 0)
    {
      g_l2cap_socket_sends++;
      return (int)len;
    }

  return ret;
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
  .sendmsg = linux_bt_l2cap_sock_sendmsg,
  .recvmsg = linux_bt_l2cap_sock_recvmsg,
  .poll = datagram_poll,
  .listen = linux_bt_l2cap_sock_listen,
  .shutdown = sock_no_shutdown,
  .connect = linux_bt_l2cap_sock_connect,
  .socketpair = sock_no_socketpair,
  .accept = sock_no_accept,
  .mmap = sock_no_mmap,
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
#ifdef SOCK_SEQPACKET
      case SOCK_SEQPACKET:
#endif
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
  pi->chan = NULL;
  INIT_LIST_HEAD(&pi->rx_busy);

  sock->ops = &g_linux_bt_l2cap_sock_ops;
  sock->state = SS_UNCONNECTED;
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
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
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
#endif
  return 0;
}

void l2cap_exit(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
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

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
static struct proto g_linux_bt_bnep_sock_proto =
{
  .name = "BNEP-staging",
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

static struct linux_bt_bnep_staging_session *
linux_bt_bnep_find_session_by_dst(const uint8_t dst[ETH_ALEN])
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_BNEP_STAGING_MAX_SESSIONS; i++)
    {
      if (g_bnep_staging_sessions[i].used &&
          linux_bt_bnep_dst_equal(g_bnep_staging_sessions[i].ci.dst, dst))
        {
          return &g_bnep_staging_sessions[i];
        }
    }

  return NULL;
}

static struct linux_bt_bnep_staging_session *
linux_bt_bnep_find_free_session(void)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_BNEP_STAGING_MAX_SESSIONS; i++)
    {
      if (!g_bnep_staging_sessions[i].used)
        {
          return &g_bnep_staging_sessions[i];
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

  for (i = 0; i < LINUX_BT_BNEP_STAGING_MAX_SESSIONS && copied < max; i++)
    {
      if (g_bnep_staging_sessions[i].used)
        {
          ci[copied] = g_bnep_staging_sessions[i].ci;
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
          struct linux_bt_bnep_staging_session *session;

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
          struct linux_bt_bnep_staging_session *session;
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
          session->ci.state = LINUX_BT_BNEP_STAGING_STATE_CONNECTED;
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

          linux_bt_bnep_staging_netdev_register(session);
          snprintf(req->device, sizeof(req->device), "%s",
                   session->ci.device);
          g_bnep_staging_session_adds++;
          return 0;
        }

      case BNEPCONNDEL:
        {
          struct linux_bt_bnep_conndel_req *req =
            (struct linux_bt_bnep_conndel_req *)arg;
          struct linux_bt_bnep_staging_session *session;

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

          linux_bt_bnep_staging_netdev_unregister(session);
          memset(session, 0, sizeof(*session));
          g_bnep_staging_session_dels++;
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
#endif

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
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
  if (g_bnep_sock_proto_registered)
    {
      bt_sock_unregister(BTPROTO_BNEP);
      g_bnep_socket_unregisters++;
      proto_unregister(&g_linux_bt_bnep_sock_proto);
      g_bnep_sock_proto_registered = false;
    }
#endif
}
#endif

int sco_init(void)
{
  return 0;
}

void sco_exit(void)
{
}

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
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
static struct proto g_linux_bt_iso_sock_proto =
{
  .name = "ISO-staging",
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
      linux_bt_hci_conn_snapshot_delete_owner(sock->sk);
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
  int ret;

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
          ret = linux_bt_hci_conn_snapshot_upsert(
            sock->sk, linux_bt_iso_link_type(pi->handle), pi->handle,
            &g_iso_bound_socks[i].bdaddr, g_iso_bound_socks[i].bdaddr_type,
            0, sock->sk->sk_state, 0, 0);
          if (ret < 0)
            {
              memset(&g_iso_bound_socks[i], 0,
                     sizeof(g_iso_bound_socks[i]));
              return ret;
            }

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
  ret = linux_bt_hci_conn_snapshot_upsert(sock->sk,
                                          linux_bt_iso_link_type(pi->handle),
                                          pi->handle, &state->bdaddr,
                                          state->bdaddr_type, 1,
                                          sock->sk->sk_state, 0, 0);
  if (ret < 0)
    {
      return ret;
    }

  g_iso_socket_connects++;
  return 0;
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

static const struct proto_ops g_linux_bt_iso_sock_ops =
{
  .family = PF_BLUETOOTH,
  .owner = THIS_MODULE,
  .release = linux_bt_iso_sock_release,
  .bind = linux_bt_iso_sock_bind,
  .sendmsg = linux_bt_iso_sock_sendmsg,
  .recvmsg = linux_bt_iso_sock_recvmsg,
  .poll = datagram_poll,
  .listen = sock_no_listen,
  .shutdown = sock_no_shutdown,
  .connect = linux_bt_iso_sock_connect,
  .socketpair = sock_no_socketpair,
  .accept = sock_no_accept,
  .mmap = sock_no_mmap,
};

static int linux_bt_iso_sock_create(struct net *net, struct socket *sock,
                                    int protocol, int kern)
{
  struct sock *sk;

  if (sock == NULL)
    {
      return -EINVAL;
    }

  switch (sock->type)
    {
#ifdef SOCK_SEQPACKET
      case SOCK_SEQPACKET:
#endif
      case SOCK_STREAM:
      case SOCK_DGRAM:
      case SOCK_RAW:
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
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
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
#endif
  return 0;
}

int iso_exit(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF
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

                  (void)linux_bt_hci_conn_snapshot_upsert(
                    g_l2cap_bound_socks[i].sk,
                    g_l2cap_bound_socks[i].bdaddr_type != 0 ? LE_LINK :
                    ACL_LINK, g_l2cap_bound_socks[i].handle,
                    &g_l2cap_bound_socks[i].bdaddr,
                    g_l2cap_bound_socks[i].bdaddr_type, 1,
                    g_l2cap_bound_socks[i].sk->sk_state, 0, 0);

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

  return 0;
}

#ifdef CONFIG_SIM_BTHWSIM
static int linux_bt_sim_l2cap_poll_one(void)
{
  uint8_t payload[1024];
  uint16_t src;
  uint16_t dst;
  uint32_t len;
  int ret;

  ret = linux_bt_hwsim_read_raw_named(LINUX_BT_HWSIM_TYPE_ACL,
                                      "vhci", &src, &dst,
                                      payload, sizeof(payload), &len);
  if (ret <= 0)
    {
      return ret;
    }

  return linux_bt_upstream_proto_sock_recv(HCI_ACLDATA_PKT, payload, len);
}

static int linux_bt_sim_l2cap_send(uint16_t psm, uint16_t cid,
                                   uint16_t handle, const void *payload,
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
  frame[1] = (uint8_t)(handle >> 8);
  frame[2] = (uint8_t)(acl_len & 0xff);
  frame[3] = (uint8_t)(acl_len >> 8);
  frame[4] = (uint8_t)(payload_len & 0xff);
  frame[5] = (uint8_t)(payload_len >> 8);
  frame[6] = (uint8_t)(cid & 0xff);
  frame[7] = (uint8_t)(cid >> 8);
  if (payload_len > 0)
    {
      memcpy(&frame[8], payload, payload_len);
    }

  return linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL,
                             LINUX_BT_HWSIM_DST_BROADCAST,
                             frame, payload_len + 8);
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
           "families=%u sock-register=%lu sock-unregister=%lu "
           "sock-create-probe=%lu proto-register=%lu "
           "proto-unregister=%lu hci-mgmt-socket-cmd=%lu "
           "hci-mgmt-socket-event=%lu hci-mgmt-socket-recv=%lu "
           "hci-mgmt-chan-register=%lu hci-mgmt-chan-unregister=%lu "
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
           "l2cap-socket-register=%lu l2cap-socket-unregister=%lu "
           "l2cap-socket-bind=%lu l2cap-socket-connect=%lu "
           "l2cap-socket-listen=%lu "
           "l2cap-socket-send=%lu "
           "l2cap-socket-recv=%lu "
           "l2cap-socket-native-recv=%lu "
           "l2cap-socket-native-attach-fail=%lu "
           "l2cap-socket-native-recv-fail=%lu "
           "bnep-socket-register=%lu "
           "bnep-socket-unregister=%lu "
           "bnep-ioctl-suppfeat=%lu "
           "bnep-ioctl-connlist=%lu "
           "bnep-ioctl-conninfo=%lu "
           "bnep-ioctl-connadd=%lu "
           "bnep-ioctl-conndel=%lu "
           "bnep-staging-active=%u "
           "bnep-staging-add=%lu "
           "bnep-staging-del=%lu "
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
           g_l2cap_socket_registers,
           g_l2cap_socket_unregisters, g_l2cap_socket_binds,
           g_l2cap_socket_connects, g_l2cap_socket_listens,
           g_l2cap_socket_sends,
           g_l2cap_socket_recvs,
           g_l2cap_socket_native_recvs,
           g_l2cap_socket_native_attach_fails,
           g_l2cap_socket_native_recv_fails,
           g_bnep_socket_registers, g_bnep_socket_unregisters,
           g_bnep_socket_ioctl_suppfeat,
           g_bnep_socket_ioctl_connlist,
           g_bnep_socket_ioctl_conninfo,
           g_bnep_socket_ioctl_connadd,
           g_bnep_socket_ioctl_conndel,
           linux_bt_bnep_staging_active_count(),
           g_bnep_staging_session_adds,
           g_bnep_staging_session_dels,
           g_bnep_staging_netdev_registers,
           g_bnep_staging_netdev_unregisters,
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
           g_iso_socket_registers, g_iso_socket_unregisters,
           g_iso_socket_binds, g_iso_socket_connects,
           g_iso_socket_sends, g_iso_socket_recvs,
           g_iso_socket_native_recvs);

  return 0;
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
#ifdef SOCK_SEQPACKET
  if (proto == BTPROTO_L2CAP || proto == BTPROTO_ISO)
    {
      sock.type = SOCK_SEQPACKET;
    }
#endif
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
  (void)ret;

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
  struct linux_bt_bnep_staging_session *active_session;
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
           "connadd-device=%s bnep-staging-active=%u "
           "bnep-netdev-registered=%u bnep-netdev-name=%s "
           "bnep-netdev-mtu=%u\n",
           action, name, BTPROTO_BNEP, create_ret, ioctl_ret,
           supp_feat, cl.cnum, ci.role, ci.state, ci.device,
           cl.cnum > 0 ? list_ci[0].state : 0,
           cl.cnum > 0 ? list_ci[0].device : "", ca.device,
           linux_bt_bnep_staging_active_count(),
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
          struct sk_buff *skb;
          int delivered;

          event[0] = HCI_EV_CMD_COMPLETE;
          event[1] = 4;
          event[2] = 1;
          event[3] = bytes[1];
          event[4] = bytes[2];
          event[5] = 0;
          delivered = linux_bt_upstream_hci_sock_recv(hdev, HCI_EVENT_PKT,
                                                      event, sizeof(event));
          raw->fallback[0] = HCI_EVENT_PKT;
          memcpy(raw->fallback + 1, event, sizeof(event));
          raw->fallback_len = sizeof(event) + 1;
          if (delivered <= 0 && raw->sock.sk != NULL)
            {
              skb = alloc_skb(raw->fallback_len, GFP_KERNEL);
              if (skb != NULL)
                {
                  skb_put_data(skb, raw->fallback, raw->fallback_len);
                  skb_queue_tail(&raw->sock.sk->sk_receive_queue, skb);
                  g_hci_data_socket_rx++;
                  raw->fallback_len = 0;
                }
            }

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
  if (ret < 0 && raw->fallback_len > 0)
    {
      size_t copied = raw->fallback_len < payload_len ?
                      raw->fallback_len : payload_len;

      memcpy(payload, raw->fallback, copied);
      if (copied < raw->fallback_len && flags != NULL)
        {
          *flags |= MSG_TRUNC;
        }

      raw->fallback_len = 0;
      return (int)copied;
    }

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
  uint8_t payload[1024];
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

#ifdef SOCK_SEQPACKET
  g_l2cap_probe_socket.type = SOCK_SEQPACKET;
#else
  g_l2cap_probe_socket.type = SOCK_RAW;
#endif
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
      (void)linux_bt_hci_conn_snapshot_upsert(
        g_l2cap_probe_socket.sk,
        state->bdaddr_type != 0 ? LE_LINK : ACL_LINK, state->handle,
        &state->bdaddr, state->bdaddr_type,
        g_l2cap_probe_socket.sk->sk_state == BT_CONNECTED ? 1 : 0,
        g_l2cap_probe_socket.sk->sk_state, 0, 0);
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
  ret = linux_bt_hci_conn_snapshot_upsert(
    g_l2cap_probe_socket.sk,
    state->bdaddr_type != 0 ? LE_LINK : ACL_LINK, state->handle,
    &state->bdaddr, state->bdaddr_type, 1,
    g_l2cap_probe_socket.sk->sk_state, 0, 0);
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
  int fallback_ret = -EOPNOTSUPP;
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
  if (send_ret < 0 || attach_ret < 0)
    {
      fallback_ret = linux_bt_sim_l2cap_send(g_l2cap_probe_psm,
                                             g_l2cap_probe_cid,
                                             g_l2cap_probe_handle,
                                             payload, payload_len);
      if (fallback_ret >= 0)
        {
          send_ret = (int)payload_len;
        }
    }

  if (send_ret >= 0)
    {
      g_l2cap_socket_sends++;
    }

  snprintf(out, out_len,
           "upstream-l2cap-write: payload-len=%u send-ret=%d "
           "native-ret=%d attach-ret=%d fallback-ret=%d\n",
           (unsigned int)payload_len, send_ret, native_ret,
           attach_ret, fallback_ret);
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
  int fallback_ret = -EOPNOTSUPP;

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
      if (send_ret < 0 || attach_ret < 0)
        {
          fallback_ret = linux_bt_sim_l2cap_send(g_l2cap_probe_psm,
                                                 g_l2cap_probe_cid,
                                                 g_l2cap_probe_handle,
                                                 payload, payload_len);
          if (fallback_ret >= 0)
            {
              send_ret = (int)payload_len;
            }
        }

      if (send_ret >= 0)
        {
          g_l2cap_socket_sends++;
        }

      snprintf(out, out_len,
               "upstream-l2cap-send: psm=0x%04x cid=0x%04x handle=0x%04x "
               "payload-len=%u create-ret=0 bind-ret=0 send-ret=%d "
               "native-ret=%d attach-ret=%d fallback-ret=%d\n",
               psm, cid, handle, (unsigned int)payload_len, send_ret,
               native_ret, attach_ret, fallback_ret);
      return send_ret < 0 ? send_ret : 0;
#else
      if (send_ret < 0)
        {
          uint8_t frame[520];
          uint16_t acl_len;
          size_t capped_len = payload_len;

          if (capped_len > sizeof(frame) - 8)
            {
              capped_len = sizeof(frame) - 8;
            }

          acl_len = (uint16_t)(capped_len + 4);
          frame[0] = (uint8_t)(handle & 0xff);
          frame[1] = (uint8_t)((handle >> 8) & 0xff);
          frame[2] = (uint8_t)(acl_len & 0xff);
          frame[3] = (uint8_t)((acl_len >> 8) & 0xff);
          frame[4] = (uint8_t)(capped_len & 0xff);
          frame[5] = (uint8_t)((capped_len >> 8) & 0xff);
          frame[6] = (uint8_t)(cid & 0xff);
          frame[7] = (uint8_t)((cid >> 8) & 0xff);
          if (capped_len > 0)
            {
              memcpy(&frame[8], payload, capped_len);
            }

          fallback_ret = linux_bt_upstream_hci_send(LINUX_BT_HCI_ACL_PKT,
                                                    frame, capped_len + 8);
          if (fallback_ret >= 0)
            {
              send_ret = (int)capped_len;
            }
        }

      snprintf(out, out_len,
               "upstream-l2cap-send: psm=0x%04x cid=0x%04x handle=0x%04x "
               "payload-len=%u create-ret=0 bind-ret=0 send-ret=%d "
               "fallback-ret=%d\n",
               psm, cid, handle, (unsigned int)payload_len, send_ret,
               fallback_ret);
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

#ifdef SOCK_SEQPACKET
  sock.type = SOCK_SEQPACKET;
#else
  sock.type = SOCK_RAW;
#endif
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
      uint8_t frame[520];
      uint16_t acl_len;
      size_t capped_len = payload_len;

      native_ret = bind_ret;
      if (capped_len > sizeof(frame) - 8)
        {
          capped_len = sizeof(frame) - 8;
        }

      acl_len = (uint16_t)(capped_len + 4);
      frame[0] = (uint8_t)(handle & 0xff);
      frame[1] = (uint8_t)((handle >> 8) & 0xff);
      frame[2] = (uint8_t)(acl_len & 0xff);
      frame[3] = (uint8_t)((acl_len >> 8) & 0xff);
      frame[4] = (uint8_t)(capped_len & 0xff);
      frame[5] = (uint8_t)((capped_len >> 8) & 0xff);
      frame[6] = (uint8_t)(cid & 0xff);
      frame[7] = (uint8_t)((cid >> 8) & 0xff);
      if (capped_len > 0)
        {
          memcpy(&frame[8], payload, capped_len);
        }

      fallback_ret = linux_bt_upstream_hci_send(LINUX_BT_HCI_ACL_PKT,
                                                frame, capped_len + 8);
      send_ret = fallback_ret < 0 ? bind_ret : (int)capped_len;
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

      (void)linux_bt_hci_conn_snapshot_upsert(
        sock.sk, state->bdaddr_type != 0 ? LE_LINK : ACL_LINK,
        state->handle, &state->bdaddr, state->bdaddr_type,
        sock.sk->sk_state == BT_CONNECTED ? 1 : 0,
        sock.sk->sk_state, 0, 0);
    }

  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  attach_ret = linux_bt_l2cap_attach_send_chan(&sock, handle, cid);
  send_ret = sock.ops != NULL && sock.ops->sendmsg != NULL ?
             sock.ops->sendmsg(&sock, &msg, payload_len) :
             -EOPNOTSUPP;
  native_ret = send_ret;
  if (send_ret < 0 && state == NULL)
    {
      uint8_t frame[520];
      uint16_t acl_len;
      size_t capped_len = payload_len;

      if (capped_len > sizeof(frame) - 8)
        {
          capped_len = sizeof(frame) - 8;
        }

      acl_len = (uint16_t)(capped_len + 4);
      frame[0] = (uint8_t)(handle & 0xff);
      frame[1] = (uint8_t)((handle >> 8) & 0xff);
      frame[2] = (uint8_t)(acl_len & 0xff);
      frame[3] = (uint8_t)((acl_len >> 8) & 0xff);
      frame[4] = (uint8_t)(capped_len & 0xff);
      frame[5] = (uint8_t)((capped_len >> 8) & 0xff);
      frame[6] = (uint8_t)(cid & 0xff);
      frame[7] = (uint8_t)((cid >> 8) & 0xff);
      if (capped_len > 0)
        {
          memcpy(&frame[8], payload, capped_len);
        }

      fallback_ret = linux_bt_upstream_hci_send(LINUX_BT_HCI_ACL_PKT,
                                                frame, capped_len + 8);
      if (fallback_ret >= 0)
        {
          send_ret = (int)capped_len;
        }
    }

out:
  snprintf(out, out_len,
           "upstream-l2cap-send: psm=0x%04x cid=0x%04x handle=0x%04x "
           "payload-len=%u create-ret=%d bind-ret=%d send-ret=%d "
           "native-ret=%d attach-ret=%d fallback-ret=%d\n",
           psm, cid, handle, (unsigned int)payload_len, create_ret,
           bind_ret, send_ret, native_ret, attach_ret, fallback_ret);

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

#ifdef SOCK_SEQPACKET
  g_iso_probe_socket.type = SOCK_SEQPACKET;
#else
  g_iso_probe_socket.type = SOCK_RAW;
#endif
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
        (void)linux_bt_hci_conn_snapshot_upsert(
          g_iso_probe_socket.sk, linux_bt_iso_link_type(pi->handle),
          pi->handle, &state->bdaddr, state->bdaddr_type,
          g_iso_probe_socket.sk->sk_state == BT_CONNECTED ? 1 : 0,
          g_iso_probe_socket.sk->sk_state, 0, 0);
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
      attach_ret = iso_sim_attach_connected(&g_iso_probe_socket,
                                            g_iso_probe_handle);
      g_iso_probe_upstream_attached = attach_ret == 0;
#endif
      snprintf(out, out_len,
               "upstream-iso-connect: addr-type=%u connect-ret=0 "
               "state=%d sim-fastpath=1 upstream-iso-attach=%d\n",
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

  g_iso_probe_socket.ops->release(&g_iso_probe_socket);
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_ISO
  if (upstream_attached && handle != 0)
    {
      detach_ret = iso_sim_detach_connected(handle);
    }
#endif
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
  int fallback_ret = -EOPNOTSUPP;

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
      uint8_t frame[260];
      size_t capped_len = payload_len;
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
                       "send-ret=%d fallback-ret=%d sim-fastpath=0 "
                       "upstream-iso-attach=1\n",
                       addr_type, handle, (unsigned int)payload_len,
                       native_ret, fallback_ret);
              g_iso_socket_sends++;
              return 0;
            }
        }

      if (capped_len > sizeof(frame) - 8)
        {
          capped_len = sizeof(frame) - 8;
        }

      memset(frame, 0, sizeof(frame));
      frame[0] = (uint8_t)(handle & 0xff);
      frame[1] = (uint8_t)((handle >> 8) & 0xff);
      frame[2] = (uint8_t)((capped_len + 4) & 0xff);
      frame[3] = (uint8_t)((capped_len + 4) >> 8);
      frame[4] = 0x01;
      frame[5] = 0x00;
      frame[6] = (uint8_t)(capped_len & 0xff);
      frame[7] = (uint8_t)(capped_len >> 8);
      if (capped_len > 0)
        {
          memcpy(&frame[8], payload, capped_len);
        }

      fallback_ret = linux_bt_upstream_hci_send(LINUX_BT_HCI_ISO_PKT,
                                                frame, capped_len + 8);
      send_ret = fallback_ret < 0 ? fallback_ret : (int)capped_len;
      snprintf(out, out_len,
               "upstream-iso-send: addr-type=%u handle=0x%04x "
               "payload-len=%u create-ret=0 bind-ret=0 send-ret=%d "
               "fallback-ret=%d native-ret=%d sim-fastpath=1 "
               "upstream-iso-attach=%u\n",
               addr_type, handle, (unsigned int)payload_len, send_ret,
               fallback_ret, native_ret,
               g_iso_probe_upstream_attached ? 1 : 0);
      if (send_ret >= 0)
        {
          g_iso_socket_sends++;
        }

      return send_ret < 0 ? send_ret : 0;
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

#ifdef SOCK_SEQPACKET
  sock.type = SOCK_SEQPACKET;
#else
  sock.type = SOCK_RAW;
#endif
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
        (void)linux_bt_hci_conn_snapshot_upsert(
          sock.sk, linux_bt_iso_link_type(pi->handle), pi->handle,
          &state->bdaddr, state->bdaddr_type,
          sock.sk->sk_state == BT_CONNECTED ? 1 : 0, sock.sk->sk_state,
          0, 0);
      }
  }

  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
  send_ret = sock.ops != NULL && sock.ops->sendmsg != NULL ?
             sock.ops->sendmsg(&sock, &msg, payload_len) :
             -EOPNOTSUPP;
  if (send_ret < 0)
    {
      uint8_t frame[260];
      size_t capped_len = payload_len;

      if (capped_len > sizeof(frame) - 8)
        {
          capped_len = sizeof(frame) - 8;
        }

      memset(frame, 0, sizeof(frame));
      frame[0] = (uint8_t)(handle & 0xff);
      frame[1] = (uint8_t)((handle >> 8) & 0xff);
      frame[6] = (uint8_t)(capped_len & 0xff);
      frame[7] = (uint8_t)((capped_len >> 8) & 0xff);
      if (capped_len > 0)
        {
          memcpy(&frame[8], payload, capped_len);
        }

      fallback_ret = linux_bt_upstream_hci_send(LINUX_BT_HCI_ISO_PKT,
                                                frame, capped_len + 8);
      if (fallback_ret >= 0)
        {
          send_ret = (int)capped_len;
        }
    }

out:
  snprintf(out, out_len,
           "upstream-iso-send: addr-type=%u handle=0x%04x "
           "payload-len=%u create-ret=%d bind-ret=%d send-ret=%d "
           "fallback-ret=%d\n",
           iso_addr_type, handle, (unsigned int)payload_len, create_ret,
           bind_ret, send_ret, fallback_ret);

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

      default:
        break;
    }
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

  if (opcode == HCI_OP_SET_EVENT_MASK ||
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
      opcode == HCI_OP_RESET)
    {
      event[5] = 0;
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

  if (event[5] == 0)
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

  if (h == NULL)
    {
      return -EINVAL;
    }

  if (h->sock.ops != NULL && h->sock.ops->release != NULL)
    {
      h->sock.ops->release(&h->sock);
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

#ifdef SOCK_SEQPACKET
  h->sock.type = SOCK_SEQPACKET;
#else
  h->sock.type = SOCK_RAW;
#endif
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
      (void)linux_bt_hci_conn_snapshot_upsert(
        h->sock.sk, ACL_LINK, state->handle, &state->bdaddr,
        state->bdaddr_type, h->sock.sk->sk_state == BT_CONNECTED ? 1 : 0,
        h->sock.sk->sk_state, 0, 0);
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
      (void)linux_bt_hci_conn_snapshot_upsert(
        h->sock.sk, ACL_LINK, h->handle,
        state != NULL ? &state->bdaddr : &any, 0, 1,
        h->sock.sk->sk_state, 0, 0);
    }

  h->connected = true;
  g_l2cap_socket_connects++;
  return 0;
}

int linux_bt_upstream_l2cap_socket_write_handle(void *handle,
                                                const void *payload,
                                                size_t payload_len,
                                                char *out,
                                                size_t out_len)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;
  struct msghdr msg;
  int send_ret;
#ifdef CONFIG_SIM_BTHWSIM
  int native_ret;
  int fallback_ret = -EOPNOTSUPP;
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

  memset(&msg, 0, sizeof(msg));
  msg.msg_iter.data = (const char *)payload;
  msg.msg_iter.count = payload_len;
#ifdef CONFIG_SIM_BTHWSIM
  attach_ret = linux_bt_l2cap_attach_send_chan(&h->sock, h->handle, h->cid);
#endif
  send_ret = h->sock.ops->sendmsg(&h->sock, &msg, payload_len);
#ifdef CONFIG_SIM_BTHWSIM
  native_ret = send_ret;
  if (h->cid != 0x0040 || send_ret < 0 || attach_ret < 0)
    {
      fallback_ret = linux_bt_sim_l2cap_send(h->psm, h->cid, h->handle,
                                             payload, payload_len);
      if (fallback_ret >= 0 && send_ret < 0)
        {
          send_ret = (int)payload_len;
        }
    }

  if (send_ret >= 0)
    {
      g_l2cap_socket_sends++;
    }

  snprintf(out, out_len,
           "upstream-l2cap-write-handle: psm=0x%04x cid=0x%04x "
           "handle=0x%04x payload-len=%u send-ret=%d native-ret=%d "
           "attach-ret=%d fallback-ret=%d\n",
           h->psm, h->cid, h->handle, (unsigned int)payload_len,
           send_ret, native_ret, attach_ret, fallback_ret);
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

  if (max_len == 0 || max_len > sizeof(buf))
    {
      max_len = sizeof(buf);
    }

  {
    size_t pending_len = 0;

    if (linux_bt_l2cap_pending_pop(h->handle, h->cid, buf, max_len,
                                   &pending_len))
      {
        linux_bt_upstream_format_hex(buf, pending_len, hex, sizeof(hex));
        g_l2cap_socket_recvs++;
        snprintf(out, out_len,
                 "upstream-l2cap-recv-handle: psm=0x%04x cid=0x%04x "
                 "handle=0x%04x recv-ret=%u flags=0x0 payload=%s "
                 "pending=1\n",
                 h->psm, h->cid, h->handle, (unsigned int)pending_len,
                 hex);
        return 0;
      }
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

  linux_bt_upstream_format_hex(buf, (size_t)recv_ret, hex, sizeof(hex));
  g_l2cap_socket_recvs++;
  snprintf(out, out_len,
           "upstream-l2cap-recv-handle: psm=0x%04x cid=0x%04x "
           "handle=0x%04x recv-ret=%d flags=0x%x payload=%s\n",
           h->psm, h->cid, h->handle, recv_ret, msg.msg_flags, hex);
  return 0;
}

int linux_bt_upstream_l2cap_socket_close_handle(void *handle)
{
  struct linux_bt_upstream_l2cap_socket_handle *h = handle;

  if (h == NULL)
    {
      return -EINVAL;
    }

  linux_bt_l2cap_clear_bound_state_by_sk(h->sock.sk);
  if (h->cid == 0)
    {
      linux_bt_l2cap_clear_bound_state_by_addr(h->psm, h->cid, h->handle);
    }

  if (h->sock.ops != NULL && h->sock.ops->release != NULL)
    {
      h->sock.ops->release(&h->sock);
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

  kmm_free(h);
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
