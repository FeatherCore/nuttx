/****************************************************************************
 * wireless/linux_bluetooth/linux_bt_upstream_hci.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <nuttx/wireless/linux_bluetooth.h>

#include <net/bluetooth/hci_core.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LINUX_BT_UPSTREAM_HCI_MAX_DEV 4

#ifdef CONFIG_SIM_BTHWSIM
#  define LINUX_BT_UPSTREAM_HCI_HWSIM_BUFSIZE 4096
#  define LINUX_BT_UPSTREAM_HCI_HWSIM_BUDGET  32
#  define LINUX_BT_UPSTREAM_HCI_HWSIM_US      5000
#  define LINUX_BT_UPSTREAM_HCI_HWSIM_STACK   8192
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct hci_dev *g_hci_devs[LINUX_BT_UPSTREAM_HCI_MAX_DEV];
static uint16_t g_hci_next_id;

#ifdef CONFIG_SIM_BTHWSIM
static pthread_t g_hci_hwsim_thread;
static uint8_t g_hci_hwsim_buf[LINUX_BT_UPSTREAM_HCI_HWSIM_BUFSIZE];
static bool g_hci_hwsim_pump_started;
static uint8_t g_hci_sim_br_connected;
static uint8_t g_hci_sim_br_upstream_conn;
static uint8_t g_hci_sim_br_role;
static uint16_t g_hci_sim_br_peer;
static uint16_t g_hci_sim_br_handle;
static uint8_t g_hci_sim_le_connected;
static uint8_t g_hci_sim_le_upstream_conn;
static uint8_t g_hci_sim_le_role;
static uint16_t g_hci_sim_le_peer;
static uint16_t g_hci_sim_le_handle;

static void linux_bt_upstream_hci_sim_br_status_clear(uint16_t handle)
{
  if (g_hci_sim_br_handle == handle)
    {
      g_hci_sim_br_connected = 0;
      g_hci_sim_br_upstream_conn = 0;
      g_hci_sim_br_role = 0;
      g_hci_sim_br_peer = 0;
      g_hci_sim_br_handle = 0;
    }
}
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_SIM_BTHWSIM
static void linux_bt_upstream_hci_hwsim_pump_start(void);
int linux_bt_compat_vhci_pump_deferred_events(struct hci_dev *hdev);
#endif

static struct hci_dev *linux_bt_upstream_hci_default_dev(void)
{
  unsigned int i;

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CORE
  {
    struct hci_dev *hdev = linux_bt_upstream_vhci_default_hdev();

    if (hdev != NULL)
      {
        return hdev;
      }
  }

  for (i = 0; i < LINUX_BT_UPSTREAM_HCI_MAX_DEV; i++)
    {
      struct hci_dev *hdev = hci_dev_get((int)i);

      if (hdev != NULL)
        {
          return hdev;
        }
    }
#else
  for (i = 0; i < LINUX_BT_UPSTREAM_HCI_MAX_DEV; i++)
    {
      if (g_hci_devs[i] != NULL)
        {
          return g_hci_devs[i];
        }
    }
#endif

  return NULL;
}

static void linux_bt_upstream_put_le16(uint8_t *p, uint16_t value)
{
  p[0] = (uint8_t)value;
  p[1] = (uint8_t)(value >> 8);
}

static int linux_bt_upstream_hci_note_rx(struct hci_dev *hdev,
                                         uint8_t pkt_type,
                                         const void *payload,
                                         size_t payload_len,
                                         bool forward_to_medium)
{
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  hdev->stat.byte_rx += payload_len;
  switch (pkt_type)
    {
      case HCI_ACLDATA_PKT:
        hdev->stat.acl_rx++;
        break;

      case HCI_SCODATA_PKT:
        hdev->stat.sco_rx++;
        break;

      case HCI_ISODATA_PKT:
        hdev->stat.iso_rx++;
        break;

      default:
        break;
    }

  if (forward_to_medium)
    {
      return linux_bt_upstream_vhci_recv_frame(pkt_type, payload,
                                               payload_len);
    }

  return 0;
}

static size_t linux_bt_upstream_hci_append(char *out, size_t out_len,
                                           size_t used,
                                           const char *fmt, ...)
{
  va_list ap;
  int ret;

  if (used >= out_len)
    {
      return used;
    }

  va_start(ap, fmt);
  ret = vsnprintf(out + used, out_len - used, fmt, ap);
  va_end(ap);

  if (ret > 0)
    {
      used += (size_t)ret;
    }

  return used;
}

#ifdef CONFIG_SIM_BTHWSIM
static int linux_bt_upstream_hci_sim_event(struct hci_dev *hdev,
                                           uint8_t event,
                                           const void *param,
                                           size_t param_len)
{
  struct sk_buff *skb;
  uint8_t *evt;

  if (hdev == NULL || (param == NULL && param_len > 0))
    {
      return -EINVAL;
    }

  skb = bt_skb_alloc(2 + param_len, GFP_KERNEL);
  if (skb == NULL)
    {
      return -ENOMEM;
    }

  evt = skb_put(skb, 2 + param_len);
  if (evt == NULL)
    {
      kfree_skb(skb);
      return -ENOMEM;
    }

  evt[0] = event;
  evt[1] = (uint8_t)param_len;
  if (param_len > 0)
    {
      memcpy(&evt[2], param, param_len);
    }

  hci_skb_pkt_type(skb) = HCI_EVENT_PKT;
  return hci_recv_frame(hdev, skb);
}

static struct hci_conn *
linux_bt_upstream_hci_sim_br_conn(struct hci_dev *hdev,
                                  const bdaddr_t *dst,
                                  uint8_t role,
                                  uint16_t handle)
{
  struct hci_conn *conn;

  conn = hci_conn_hash_lookup_ba(hdev, ACL_LINK, (bdaddr_t *)dst);
  if (conn != NULL)
    {
      conn->handle = handle;
      conn->state = BT_CONNECTED;
      conn->role = role;
      conn->out = role == HCI_ROLE_MASTER;
      return conn;
    }

  conn = kzalloc(sizeof(*conn), GFP_KERNEL);
  if (conn == NULL)
    {
      return ERR_PTR(-ENOMEM);
    }

  INIT_LIST_HEAD(&conn->list);
  INIT_LIST_HEAD(&conn->chan_list);
  INIT_LIST_HEAD(&conn->link_list);
  skb_queue_head_init(&conn->data_q);
  skb_queue_head_init(&conn->tx_q.queue);

  bacpy(&conn->dst, dst);
  bacpy(&conn->src, &hdev->bdaddr);
  conn->handle = handle;
  conn->hdev = hdev;
  conn->type = ACL_LINK;
  conn->role = role;
  conn->out = role == HCI_ROLE_MASTER;
  conn->mode = HCI_CM_ACTIVE;
  conn->state = BT_CONNECTED;
  conn->auth_type = HCI_AT_GENERAL_BONDING;
  conn->sec_level = BT_SECURITY_LOW;
  conn->pending_sec_level = BT_SECURITY_LOW;
  conn->io_capability = hdev->io_capability;
  conn->remote_auth = 0xff;
  conn->key_type = 0xff;
  conn->rssi = HCI_RSSI_INVALID;
  conn->tx_power = HCI_TX_POWER_INVALID;
  conn->max_tx_power = HCI_TX_POWER_INVALID;
  conn->sync_handle = HCI_SYNC_HANDLE_INVALID;
  conn->disc_timeout = HCI_DISCONN_TIMEOUT;
  conn->auth_payload_timeout = DEFAULT_AUTH_PAYLOAD_TIMEOUT;
  conn->conn_timeout = HCI_DISCONN_TIMEOUT;
  conn->mtu = hdev->acl_mtu;
  atomic_set(&conn->refcnt, 1);

  hci_conn_hash_add(hdev, conn);
  return conn;
}

int linux_bt_upstream_hci_sim_br_complete(struct hci_dev *hdev,
                                          const void *bdaddr,
                                          uint8_t role,
                                          uint16_t handle)
{
  struct hci_conn *conn;
  bdaddr_t dst;
  const uint8_t *addr = bdaddr;

  if (hdev == NULL || bdaddr == NULL)
    {
      return -EINVAL;
    }

  memcpy(&dst, bdaddr, sizeof(dst));
  conn = linux_bt_upstream_hci_sim_br_conn(hdev, &dst, role, handle);
  if (IS_ERR(conn))
    {
      g_hci_sim_br_upstream_conn = 0;
      return (int)PTR_ERR(conn);
    }

  g_hci_sim_br_connected = 1;
  g_hci_sim_br_upstream_conn = 1;
  g_hci_sim_br_role = role;
  g_hci_sim_br_peer = addr[0];
  g_hci_sim_br_handle = handle;

  mgmt_device_connected(hdev, conn, NULL, 0);

  linux_bt_upstream_hci_hwsim_pump_start();
  return 0;
}

static int linux_bt_upstream_hci_sim_br_disconnect(struct hci_dev *hdev,
                                                   uint16_t handle,
                                                   uint8_t reason)
{
  struct hci_conn *conn;
  bdaddr_t dst;
  bool mgmt_connected;

  if (hdev == NULL)
    {
      return -EINVAL;
    }

  conn = hci_conn_hash_lookup_handle(hdev, handle);
  if (conn == NULL || conn->type != ACL_LINK)
    {
      return -ENOENT;
    }

  bacpy(&dst, &conn->dst);
  mgmt_connected = test_and_clear_bit(HCI_CONN_MGMT_CONNECTED,
                                      &conn->flags);
  mgmt_device_disconnected(hdev, &dst, conn->type, 0, reason,
                           mgmt_connected);

  hci_conn_hash_del(hdev, conn);
  kfree(conn);

  linux_bt_upstream_hci_sim_br_status_clear(handle);

  return 0;
}

static struct hci_conn *
linux_bt_upstream_hci_sim_le_conn(struct hci_dev *hdev,
                                  const bdaddr_t *dst,
                                  uint8_t dst_type,
                                  uint8_t role,
                                  uint16_t handle)
{
  struct hci_conn *conn;

  conn = hci_conn_hash_lookup_le(hdev, (bdaddr_t *)dst, dst_type);
  if (conn != NULL)
    {
      conn->handle = handle;
      conn->state = BT_CONNECTED;
      conn->role = role;
      conn->out = role == HCI_ROLE_MASTER;
      return conn;
    }

  conn = kzalloc(sizeof(*conn), GFP_KERNEL);
  if (conn == NULL)
    {
      return ERR_PTR(-ENOMEM);
    }

  INIT_LIST_HEAD(&conn->list);
  INIT_LIST_HEAD(&conn->chan_list);
  INIT_LIST_HEAD(&conn->link_list);
  skb_queue_head_init(&conn->data_q);
  skb_queue_head_init(&conn->tx_q.queue);

  bacpy(&conn->dst, dst);
  bacpy(&conn->src, &hdev->bdaddr);
  conn->dst_type = dst_type;
  conn->src_type = ADDR_LE_DEV_PUBLIC;
  conn->handle = handle;
  conn->hdev = hdev;
  conn->type = LE_LINK;
  conn->role = role;
  conn->out = role == HCI_ROLE_MASTER;
  conn->mode = HCI_CM_ACTIVE;
  conn->state = BT_CONNECTED;
  conn->auth_type = HCI_AT_GENERAL_BONDING;
  conn->sec_level = BT_SECURITY_LOW;
  conn->pending_sec_level = BT_SECURITY_LOW;
  conn->io_capability = hdev->io_capability;
  conn->remote_auth = 0xff;
  conn->key_type = 0xff;
  conn->rssi = HCI_RSSI_INVALID;
  conn->tx_power = HCI_TX_POWER_INVALID;
  conn->max_tx_power = HCI_TX_POWER_INVALID;
  conn->sync_handle = HCI_SYNC_HANDLE_INVALID;
  conn->sid = HCI_SID_INVALID;
  conn->disc_timeout = HCI_DISCONN_TIMEOUT;
  conn->auth_payload_timeout = DEFAULT_AUTH_PAYLOAD_TIMEOUT;
  conn->conn_timeout = HCI_LE_CONN_TIMEOUT;
  conn->le_adv_phy = HCI_ADV_PHY_1M;
  conn->le_adv_sec_phy = 0;
  conn->mtu = hdev->le_mtu != 0 ? hdev->le_mtu : hdev->acl_mtu;
  atomic_set(&conn->refcnt, 1);

  hci_conn_hash_add(hdev, conn);
  return conn;
}

int linux_bt_upstream_hci_sim_le_complete(struct hci_dev *hdev,
                                          const void *bdaddr,
                                          uint8_t bdaddr_type,
                                          uint8_t role,
                                          uint16_t handle)
{
  struct hci_conn *conn;
  bdaddr_t dst;
  const uint8_t *addr = bdaddr;

  if (hdev == NULL || bdaddr == NULL)
    {
      return -EINVAL;
    }

  memcpy(&dst, bdaddr, sizeof(dst));
  conn = linux_bt_upstream_hci_sim_le_conn(hdev, &dst, bdaddr_type, role,
                                           handle);
  if (IS_ERR(conn))
    {
      g_hci_sim_le_upstream_conn = 0;
      return (int)PTR_ERR(conn);
    }

  g_hci_sim_le_connected = 1;
  g_hci_sim_le_upstream_conn = 1;
  g_hci_sim_le_role = role;
  g_hci_sim_le_peer = addr[0];
  g_hci_sim_le_handle = handle;

  mgmt_device_connected(hdev, conn, NULL, 0);

  linux_bt_upstream_hci_hwsim_pump_start();
  return 0;
}

int linux_bt_upstream_hci_sim_le_disconnect(struct hci_dev *hdev,
                                            uint16_t handle,
                                            uint8_t reason)
{
  struct hci_conn *conn;
  bdaddr_t dst;
  uint8_t dst_type;
  bool mgmt_connected;

  if (hdev == NULL)
    {
      return -EINVAL;
    }

  conn = hci_conn_hash_lookup_handle(hdev, handle);
  if (conn == NULL || conn->type != LE_LINK)
    {
      return -ENOENT;
    }

  bacpy(&dst, &conn->dst);
  dst_type = conn->dst_type;
  mgmt_connected = test_and_clear_bit(HCI_CONN_MGMT_CONNECTED,
                                      &conn->flags);
  mgmt_device_disconnected(hdev, &dst, conn->type, dst_type, reason,
                           mgmt_connected);

  hci_conn_hash_del(hdev, conn);
  kfree(conn);

  if (g_hci_sim_le_handle == handle)
    {
      g_hci_sim_le_connected = 0;
      g_hci_sim_le_upstream_conn = 0;
      g_hci_sim_le_role = 0;
      g_hci_sim_le_peer = 0;
      g_hci_sim_le_handle = 0;
    }

  return 0;
}

int linux_bt_upstream_hci_peer_le_complete(uint16_t peer,
                                           uint16_t handle)
{
  struct hci_dev *hdev;
  uint8_t le[1 + sizeof(struct hci_ev_le_conn_complete)];
  struct hci_ev_le_conn_complete *ev;
  bdaddr_t dst;
  uint8_t *addr = (uint8_t *)&dst;

  hdev = linux_bt_upstream_vhci_default_hdev();
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  addr[0] = (uint8_t)peer;
  addr[1] = 0x00;
  addr[2] = 0x00;
  addr[3] = 0x00;
  addr[4] = 0xfe;
  addr[5] = 0x02;

  memset(le, 0, sizeof(le));
  le[0] = HCI_EV_LE_CONN_COMPLETE;
  ev = (void *)&le[1];
  ev->status = 0;
  ev->handle = cpu_to_le16(handle);
  ev->role = HCI_ROLE_SLAVE;
  ev->bdaddr_type = ADDR_LE_DEV_PUBLIC;
  bacpy(&ev->bdaddr, &dst);
  ev->interval = cpu_to_le16(0x0018);
  ev->latency = cpu_to_le16(0x0000);
  ev->supervision_timeout = cpu_to_le16(0x01f4);

  return linux_bt_upstream_hci_sim_event(hdev, HCI_EV_LE_META, le,
                                         sizeof(le));
}

int linux_bt_upstream_hci_peer_le_disconnect(uint16_t handle,
                                             uint8_t reason)
{
  struct hci_dev *hdev;
  struct hci_ev_disconn_complete ev;
  int ret;

  hdev = linux_bt_upstream_vhci_default_hdev();
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  memset(&ev, 0, sizeof(ev));
  ev.status = 0;
  ev.handle = cpu_to_le16(handle);
  ev.reason = reason;

  ret = linux_bt_upstream_hci_sim_event(hdev, HCI_EV_DISCONN_COMPLETE,
                                        &ev, sizeof(ev));
  if (ret >= 0)
    {
      int cleanup;

      cleanup = linux_bt_upstream_hci_sim_le_disconnect(hdev, handle,
                                                        reason);
      if (cleanup < 0 && cleanup != -ENOENT)
        {
          return cleanup;
        }
    }

  return ret;
}

int linux_bt_upstream_hci_peer_br_complete(uint16_t peer,
                                           uint16_t handle)
{
  struct hci_dev *hdev;
  struct hci_ev_conn_complete ev;
  bdaddr_t dst;
  uint8_t *addr = (uint8_t *)&dst;

  hdev = linux_bt_upstream_vhci_default_hdev();
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  addr[0] = (uint8_t)peer;
  addr[1] = 0x00;
  addr[2] = 0x00;
  addr[3] = 0x00;
  addr[4] = 0xfe;
  addr[5] = 0x02;

  memset(&ev, 0, sizeof(ev));
  ev.status = 0;
  ev.handle = cpu_to_le16(handle);
  bacpy(&ev.bdaddr, &dst);
  ev.link_type = ACL_LINK;
  ev.encr_mode = 0;

  return linux_bt_upstream_hci_sim_event(hdev, HCI_EV_CONN_COMPLETE,
                                         &ev, sizeof(ev));
}

int linux_bt_upstream_hci_peer_br_disconnect(uint16_t handle,
                                             uint8_t reason)
{
  struct hci_dev *hdev;
  struct hci_ev_disconn_complete ev;
  int ret;

  hdev = linux_bt_upstream_vhci_default_hdev();
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  memset(&ev, 0, sizeof(ev));
  ev.status = 0;
  ev.handle = cpu_to_le16(handle);
  ev.reason = reason;

  ret = linux_bt_upstream_hci_sim_event(hdev, HCI_EV_DISCONN_COMPLETE,
                                        &ev, sizeof(ev));
  if (ret >= 0)
    {
      int cleanup;

      cleanup = linux_bt_upstream_hci_sim_br_disconnect(hdev, handle,
                                                        reason);
      if (cleanup < 0 && cleanup != -ENOENT)
        {
          return cleanup;
        }
    }

  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CORE
struct hci_dev *hci_alloc_dev_priv(int sizeof_priv)
{
  struct hci_dev *hdev;

  hdev = kzalloc(sizeof(*hdev) + sizeof_priv, GFP_KERNEL);
  if (hdev == NULL)
    {
      return NULL;
    }

  snprintf(hdev->name_storage, sizeof(hdev->name_storage), "hci%u", 0u);
  hdev->name = hdev->name_storage;
  atomic_set(&hdev->promisc, 0);
  return hdev;
}

struct hci_dev *hci_alloc_dev(void)
{
  return hci_alloc_dev_priv(0);
}

void hci_free_dev(struct hci_dev *hdev)
{
  kfree(hdev);
}

int hci_register_dev(struct hci_dev *hdev)
{
  unsigned int i;

  if (hdev == NULL)
    {
      return -EINVAL;
    }

  for (i = 0; i < LINUX_BT_UPSTREAM_HCI_MAX_DEV; i++)
    {
      if (g_hci_devs[i] == NULL)
        {
          hdev->id = g_hci_next_id++;
          snprintf(hdev->name_storage, sizeof(hdev->name_storage), "hci%u",
                   hdev->id);
          hdev->name = hdev->name_storage;
          g_hci_devs[i] = hdev;
          return 0;
        }
    }

  return -ENOSPC;
}

void hci_unregister_dev(struct hci_dev *hdev)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_UPSTREAM_HCI_MAX_DEV; i++)
    {
      if (g_hci_devs[i] == hdev)
        {
          g_hci_devs[i] = NULL;
          return;
        }
    }
}

void hci_set_drvdata(struct hci_dev *hdev, void *data)
{
  if (hdev != NULL)
    {
      hdev->driver_data = data;
    }
}

void *hci_get_drvdata(struct hci_dev *hdev)
{
  return hdev != NULL ? hdev->driver_data : NULL;
}

struct hci_dev *hci_dev_get(int index)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_UPSTREAM_HCI_MAX_DEV; i++)
    {
      if (g_hci_devs[i] != NULL && g_hci_devs[i]->id == (uint16_t)index)
        {
          return hci_dev_hold(g_hci_devs[i]);
        }
    }

  return NULL;
}

void hci_set_quirk(struct hci_dev *hdev, unsigned int quirk)
{
  unsigned int word = quirk / (8 * sizeof(unsigned long));
  unsigned int bit = quirk % (8 * sizeof(unsigned long));

  if (hdev != NULL && word < ARRAY_SIZE(hdev->quirks))
    {
      hdev->quirks[word] |= 1UL << bit;
    }
}

bool hci_test_quirk(struct hci_dev *hdev, unsigned int quirk)
{
  unsigned int word = quirk / (8 * sizeof(unsigned long));
  unsigned int bit = quirk % (8 * sizeof(unsigned long));

  if (hdev == NULL || word >= ARRAY_SIZE(hdev->quirks))
    {
      return false;
    }

  return (hdev->quirks[word] & (1UL << bit)) != 0;
}

int hci_recv_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
  uint8_t pkt_type;
  int ret;

  if (hdev == NULL || skb == NULL)
    {
      kfree_skb(skb);
      return -EINVAL;
    }

  pkt_type = hci_skb_pkt_type(skb);

  /* Linux VHCI receive semantics:
   *
   * hci_recv_frame() is the controller-to-host boundary.  In the native Linux
   * driver, bytes written to /dev/vhci are delivered up into the Bluetooth
   * host stack; they are not transmitted back to the controller transport.
   *
   * The hwsim host-to-medium direction is handled by draining upstream
   * hci_vhci.c's read queue from linux_bt_upstream_vhci_drain_default().
   */

  ret = linux_bt_upstream_hci_note_rx(hdev, pkt_type, skb->data, skb->len,
                                      false);
  if (ret >= 0)
    {
      linux_bt_upstream_hci_sock_recv(hdev, pkt_type, skb->data, skb->len);
      linux_bt_upstream_proto_sock_recv(pkt_type, skb->data, skb->len);
    }

  kfree_skb(skb);
  return ret;
}
#endif

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CORE
struct hci_dev *hci_alloc_dev(void)
{
  return hci_alloc_dev_priv(0);
}

