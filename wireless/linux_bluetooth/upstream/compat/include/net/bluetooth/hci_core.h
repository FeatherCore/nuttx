/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/net/bluetooth/hci_core.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_CORE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_CORE_H

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/bitops.h>
#include <linux/idr.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/rcupdate.h>
#include <linux/rfkill.h>
#include <linux/spinlock.h>
#include <linux/srcu.h>
#include <linux/suspend.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci.h>
#include <net/bluetooth/hci_sock.h>
#include <net/bluetooth/iso.h>

struct dentry;
struct sock;
struct notifier_block;
struct workqueue_struct;
struct hci_dev;
struct hci_conn;

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_DEVICE_STRUCT_DEFINED
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_DEVICE_STRUCT_DEFINED
struct device
{
  const struct device_type *type;
  void *parent;
  void *kobj;
  char name[16];
};
#endif

static inline int dev_set_name(struct device *dev, const char *fmt,
                               unsigned int id)
{
  if (dev == NULL)
    {
      return -EINVAL;
    }

  snprintf(dev->name, sizeof(dev->name), fmt, id);
  return 0;
}

static inline const char *dev_name(const struct device *dev)
{
  return dev != NULL ? dev->name : "";
}

static inline int device_add(struct device *dev)
{
  (void)dev;
  return 0;
}

static inline void device_del(struct device *dev)
{
  (void)dev;
}

static inline void put_device(struct device *dev)
{
  (void)dev;
}

struct hci_link;

#define HCI_PRIO_MAX 7
#define HCI_MAX_ID 10000
#define SUSPEND_NOTIFIER_TIMEOUT msecs_to_jiffies(2000)
#define HCI_MAX_ADV_INSTANCES 5
#define HCI_DEFAULT_ADV_DURATION 2
#define HCI_ADV_TX_POWER_NO_PREFERENCE 0x7f
#define HCI_MIN_ADV_MONITOR_HANDLE 1
#define HCI_MAX_ADV_MONITOR_NUM_HANDLES 32
#define HCI_MAX_ADV_MONITOR_NUM_PATTERNS 16
#define HCI_ADV_MONITOR_EXT_NONE 1
#define HCI_ADV_MONITOR_EXT_MSFT 2
#define HCI_CONN_HANDLE_MAX 0x0eff
#define HCI_CONN_HANDLE_UNSET(handle) ((handle) > HCI_CONN_HANDLE_MAX)
#define HCI_MIN_ENC_KEY_SIZE 7
#define HCI_DEFAULT_RPA_TIMEOUT (15 * 60)
#define DEFAULT_CONN_INFO_MIN_AGE 1000
#define DEFAULT_CONN_INFO_MAX_AGE 3000
#define DEFAULT_AUTH_PAYLOAD_TIMEOUT 0x0bb8
#define HCI_MAX_PAGES 3
#define INQUIRY_CACHE_AGE_MAX msecs_to_jiffies(30000)
#define INQUIRY_ENTRY_AGE_MAX msecs_to_jiffies(60000)
#define DISCOV_LE_TIMEOUT 10240
#define DISCOV_INTERLEAVED_TIMEOUT msecs_to_jiffies(5120)
#define DISCOV_BREDR_INQUIRY_LEN 0x08
#define DISCOV_LE_SCAN_INT_FAST 0x0060
#define DISCOV_LE_SCAN_WIN_FAST 0x0030
#define DISCOV_LE_SCAN_INT_SLOW1 0x0800
#define DISCOV_LE_SCAN_WIN_SLOW1 0x0012
#define DISCOV_LE_SCAN_INT 0x0010
#define DISCOV_LE_SCAN_WIN 0x0010
#define DISCOV_LE_SCAN_INT_CONN 0x0060
#define DISCOV_LE_SCAN_WIN_CONN 0x0060
#define DISCOV_LE_FAST_ADV_INT_MIN 0x00a0
#define DISCOV_LE_FAST_ADV_INT_MAX 0x00f0
#define DISCOV_LE_PER_ADV_INT_MIN 0x00a0
#define DISCOV_LE_PER_ADV_INT_MAX 0x00a0
#define NAME_RESOLVE_DURATION msecs_to_jiffies(10240)
#define DISCOV_TYPE_BREDR BIT(BDADDR_BREDR)
#define DISCOV_TYPE_LE (BIT(BDADDR_LE_PUBLIC) | BIT(BDADDR_LE_RANDOM))
#define DISCOV_TYPE_INTERLEAVED \
  (BIT(BDADDR_BREDR) | BIT(BDADDR_LE_PUBLIC) | BIT(BDADDR_LE_RANDOM))
#define DISCOV_INTERLEAVED_INQUIRY_LEN 0x04
#define HCI_FORCE_STATIC_ADDR 0x01
#define PAGE_SCAN_TYPE_STANDARD 0x00
#define PAGE_SCAN_TYPE_INTERLACED 0x01
#define HCI_PROTO_DEFER 0x01
#define HCI_NOTIFY_CONN_ADD 1
#define HCI_NOTIFY_CONN_DEL 2
#define HCI_NOTIFY_VOICE_SETTING 3
#define HCI_NOTIFY_ENABLE_SCO_CVSD 4
#define HCI_NOTIFY_ENABLE_SCO_TRANSP 5
#define HCI_NOTIFY_DISABLE_SCO 6
#define HCI_DEV_REG 1
#define HCI_DEV_UNREG 2
#define HCI_DEV_UP 3
#define HCI_DEV_DOWN 4
#define HCI_DEV_SUSPEND 5
#define HCI_DEV_RESUME 6
#define HCI_DEV_OPEN 7
#define HCI_DEV_CLOSE 8
#define HCI_DEV_SETUP 9
#ifndef BT_POWER_FORCE_ACTIVE_ON
#define BT_POWER_FORCE_ACTIVE_ON 1
#endif
#ifndef BT_POWER_FORCE_ACTIVE_OFF
#define BT_POWER_FORCE_ACTIVE_OFF 0
#endif
#define HCI_REQ_DONE 0
#define HCI_REQ_PEND 1
#define HCI_REQ_CANCELED 2
#ifndef INVALID_LINK
#define INVALID_LINK 0xff
#endif
#define DATA_CMP(_d1, _l1, _d2, _l2) \
  ((_l1) == (_l2) ? memcmp((_d1), (_d2), (_l1)) : (_l1) - (_l2))
#define ADV_DATA_CMP(_adv, _data, _len) \
  DATA_CMP((_adv)->adv_data, (_adv)->adv_data_len, (_data), (_len))
#define SCAN_RSP_CMP(_adv, _data, _len) \
  DATA_CMP((_adv)->scan_rsp_data, (_adv)->scan_rsp_len, (_data), (_len))

extern struct dentry *bt_debugfs;
extern struct list_head hci_dev_list;
extern rwlock_t hci_dev_list_lock;

struct inquiry_data
{
  bdaddr_t bdaddr;
  uint8_t pscan_rep_mode;
  uint8_t pscan_period_mode;
  uint8_t pscan_mode;
  uint8_t dev_class[3];
  uint16_t clock_offset;
  int8_t rssi;
  uint8_t ssp_mode;
};