void hci_set_drvdata(struct hci_dev *hdev, void *data)
{
  if (hdev != NULL)
    {
      hdev->driver_data = data;
    }
}

void *hci_get_drvdata(struct hci_dev *hdev)
{
  return hdev != NULL ? hdev->driver_data : NULL;
}

void hci_set_quirk(struct hci_dev *hdev, unsigned int quirk)
{
  unsigned int word = quirk / (8 * sizeof(unsigned long));
  unsigned int bit = quirk % (8 * sizeof(unsigned long));

  if (hdev != NULL && word < ARRAY_SIZE(hdev->quirks))
    {
      hdev->quirks[word] |= 1UL << bit;
    }
}

bool hci_test_quirk(struct hci_dev *hdev, unsigned int quirk)
{
  unsigned int word = quirk / (8 * sizeof(unsigned long));
  unsigned int bit = quirk % (8 * sizeof(unsigned long));

  if (hdev == NULL || word >= ARRAY_SIZE(hdev->quirks))
    {
      return false;
    }

  return (hdev->quirks[word] & (1UL << bit)) != 0;
}
#endif

int linux_bt_upstream_hci_recv_medium(uint8_t pkt_type,
                                      const void *payload,
                                      size_t payload_len)
{
  int ret;

  ret = linux_bt_upstream_hci_note_rx(linux_bt_upstream_hci_default_dev(),
                                      pkt_type, payload, payload_len,
                                      false);
  if (ret >= 0)
    {
      linux_bt_upstream_proto_sock_recv(pkt_type, payload, payload_len);
    }

  return ret;
}

#ifdef CONFIG_SIM_BTHWSIM
static int linux_bt_upstream_hci_hwsim_pump_one(uint16_t medium_type,
                                                uint8_t pkt_type)
{
  uint16_t src;
  uint16_t dst;
  uint32_t len;
  unsigned int i;
  int count = 0;
  int ret;

  for (i = 0; i < LINUX_BT_UPSTREAM_HCI_HWSIM_BUDGET; i++)
    {
      len = 0;
      ret = linux_bt_hwsim_read_raw_named(medium_type, "vhci", &src, &dst,
                                          g_hci_hwsim_buf,
                                          sizeof(g_hci_hwsim_buf), &len);
      if (ret <= 0 || len == 0)
        {
          return count;
        }

      linux_bt_upstream_hci_recv_medium(pkt_type, g_hci_hwsim_buf, len);
      count++;
    }

  return count;
}

static int linux_bt_upstream_hci_hwsim_pump_all(void)
{
  int count = 0;

  count += linux_bt_upstream_hci_hwsim_pump_one(LINUX_BT_HWSIM_TYPE_ACL,
                                                HCI_ACLDATA_PKT);
  count += linux_bt_upstream_hci_hwsim_pump_one(LINUX_BT_HWSIM_TYPE_ISO,
                                                HCI_ISODATA_PKT);
  return count;
}