struct inquiry_entry
{
  struct list_head all;
  struct list_head list;
  enum
    {
      NAME_NOT_KNOWN,
      NAME_NEEDED,
      NAME_PENDING,
      NAME_KNOWN,
    } name_state;
  uint32_t timestamp;
  struct inquiry_data data;
};

struct discovery_state
{
  int type;
  enum
    {
      DISCOVERY_STOPPED,
      DISCOVERY_STARTING,
      DISCOVERY_FINDING,
      DISCOVERY_RESOLVING,
      DISCOVERY_STOPPING,
    } state;
  struct list_head all;
  struct list_head unknown;
  struct list_head resolve;
  uint32_t timestamp;
  bdaddr_t last_adv_addr;
  uint8_t last_adv_addr_type;
  int8_t last_adv_rssi;
  uint32_t last_adv_flags;
  uint8_t last_adv_data[HCI_MAX_EXT_AD_LENGTH];
  uint8_t last_adv_data_len;
  bool report_invalid_rssi;
  bool result_filtering;
  bool limited;
  int8_t rssi;
  uint16_t uuid_count;
  uint8_t (*uuids)[16];
  unsigned long name_resolve_timeout;
  spinlock_t lock;
};

struct hci_cb
{
  struct list_head list;
  const char *name;
  void (*connect_cfm)(struct hci_conn *conn, __u8 status);
  void (*disconn_cfm)(struct hci_conn *conn, __u8 status);
  void (*security_cfm)(struct hci_conn *conn, __u8 status, __u8 encrypt);
  void (*key_change_cfm)(struct hci_conn *conn, __u8 status);
  void (*role_switch_cfm)(struct hci_conn *conn, __u8 status, __u8 role);
};

struct hci_request
{
  struct hci_dev *hdev;
  struct sk_buff_head cmd_q;
  int err;
};

typedef int (*hci_cmd_sync_work_func_t)(struct hci_dev *hdev, void *data);
typedef void (*hci_cmd_sync_work_destroy_t)(struct hci_dev *hdev, void *data,
                                            int err);

struct hci_cmd_sync_work_entry
{
  struct list_head list;
  hci_cmd_sync_work_func_t func;
  void *data;
  hci_cmd_sync_work_destroy_t destroy;
};

typedef void (*hci_req_complete_t)(struct hci_dev *hdev, uint8_t status,
                                   uint16_t opcode);
typedef void (*hci_req_complete_skb_t)(struct hci_dev *hdev, uint8_t status,
                                       uint16_t opcode,
                                       struct sk_buff *skb);

enum suspend_tasks
{
  SUSPEND_PAUSE_DISCOVERY,
  SUSPEND_UNPAUSE_DISCOVERY,
  SUSPEND_PAUSE_ADVERTISING,
  SUSPEND_UNPAUSE_ADVERTISING,
  SUSPEND_SCAN_DISABLE,
  SUSPEND_SCAN_ENABLE,
  SUSPEND_DISCONNECTING,
  SUSPEND_POWERING_DOWN,
  SUSPEND_PREPARE_NOTIFIER,
  SUSPEND_SET_ADV_FILTER,
  __SUSPEND_NUM_TASKS
};

enum suspended_state
{
  BT_RUNNING = 0,
  BT_SUSPEND_DISCONNECT,
  BT_SUSPEND_CONFIGURE_WAKE,
};

struct tx_queue
{
  struct sk_buff_head queue;
  unsigned int extra;
  unsigned int tracked;
};

struct hci_conn_hash
{
  struct list_head list;
  unsigned int acl_num;
  unsigned int sco_num;
  unsigned int cis_num;
  unsigned int bis_num;
  unsigned int pa_num;
  unsigned int le_num;
  unsigned int le_num_peripheral;
};

struct bdaddr_list
{
  struct list_head list;
  bdaddr_t bdaddr;
  uint8_t bdaddr_type;
};

struct bdaddr_list_with_irk
{
  struct list_head list;
  bdaddr_t bdaddr;
  uint8_t bdaddr_type;
  uint8_t peer_irk[16];
  uint8_t local_irk[16];
};

enum hci_conn_flags
{
  HCI_CONN_FLAG_REMOTE_WAKEUP = BIT(0),
  HCI_CONN_FLAG_DEVICE_PRIVACY = BIT(1),
  HCI_CONN_FLAG_ADDRESS_RESOLUTION = BIT(2),
};
typedef uint8_t hci_conn_flags_t;

enum
{
  HCI_CONN_AUTH_PEND,
  HCI_CONN_ENCRYPT_PEND,
  HCI_CONN_RSWITCH_PEND,
  HCI_CONN_MODE_CHANGE_PEND,
  HCI_CONN_SCO_SETUP_PEND,
  HCI_CONN_LE_SMP_PEND,
  HCI_CONN_MGMT_CONNECTED,
  HCI_CONN_SSP_ENABLED,
  HCI_CONN_SC_ENABLED,
  HCI_CONN_AES_CCM,
  HCI_CONN_POWER_SAVE,
  HCI_CONN_REMOTE_OOB,
  HCI_CONN_STK_ENCRYPT,
  HCI_CONN_AUTH_INITIATOR,
  HCI_CONN_DROP,
  HCI_CONN_PARAM_REMOVAL_PEND,
  HCI_CONN_NEW_LINK_KEY,
  HCI_CONN_SCANNING,
  HCI_CONN_AUTH,
  HCI_CONN_ENCRYPT,
  HCI_CONN_SECURE,
  HCI_CONN_FIPS,
  HCI_CONN_CONFIRMED,
  HCI_CONN_BIG_CREATED,
  HCI_CONN_PA_SYNC,
  HCI_CONN_BIG_SYNC,
  HCI_CONN_PER_ADV,
  HCI_CONN_CREATE_CIS,
  HCI_CONN_BIG_SYNC_FAILED,
  HCI_CONN_PA_SYNC_FAILED,
  HCI_CONN_CREATE_PA_SYNC,
  HCI_CONN_CREATE_BIG_SYNC,
  HCI_CONN_CANCEL,
  HCI_CONN_FLUSH_KEY,
  HCI_CONN_AUTH_FAILURE,
};

struct bdaddr_list_with_flags
{
  struct list_head list;
  bdaddr_t bdaddr;
  uint8_t bdaddr_type;
  hci_conn_flags_t flags;
};

struct bt_uuid
{
  struct list_head list;
  uint8_t uuid[16];
  uint8_t size;
  uint8_t svc_hint;
};

struct blocked_key
{
  struct list_head list;
  struct rcu_head rcu;
  uint8_t type;
  uint8_t val[16];
};

struct smp_csrk
{
  bdaddr_t bdaddr;
  uint8_t bdaddr_type;
  uint8_t type;
  uint8_t val[16];
};

#ifndef LINUX_BT_HCI_DEV_STATS_DEFINED
struct hci_dev_stats
{
  unsigned long byte_tx;
  unsigned long cmd_tx;
  unsigned long acl_tx;
  unsigned long sco_tx;
  unsigned long iso_tx;
  unsigned long byte_rx;
  unsigned long acl_rx;
  unsigned long sco_rx;
  unsigned long iso_rx;
  unsigned long err_rx;
};
#endif