static void *linux_bt_upstream_hci_hwsim_thread(void *arg)
{
  (void)arg;

  while (g_hci_hwsim_pump_started)
    {
      linux_bt_upstream_hci_hwsim_pump_all();
      usleep(LINUX_BT_UPSTREAM_HCI_HWSIM_US);
    }

  return NULL;
}

static void linux_bt_upstream_hci_hwsim_pump_start(void)
{
  pthread_attr_t attr;

  if (g_hci_hwsim_pump_started)
    {
      return;
    }

  g_hci_hwsim_pump_started = true;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setstacksize(&attr, LINUX_BT_UPSTREAM_HCI_HWSIM_STACK);
  if (pthread_create(&g_hci_hwsim_thread, &attr,
                     linux_bt_upstream_hci_hwsim_thread, NULL) != 0)
    {
      g_hci_hwsim_pump_started = false;
    }

  pthread_attr_destroy(&attr);
}
#endif

int linux_bt_upstream_hci_hwsim_pump(unsigned int rounds)
{
#ifdef CONFIG_SIM_BTHWSIM
  unsigned int i;
  int total = 0;

  if (rounds == 0)
    {
      rounds = 1;
    }

  linux_bt_upstream_hci_hwsim_pump_start();
  for (i = 0; i < rounds; i++)
    {
      total += linux_bt_upstream_hci_hwsim_pump_all();
      usleep(LINUX_BT_UPSTREAM_HCI_HWSIM_US);
    }

  return total;
#else
  return -ENODEV;
#endif
}

int linux_bt_upstream_hci_status(char *out, size_t out_len)
{
  size_t used = 0;
  unsigned int i;
  unsigned int count = 0;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  out[0] = '\0';
  used = linux_bt_upstream_hci_append(out, out_len, used,
                                      "upstream-hci: max-dev=%u next-id=%u\n",
                                      LINUX_BT_UPSTREAM_HCI_MAX_DEV,
                                      g_hci_next_id);

#ifdef CONFIG_SIM_BTHWSIM
  if (g_hci_sim_br_connected)
    {
      used = linux_bt_upstream_hci_append(
        out, out_len, used,
        "  sim-conn br=1 peer=%u role=%u handle=0x%04x "
        "upstream-hci-conn=%u\n",
        g_hci_sim_br_peer, g_hci_sim_br_role, g_hci_sim_br_handle,
        g_hci_sim_br_upstream_conn ? 1 : 0);
    }

  if (g_hci_sim_le_connected)
    {
      used = linux_bt_upstream_hci_append(
        out, out_len, used,
        "  sim-conn le=1 peer=%u role=%u handle=0x%04x "
        "upstream-hci-conn=%u\n",
        g_hci_sim_le_peer, g_hci_sim_le_role, g_hci_sim_le_handle,
        g_hci_sim_le_upstream_conn ? 1 : 0);
    }
#endif

  for (i = 0; i < LINUX_BT_UPSTREAM_HCI_MAX_DEV; i++)
    {
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CORE
      struct hci_dev *hdev;

      if (i == 0)
        {
          hdev = linux_bt_upstream_vhci_default_hdev();
        }
      else
        {
          hdev = hci_dev_get((int)i);
        }
#else
      struct hci_dev *hdev = g_hci_devs[i];
#endif

      if (hdev == NULL)
        {
          continue;
        }

      count++;
      {
        uint8_t bdaddr[6];

        memcpy(bdaddr, &hdev->bdaddr, sizeof(bdaddr));

        used = linux_bt_upstream_hci_append(
          out, out_len, used,
          "    conn-hash acl=%u sco=%u le=%u le-peripheral=%u "
          "cis=%u bis=%u pa=%u\n",
          hdev->conn_hash.acl_num, hdev->conn_hash.sco_num,
          hdev->conn_hash.le_num, hdev->conn_hash.le_num_peripheral,
          hdev->conn_hash.cis_num, hdev->conn_hash.bis_num,
          hdev->conn_hash.pa_num);

        {
          struct hci_conn *conn;
          unsigned int conn_index = 0;

          list_for_each_entry(conn, &hdev->conn_hash.list, list)
            {
              used = linux_bt_upstream_hci_append(
                out, out_len, used,
                "    conn[%u] type=%u role=%u state=%u out=%u "
                "handle=0x%04x\n",
                conn_index, conn->type, conn->role, conn->state,
                conn->out ? 1 : 0, conn->handle);
              conn_index++;
            }
        }

        used = linux_bt_upstream_hci_append(
          out, out_len, used,
          "  slot=%u id=%u name=%s bus=%u quirks=%08lx:%08lx "
          "flags=%08lx dev-flags=%08lx:%08lx promisc=%d "
          "rx_bytes=%lu rx_acl=%lu rx_sco=%lu rx_iso=%lu "
          "tx_bytes=%lu tx_cmd=%lu tx_acl=%lu tx_sco=%lu tx_iso=%lu\n",
          i, hdev->id, hdev->name, hdev->bus,
          hdev->quirks[0], hdev->quirks[1],
          hdev->flags, hdev->dev_flags[0], hdev->dev_flags[1],
          atomic_read(&hdev->promisc),
          hdev->stat.byte_rx, hdev->stat.acl_rx, hdev->stat.sco_rx,
          hdev->stat.iso_rx, hdev->stat.byte_tx, hdev->stat.cmd_tx,
          hdev->stat.acl_tx, hdev->stat.sco_tx, hdev->stat.iso_tx);

        used = linux_bt_upstream_hci_append(
          out, out_len, used,
          "    conn-hash acl=%u sco=%u le=%u le-peripheral=%u "
          "cis=%u bis=%u pa=%u\n",
          hdev->conn_hash.acl_num, hdev->conn_hash.sco_num,
          hdev->conn_hash.le_num, hdev->conn_hash.le_num_peripheral,
          hdev->conn_hash.cis_num, hdev->conn_hash.bis_num,
          hdev->conn_hash.pa_num);

        used = linux_bt_upstream_hci_append(
          out, out_len, used,
          "    identity bdaddr=%02x:%02x:%02x:%02x:%02x:%02x "
          "hci=%u.%u rev=0x%04x lmp=%u.%u subver=0x%04x "
          "manufacturer=0x%04x\n",
          bdaddr[5], bdaddr[4], bdaddr[3], bdaddr[2], bdaddr[1],
          bdaddr[0], hdev->hci_ver >> 4, hdev->hci_ver & 0x0f,
          hdev->hci_rev, hdev->lmp_ver >> 4, hdev->lmp_ver & 0x0f,
          hdev->lmp_subver, hdev->manufacturer);

        used = linux_bt_upstream_hci_append(
          out, out_len, used,
          "    buffers acl=%u:%u sco=%u:%u le=%u:%u iso=%u:%u "
          "features=%02x %02x %02x %02x %02x %02x %02x %02x "
          "le-features=%02x %02x %02x %02x %02x %02x %02x %02x\n",
          hdev->acl_mtu, hdev->acl_pkts, hdev->sco_mtu, hdev->sco_pkts,
          hdev->le_mtu, hdev->le_pkts, hdev->iso_mtu, hdev->iso_pkts,
          hdev->features[0][0], hdev->features[0][1],
          hdev->features[0][2], hdev->features[0][3],
          hdev->features[0][4], hdev->features[0][5],
          hdev->features[0][6], hdev->features[0][7],
          hdev->le_features[0], hdev->le_features[1],
          hdev->le_features[2], hdev->le_features[3],
          hdev->le_features[4], hdev->le_features[5],
          hdev->le_features[6], hdev->le_features[7]);

        {
          struct hci_conn *conn;
          unsigned int conn_index = 0;

          list_for_each_entry(conn, &hdev->conn_hash.list, list)
            {
              uint8_t dst[6];
              uint8_t src[6];

              memcpy(dst, &conn->dst, sizeof(dst));
              memcpy(src, &conn->src, sizeof(src));

              used = linux_bt_upstream_hci_append(
                out, out_len, used,
                "      conn[%u] type=%u role=%u state=%u out=%u "
                "handle=0x%04x dst-type=%u "
                "dst=%02x:%02x:%02x:%02x:%02x:%02x src-type=%u "
                "src=%02x:%02x:%02x:%02x:%02x:%02x\n",
                conn_index, conn->type, conn->role, conn->state,
                conn->out ? 1 : 0, conn->handle, conn->dst_type,
                dst[5], dst[4], dst[3], dst[2], dst[1], dst[0],
                conn->src_type, src[5], src[4], src[3], src[2], src[1],
                src[0]);
              conn_index++;
            }
        }
      }
    }

  if (count == 0)
    {
      used = linux_bt_upstream_hci_append(out, out_len, used,
                                          "  no registered hci_dev\n");
    }

  return count;
}

int linux_bt_upstream_hci_send(uint8_t pkt_type,
                               const void *payload,
                               size_t payload_len)
{
  struct hci_dev *hdev = linux_bt_upstream_hci_default_dev();
  struct sk_buff *skb;
  void *dst;
  int ret;

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  if (payload == NULL && payload_len > 0)
    {
      return -EINVAL;
    }

  if (hdev->send == NULL)
    {
      return -ENOSYS;
    }

#ifdef CONFIG_SIM_BTHWSIM
  linux_bt_upstream_hci_hwsim_pump_start();
  linux_bt_upstream_hci_hwsim_pump_all();
#endif

  skb = bt_skb_alloc(payload_len + 1, GFP_KERNEL);
  if (skb == NULL)
    {
      return -ENOMEM;
    }

  skb_reserve(skb, 1);
  dst = skb_put(skb, payload_len);
  if (payload_len > 0 && dst == NULL)
    {
      kfree_skb(skb);
      return -ENOMEM;
    }

  if (payload_len > 0)
    {
      memcpy(dst, payload, payload_len);
    }

  hci_skb_pkt_type(skb) = pkt_type;
  hdev->stat.byte_tx += payload_len;
  switch (pkt_type)
    {
      case HCI_COMMAND_PKT:
        hdev->stat.cmd_tx++;
        break;

      case HCI_ACLDATA_PKT:
        hdev->stat.acl_tx++;
        break;

      case HCI_SCODATA_PKT:
        hdev->stat.sco_tx++;
        break;

      case HCI_ISODATA_PKT:
        hdev->stat.iso_tx++;
        break;

      default:
        break;
    }

  ret = hdev->send(hdev, skb);
#ifdef CONFIG_SIM_BTHWSIM
  if (ret >= 0 && pkt_type == HCI_COMMAND_PKT)
    {
      (void)linux_bt_compat_vhci_pump_deferred_events(hdev);
    }

  linux_bt_upstream_hci_hwsim_pump_all();
#endif
  if (ret < 0)
    {
      kfree_skb(skb);
    }

  return ret;
}