struct smp_ltk
{
  struct list_head list;
  struct rcu_head rcu;
  bdaddr_t bdaddr;
  uint8_t bdaddr_type;
  uint8_t authenticated;
  uint8_t type;
  uint8_t enc_size;
  uint16_t ediv;
  uint64_t rand;
  uint8_t val[16];
};

struct smp_irk
{
  struct list_head list;
  struct rcu_head rcu;
  bdaddr_t rpa;
  bdaddr_t bdaddr;
  uint8_t addr_type;
  uint8_t val[16];
};

struct link_key
{
  struct list_head list;
  struct rcu_head rcu;
  bdaddr_t bdaddr;
  uint8_t type;
  uint8_t val[HCI_LINK_KEY_SIZE];
  uint8_t pin_len;
};

struct oob_data
{
  struct list_head list;
  bdaddr_t bdaddr;
  uint8_t bdaddr_type;
  uint8_t present;
  uint8_t hash192[16];
  uint8_t rand192[16];
  uint8_t hash256[16];
  uint8_t rand256[16];
};

struct adv_info
{
  struct list_head list;
  bool enabled;
  bool pending;
  bool periodic;
  bool periodic_enabled;
  uint8_t mesh;
  uint8_t instance;
  uint8_t handle;
  uint8_t sid;
  uint32_t flags;
  uint16_t timeout;
  uint16_t remaining_time;
  uint16_t duration;
  uint16_t adv_data_len;
  uint8_t adv_data[HCI_MAX_EXT_AD_LENGTH];
  bool adv_data_changed;
  uint16_t scan_rsp_len;
  uint8_t scan_rsp_data[HCI_MAX_EXT_AD_LENGTH];
  bool scan_rsp_changed;
  uint16_t per_adv_data_len;
  uint8_t per_adv_data[HCI_MAX_PER_AD_LENGTH];
  int8_t tx_power;
  uint32_t min_interval;
  uint32_t max_interval;
  bdaddr_t random_addr;
  bool rpa_expired;
  struct delayed_work rpa_expired_cb;
};

struct monitored_device
{
  struct list_head list;
  bdaddr_t bdaddr;
  uint8_t addr_type;
  uint16_t handle;
  bool notified;
};

struct adv_pattern
{
  struct list_head list;
  uint8_t ad_type;
  uint8_t offset;
  uint8_t length;
  uint8_t value[HCI_MAX_EXT_AD_LENGTH];
};

struct adv_rssi_thresholds
{
  int8_t low_threshold;
  int8_t high_threshold;
  uint16_t low_threshold_timeout;
  uint16_t high_threshold_timeout;
  uint8_t sampling_period;
};

struct adv_monitor
{
  struct list_head patterns;
  struct adv_rssi_thresholds rssi;
  uint16_t handle;
  enum
    {
      ADV_MONITOR_STATE_NOT_REGISTERED,
      ADV_MONITOR_STATE_REGISTERED,
      ADV_MONITOR_STATE_OFFLOADED
    } state;
};

struct hci_conn_params
{
  struct list_head list;
  struct list_head action;
  bdaddr_t addr;
  uint8_t addr_type;
  uint16_t conn_min_interval;
  uint16_t conn_max_interval;
  uint16_t conn_latency;
  uint16_t supervision_timeout;
  enum
    {
      HCI_AUTO_CONN_DISABLED,
      HCI_AUTO_CONN_REPORT,
      HCI_AUTO_CONN_DIRECT,
      HCI_AUTO_CONN_ALWAYS,
      HCI_AUTO_CONN_LINK_LOSS,
      HCI_AUTO_CONN_EXPLICIT,
    } auto_connect;
  struct hci_conn *conn;
  bool explicit_connect;
  hci_conn_flags_t flags;
  uint8_t privacy_mode;
};