int linux_bt_upstream_hci_connect_br_probe(uint16_t peer,
                                           char *out, size_t out_len)
{
  struct hci_dev *hdev;
  bdaddr_t dst;
  uint8_t *addr = (uint8_t *)&dst;
  size_t used = 0;
#ifdef CONFIG_SIM_BTHWSIM
  uint16_t handle;
  char ctrl[96];
  int ctrl_len;
  uint8_t *src;
  uint8_t upstream_conn = 0;
  uint8_t command_conn = 0;
  uint8_t event_conn = 0;
  uint8_t cmd[3 + sizeof(struct hci_cp_create_conn)];
  struct hci_cp_create_conn cp;
  struct hci_conn *sim_conn;
#endif

  if (out != NULL && out_len > 0)
    {
      out[0] = '\0';
    }

  hdev = linux_bt_upstream_vhci_default_hdev();
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  addr[0] = (uint8_t)peer;
  addr[1] = 0x00;
  addr[2] = 0x00;
  addr[3] = 0x00;
  addr[4] = 0xfe;
  addr[5] = 0x02;

#ifdef CONFIG_SIM_BTHWSIM
  handle = (uint16_t)(0x0050 + (peer & 0x00ff));

  memset(&cp, 0, sizeof(cp));
  bacpy(&cp.bdaddr, &dst);
  cp.pkt_type = cpu_to_le16(0xcc18);
  cp.pscan_rep_mode = 0x01;
  cp.pscan_mode = 0x00;
  cp.clock_offset = cpu_to_le16(0x0000);
  cp.role_switch = 0x01;

  linux_bt_upstream_put_le16(cmd, HCI_OP_CREATE_CONN);
  cmd[2] = sizeof(cp);
  memcpy(&cmd[3], &cp, sizeof(cp));

  if (linux_bt_upstream_hci_send(HCI_COMMAND_PKT, cmd, sizeof(cmd)) >= 0)
    {
      command_conn = 1;
    }

  sim_conn = hci_conn_hash_lookup_ba(hdev, ACL_LINK, &dst);
  if (sim_conn != NULL && sim_conn->handle == handle)
    {
      sim_conn->state = BT_CONNECTED;
      sim_conn->role = HCI_ROLE_MASTER;
      sim_conn->out = 1;
      upstream_conn = 1;
      event_conn = 1;
    }
  if (!upstream_conn)
    return -ENOTCONN;

  src = (uint8_t *)&hdev->bdaddr;
  ctrl_len = snprintf(ctrl, sizeof(ctrl),
                      "HCI_CMD_CONNECT src=%u dst=%u transport=bredr "
                      "handle=0x%04x",
                      src[0], peer, handle);
  if (ctrl_len > 0)
    {
      if (!command_conn)
        {
          (void)linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_CTRL, peer, ctrl,
                                    (size_t)ctrl_len);
        }
    }

  g_hci_sim_br_connected = 1;
  g_hci_sim_br_upstream_conn = upstream_conn;
  g_hci_sim_br_role = HCI_ROLE_MASTER;
  g_hci_sim_br_peer = peer;
  g_hci_sim_br_handle = handle;

  used = linux_bt_upstream_hci_append(out, out_len, used,
                                      "upstream-hci-connect-br: peer=%u "
                                      "handle=0x%04x state=%u "
                                      "hci-owner-path=1 "
                                      "sim-status-mirror=diagnostic "
                                      "command-path=%u event-path=%u "
                                      "fallback=0 "
                                      "upstream-hci-conn=%u\n",
                                      peer, handle, BT_CONNECTED,
                                      command_conn, event_conn,
                                      upstream_conn);
  (void)used;
  return 0;
#else
  (void)used;
  return -ENOSYS;
#endif
}

int linux_bt_upstream_hci_disconnect_br_probe(uint16_t peer,
                                              char *out, size_t out_len)
{
#ifdef CONFIG_SIM_BTHWSIM
  struct hci_dev *hdev;
  uint16_t handle;
  char ctrl[96];
  int ctrl_len;
  int ret;
  uint8_t cmd[3 + sizeof(struct hci_cp_disconnect)];
  struct hci_cp_disconnect cp;
  struct hci_conn *conn;
  uint8_t command_path = 0;
  uint8_t event_path = 0;
  size_t used = 0;
  uint8_t *src;

  if (out != NULL && out_len > 0)
    {
      out[0] = '\0';
    }

  hdev = linux_bt_upstream_vhci_default_hdev();
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  handle = (uint16_t)(0x0050 + (peer & 0x00ff));

  memset(&cp, 0, sizeof(cp));
  cp.handle = cpu_to_le16(handle);
  cp.reason = 0x13;
  linux_bt_upstream_put_le16(cmd, HCI_OP_DISCONNECT);
  cmd[2] = sizeof(cp);
  memcpy(&cmd[3], &cp, sizeof(cp));

  ret = linux_bt_upstream_hci_send(HCI_COMMAND_PKT, cmd, sizeof(cmd));
  if (ret >= 0)
    {
      command_path = 1;
      conn = hci_conn_hash_lookup_handle(hdev, handle);
      if (conn == NULL)
        {
          event_path = 1;
        }
    }

  if (!event_path)
    {
      return -ENOTCONN;
    }

  src = (uint8_t *)&hdev->bdaddr;
  ctrl_len = snprintf(ctrl, sizeof(ctrl),
                      "HCI_CMD_DISCONNECT src=%u dst=%u transport=bredr "
                      "handle=0x%04x reason=0x13",
                      src[0], peer, handle);
  if (ctrl_len > 0)
    {
      (void)linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_CTRL, peer, ctrl,
                                (size_t)ctrl_len);
    }

  if (event_path)
    {
      linux_bt_upstream_hci_sim_br_status_clear(handle);
    }

  used = linux_bt_upstream_hci_append(out, out_len, used,
                                      "upstream-hci-disconnect-br: peer=%u "
                                      "handle=0x%04x local-ret=%d "
                                      "command-path=%u event-path=%u "
                                      "fallback=0 hci-owner-path=1\n",
                                      peer, handle, ret, command_path,
                                      event_path);
  (void)used;
  return 0;
#else
  (void)peer;
  if (out != NULL && out_len > 0)
    {
      out[0] = '\0';
    }

  return -ENOSYS;
#endif
}

int linux_bt_upstream_hci_connect_le_probe(uint16_t peer,
                                           char *out, size_t out_len)
{
#ifndef CONFIG_SIM_BTHWSIM
  struct hci_conn *conn;
#endif
  struct hci_dev *hdev;
  bdaddr_t dst;
  uint8_t *addr = (uint8_t *)&dst;
  size_t used = 0;
#ifdef CONFIG_SIM_BTHWSIM
  uint16_t handle;
  char ctrl[96];
  int ctrl_len;
  uint8_t *src;
  uint8_t upstream_conn = 0;
  uint8_t command_conn = 0;
  uint8_t event_conn = 0;
  uint8_t cmd[3 + sizeof(struct hci_cp_le_create_conn)];
  struct hci_cp_le_create_conn cp;
  struct hci_conn *sim_conn;
#endif

  if (out != NULL && out_len > 0)
    {
      out[0] = '\0';
    }

  hdev = linux_bt_upstream_vhci_default_hdev();
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  addr[0] = (uint8_t)peer;
  addr[1] = 0x00;
  addr[2] = 0x00;
  addr[3] = 0x00;
  addr[4] = 0xfe;
  addr[5] = 0x02;

  if (!hci_dev_test_flag(hdev, HCI_LE_ENABLED) && lmp_le_capable(hdev))
    {
      hci_dev_set_flag(hdev, HCI_LE_ENABLED);
    }

#ifdef CONFIG_SIM_BTHWSIM
  handle = (uint16_t)(0x0100 | (peer & 0x00ff));

  memset(&cp, 0, sizeof(cp));
  cp.scan_interval = cpu_to_le16(0x0010);
  cp.scan_window = cpu_to_le16(0x0010);
  cp.peer_addr_type = ADDR_LE_DEV_PUBLIC;
  bacpy(&cp.peer_addr, &dst);
  cp.own_address_type = ADDR_LE_DEV_PUBLIC;
  cp.conn_interval_min = cpu_to_le16(0x0018);
  cp.conn_interval_max = cpu_to_le16(0x0028);
  cp.conn_latency = cpu_to_le16(0x0000);
  cp.supervision_timeout = cpu_to_le16(0x01f4);

  linux_bt_upstream_put_le16(cmd, HCI_OP_LE_CREATE_CONN);
  cmd[2] = sizeof(cp);
  memcpy(&cmd[3], &cp, sizeof(cp));

  if (linux_bt_upstream_hci_send(HCI_COMMAND_PKT, cmd, sizeof(cmd)) >= 0)
    {
      command_conn = 1;
    }

  sim_conn = hci_conn_hash_lookup_le(hdev, &dst, ADDR_LE_DEV_PUBLIC);
  if (sim_conn != NULL && sim_conn->handle == handle)
    {
      upstream_conn = 1;
      event_conn = 1;
    }
  if (!upstream_conn)
    return -ENOTCONN;

  src = (uint8_t *)&hdev->bdaddr;
  ctrl_len = snprintf(ctrl, sizeof(ctrl),
                      "HCI_CMD_CONNECT src=%u dst=%u transport=le "
                      "handle=0x%04x",
                      src[0], peer, handle);
  if (ctrl_len > 0)
    {
      if (!command_conn)
        {
          (void)linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_CTRL, peer, ctrl,
                                    (size_t)ctrl_len);
        }
    }

  g_hci_sim_le_connected = 1;
  g_hci_sim_le_upstream_conn = upstream_conn;
  g_hci_sim_le_role = HCI_ROLE_MASTER;
  g_hci_sim_le_peer = peer;
  g_hci_sim_le_handle = handle;

  used = linux_bt_upstream_hci_append(out, out_len, used,
                                      "upstream-hci-connect-le: peer=%u "
                                      "handle=0x%04x state=%u "
                                      "hci-owner-path=1 "
                                      "sim-status-mirror=diagnostic "
                                      "command-path=%u event-path=%u "
                                      "fallback=0 "
                                      "upstream-hci-conn=%u\n",
                                      peer, handle, BT_CONNECTED,
                                      command_conn, event_conn,
                                      upstream_conn);
  (void)used;
  return 0;
#else
  conn = hci_connect_le(hdev, &dst, ADDR_LE_DEV_PUBLIC, false,
                        BT_SECURITY_LOW, HCI_LE_CONN_TIMEOUT,
                        HCI_ROLE_MASTER, HCI_ADV_PHY_1M, 0);
  if (IS_ERR(conn))
    {
      return (int)PTR_ERR(conn);
    }

  used = linux_bt_upstream_hci_append(out, out_len, used,
                                      "upstream-hci-connect-le: peer=%u "
                                      "handle=0x%04x state=%u kept=1\n",
                                      peer, conn->handle, conn->state);
  (void)used;
  return 0;
#endif
}

int linux_bt_upstream_hci_disconnect_le_probe(uint16_t peer,
                                              char *out, size_t out_len)
{
#ifdef CONFIG_SIM_BTHWSIM
  struct hci_dev *hdev;
  uint16_t handle;
  char ctrl[96];
  int ctrl_len;
  int ret;
  uint8_t cmd[3 + sizeof(struct hci_cp_disconnect)];
  struct hci_cp_disconnect cp;
  struct hci_conn *conn;
  uint8_t command_path = 0;
  uint8_t event_path = 0;
  size_t used = 0;
  uint8_t *src;

  if (out != NULL && out_len > 0)
    {
      out[0] = '\0';
    }

  hdev = linux_bt_upstream_vhci_default_hdev();
  if (hdev == NULL)
    {
      return -ENODEV;
    }

  handle = (uint16_t)(0x0100 | (peer & 0x00ff));

  memset(&cp, 0, sizeof(cp));
  cp.handle = cpu_to_le16(handle);
  cp.reason = 0x13;
  linux_bt_upstream_put_le16(cmd, HCI_OP_DISCONNECT);
  cmd[2] = sizeof(cp);
  memcpy(&cmd[3], &cp, sizeof(cp));

  ret = linux_bt_upstream_hci_send(HCI_COMMAND_PKT, cmd, sizeof(cmd));
  if (ret >= 0)
    {
      command_path = 1;
      conn = hci_conn_hash_lookup_handle(hdev, handle);
      if (conn == NULL)
        {
          event_path = 1;
        }
    }

  if (!event_path)
    {
      return -ENOTCONN;
    }

  src = (uint8_t *)&hdev->bdaddr;
  ctrl_len = snprintf(ctrl, sizeof(ctrl),
                      "HCI_CMD_DISCONNECT src=%u dst=%u transport=le "
                      "handle=0x%04x reason=0x13",
                      src[0], peer, handle);
  if (ctrl_len > 0)
    {
      if (!command_path)
        {
          (void)linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_CTRL, peer, ctrl,
                                    (size_t)ctrl_len);
        }
    }

  used = linux_bt_upstream_hci_append(out, out_len, used,
                                      "upstream-hci-disconnect-le: peer=%u "
                                      "handle=0x%04x local-ret=%d "
                                      "command-path=%u event-path=%u "
                                      "fallback=0 "
                                      "hci-owner-path=1\n",
                                      peer, handle, ret, command_path,
                                      event_path);
  (void)used;
  return 0;