struct hci_dev
{
  struct list_head list;
  struct srcu_struct srcu;
  struct mutex lock;
  struct ida unset_handle_ida;
  uint16_t id;
  uint8_t bus;
  const char *name;
  char name_storage[16];
  void *driver_data;
  bdaddr_t bdaddr;
  bdaddr_t setup_addr;
  bdaddr_t public_addr;
  bdaddr_t random_addr;
  bdaddr_t static_addr;
  uint8_t adv_addr_type;
  uint8_t dev_name[HCI_MAX_NAME_LENGTH];
  uint8_t short_name[HCI_MAX_SHORT_NAME_LENGTH];
  uint8_t eir[HCI_MAX_EIR_LENGTH];
  uint16_t appearance;
  uint8_t dev_class[3];
  uint8_t major_class;
  uint8_t minor_class;
  uint8_t max_page;
  uint8_t features[HCI_MAX_PAGES][8];
  uint8_t le_features[8];
  uint8_t le_accept_list_size;
  uint8_t le_resolv_list_size;
  uint8_t le_num_of_adv_sets;
  uint8_t le_states[8];
  uint8_t commands[64];
  uint8_t hci_ver;
  uint16_t hci_rev;
  uint8_t lmp_ver;
  uint16_t manufacturer;
  uint16_t lmp_subver;
  uint16_t voice_setting;
  uint8_t io_capability;
  int8_t inq_tx_power;
  uint8_t err_data_reporting;
  uint16_t stored_max_keys;
  uint16_t stored_num_keys;
  uint8_t ssp_debug_mode;
  unsigned long flags;
  unsigned long dev_flags[4];
  unsigned long quirks[2];
  struct dentry *debugfs;
  struct device dev;
  struct hci_dev_stats stat;
  uint8_t scan_enable;
  uint8_t auth_enable;
  uint8_t encrypt_mode;
  uint32_t pkt_type;
  uint32_t link_policy;
  uint32_t link_mode;
  uint16_t acl_mtu;
  uint16_t acl_pkts;
  uint16_t sco_mtu;
  uint16_t sco_pkts;
  uint16_t esco_type;
  uint16_t idle_timeout;
  uint16_t sniff_min_interval;
  uint16_t sniff_max_interval;
  uint8_t num_iac;
  uint16_t page_scan_interval;
  uint16_t page_scan_window;
  uint8_t page_scan_type;
  uint8_t def_page_scan_type;
  uint16_t def_page_scan_int;
  uint16_t def_page_scan_window;
  uint8_t def_inq_scan_type;
  uint16_t def_inq_scan_int;
  uint16_t def_inq_scan_window;
  uint16_t def_br_lsto;
  uint16_t def_page_timeout;
  uint16_t le_conn_min_interval;
  uint16_t le_conn_max_interval;
  uint16_t le_conn_latency;
  uint16_t le_supv_timeout;
  uint16_t le_adv_min_interval;
  uint16_t le_adv_max_interval;
  uint8_t le_adv_channel_map;
  uint8_t le_scan_type;
  uint16_t le_scan_interval;
  uint16_t le_scan_window;
  uint16_t le_scan_int_suspend;
  uint16_t le_scan_window_suspend;
  uint16_t le_scan_int_discovery;
  uint16_t le_scan_window_discovery;
  uint16_t le_scan_int_adv_monitor;
  uint16_t le_scan_window_adv_monitor;
  uint16_t le_scan_int_connect;
  uint16_t le_scan_window_connect;
  uint16_t le_def_tx_len;
  uint16_t le_def_tx_time;
  uint16_t le_max_tx_len;
  uint16_t le_max_tx_time;
  uint16_t le_max_rx_len;
  uint16_t le_max_rx_time;
  uint8_t le_max_key_size;
  uint8_t le_min_key_size;
  uint16_t def_multi_adv_rotation_duration;
  unsigned long def_le_autoconnect_timeout;
  int8_t min_le_tx_power;
  int8_t max_le_tx_power;
  unsigned long discov_interleaved_timeout;
  unsigned long conn_info_min_age;
  unsigned long conn_info_max_age;
  uint16_t auth_payload_timeout;
  uint8_t min_enc_key_size;
  uint8_t max_enc_key_size;
  uint8_t pairing_opts;
  uint16_t advmon_allowlist_duration;
  uint16_t advmon_no_filter_duration;
  bool enable_advmon_interleave_scan;
  unsigned long acl_last_tx;
  unsigned long le_last_tx;
  struct notifier_block suspend_notifier;
  unsigned int auto_accept_delay;
  atomic_t cmd_cnt;
  unsigned int acl_cnt;
  unsigned int sco_cnt;
  unsigned int le_cnt;
  unsigned int iso_cnt;
  unsigned int le_mtu;
  unsigned int iso_mtu;
  unsigned int le_pkts;
  unsigned int iso_pkts;
  uint8_t le_tx_def_phys;
  uint8_t le_rx_def_phys;
  struct workqueue_struct *workqueue;
  struct workqueue_struct *req_workqueue;
  struct work_struct power_on;
  struct delayed_work power_off;
  struct work_struct error_reset;
  struct work_struct cmd_sync_work;
  struct list_head cmd_sync_work_list;
  struct mutex cmd_sync_work_lock;
  struct mutex unregister_lock;
  struct work_struct cmd_sync_cancel_work;
  struct work_struct reenable_adv_work;
  uint16_t discov_timeout;
  struct delayed_work discov_off;
  struct delayed_work service_cache;
  struct delayed_work cmd_timer;
  struct delayed_work ncmd_timer;
  struct work_struct rx_work;
  struct work_struct cmd_work;
  struct work_struct tx_work;
  struct delayed_work le_scan_disable;
  struct sk_buff_head rx_q;
  struct sk_buff_head raw_q;
  struct sk_buff_head cmd_q;
  struct sk_buff *sent_cmd;
  struct sk_buff *recv_event;
  struct mutex req_lock;
  wait_queue_head_t req_wait_q;
  uint32_t req_status;
  uint32_t req_result;
  struct sk_buff *req_skb;
  struct sk_buff *req_rsp;
  void *smp_data;
  void *smp_bredr_data;
  struct discovery_state discovery;
  bool discovery_paused;
  int advertising_old_state;
  bool advertising_paused;
  enum suspended_state suspend_state_next;
  enum suspended_state suspend_state;
  bool scanning_paused;
  bool suspended;
  uint8_t wake_reason;
  bdaddr_t wake_addr;
  uint8_t wake_addr_type;
  atomic_t promisc;
  struct hci_conn_hash conn_hash;
  struct list_head mesh_pending;
  struct mutex mgmt_pending_lock;
  struct list_head mgmt_pending;
  struct list_head reject_list;
  struct list_head accept_list;
  struct list_head uuids;
  struct list_head link_keys;
  struct list_head long_term_keys;
  struct list_head identity_resolving_keys;
  struct list_head remote_oob_data;
  struct list_head le_accept_list;
  struct list_head le_resolv_list;
  struct list_head le_conn_params;
  struct list_head pend_le_conns;
  struct list_head pend_le_reports;
  struct list_head blocked_keys;
  struct list_head local_codecs;
  const char *hw_info;
  const char *fw_info;
  void *dump;
  struct rfkill *rfkill;
  hci_conn_flags_t conn_flags;
  int8_t adv_tx_power;
  uint8_t adv_data[HCI_MAX_EXT_AD_LENGTH];
  uint8_t adv_data_len;
  uint8_t scan_rsp_data[HCI_MAX_EXT_AD_LENGTH];
  uint8_t scan_rsp_data_len;
  uint8_t per_adv_data[HCI_MAX_PER_AD_LENGTH];
  uint8_t per_adv_data_len;
  struct list_head adv_instances;
  unsigned int adv_instance_cnt;
  uint8_t cur_adv_instance;
  uint16_t adv_instance_timeout;
  struct delayed_work adv_instance_expire;
  struct idr adv_monitors_idr;
  unsigned int adv_monitors_cnt;
  uint8_t irk[16];
  uint32_t rpa_timeout;
  struct delayed_work rpa_expired;
  bdaddr_t rpa;
  uint8_t mesh_send_ref;
  struct delayed_work mesh_send_done;
  uint8_t mesh_ad_types[8];
  enum
    {
      INTERLEAVE_SCAN_NONE,
      INTERLEAVE_SCAN_NO_FILTER,
      INTERLEAVE_SCAN_ALLOWLIST
    } interleave_scan_state;
  struct delayed_work interleave_scan;
  struct list_head monitored_devices;
  bool advmon_pend_notify;
  uint8_t hw_error_code;
  uint32_t clock;
  uint16_t devid_source;
  uint16_t devid_vendor;
  uint16_t devid_product;
  uint16_t devid_version;

  int (*open)(struct hci_dev *hdev);
  int (*close)(struct hci_dev *hdev);
  int (*flush)(struct hci_dev *hdev);
  int (*send)(struct hci_dev *hdev, struct sk_buff *skb);
  void (*notify)(struct hci_dev *hdev, unsigned int evt);
  uint8_t (*classify_pkt_type)(struct hci_dev *hdev, struct sk_buff *skb);
  void (*hw_error)(struct hci_dev *hdev, uint8_t code);
  int (*post_init)(struct hci_dev *hdev);
  int (*set_diag)(struct hci_dev *hdev, bool enable);
  int (*set_bdaddr)(struct hci_dev *hdev, const bdaddr_t *bdaddr);
  int (*set_quality_report)(struct hci_dev *hdev, bool enable);
  void (*reset)(struct hci_dev *hdev);
  int (*get_data_path_id)(struct hci_dev *hdev, u8 *data_path_id);
  int (*get_codec_config_data)(struct hci_dev *hdev, __u8 type,
                               struct bt_codec *codec, __u8 *vnd_len,
                               __u8 **vnd_data);
  bool (*wakeup)(struct hci_dev *hdev);
  int (*shutdown)(struct hci_dev *hdev);
  int (*setup)(struct hci_dev *hdev);
};

enum conn_reasons
{
  CONN_REASON_PAIR_DEVICE,
  CONN_REASON_L2CAP_CHAN,
  CONN_REASON_SCO_CONNECT,
  CONN_REASON_ISO_CONNECT,
};