#else
  (void)peer;
  if (out != NULL && out_len > 0)
    {
      out[0] = '\0';
    }

  return -ENOSYS;
#endif
}

int linux_bt_upstream_a2dp_source_sample_peer(uint16_t peer)
{
  static const char media[] = "A2DP:SBC:synthetic-frame";
  uint8_t acl[4 + 4 + sizeof(media) - 1];
  uint16_t handle;
  uint16_t pb_first_non_flush = 0x02;
  uint16_t cid = 0x0041;
  uint16_t l2cap_len = sizeof(media) - 1;
  uint16_t acl_len = 4 + l2cap_len;

  handle = peer != 0 ? (uint16_t)(0x0050 + (peer & 0x00ff)) : 0x0040;

  linux_bt_upstream_put_le16(&acl[0],
                             (uint16_t)(handle | (pb_first_non_flush << 12)));
  linux_bt_upstream_put_le16(&acl[2], acl_len);
  linux_bt_upstream_put_le16(&acl[4], l2cap_len);
  linux_bt_upstream_put_le16(&acl[6], cid);
  memcpy(&acl[8], media, sizeof(media) - 1);

  return linux_bt_upstream_hci_send(HCI_ACLDATA_PKT, acl, sizeof(acl));
}

int linux_bt_upstream_a2dp_source_sample(void)
{
  return linux_bt_upstream_a2dp_source_sample_peer(0);
}

int linux_bt_upstream_le_audio_source_sample(uint8_t big, uint8_t bis)
{
  static const char media[] = "LE-AUDIO:LC3:synthetic-frame";
  uint8_t iso[4 + 4 + sizeof(media) - 1];
  uint16_t handle = (uint16_t)(0x0100 | ((big & 0x0f) << 4) |
                              (bis & 0x0f));
  uint16_t iso_len = 4 + sizeof(media) - 1;

  linux_bt_upstream_put_le16(&iso[0], handle);
  linux_bt_upstream_put_le16(&iso[2], iso_len);
  linux_bt_upstream_put_le16(&iso[4], 1);
  linux_bt_upstream_put_le16(&iso[6], sizeof(media) - 1);
  memcpy(&iso[8], media, sizeof(media) - 1);

  return linux_bt_upstream_hci_send(HCI_ISODATA_PKT, iso, sizeof(iso));
}

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CORE
int hci_suspend_dev(struct hci_dev *hdev)
{
  (void)hdev;
  return 0;
}

int hci_resume_dev(struct hci_dev *hdev)
{
  (void)hdev;
  return 0;
}

int hci_dev_open(__u16 dev)
{
  struct hci_dev *hdev = hci_dev_get(dev);

  if (hdev == NULL)
    {
      return -ENODEV;
    }

  set_bit(HCI_UP, &hdev->flags);
  return 0;
}

int hci_dev_do_close(struct hci_dev *hdev)
{
  if (hdev != NULL)
    {
      clear_bit(HCI_UP, &hdev->flags);
    }

  return 0;
}
#endif

void hci_set_msft_opcode(struct hci_dev *hdev, __u16 opcode)
{
  (void)hdev;
  (void)opcode;
}

void hci_set_aosp_capable(struct hci_dev *hdev)
{
  (void)hdev;
}

void hci_devcd_register(struct hci_dev *hdev,
                        void (*dump)(struct hci_dev *hdev),
                        void (*dump_hdr)(struct hci_dev *hdev,
                                         struct sk_buff *skb),
                        void *data)
{
  (void)hdev;
  (void)dump;
  (void)dump_hdr;
  (void)data;
}

int hci_devcd_init(struct hci_dev *hdev, unsigned int len)
{
  (void)hdev;
  (void)len;
  return 0;
}

void hci_devcd_append(struct hci_dev *hdev, struct sk_buff *skb)
{
  (void)hdev;
  kfree_skb(skb);
}

void hci_devcd_complete(struct hci_dev *hdev)
{
  (void)hdev;
}

void hci_devcd_abort(struct hci_dev *hdev)
{
  (void)hdev;
}

int hci_drv_process_cmd(struct hci_dev *hdev, struct sk_buff *skb)
{
  int ret = 0;

  if (hdev != NULL && hdev->send != NULL && skb != NULL)
    {
      ret = hdev->send(hdev, skb);
      if (ret >= 0)
        {
          return ret;
        }
    }

  kfree_skb(skb);
  return ret;
}

uint8_t eir_append_local_name(struct hci_dev *hdev, uint8_t *eir,
                              uint8_t ad_len)
{
  const char *name = "FeatherBT";
  size_t len;

  (void)hdev;
  if (eir == NULL)
    {
      return ad_len;
    }

  len = strlen(name);
  if (len > 29)
    {
      len = 29;
    }

  eir[ad_len++] = (uint8_t)(len + 1);
  eir[ad_len++] = 0x09;
  memcpy(&eir[ad_len], name, len);
  return (uint8_t)(ad_len + len);
}

uint8_t eir_append_service_data(uint8_t *eir, uint16_t eir_len,
                                uint16_t uuid, uint8_t *data,
                                uint8_t data_len)
{
  if (eir == NULL)
    {
      return (uint8_t)eir_len;
    }

  eir[eir_len++] = (uint8_t)(data_len + 3);
  eir[eir_len++] = 0x16;
  eir[eir_len++] = (uint8_t)uuid;
  eir[eir_len++] = (uint8_t)(uuid >> 8);
  if (data_len > 0 && data != NULL)
    {
      memcpy(&eir[eir_len], data, data_len);
      eir_len += data_len;
    }

  return (uint8_t)eir_len;
}

void eir_create(struct hci_dev *hdev, uint8_t *data)
{
  if (data != NULL)
    {
      (void)eir_append_local_name(hdev, data, 0);
    }
}

uint8_t eir_create_adv_data(struct hci_dev *hdev, uint8_t instance,
                            uint8_t *ptr, uint8_t size)
{
  (void)instance;
  if (ptr == NULL || size == 0)
    {
      return 0;
    }

  return eir_append_local_name(hdev, ptr, 0);
}

uint8_t eir_create_scan_rsp(struct hci_dev *hdev, uint8_t instance,
                            uint8_t *ptr)
{
  (void)instance;
  return ptr != NULL ? eir_append_local_name(hdev, ptr, 0) : 0;
}

uint8_t eir_create_per_adv_data(struct hci_dev *hdev, uint8_t instance,
                                uint8_t *ptr)
{
  (void)instance;
  return ptr != NULL ? eir_append_local_name(hdev, ptr, 0) : 0;
}

void *eir_get_service_data(uint8_t *eir, size_t eir_len, uint16_t uuid,
                           size_t *len)
{
  size_t pos = 0;

  while (eir != NULL && pos + 1 < eir_len)
    {
      uint8_t field_len = eir[pos];

      if (field_len == 0 || pos + field_len >= eir_len)
        {
          break;
        }

      if (eir[pos + 1] == 0x16 && field_len >= 3 &&
          eir[pos + 2] == (uint8_t)uuid &&
          eir[pos + 3] == (uint8_t)(uuid >> 8))
        {
          if (len != NULL)
            {
              *len = field_len - 3;
            }

          return &eir[pos + 4];
        }

      pos += field_len + 1;
    }

  return NULL;
}