struct hci_conn
{
  struct list_head list;
  atomic_t refcnt;
  bdaddr_t dst;
  uint8_t dst_type;
  bdaddr_t src;
  uint8_t src_type;
  bdaddr_t init_addr;
  uint8_t init_addr_type;
  bdaddr_t resp_addr;
  uint8_t resp_addr_type;
  uint8_t adv_instance;
  uint16_t handle;
  uint16_t sync_handle;
  uint8_t sid;
  uint16_t state;
  uint16_t mtu;
  uint8_t mode;
  uint8_t type;
  uint8_t role;
  bool out;
  uint8_t attempt;
  uint8_t dev_class[3];
  uint8_t features[3][8];
  uint16_t pkt_type;
  uint16_t link_policy;
  uint8_t key_type;
  uint8_t auth_type;
  uint8_t sec_level;
  uint8_t pending_sec_level;
  uint8_t pin_length;
  uint8_t enc_key_size;
  uint8_t io_capability;
  uint32_t passkey_notify;
  uint8_t passkey_entered;
  uint16_t disc_timeout;
  uint16_t conn_timeout;
  uint16_t setting;
  uint16_t auth_payload_timeout;
  uint16_t le_conn_min_interval;
  uint16_t le_conn_max_interval;
  uint16_t le_conn_interval;
  uint16_t le_conn_latency;
  uint16_t le_supv_timeout;
  uint8_t le_adv_data[HCI_MAX_EXT_AD_LENGTH];
  uint8_t le_adv_data_len;
  uint8_t le_per_adv_data[HCI_MAX_PER_AD_TOT_LEN];
  uint16_t le_per_adv_data_len;
  uint16_t le_per_adv_data_offset;
  uint8_t le_adv_phy;
  uint8_t le_adv_sec_phy;
  uint8_t le_tx_phy;
  uint8_t le_rx_phy;
  int8_t rssi;
  int8_t tx_power;
  int8_t max_tx_power;
  struct bt_iso_qos iso_qos;
  uint8_t num_bis;
  uint8_t bis[HCI_MAX_ISO_BIS];
  unsigned long flags;
  enum conn_reasons conn_reason;
  uint8_t abort_reason;
  uint32_t clock;
  uint16_t clock_accuracy;
  unsigned long conn_info_timestamp;
  uint8_t remote_cap;
  uint8_t remote_auth;
  unsigned int sent;
  struct sk_buff_head data_q;
  struct list_head chan_list;
  struct tx_queue tx_q;
  struct delayed_work disc_work;
  struct delayed_work auto_accept_work;
  struct delayed_work idle_work;
  struct delayed_work le_conn_timeout;
  struct hci_dev *hdev;
  struct dentry *debugfs;
  void *l2cap_data;
  void *sco_data;
  void *iso_data;
  struct list_head link_list;
  struct hci_conn *parent;
  struct hci_link *link;
  struct bt_codec codec;
  void (*connect_cfm_cb)(struct hci_conn *conn, uint8_t status);
  void (*security_cfm_cb)(struct hci_conn *conn, uint8_t status);
  void (*disconn_cfm_cb)(struct hci_conn *conn, uint8_t reason);
  void (*cleanup)(struct hci_conn *conn);
};

struct hci_link
{
  struct list_head list;
  struct hci_conn *conn;
};

struct hci_chan
{
  struct list_head list;
  uint16_t handle;
  struct hci_conn *conn;
  struct sk_buff_head data_q;
  unsigned int sent;
  uint8_t state;
};

struct hci_chan *hci_chan_create(struct hci_conn *conn);

enum devcoredump_state
{
  HCI_DEVCOREDUMP_DONE,
  HCI_DEVCOREDUMP_ABORT,
  HCI_DEVCOREDUMP_TIMEOUT,
};

struct hci_dev *hci_alloc_dev_priv(int sizeof_priv);
struct hci_dev *hci_alloc_dev(void);
void hci_free_dev(struct hci_dev *hdev);
int hci_register_dev(struct hci_dev *hdev);
void hci_unregister_dev(struct hci_dev *hdev);
void hci_set_drvdata(struct hci_dev *hdev, void *data);
void *hci_get_drvdata(struct hci_dev *hdev);
void hci_set_quirk(struct hci_dev *hdev, unsigned int quirk);
bool hci_test_quirk(struct hci_dev *hdev, unsigned int quirk);
struct hci_dev *hci_dev_get(int index);
static inline struct hci_dev *hci_dev_hold(struct hci_dev *hdev)
{
  return hdev;
}

static inline void hci_dev_put(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void hci_dev_set_flag(struct hci_dev *hdev, unsigned int nr)
{
  if (hdev != NULL)
    {
      set_bit(nr, hdev->dev_flags);
    }
}

static inline void hci_dev_clear_flag(struct hci_dev *hdev, unsigned int nr)
{
  if (hdev != NULL)
    {
      clear_bit(nr, hdev->dev_flags);
    }
}

static inline int hci_dev_test_flag(struct hci_dev *hdev, unsigned int nr)
{
  return hdev != NULL ? test_bit(nr, hdev->dev_flags) : 0;
}

static inline int hci_dev_test_and_clear_flag(struct hci_dev *hdev,
                                              unsigned int nr)
{
  int old;

  if (hdev == NULL)
    {
      return 0;
    }

  old = test_bit(nr, hdev->dev_flags);
  clear_bit(nr, hdev->dev_flags);
  return old;
}

static inline void hci_dev_change_flag(struct hci_dev *hdev,
                                       unsigned int nr)
{
  if (hdev != NULL)
    {
      if (hci_dev_test_flag(hdev, nr))
        {
          hci_dev_clear_flag(hdev, nr);
        }
      else
        {
          hci_dev_set_flag(hdev, nr);
        }
    }
}

static inline int hci_dev_test_and_change_flag(struct hci_dev *hdev,
                                               unsigned int nr)
{
  int old = hci_dev_test_flag(hdev, nr);

  hci_dev_change_flag(hdev, nr);
  return old;
}

static inline int hci_dev_test_and_set_flag(struct hci_dev *hdev,
                                            unsigned int nr)
{
  return hdev != NULL ? test_and_set_bit(nr, hdev->dev_flags) : 0;
}

static inline void hci_dev_lock(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void hci_dev_unlock(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline bool hdev_is_powered(struct hci_dev *hdev)
{
  return hdev != NULL && test_bit(HCI_UP, &hdev->flags);
}

static inline bool hci_is_identity_address(bdaddr_t *bdaddr, uint8_t type)
{
  (void)bdaddr;
  return type == BDADDR_BREDR || bdaddr_type_is_le(type);
}

static inline bool hci_bdaddr_is_rpa(bdaddr_t *bdaddr, uint8_t type)
{
  (void)bdaddr;
  (void)type;
  return false;
}

static inline bool lmp_bredr_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_le_capable(struct hci_dev *hdev)
{
  return hdev != NULL && (hdev->features[0][4] & 0x40) != 0;
}

static inline bool lmp_host_le_capable(struct hci_dev *hdev)
{
  return hdev != NULL && (hdev->features[1][0] & LMP_HOST_LE) != 0;
}

static inline bool lmp_encrypt_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_sc_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_sniff_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return false;
}

static inline bool lmp_sniffsubr_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return false;
}

static inline bool lmp_esco_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_esco_2m_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_edr_2m_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_edr_3m_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_edr_3slot_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_edr_5slot_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_no_flush_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_ssp_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_host_ssp_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_host_sc_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_host_le_br_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_ext_inq_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_inq_rssi_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_inq_tx_pwr_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_ext_feat_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_pause_enc_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_lsto_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_rswitch_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_hold_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return false;
}

static inline bool lmp_park_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return false;
}

static inline bool lmp_sco_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_cpb_central_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_cpb_peripheral_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_ping_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool lmp_sync_train_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return false;
}

static inline bool ext_adv_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool ll_privacy_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return false;
}

static inline bool cis_central_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool bis_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool cis_capable(struct hci_dev *hdev)
{
  return hdev != NULL && (hdev->le_features[3] & 0x10) != 0;
}

static inline bool cis_peripheral_capable(struct hci_dev *hdev)
{
  return cis_capable(hdev);
}

static inline bool iso_capable(struct hci_dev *hdev)
{
  return cis_capable(hdev) || bis_capable(hdev);
}

static inline bool iso_enabled(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool cis_central_enabled(struct hci_dev *hdev)
{
  return cis_central_capable(hdev);
}

static inline bool cis_peripheral_enabled(struct hci_dev *hdev)
{
  return cis_peripheral_capable(hdev);
}

static inline bool bis_enabled(struct hci_dev *hdev)
{
  return bis_capable(hdev);
}

static inline bool sync_recv_capable(struct hci_dev *hdev)
{
  return bis_capable(hdev);
}

static inline bool sync_recv_enabled(struct hci_dev *hdev)
{
  return sync_recv_capable(hdev);
}

static inline bool ll_privacy_enabled(struct hci_dev *hdev)
{
  return ll_privacy_capable(hdev);
}

static inline bool le_2m_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool le_coded_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool privacy_mode_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return false;
}

static inline bool use_enhanced_conn_complete(struct hci_dev *hdev)
{
  return hdev != NULL &&
         (ll_privacy_capable(hdev) || ext_adv_capable(hdev)) &&
         !hci_test_quirk(hdev, HCI_QUIRK_BROKEN_EXT_CREATE_CONN);
}

static inline bool use_ext_conn(struct hci_dev *hdev)
{
  return hdev != NULL && (hdev->commands[37] & 0x80) &&
         !hci_test_quirk(hdev, HCI_QUIRK_BROKEN_EXT_CREATE_CONN);
}

static inline bool enhanced_sync_conn_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool read_voice_setting_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool read_key_size_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool mws_transport_config_capable(struct hci_dev *hdev)
{
  (void)hdev;
  return false;
}

static inline bool bredr_sc_enabled(struct hci_dev *hdev)
{
  return hci_dev_test_flag(hdev, HCI_SC_ENABLED);
}

static inline bool hci_dev_le_state_simultaneous(struct hci_dev *hdev)
{
  (void)hdev;
  return true;
}

static inline bool rpa_valid(bdaddr_t *bdaddr)
{
  (void)bdaddr;
  return false;
}

static inline bool adv_rpa_valid(struct adv_info *adv)
{
  return adv != NULL && bacmp(&adv->random_addr, BDADDR_ANY) != 0;
}

static inline uint8_t max_adv_len(struct hci_dev *hdev)
{
  (void)hdev;
  return HCI_MAX_EXT_AD_LENGTH;
}

static inline uint8_t scan_1m(struct hci_dev *hdev)
{
  (void)hdev;
  return 0x01;
}

static inline uint8_t scan_2m(struct hci_dev *hdev)
{
  (void)hdev;
  return 0x02;
}

static inline uint8_t scan_coded(struct hci_dev *hdev)
{
  (void)hdev;
  return 0x04;
}

#define INTERVAL_TO_MS(x) (((x) * 10) / 0x10)

static inline unsigned int hci_iso_count(struct hci_dev *hdev)
{
  return hdev != NULL ? hdev->conn_hash.cis_num + hdev->conn_hash.bis_num : 0;
}

static inline unsigned int hci_conn_num(struct hci_dev *hdev, __u8 type)
{
  if (hdev == NULL)
    {
      return 0;
    }

  switch (type)
    {
      case ACL_LINK:
        return hdev->conn_hash.acl_num;
      case LE_LINK:
        return hdev->conn_hash.le_num;
      case SCO_LINK:
      case ESCO_LINK:
        return hdev->conn_hash.sco_num;
      case CIS_LINK:
        return hdev->conn_hash.cis_num;
      case BIS_LINK:
        return hdev->conn_hash.bis_num;
      case PA_LINK:
        return hdev->conn_hash.pa_num;
      default:
        break;
    }

  return 0;
}

static inline bool use_ext_scan(struct hci_dev *hdev)
{
  return ext_adv_capable(hdev);
}

#define hci_req_sync_lock(hdev) mutex_lock(&(hdev)->req_lock)
#define hci_req_sync_unlock(hdev) mutex_unlock(&(hdev)->req_lock)

static inline struct smp_irk *hci_get_irk(struct hci_dev *hdev,
                                          bdaddr_t *bdaddr,
                                          uint8_t addr_type)
{
  (void)hdev;
  (void)bdaddr;
  (void)addr_type;
  return NULL;
}

static inline bool hci_conn_sc_enabled(struct hci_conn *conn)
{
  return conn != NULL && test_bit(HCI_CONN_SC_ENABLED, &conn->flags);
}

static inline bool hci_conn_ssp_enabled(struct hci_conn *conn)
{
  return conn != NULL && test_bit(HCI_CONN_SSP_ENABLED, &conn->flags);
}

static inline void hci_conn_hash_add(struct hci_dev *hdev,
                                     struct hci_conn *conn)
{
  struct hci_conn_hash *hash;

  if (hdev == NULL || conn == NULL)
    {
      return;
    }

  hash = &hdev->conn_hash;
  list_add_tail(&conn->list, &hash->list);
  switch (conn->type)
    {
      case ACL_LINK:
        hash->acl_num++;
        break;
      case LE_LINK:
        hash->le_num++;
        if (conn->role == HCI_ROLE_SLAVE)
          {
            hash->le_num_peripheral++;
          }
        break;
      case SCO_LINK:
      case ESCO_LINK:
        hash->sco_num++;
        break;
      case CIS_LINK:
        hash->cis_num++;
        break;
      case BIS_LINK:
        hash->bis_num++;
        break;
      case PA_LINK:
        hash->pa_num++;
        break;
      default:
        break;
    }
}

static inline void hci_conn_hash_del(struct hci_dev *hdev,
                                     struct hci_conn *conn)
{
  struct hci_conn_hash *hash;

  if (hdev == NULL || conn == NULL)
    {
      return;
    }

  hash = &hdev->conn_hash;
  list_del(&conn->list);
  switch (conn->type)
    {
      case ACL_LINK:
        if (hash->acl_num > 0)
          {
            hash->acl_num--;
          }
        break;
      case LE_LINK:
        if (hash->le_num > 0)
          {
            hash->le_num--;
          }
        if (conn->role == HCI_ROLE_SLAVE && hash->le_num_peripheral > 0)
          {
            hash->le_num_peripheral--;
          }
        break;
      case SCO_LINK:
      case ESCO_LINK:
        if (hash->sco_num > 0)
          {
            hash->sco_num--;
          }
        break;
      case CIS_LINK:
        if (hash->cis_num > 0)
          {
            hash->cis_num--;
          }
        break;
      case BIS_LINK:
        if (hash->bis_num > 0)
          {
            hash->bis_num--;
          }
        break;
      case PA_LINK:
        if (hash->pa_num > 0)
          {
            hash->pa_num--;
          }
        break;
      default:
        break;
    }
}

static inline bool hci_conn_valid(struct hci_dev *hdev,
                                  struct hci_conn *conn)
{
  struct hci_conn *iter;

  if (hdev == NULL || conn == NULL)
    {
      return false;
    }

  list_for_each_entry(iter, &hdev->conn_hash.list, list)
    {
      if (iter == conn)
        {
          return true;
        }
    }

  return false;
}

static inline __u8 hci_conn_lookup_type(struct hci_dev *hdev,
                                        __u16 handle)
{
  struct hci_conn *conn;

  if (hdev == NULL)
    {
      return INVALID_LINK;
    }

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->handle == handle)
        {
          return conn->type;
        }
    }

  return INVALID_LINK;
}

typedef void (*hci_conn_func_t)(struct hci_conn *conn, void *data);

static inline void hci_conn_hash_list_state(struct hci_dev *hdev,
                                            hci_conn_func_t func,
                                            __u8 type, __u16 state,
                                            void *data)
{
  struct hci_conn *conn;

  if (hdev == NULL || func == NULL)
    {
      return;
    }

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == type && conn->state == state)
        {
          func(conn, data);
        }
    }
}

static inline void hci_conn_hash_list_flag(struct hci_dev *hdev,
                                           hci_conn_func_t func,
                                           __u8 type, __u8 flag,
                                           void *data)
{
  struct hci_conn *conn;

  if (hdev == NULL || func == NULL)
    {
      return;
    }

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == type && test_bit(flag, &conn->flags))
        {
          func(conn, data);
        }
    }
}

static inline struct hci_conn *hci_lookup_le_connect(struct hci_dev *hdev)
{
  struct hci_conn *conn;

  if (hdev == NULL)
    {
      return NULL;
    }

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == LE_LINK && conn->state == BT_CONNECT &&
          !test_bit(HCI_CONN_SCANNING, &conn->flags))
        {
          return conn;
        }
    }

  return NULL;
}

static inline bool hci_is_le_conn_scanning(struct hci_dev *hdev)
{
  struct hci_conn *conn;

  if (hdev == NULL)
    {
      return false;
    }

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == LE_LINK && test_bit(HCI_CONN_SCANNING,
                                            &conn->flags))
        {
          return true;
        }
    }

  return false;
}

static inline unsigned int hci_conn_count(struct hci_dev *hdev)
{
  if (hdev == NULL)
    {
      return 0;
    }

  return hdev->conn_hash.acl_num + hdev->conn_hash.sco_num +
         hdev->conn_hash.le_num + hdev->conn_hash.cis_num +
         hdev->conn_hash.bis_num + hdev->conn_hash.pa_num;
}

static inline struct hci_conn *
hci_conn_hash_lookup_role(struct hci_dev *hdev, __u8 type, __u8 role,
                          bdaddr_t *ba)
{
  struct hci_conn *conn;

  if (hdev == NULL || ba == NULL)
    {
      return NULL;
    }

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == type && conn->role == role &&
          !bacmp(&conn->dst, ba))
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *
hci_conn_hash_lookup_create_pa_sync(struct hci_dev *hdev)
{
  struct hci_conn *conn;

  if (hdev == NULL)
    {
      return NULL;
    }

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == PA_LINK &&
          test_bit(HCI_CONN_CREATE_PA_SYNC, &conn->flags))
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *
hci_conn_hash_lookup_big_sync_pend(struct hci_dev *hdev,
                                   __u8 handle, __u8 num_bis)
{
  struct hci_conn *conn;

  if (hdev == NULL)
    {
      return NULL;
    }

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == PA_LINK &&
          conn->iso_qos.bcast.big == handle && conn->num_bis == num_bis)
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *
hci_conn_hash_lookup_pa_sync_handle(struct hci_dev *hdev, __u16 sync_handle)
{
  struct hci_conn *conn;

  if (hdev == NULL)
    {
      return NULL;
    }

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == PA_LINK && conn->state != BT_LISTEN &&
          conn->sync_handle == sync_handle)
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *hci_conn_hash_lookup_handle(
                                      struct hci_dev *hdev, uint16_t handle)
{
  struct hci_conn *conn;

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->handle == handle)
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *hci_conn_hash_lookup_ba(struct hci_dev *hdev,
                                                       uint8_t type,
                                                       bdaddr_t *ba)
{
  struct hci_conn *conn;

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == type && !bacmp(&conn->dst, ba))
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *hci_conn_hash_lookup_le(struct hci_dev *hdev,
                                                       bdaddr_t *ba,
                                                       uint8_t ba_type)
{
  struct hci_conn *conn;

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == LE_LINK && conn->dst_type == ba_type &&
          !bacmp(&conn->dst, ba))
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *hci_conn_hash_lookup_cis(struct hci_dev *hdev,
                                                        bdaddr_t *ba,
                                                        uint8_t ba_type,
                                                        uint8_t cig,
                                                        uint8_t cis)
{
  struct hci_conn *conn;

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type != CIS_LINK ||
          conn->iso_qos.ucast.cig != cig ||
          conn->iso_qos.ucast.cis != cis)
        {
          continue;
        }

      if (ba == NULL || (conn->dst_type == ba_type &&
                         !bacmp(&conn->dst, ba)))
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *hci_conn_hash_lookup_cig(struct hci_dev *hdev,
                                                        uint8_t cig)
{
  struct hci_conn *conn;

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == CIS_LINK && conn->iso_qos.ucast.cig == cig)
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *hci_conn_hash_lookup_big(struct hci_dev *hdev,
                                                        uint8_t big)
{
  struct hci_conn *conn;

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == BIS_LINK && conn->iso_qos.bcast.big == big)
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *hci_conn_hash_lookup_big_state(
                                      struct hci_dev *hdev, uint8_t big,
                                      uint16_t state, uint8_t role)
{
  struct hci_conn *conn;

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == BIS_LINK && conn->iso_qos.bcast.big == big &&
          conn->state == state && conn->role == role)
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *hci_conn_hash_lookup_bis(struct hci_dev *hdev,
                                                        bdaddr_t *ba,
                                                        uint8_t bis)
{
  struct hci_conn *conn;

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == BIS_LINK && conn->iso_qos.bcast.bis == bis &&
          (ba == NULL || !bacmp(&conn->dst, ba)))
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *hci_conn_hash_lookup_per_adv_bis(
                                      struct hci_dev *hdev, bdaddr_t *ba,
                                      uint8_t big, uint8_t bis)
{
  struct hci_conn *conn;

  list_for_each_entry(conn, &hdev->conn_hash.list, list)
    {
      if (conn->type == BIS_LINK && conn->iso_qos.bcast.big == big &&
          conn->iso_qos.bcast.bis == bis &&
          (ba == NULL || !bacmp(&conn->dst, ba)))
        {
          return conn;
        }
    }

  return NULL;
}

static inline struct hci_conn *hci_conn_get(struct hci_conn *conn)
{
  if (conn != NULL)
    {
      atomic_inc(&conn->refcnt);
    }

  return conn;
}

static inline void hci_conn_put(struct hci_conn *conn)
{
  if (conn != NULL)
    {
      atomic_dec(&conn->refcnt);
    }
}

static inline struct hci_conn *hci_conn_hold(struct hci_conn *conn)
{
  return hci_conn_get(conn);
}

static inline void hci_conn_drop(struct hci_conn *conn)
{
  hci_conn_put(conn);
}

static inline void discovery_init(struct hci_dev *hdev)
{
  if (hdev != NULL)
    {
      INIT_LIST_HEAD(&hdev->discovery.all);
      INIT_LIST_HEAD(&hdev->discovery.unknown);
      INIT_LIST_HEAD(&hdev->discovery.resolve);
      hdev->discovery.state = DISCOVERY_STOPPED;
    }
}

static inline void hci_discovery_filter_clear(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline int inquiry_cache_empty(struct hci_dev *hdev)
{
  (void)hdev;
  return 1;
}

static inline long inquiry_cache_age(struct hci_dev *hdev)
{
  (void)hdev;
  return INQUIRY_CACHE_AGE_MAX + 1;
}

static inline long inquiry_entry_age(struct inquiry_entry *e)
{
  (void)e;
  return INQUIRY_ENTRY_AGE_MAX + 1;
}

static inline void hci_devcd_setup(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void hci_devcd_reset(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void hci_init_sysfs(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void hci_conn_init_sysfs(struct hci_conn *conn)
{
  (void)conn;
}

static inline void hci_conn_add_sysfs(struct hci_conn *conn)
{
  (void)conn;
}

static inline void hci_conn_del_sysfs(struct hci_conn *conn)
{
  (void)conn;
}

static inline void hci_dev_clear_volatile_flags(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline int hci_proto_connect_ind(struct hci_dev *hdev,
                                        bdaddr_t *bdaddr, __u8 type,
                                        hci_conn_flags_t *flags)
{
  (void)hdev;
  (void)bdaddr;
  (void)type;
  (void)flags;
  return HCI_PROTO_DEFER;
}

static inline int hci_proto_disconn_ind(struct hci_conn *conn)
{
  (void)conn;
  return HCI_ERROR_REMOTE_USER_TERM;
}

static inline void hci_connect_cfm(struct hci_conn *conn, __u8 status)
{
  if (conn != NULL && conn->connect_cfm_cb != NULL)
    {
      conn->connect_cfm_cb(conn, status);
    }
}

static inline void hci_disconn_cfm(struct hci_conn *conn, __u8 reason)
{
  if (conn != NULL && conn->disconn_cfm_cb != NULL)
    {
      conn->disconn_cfm_cb(conn, reason);
    }
}

static inline void hci_encrypt_cfm(struct hci_conn *conn, __u8 status)
{
  if (conn != NULL && conn->security_cfm_cb != NULL)
    {
      conn->security_cfm_cb(conn, status);
    }
}

static inline void hci_auth_cfm(struct hci_conn *conn, __u8 status)
{
  if (conn != NULL && conn->security_cfm_cb != NULL)
    {
      conn->security_cfm_cb(conn, status);
    }
}

static inline void hci_key_change_cfm(struct hci_conn *conn, __u8 status)
{
  if (conn != NULL)
    {
      (void)status;
    }
}

static inline void hci_role_switch_cfm(struct hci_conn *conn, __u8 status,
                                       __u8 role)
{
  if (conn != NULL)
    {
      conn->role = role;
      (void)status;
    }
}

static inline int hci_check_conn_params(u16 min, u16 max, u16 latency,
                                        u16 timeout)
{
  if (min > max || min < 0x0006 || max > 0x0c80 || timeout < 0x000a)
    {
      return -EINVAL;
    }

  (void)latency;
  return 0;
}

static inline int sco_recv_scodata(struct hci_dev *hdev, u16 handle,
                                   struct sk_buff *skb)
{
  (void)hdev;
  (void)handle;
  kfree_skb(skb);
  return 0;
}

int hci_recv_frame(struct hci_dev *hdev, struct sk_buff *skb);
int hci_remove_remote_oob_data(struct hci_dev *hdev, bdaddr_t *bdaddr,
                               uint8_t bdaddr_type);
int hci_set_adv_instance_data(struct hci_dev *hdev, uint8_t instance,
                              uint16_t adv_data_len, uint8_t *adv_data,
                              uint16_t scan_rsp_len,
                              uint8_t *scan_rsp_data);
int hci_abort_conn(struct hci_conn *conn, uint8_t reason);
struct hci_conn *hci_conn_add(struct hci_dev *hdev, int type, bdaddr_t *dst,
                              uint8_t role, uint16_t handle);
struct hci_conn *hci_conn_add_unset(struct hci_dev *hdev, int type,
                                    bdaddr_t *dst, uint8_t role);
void hci_conn_del(struct hci_conn *conn);
void hci_conn_hash_flush(struct hci_dev *hdev);
void hci_send_to_sock(struct hci_dev *hdev, struct sk_buff *skb);
void hci_send_to_monitor(struct hci_dev *hdev, struct sk_buff *skb);
int linux_bt_upstream_hci_sock_recv(struct hci_dev *hdev, uint8_t pkt_type,
                                    const void *payload,
                                    size_t payload_len);
int hci_suspend_dev(struct hci_dev *hdev);
int hci_resume_dev(struct hci_dev *hdev);
int hci_dev_open(__u16 dev);
int hci_dev_do_close(struct hci_dev *hdev);
void hci_set_msft_opcode(struct hci_dev *hdev, __u16 opcode);
void hci_set_aosp_capable(struct hci_dev *hdev);
void hci_devcd_register(struct hci_dev *hdev,
                        void (*dump)(struct hci_dev *hdev),
                        void (*dump_hdr)(struct hci_dev *hdev,
                                         struct sk_buff *skb),
                        void *data);
int hci_devcd_init(struct hci_dev *hdev, unsigned int len);
void hci_devcd_append(struct hci_dev *hdev, struct sk_buff *skb);
void hci_devcd_complete(struct hci_dev *hdev);
void hci_devcd_abort(struct hci_dev *hdev);

#define HCI_MGMT_VAR_LEN       BIT(0)
#define HCI_MGMT_NO_HDEV       BIT(1)
#define HCI_MGMT_UNTRUSTED     BIT(2)
#define HCI_MGMT_UNCONFIGURED  BIT(3)
#define HCI_MGMT_HDEV_OPTIONAL BIT(4)

struct hci_mgmt_handler
{
  int (*func)(struct sock *sk, struct hci_dev *hdev, void *data,
              uint16_t data_len);
  size_t data_len;
  unsigned long flags;
};

struct hci_mgmt_chan
{
  struct list_head list;
  unsigned short channel;
  size_t handler_count;
  const struct hci_mgmt_handler *handlers;
  void (*hdev_init)(struct sock *sk, struct hci_dev *hdev);
};

int hci_mgmt_chan_register(struct hci_mgmt_chan *c);
void hci_mgmt_chan_unregister(struct hci_mgmt_chan *c);

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_CORE_H */
