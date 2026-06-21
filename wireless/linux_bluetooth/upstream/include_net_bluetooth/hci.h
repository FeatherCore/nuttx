#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_H

#include <linux/types.h>

#include <sys/ioctl.h>

#include <net/bluetooth/bluetooth.h>

#define HCI_MAX_DEV          16
#define HCI_MAX_NAME_LENGTH  248
#define HCI_MAX_SHORT_NAME_LENGTH 10
#define HCI_MAX_AD_LENGTH    31
#define HCI_MAX_EXT_AD_LENGTH 251
#define HCI_MAX_PER_AD_LENGTH 252
#define HCI_MAX_ACL_SIZE     2048
#define HCI_MAX_FRAME_SIZE   (HCI_MAX_ACL_SIZE + 4)
#define HCI_MAX_ISO_BIS      31
#define HCI_MAX_PER_AD_TOT_LEN 1650
#define HCI_LINK_KEY_SIZE    16
#define HCI_MAX_EIR_LENGTH   240

#define HCI_COMMAND_PKT      0x01
#define HCI_ACLDATA_PKT      0x02
#define HCI_SCODATA_PKT      0x03
#define HCI_EVENT_PKT        0x04
#define HCI_ISODATA_PKT      0x05
#define HCI_DIAG_PKT         0xf0
#define HCI_DRV_PKT         0xf1
#define HCI_VENDOR_PKT       0xff

#define SCO_LINK             0x00
#define ACL_LINK             0x01
#define ESCO_LINK            0x02
#define LE_LINK              0x80
#define CIS_LINK             0x82
#define BIS_LINK             0x83
#define PA_LINK              0x84

#define HCI_VIRTUAL          0

#define ADDR_LE_DEV_PUBLIC   0x00
#define ADDR_LE_DEV_RANDOM   0x01

enum
{
  HCI_UP,
  HCI_INIT,
  HCI_RUNNING,
  HCI_PSCAN,
  HCI_ISCAN,
  HCI_AUTH,
  HCI_ENCRYPT,
  HCI_INQUIRY,
  HCI_RAW,
  HCI_RESET,
};

enum
{
  HCI_SETUP,
  HCI_CONFIG,
  HCI_DEBUGFS_CREATED,
  HCI_POWERING_DOWN,
  HCI_AUTO_OFF,
  HCI_RFKILLED,
  HCI_MGMT,
  HCI_BONDABLE,
  HCI_SERVICE_CACHE,
  HCI_KEEP_DEBUG_KEYS,
  HCI_USE_DEBUG_KEYS,
  HCI_UNREGISTER,
  HCI_UNCONFIGURED,
  HCI_USER_CHANNEL,
  HCI_EXT_CONFIGURED,
  HCI_LE_ADV,
  HCI_LE_ADV_0,
  HCI_LE_PER_ADV,
  HCI_LE_SCAN,
  HCI_SSP_ENABLED,
  HCI_SC_ENABLED,
  HCI_SC_ONLY,
  HCI_PRIVACY,
  HCI_LIMITED_PRIVACY,
  HCI_RPA_EXPIRED,
  HCI_RPA_RESOLVING,
  HCI_LE_ENABLED,
  HCI_ADVERTISING,
  HCI_ADVERTISING_CONNECTABLE,
  HCI_CONNECTABLE,
  HCI_DISCOVERABLE,
  HCI_LIMITED_DISCOVERABLE,
  HCI_LINK_SECURITY,
  HCI_PERIODIC_INQ,
  HCI_FAST_CONNECTABLE,
  HCI_BREDR_ENABLED,
  HCI_LE_SCAN_INTERRUPTED,
  HCI_WIDEBAND_SPEECH_ENABLED,
  HCI_EVENT_FILTER_CONFIGURED,
  HCI_PA_SYNC,
  HCI_SCO_FLOWCTL,
  HCI_DUT_MODE,
  HCI_VENDOR_DIAG,
  HCI_FORCE_BREDR_SMP,
  HCI_FORCE_STATIC_ADDR,
  HCI_LL_RPA_RESOLUTION,
  HCI_CMD_PENDING,
  HCI_FORCE_NO_MITM,
  HCI_QUALITY_REPORT,
  HCI_OFFLOAD_CODECS_ENABLED,
  HCI_LE_SIMULTANEOUS_ROLES,
  HCI_CMD_DRAIN_WORKQUEUE,
  HCI_MESH_EXPERIMENTAL,
  HCI_MESH,
  HCI_MESH_SENDING,
  __HCI_NUM_FLAGS,
};

#define HCI_LM_MASTER        0x0001
#define HCI_LM_AUTH          0x0002
#define HCI_LM_ENCRYPT       0x0004
#define HCI_LM_TRUSTED       0x0008
#define HCI_LM_RELIABLE      0x0010
#define HCI_LM_SECURE        0x0020
#define HCI_LM_FIPS          0x0040
#define HCI_LM_ACCEPT        0x8000

#define HCI_CMD_TIMEOUT      msecs_to_jiffies(2000)
#define HCI_AUTO_OFF_TIMEOUT msecs_to_jiffies(2000)
#define HCI_ACL_TX_TIMEOUT   msecs_to_jiffies(45000)

#define HCI_DM1              0x0008
#define HCI_DH1              0x0010
#define HCI_DM3              0x0400
#define HCI_DM5              0x4000
#define HCI_DH3              0x0800
#define HCI_DH5              0x8000
#define HCI_HV1              0x0020
#define HCI_HV2              0x0040
#define HCI_HV3              0x0080
#define HCI_2DH1             0x0002
#define HCI_3DH1             0x0004
#define HCI_2DH3             0x0100
#define HCI_3DH3             0x0200
#define HCI_2DH5             0x1000
#define HCI_3DH5             0x2000

#define ESCO_HV1             0x0001
#define ESCO_HV2             0x0002
#define ESCO_HV3             0x0004
#define ESCO_EV3             0x0008
#define ESCO_EV4             0x0010
#define ESCO_EV5             0x0020
#define ESCO_2EV3            0x0040
#define ESCO_3EV3            0x0080
#define ESCO_2EV5            0x0100
#define ESCO_3EV5            0x0200

#define ACL_PTYPE_MASK       (HCI_DM1 | HCI_DH1 | HCI_DM3 | HCI_DH3 | \
                              HCI_DM5 | HCI_DH5)
#define SCO_ESCO_MASK        (ESCO_HV1 | ESCO_HV2 | ESCO_HV3 | ESCO_EV3 | \
                              ESCO_EV4 | ESCO_EV5 | ESCO_2EV3 | \
                              ESCO_3EV3 | ESCO_2EV5 | ESCO_3EV5)
#define EDR_ESCO_MASK        (ESCO_2EV3 | ESCO_3EV3 | ESCO_2EV5 | ESCO_3EV5)
#define SCO_PTYPE_MASK       SCO_ESCO_MASK
#define SCO_AIRMODE_MASK     0x0003
#define SCO_AIRMODE_CVSD     0x0000
#define SCO_AIRMODE_TRANSP   0x0003

#define HCI_CM_ACTIVE        0x0000
#define HCI_CM_SNIFF         0x0002
#define HCI_LP_SNIFF         0x0004

#define HCI_LK_COMBINATION              0x00
#define HCI_LK_LOCAL_UNIT               0x01
#define HCI_LK_REMOTE_UNIT              0x02
#define HCI_LK_DEBUG_COMBINATION        0x03
#define HCI_LK_UNAUTH_COMBINATION_P192  0x04
#define HCI_LK_AUTH_COMBINATION_P192    0x05
#define HCI_LK_CHANGED_COMBINATION      0x06
#define HCI_LK_UNAUTH_COMBINATION_P256  0x07
#define HCI_LK_AUTH_COMBINATION_P256    0x08

#define HCI_ERROR_LOCAL_HOST_TERM 0x16
#define HCI_ERROR_UNKNOWN_CONN_ID 0x02
#define HCI_ERROR_REMOTE_USER_TERM 0x13
#define HCI_ERROR_INVALID_PARAMETERS 0x12
#define HCI_ERROR_ADVERTISING_TIMEOUT 0x3c
#define HCI_RSSI_INVALID 127
#define HCI_ROLE_MASTER 0x00
#define HCI_ROLE_SLAVE  0x01
#define HCI_AT_NO_BONDING              0x00
#define HCI_AT_NO_BONDING_MITM         0x01
#define HCI_AT_DEDICATED_BONDING       0x02
#define HCI_AT_DEDICATED_BONDING_MITM  0x03
#define HCI_AT_GENERAL_BONDING         0x04
#define HCI_AT_GENERAL_BONDING_MITM    0x05
#define HCI_IO_DISPLAY_ONLY            0x00
#define HCI_IO_DISPLAY_YESNO           0x01
#define HCI_IO_KEYBOARD_ONLY           0x02
#define HCI_IO_NO_INPUT_OUTPUT         0x03
#define LMP_HOST_SSP                   0x01
#define LMP_HOST_LE                    0x02
#define LMP_HOST_LE_BREDR              0x04
#define LMP_HOST_SC                    0x08
#define LMP_3SLOT                      0x01
#define LMP_5SLOT                      0x02
#define LMP_HV2                        0x10
#define LMP_HV3                        0x20
#define LMP_EV4                        0x01
#define LMP_EV5                        0x02
#define LMP_EDR_ESCO_2M                0x20
#define LMP_EDR_ESCO_3M                0x40
#define LMP_EDR_3S_ESCO                0x80
#define HCI_ERROR_AUTH_FAILURE         0x05
#define HCI_ERROR_REJ_BAD_ADDR         0x0f
#define HCI_ERROR_COMMAND_DISALLOWED   0x0c
#define HCI_ERROR_UNSPECIFIED          0x1f
#define HCI_TX_POWER_INVALID 127
#define HCI_SYNC_HANDLE_INVALID 0xffff
#define HCI_MIN_LE_MTU 0x001b
#define HCI_DISCONN_TIMEOUT msecs_to_jiffies(2000)

#define HCI_OP_NOP                  0x0000
#define HCI_OP_RESET                0x0c03
#define HCI_OP_CHANGE_CONN_PTYPE    0x040f
struct hci_cp_change_conn_ptype {
	__le16   handle;
	__le16   pkt_type;
} __packed;

#define HCI_OP_AUTH_REQUESTED       0x0411
#define HCI_OP_SET_CONN_ENCRYPT     0x0413
#define HCI_OP_READ_CLOCK_OFFSET    0x041f
#define HCI_OP_ADD_SCO              0x0407
#define HCI_OP_REMOTE_NAME_REQ_CANCEL 0x041a
#define HCI_OP_SWITCH_ROLE          0x080b
#define HCI_OP_ROLE_DISCOVERY       0x0809
#define HCI_OP_READ_LINK_POLICY     0x080c
#define HCI_OP_WRITE_LINK_POLICY    0x080d
#define HCI_OP_READ_DEF_LINK_POLICY 0x080e
#define HCI_OP_EXIT_SNIFF_MODE      0x0804
#define HCI_OP_SNIFF_MODE           0x0803
#define HCI_OP_SNIFF_SUBRATE        0x0811
#define HCI_OP_SETUP_SYNC_CONN      0x0428
#define HCI_OP_ENHANCED_SETUP_SYNC_CONN 0x043d
#define HCI_OP_SET_EVENT_FLT        0x0c05
#define HCI_OP_READ_STORED_LINK_KEY 0x0c0d
#define HCI_OP_DELETE_STORED_LINK_KEY 0x0c12
#define HCI_OP_WRITE_LOCAL_NAME     0x0c13
#define HCI_OP_READ_LOCAL_NAME      0x0c14
#define HCI_OP_WRITE_DEF_LINK_POLICY 0x080f
#define HCI_OP_WRITE_SCAN_ENABLE    0x0c1a
#define HCI_OP_WRITE_AUTH_ENABLE    0x0c20
#define HCI_OP_WRITE_ENCRYPT_MODE   0x0c22
#define HCI_OP_READ_CLASS_OF_DEV    0x0c23
#define HCI_OP_WRITE_CLASS_OF_DEV   0x0c24
#define HCI_OP_READ_VOICE_SETTING   0x0c25
#define HCI_OP_WRITE_VOICE_SETTING  0x0c26
#define HCI_OP_READ_PAGE_SCAN_ACTIVITY 0x0c1b
#define HCI_OP_WRITE_PAGE_SCAN_ACTIVITY 0x0c1c
#define HCI_OP_READ_PAGE_SCAN_TYPE  0x0c46
#define HCI_OP_WRITE_PAGE_SCAN_TYPE 0x0c47
#define HCI_OP_READ_NUM_SUPPORTED_IAC 0x0c38
#define HCI_OP_WRITE_SSP_MODE       0x0c56
#define HCI_OP_WRITE_SC_SUPPORT     0x0c7a
#define HCI_OP_READ_AUTH_PAYLOAD_TO 0x0c7b
#define HCI_OP_WRITE_AUTH_PAYLOAD_TO 0x0c7c
#define HCI_OP_READ_LOCAL_VERSION   0x1001
#define HCI_OP_READ_LOCAL_COMMANDS  0x1002
#define HCI_OP_READ_LOCAL_FEATURES  0x1003
#define HCI_OP_READ_LOCAL_EXT_FEATURES 0x1004
#define HCI_OP_READ_BUFFER_SIZE     0x1005
#define HCI_OP_READ_BD_ADDR         0x1009
#define HCI_OP_READ_LOCAL_PAIRING_OPTS 0x100c
#define HCI_OP_READ_ENC_KEY_SIZE    0x1408
#define HCI_OP_LE_ACCEPT_CIS        0x2066
#define HCI_OP_LE_REJECT_CIS        0x2067
#define HCI_OP_LE_BIG_CREATE_SYNC   0x206b
#define HCI_OP_LE_BIG_TERM_SYNC     0x206c
#define HCI_OP_USER_CONFIRM_REPLY   0x042c
#define HCI_OP_LE_SET_ADV_ENABLE    0x200a
#define HCI_OP_LE_CONN_UPDATE       0x2013
#define HCI_OP_LE_START_ENC         0x2019
#define HCI_OP_LE_SET_EXT_ADV_ENABLE 0x2039
#define HCI_OP_LE_SET_CIG_PARAMS    0x2062
#define HCI_OP_LE_CREATE_BIG        0x2068
#define HCI_OP_LE_SETUP_ISO_PATH    0x206e
#define HCI_CONFIGURE_DATA_PATH     0x0c83

#define HCI_EV_CONN_COMPLETE              0x03
#define HCI_EV_INQUIRY_COMPLETE          0x01
#define HCI_EV_INQUIRY_RESULT            0x02
#define HCI_EV_CONN_REQUEST              0x04
#define HCI_EV_DISCONN_COMPLETE          0x05
#define HCI_EV_AUTH_COMPLETE             0x06
#define HCI_EV_REMOTE_NAME               0x07
#define HCI_EV_ENCRYPT_CHANGE            0x08
#define HCI_EV_CHANGE_LINK_KEY_COMPLETE  0x09
#define HCI_EV_REMOTE_FEATURES           0x0b
#define HCI_EV_REMOTE_VERSION            0x0c
#define HCI_EV_HARDWARE_ERROR             0x10
#define HCI_EV_ROLE_CHANGE               0x12
#define HCI_EV_NUM_COMP_PKTS             0x13
#define HCI_EV_MODE_CHANGE               0x14
#define HCI_EV_PIN_CODE_REQ              0x16
#define HCI_EV_LINK_KEY_REQ              0x17
#define HCI_EV_LINK_KEY_NOTIFY           0x18
#define HCI_EV_CLOCK_OFFSET              0x1c
#define HCI_EV_PKT_TYPE_CHANGE           0x1d
#define HCI_EV_PSCAN_REP_MODE            0x20
#define HCI_EV_INQUIRY_RESULT_WITH_RSSI  0x22
#define HCI_EV_REMOTE_EXT_FEATURES       0x23
#define HCI_EV_SYNC_CONN_COMPLETE        0x2c
#define HCI_EV_EXTENDED_INQUIRY_RESULT   0x2f
#define HCI_EV_KEY_REFRESH_COMPLETE      0x30
#define HCI_EV_IO_CAPA_REQUEST           0x31
#define HCI_EV_IO_CAPA_REPLY             0x32
#define HCI_EV_USER_CONFIRM_REQUEST      0x33
#define HCI_EV_USER_PASSKEY_REQUEST      0x34
#define HCI_EV_REMOTE_OOB_DATA_REQUEST   0x35
#define HCI_EV_SIMPLE_PAIR_COMPLETE      0x36
#define HCI_EV_USER_PASSKEY_NOTIFY       0x3b
#define HCI_EV_KEYPRESS_NOTIFY           0x3c
#define HCI_EV_REMOTE_HOST_FEATURES      0x3d
#define HCI_EV_LE_META                    0x3e
#define HCI_EV_VENDOR                    0xff
#define HCI_EV_LE_CONN_COMPLETE           0x01
#define HCI_EV_LE_ENHANCED_CONN_COMPLETE  0x0a
#define HCI_EVT_LE_CIS_ESTABLISHED        0x19

#define HCI_FLT_CLEAR_ALL                 0x00
#define HCI_FLT_INQ_RESULT                0x01
#define HCI_FLT_CONN_SETUP                0x02
#define AUTH_DISABLED                     0x00
#define AUTH_ENABLED                      0x01
#define ENCRYPT_DISABLED                  0x00
#define ENCRYPT_P2P                       0x01
#define ENCRYPT_BOTH                      0x02
#define LE_SCAN_PASSIVE                   0x00
#define LE_SCAN_ACTIVE                    0x01

#define HCI_LE_SET_PHY_1M    0x01
#define HCI_LE_SET_PHY_2M    0x02
#define HCI_LE_SET_PHY_CODED 0x04
#define HCI_OP_LE_SET_PHY    0x2032
struct hci_cp_le_set_phy {
	__le16  handle;
	__u8    all_phys;
	__u8    tx_phys;
	__u8    rx_phys;
	__le16  phy_opts;
} __packed;

#define HCI_REQ_PEND 1
#define HCI_REQ_START 2
#define HCI_CMD_PENDING 3
#define HCI_REQ_SKB 4

#define IREQ_CACHE_FLUSH 0x0001

#define HCI_SID_INVALID     0xff
#define HCI_LE_CONN_TIMEOUT msecs_to_jiffies(20000)

#define HCI_FLT_TYPE_BITS    31
#define HCI_FLT_EVENT_BITS   63
#define HCI_EV_CMD_COMPLETE  0x0e
#define HCI_EV_CMD_STATUS    0x0f

#define HCIDEVUP       0x400448c9
#define HCIDEVDOWN     0x400448ca
#define HCIDEVRESET    0x400448cb
#define HCIDEVRESTAT   0x400448cc
#define HCIGETDEVLIST  0x800448d2
#define HCIGETDEVINFO  0x800448d3
#define HCIGETCONNLIST 0x800448d4
#define HCIGETCONNINFO 0x800448d5
#define HCIGETAUTHINFO 0x800448d7
#define HCISETRAW      0x400448dc
#define HCISETSCAN     0x400448dd
#define HCISETAUTH     0x400448de
#define HCISETENCRYPT  0x400448df
#define HCISETPTYPE    0x400448e0
#define HCISETLINKPOL  0x400448e1
#define HCISETLINKMODE 0x400448e2
#define HCISETACLMTU   0x400448e3
#define HCISETSCOMTU   0x400448e4
#define HCIBLOCKADDR   0x400448e6
#define HCIUNBLOCKADDR 0x400448e7

#define SCAN_DISABLED  0x00
#define SCAN_INQUIRY   0x01
#define SCAN_PAGE      0x02

#define HCI_DATA_DIR   1
#define HCI_FILTER     2
#define HCI_TIME_STAMP 3

#define HCI_CMSG_DIR    0x0001
#define HCI_CMSG_TSTAMP 0x0002

#define LINUX_BT_HCI_DEV_STATS_DEFINED 1
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
  unsigned long err_tx;
  unsigned long evt_rx;
};

struct sockaddr_hci
{
  sa_family_t hci_family;
  unsigned short hci_dev;
  unsigned short hci_channel;
};

#define HCI_DEV_NONE        0xffff
#define HCI_CHANNEL_RAW     0
#define HCI_CHANNEL_USER    1
#define HCI_CHANNEL_MONITOR 2
#define HCI_CHANNEL_CONTROL 3
#define HCI_CHANNEL_LOGGING 4

struct hci_filter
{
  uint32_t type_mask;
  uint32_t event_mask[2];
  uint16_t opcode;
};

struct hci_dev_info
{
  uint16_t dev_id;
  char name[16];
  bdaddr_t bdaddr;
  uint32_t flags;
  uint8_t type;
  uint8_t features[8];
  uint32_t pkt_type;
  uint32_t link_policy;
  uint32_t link_mode;
  uint16_t acl_mtu;
  uint16_t acl_pkts;
  uint16_t sco_mtu;
  uint16_t sco_pkts;
  struct hci_dev_stats stat;
};

struct hci_conn_info
{
  uint16_t handle;
  bdaddr_t bdaddr;
  uint8_t type;
  uint8_t out;
  uint16_t state;
  uint32_t link_mode;
};

struct hci_dev_req
{
  uint16_t dev_id;
  uint32_t dev_opt;
};

struct hci_dev_list_req
{
  uint16_t dev_num;
  struct hci_dev_req dev_req[];
};

struct hci_conn_list_req
{
  uint16_t dev_id;
  uint16_t conn_num;
  struct hci_conn_info conn_info[];
};

struct hci_conn_info_req
{
  bdaddr_t bdaddr;
  uint8_t type;
  struct hci_conn_info conn_info[];
};

struct hci_auth_info_req
{
  bdaddr_t bdaddr;
  uint8_t type;
};

struct inquiry_info {
	bdaddr_t bdaddr;
	__u8     pscan_rep_mode;
	__u8     pscan_period_mode;
	__u8     pscan_mode;
	__u8     dev_class[3];
	__le16   clock_offset;
} __packed;

struct hci_inquiry_req
{
  __le16 dev_id;
  __le16 flags;
  uint8_t lap[3];
  uint8_t length;
  uint8_t num_rsp;
} __packed;

struct hci_command_hdr {
	__le16	opcode;		/* OCF & OGF */
	__u8	plen;
} __packed;

struct hci_event_hdr {
	__u8	evt;
	__u8	plen;
} __packed;

struct hci_ev_le_meta {
	__u8     subevent;
} __packed;

struct hci_acl_hdr {
	__le16	handle;		/* Handle & Flags(PB, BC) */
	__le16	dlen;
} __packed;

struct hci_sco_hdr {
	__le16	handle;
	__u8	dlen;
} __packed;

struct hci_iso_hdr {
	__le16	handle;
	__le16	dlen;
	__u8	data[];
} __packed;

#define HCI_COMMAND_HDR_SIZE sizeof(struct hci_command_hdr)
#define HCI_EVENT_HDR_SIZE sizeof(struct hci_event_hdr)
#define HCI_ACL_HDR_SIZE sizeof(struct hci_acl_hdr)
#define HCI_SCO_HDR_SIZE sizeof(struct hci_sco_hdr)
#define HCI_ISO_HDR_SIZE sizeof(struct hci_iso_hdr)

#define hci_command_hdr(skb) ((struct hci_command_hdr *)((skb)->data))
#define hci_event_hdr(skb) ((struct hci_event_hdr *)((skb)->data))
#define hci_acl_hdr(skb) ((struct hci_acl_hdr *)((skb)->data))
#define hci_sco_hdr(skb) ((struct hci_sco_hdr *)((skb)->data))
#define hci_iso_hdr(skb) ((struct hci_iso_hdr *)((skb)->data))
#define hci_handle(h) ((uint16_t)((h) & 0x0fff))
#define hci_flags(h) ((uint16_t)((h) >> 12))
#define hci_handle_pack(h, f) ((uint16_t)((h) | ((f) << 12)))
#define hci_iso_flags_pack(pb, ts) ((uint16_t)(((pb) << 12) | ((ts) << 14)))

#define ACL_CONT  0x01
#define ACL_START 0x02
#define ISO_START 0x00
#define ISO_CONT  0x01
#define ISO_SINGLE 0x02
#define ISO_END   0x03

static inline void hci_cpu_to_le24(uint32_t value, uint8_t dst[3])
{
  dst[0] = value & 0xff;
  dst[1] = (value >> 8) & 0xff;
  dst[2] = (value >> 16) & 0xff;
}

struct hci_cp_auth_requested {
	__le16   handle;
} __packed;

struct hci_cp_set_conn_encrypt {
	__le16   handle;
	__u8     encrypt;
} __packed;

struct hci_cp_switch_role {
	bdaddr_t bdaddr;
	__u8     role;
} __packed;

struct hci_cp_exit_sniff_mode {
	__le16   handle;
} __packed;

struct hci_cp_read_clock_offset {
	__le16   handle;
} __packed;

struct hci_cp_add_sco {
	__le16   handle;
	__le16   pkt_type;
} __packed;

struct hci_cp_setup_sync_conn {
	__le16   handle;
	__le32   tx_bandwidth;
	__le32   rx_bandwidth;
	__le16   max_latency;
	__le16   voice_setting;
	__u8     retrans_effort;
	__le16   pkt_type;
} __packed;

struct hci_coding_format {
	__u8	id;
	__le16	cid;
	__le16	vid;
} __packed;

struct hci_cp_enhanced_setup_sync_conn {
	__le16   handle;
	__le32   tx_bandwidth;
	__le32   rx_bandwidth;
	struct	 hci_coding_format tx_coding_format;
	struct	 hci_coding_format rx_coding_format;
	__le16	 tx_codec_frame_size;
	__le16	 rx_codec_frame_size;
	__le32	 in_bandwidth;
	__le32	 out_bandwidth;
	struct	 hci_coding_format in_coding_format;
	struct	 hci_coding_format out_coding_format;
	__le16   in_coded_data_size;
	__le16	 out_coded_data_size;
	__u8	 in_pcm_data_format;
	__u8	 out_pcm_data_format;
	__u8	 in_pcm_sample_payload_msb_pos;
	__u8	 out_pcm_sample_payload_msb_pos;
	__u8	 in_data_path;
	__u8	 out_data_path;
	__u8	 in_transport_unit_size;
	__u8	 out_transport_unit_size;
	__le16   max_latency;
	__le16   pkt_type;
	__u8     retrans_effort;
} __packed;

struct hci_op_configure_data_path {
	__u8	direction;
	__u8	data_path_id;
	__u8	vnd_len;
	__u8	vnd_data[];
} __packed;

struct hci_cp_le_conn_update {
	__le16   handle;
	__le16   conn_interval_min;
	__le16   conn_interval_max;
	__le16   conn_latency;
	__le16   supervision_timeout;
	__le16   min_ce_len;
	__le16   max_ce_len;
} __packed;

struct hci_cp_le_start_enc {
	__le16	handle;
	__le64	rand;
	__le16	ediv;
	__u8	ltk[16];
} __packed;

struct hci_cp_sniff_subrate {
	__le16   handle;
	__le16   max_latency;
	__le16   min_remote_timeout;
	__le16   min_local_timeout;
} __packed;

struct hci_cp_sniff_mode {
	__le16   handle;
	__le16   max_interval;
	__le16   min_interval;
	__le16   attempt;
	__le16   timeout;
} __packed;

struct hci_cp_le_set_ext_adv_enable {
	__u8  enable;
	__u8  num_of_sets;
	__u8  data[];
} __packed;

struct hci_cis_params {
	__u8    cis_id;
	__le16  c_sdu;
	__le16  p_sdu;
	__u8    c_phy;
	__u8    p_phy;
	__u8    c_rtn;
	__u8    p_rtn;
} __packed;

struct hci_cp_le_set_cig_params {
	__u8    cig_id;
	__u8    c_interval[3];
	__u8    p_interval[3];
	__u8    sca;
	__u8    packing;
	__u8    framing;
	__le16  c_latency;
	__le16  p_latency;
	__u8    num_cis;
	struct hci_cis_params cis[] __counted_by(num_cis);
} __packed;

struct hci_bis_params
{
  uint8_t sdu_interval[3];
  __le16 sdu;
  __le16 latency;
  uint8_t rtn;
  uint8_t phy;
  uint8_t packing;
  uint8_t framing;
  uint8_t encryption;
  uint8_t bcode[16];
} __packed;

struct hci_cp_le_create_big {
	__u8    handle;
	__u8    adv_handle;
	__u8    num_bis;
	struct hci_bis bis;
} __packed;

struct hci_cp_le_setup_iso_path {
	__le16  handle;
	__u8    direction;
	__u8    path;
	__u8    codec;
	__le16  codec_cid;
	__le16  codec_vid;
	__u8    delay[3];
	__u8    codec_cfg_len;
	__u8    codec_cfg[];
} __packed;

struct hci_rp_le_set_cig_params {
	__u8    status;
	__u8    cig_id;
	__u8    num_handles;
	__le16  handle[];
} __packed;

struct hci_rp_le_setup_iso_path {
	__u8    status;
	__le16  handle;
} __packed;

struct hci_evt_le_create_big_complete {
	__u8    status;
	__u8    handle;
	__u8    sync_delay[3];
	__u8    transport_delay[3];
	__u8    phy;
	__u8    nse;
	__u8    bn;
	__u8    pto;
	__u8    irc;
	__le16  max_pdu;
	__le16  interval;
	__u8    num_bis;
	__le16  bis_handle[];
} __packed;

#define HCI_EV_LE_ADVERTISING_REPORT       0x02
#define HCI_EV_LE_CONN_UPDATE_COMPLETE     0x03
#define HCI_EV_LE_REMOTE_FEAT_COMPLETE     0x04
#define HCI_EV_LE_LTK_REQ                  0x05
#define HCI_EV_LE_REMOTE_CONN_PARAM_REQ    0x06
#define HCI_EV_LE_DIRECT_ADV_REPORT        0x0b
#define HCI_EV_LE_PHY_UPDATE_COMPLETE      0x0c
#define HCI_EV_LE_EXT_ADV_REPORT           0x0d
#define HCI_EV_LE_PA_SYNC_ESTABLISHED      0x0e
#define HCI_EV_LE_PER_ADV_REPORT           0x0f
#define HCI_EV_LE_PA_SYNC_LOST             0x10
#define HCI_EV_LE_EXT_ADV_SET_TERM         0x12
#define HCI_EVT_LE_CIS_REQ                 0x1a
#define HCI_EVT_LE_CREATE_BIG_COMPLETE     0x1b
#define HCI_EVT_LE_BIG_SYNC_ESTABLISHED    0x1d
#define HCI_EVT_LE_BIG_SYNC_LOST           0x1e
#define HCI_EVT_LE_BIG_INFO_ADV_REPORT     0x22

struct hci_ev_le_conn_complete {
	__u8     status;
	__le16   handle;
	__u8     role;
	__u8     bdaddr_type;
	bdaddr_t bdaddr;
	__le16   interval;
	__le16   latency;
	__le16   supervision_timeout;
	__u8     clk_accurancy;
} __packed;

struct hci_ev_le_advertising_info {
	__u8	 type;
	__u8	 bdaddr_type;
	bdaddr_t bdaddr;
	__u8	 length;
	__u8	 data[];
} __packed;

struct hci_ev_le_advertising_report {
	__u8    num;
	struct hci_ev_le_advertising_info info[];
} __packed;

struct hci_ev_le_conn_update_complete {
	__u8     status;
	__le16   handle;
	__le16   interval;
	__le16   latency;
	__le16   supervision_timeout;
} __packed;

struct hci_ev_le_remote_feat_complete {
	__u8     status;
	__le16   handle;
	__u8     features[8];
} __packed;

struct hci_ev_le_ltk_req {
	__le16	handle;
	__le64	rand;
	__le16	ediv;
} __packed;

struct hci_ev_le_remote_conn_param_req {
	__le16 handle;
	__le16 interval_min;
	__le16 interval_max;
	__le16 latency;
	__le16 timeout;
} __packed;

struct hci_ev_le_direct_adv_info {
	__u8	 type;
	__u8	 bdaddr_type;
	bdaddr_t bdaddr;
	__u8	 direct_addr_type;
	bdaddr_t direct_addr;
	__s8	 rssi;
} __packed;

struct hci_ev_le_direct_adv_report {
	__u8	 num;
	struct hci_ev_le_direct_adv_info info[];
} __packed;

struct hci_ev_le_phy_update_complete {
	__u8  status;
	__le16 handle;
	__u8  tx_phy;
	__u8  rx_phy;
} __packed;

struct hci_ev_le_ext_adv_info {
	__le16   type;
	__u8	 bdaddr_type;
	bdaddr_t bdaddr;
	__u8	 primary_phy;
	__u8	 secondary_phy;
	__u8	 sid;
	__u8	 tx_power;
	__s8	 rssi;
	__le16   interval;
	__u8     direct_addr_type;
	bdaddr_t direct_addr;
	__u8     length;
	__u8     data[];
} __packed;

struct hci_ev_le_ext_adv_report {
	__u8     num;
	struct hci_ev_le_ext_adv_info info[];
} __packed;

struct hci_ev_le_pa_sync_established {
	__u8      status;
	__le16    handle;
	__u8      sid;
	__u8      bdaddr_type;
	bdaddr_t  bdaddr;
	__u8      phy;
	__le16    interval;
	__u8      clock_accuracy;
} __packed;

struct hci_ev_le_enh_conn_complete {
	__u8      status;
	__le16    handle;
	__u8      role;
	__u8      bdaddr_type;
	bdaddr_t  bdaddr;
	bdaddr_t  local_rpa;
	bdaddr_t  peer_rpa;
	__le16    interval;
	__le16    latency;
	__le16    supervision_timeout;
	__u8      clk_accurancy;
} __packed;

struct hci_ev_le_per_adv_report {
	__le16	 sync_handle;
	__u8	 tx_power;
	__u8	 rssi;
	__u8	 cte_type;
	__u8	 data_status;
	__u8     length;
	__u8     data[];
} __packed;

struct hci_ev_le_pa_sync_lost {
	__le16 handle;
} __packed;

struct hci_evt_le_ext_adv_set_term {
	__u8	status;
	__u8	handle;
	__le16	conn_handle;
	__u8	num_evts;
} __packed;

struct hci_evt_le_cis_established {
	__u8  status;
	__le16 handle;
	__u8  cig_sync_delay[3];
	__u8  cis_sync_delay[3];
	__u8  c_latency[3];
	__u8  p_latency[3];
	__u8  c_phy;
	__u8  p_phy;
	__u8  nse;
	__u8  c_bn;
	__u8  p_bn;
	__u8  c_ft;
	__u8  p_ft;
	__le16 c_mtu;
	__le16 p_mtu;
	__le16 interval;
} __packed;

struct hci_evt_le_cis_req {
	__le16 acl_handle;
	__le16 cis_handle;
	__u8  cig_id;
	__u8  cis_id;
} __packed;

struct hci_evt_le_big_sync_established {
	__u8    status;
	__u8    handle;
	__u8    latency[3];
	__u8    nse;
	__u8    bn;
	__u8    pto;
	__u8    irc;
	__le16  max_pdu;
	__le16  interval;
	__u8    num_bis;
	__le16  bis[];
} __packed;

struct hci_evt_le_big_sync_lost {
	__u8    handle;
	__u8    reason;
} __packed;

struct hci_evt_le_big_info_adv_report {
	__le16  sync_handle;
	__u8    num_bis;
	__u8    nse;
	__le16  iso_interval;
	__u8    bn;
	__u8    pto;
	__u8    irc;
	__le16  max_pdu;
	__u8    sdu_interval[3];
	__le16  max_sdu;
	__u8    phy;
	__u8    framing;
	__u8    encryption;
} __packed;

enum
{
  HCI_QUIRK_FIXUP_BUFFER_SIZE,
  HCI_QUIRK_NON_PERSISTENT_SETUP,
  HCI_QUIRK_USE_BDADDR_PROPERTY,
  HCI_QUIRK_INVALID_BDADDR,
  HCI_QUIRK_USE_DEBUG_KEYS,
  HCI_QUIRK_STRICT_DUPLICATE_FILTER,
  HCI_QUIRK_SIMULTANEOUS_DISCOVERY,
  HCI_QUIRK_NON_PERSISTENT_DIAG,
  HCI_QUIRK_USE_MSFT_EXT_ADDRESS_FILTER,
  HCI_QUIRK_BROKEN_EXT_SCAN,
  HCI_QUIRK_BROKEN_LOCAL_EXT_FEATURES_PAGE_2,
  HCI_QUIRK_BROKEN_READ_TRANSMIT_POWER,
  HCI_QUIRK_BROKEN_FILTER_CLEAR_ALL,
  HCI_QUIRK_BROKEN_ENHANCED_SETUP_SYNC_CONN,
  HCI_QUIRK_BROKEN_LE_CODED,
  HCI_QUIRK_BROKEN_EXT_CREATE_CONN,
  HCI_QUIRK_BROKEN_LE_READ_TRANSMIT_POWER,
  HCI_QUIRK_BROKEN_SET_RPA_TIMEOUT,
  HCI_QUIRK_BROKEN_EXT_ADV,
  HCI_QUIRK_BROKEN_ADV_MONITORING,
  HCI_QUIRK_VALID_LE_STATES,
  HCI_QUIRK_WIDEBAND_SPEECH_SUPPORTED,
  HCI_QUIRK_VALID_LE_STATES_WINDOW,
  HCI_QUIRK_BROKEN_MWS_TRANSPORT_CONFIG,
  HCI_QUIRK_BROKEN_ERR_DATA_REPORTING,
  HCI_QUIRK_BROKEN_READ_ENC_KEY_SIZE,
  HCI_QUIRK_BROKEN_LE_CONN_SCAN_PARAMS,
  HCI_QUIRK_BROKEN_READ_PAGE_SCAN_TYPE,
  HCI_QUIRK_BROKEN_READ_TRANSMIT_POWER_LEVEL,
  HCI_QUIRK_BROKEN_READ_ENHANCED_TX_POWER,
  HCI_QUIRK_BROKEN_SET_RPA_TIMEOUT_COMMAND,
  HCI_QUIRK_SYNC_FLOWCTL_SUPPORTED,
  HCI_QUIRK_EXTERNAL_CONFIG,
  HCI_QUIRK_NO_SUSPEND_NOTIFIER,
  HCI_QUIRK_RAW_DEVICE,
};

/* Linux HCI command return/event ABI used by upstream hci_event.c. */

struct hci_ev_status {
	__u8    status;
} __packed;

struct hci_rp_remote_name_req_cancel {
	__u8     status;
	bdaddr_t bdaddr;
} __packed;

struct hci_rp_role_discovery {
	__u8     status;
	__le16   handle;
	__u8     role;
} __packed;

struct hci_rp_read_link_policy {
	__u8     status;
	__le16   handle;
	__le16   policy;
} __packed;

struct hci_rp_write_link_policy {
	__u8     status;
	__le16   handle;
} __packed;

struct hci_rp_read_def_link_policy {
	__u8     status;
	__le16   policy;
} __packed;

struct hci_cp_read_stored_link_key {
	bdaddr_t bdaddr;
	__u8     read_all;
} __packed;

struct hci_rp_read_stored_link_key {
	__u8     status;
	__le16   max_keys;
	__le16   num_keys;
} __packed;

struct hci_cp_delete_stored_link_key {
	bdaddr_t bdaddr;
	__u8     delete_all;
} __packed;

struct hci_rp_delete_stored_link_key {
	__u8     status;
	__le16   num_keys;
} __packed;

struct hci_cp_write_local_name {
	__u8     name[HCI_MAX_NAME_LENGTH];
} __packed;

struct hci_rp_read_local_name {
	__u8     status;
	__u8     name[HCI_MAX_NAME_LENGTH];
} __packed;

struct hci_cp_set_event_filter {
	__u8		flt_type;
	__u8		cond_type;
	struct {
		bdaddr_t bdaddr;
		__u8 auto_accept;
	} __packed	addr_conn_flt;
} __packed;

struct hci_rp_read_class_of_dev {
	__u8     status;
	__u8     dev_class[3];
} __packed;

struct hci_cp_write_class_of_dev {
	__u8     dev_class[3];
} __packed;

struct hci_rp_read_voice_setting {
	__u8     status;
	__le16   voice_setting;
} __packed;

struct hci_cp_write_voice_setting {
	__le16   voice_setting;
} __packed;

struct hci_rp_read_num_supported_iac {
	__u8	status;
	__u8	num_iac;
} __packed;

struct hci_cp_write_current_iac_lap {
	__u8	num_iac;
	__u8	iac_lap[6];
} __packed;

struct hci_cp_write_ssp_mode {
	__u8     mode;
} __packed;

struct hci_cp_write_sc_support {
	__u8	support;
} __packed;

struct hci_cp_read_auth_payload_to {
	__le16  handle;
} __packed;

struct hci_rp_read_auth_payload_to {
	__u8    status;
	__le16  handle;
	__le16  timeout;
} __packed;

struct hci_cp_write_auth_payload_to {
	__le16  handle;
	__le16  timeout;
} __packed;

struct hci_rp_write_auth_payload_to {
	__u8    status;
	__le16  handle;
} __packed;

struct hci_rp_read_local_version {
	__u8     status;
	__u8     hci_ver;
	__le16   hci_rev;
	__u8     lmp_ver;
	__le16   manufacturer;
	__le16   lmp_subver;
} __packed;

struct hci_rp_read_local_commands {
	__u8     status;
	__u8     commands[64];
} __packed;

struct hci_rp_read_local_features {
	__u8     status;
	__u8     features[8];
} __packed;

struct hci_cp_read_local_ext_features {
	__u8     page;
} __packed;

struct hci_rp_read_local_ext_features {
	__u8     status;
	__u8     page;
	__u8     max_page;
	__u8     features[8];
} __packed;

struct hci_rp_read_buffer_size {
	__u8     status;
	__le16   acl_mtu;
	__u8     sco_mtu;
	__le16   acl_max_pkt;
	__le16   sco_max_pkt;
} __packed;

struct hci_rp_read_bd_addr {
	__u8     status;
	bdaddr_t bdaddr;
} __packed;

struct hci_rp_read_local_pairing_opts {
	__u8     status;
	__u8     pairing_opts;
	__u8     max_key_size;
} __packed;

struct hci_rp_read_page_scan_activity {
	__u8     status;
	__le16   interval;
	__le16   window;
} __packed;

struct hci_cp_write_page_scan_activity {
	__le16   interval;
	__le16   window;
} __packed;

struct hci_rp_read_page_scan_type {
	__u8     status;
	__u8     type;
} __packed;

struct hci_cp_read_clock {
	__le16   handle;
	__u8     which;
} __packed;

struct hci_rp_read_clock {
	__u8     status;
	__le16   handle;
	__le32   clock;
	__le16   accuracy;
} __packed;

struct hci_cp_read_enc_key_size {
	__le16   handle;
} __packed;

struct hci_rp_read_enc_key_size {
	__u8     status;
	__le16   handle;
	__u8     key_size;
} __packed;

struct hci_cp_le_accept_cis {
	__le16  handle;
} __packed;

struct hci_cp_le_reject_cis {
	__le16  handle;
	__u8    reason;
} __packed;

struct hci_cp_le_big_create_sync {
	__u8    handle;
	__le16  sync_handle;
	__u8    encryption;
	__u8    bcode[16];
	__u8    mse;
	__le16  timeout;
	__u8    num_bis;
	__u8    bis[] __counted_by(num_bis);
} __packed;

struct hci_cp_le_big_term_sync {
	__u8    handle;
} __packed;

struct hci_ev_inquiry_result {
	__u8    num;
	struct inquiry_info info[];
};

struct hci_ev_conn_complete {
	__u8     status;
	__le16   handle;
	bdaddr_t bdaddr;
	__u8     link_type;
	__u8     encr_mode;
} __packed;

struct hci_ev_conn_request {
	bdaddr_t bdaddr;
	__u8     dev_class[3];
	__u8     link_type;
} __packed;

struct hci_ev_disconn_complete {
	__u8     status;
	__le16   handle;
	__u8     reason;
} __packed;

struct hci_ev_auth_complete {
	__u8     status;
	__le16   handle;
} __packed;

struct hci_ev_remote_name {
	__u8     status;
	bdaddr_t bdaddr;
	__u8     name[HCI_MAX_NAME_LENGTH];
} __packed;

struct hci_ev_encrypt_change {
	__u8     status;
	__le16   handle;
	__u8     encrypt;
} __packed;

struct hci_ev_change_link_key_complete {
	__u8     status;
	__le16   handle;
} __packed;

struct hci_ev_remote_features {
	__u8     status;
	__le16   handle;
	__u8     features[8];
} __packed;

struct hci_ev_remote_version {
	__u8     status;
	__le16   handle;
	__u8     lmp_ver;
	__le16   manufacturer;
	__le16   lmp_subver;
} __packed;

struct hci_ev_cmd_complete {
	__u8     ncmd;
	__le16   opcode;
} __packed;

struct hci_ev_cmd_status {
	__u8     status;
	__u8     ncmd;
	__le16   opcode;
} __packed;

struct hci_ev_hardware_error {
	__u8     code;
} __packed;

struct hci_ev_role_change {
	__u8     status;
	bdaddr_t bdaddr;
	__u8     role;
} __packed;

struct hci_comp_pkts_info {
	__le16   handle;
	__le16   count;
} __packed;

struct hci_ev_num_comp_pkts {
	__u8     num;
	struct hci_comp_pkts_info handles[];
} __packed;

struct hci_ev_mode_change {
	__u8     status;
	__le16   handle;
	__u8     mode;
	__le16   interval;
} __packed;

struct hci_ev_pin_code_req {
	bdaddr_t bdaddr;
} __packed;

struct hci_ev_link_key_req {
	bdaddr_t bdaddr;
} __packed;

struct hci_ev_link_key_notify {
	bdaddr_t bdaddr;
	__u8     link_key[HCI_LINK_KEY_SIZE];
	__u8     key_type;
} __packed;

struct hci_ev_clock_offset {
	__u8     status;
	__le16   handle;
	__le16   clock_offset;
} __packed;

struct hci_ev_pkt_type_change {
	__u8     status;
	__le16   handle;
	__le16   pkt_type;
} __packed;

struct hci_ev_pscan_rep_mode {
	bdaddr_t bdaddr;
	__u8     pscan_rep_mode;
} __packed;

struct inquiry_info_rssi {
	bdaddr_t bdaddr;
	__u8     pscan_rep_mode;
	__u8     pscan_period_mode;
	__u8     dev_class[3];
	__le16   clock_offset;
	__s8     rssi;
} __packed;

struct inquiry_info_rssi_pscan {
	bdaddr_t bdaddr;
	__u8     pscan_rep_mode;
	__u8     pscan_period_mode;
	__u8     pscan_mode;
	__u8     dev_class[3];
	__le16   clock_offset;
	__s8     rssi;
} __packed;

struct hci_ev_inquiry_result_rssi {
	__u8     num;
	__u8     data[];
} __packed;

struct hci_ev_remote_ext_features {
	__u8     status;
	__le16   handle;
	__u8     page;
	__u8     max_page;
	__u8     features[8];
} __packed;

struct hci_ev_sync_conn_complete {
	__u8     status;
	__le16   handle;
	bdaddr_t bdaddr;
	__u8     link_type;
	__u8     tx_interval;
	__u8     retrans_window;
	__le16   rx_pkt_len;
	__le16   tx_pkt_len;
	__u8     air_mode;
} __packed;

struct extended_inquiry_info {
	bdaddr_t bdaddr;
	__u8     pscan_rep_mode;
	__u8     pscan_period_mode;
	__u8     dev_class[3];
	__le16   clock_offset;
	__s8     rssi;
	__u8     data[240];
} __packed;

struct hci_ev_ext_inquiry_result {
	__u8     num;
	struct extended_inquiry_info info[];
} __packed;

struct hci_ev_key_refresh_complete {
	__u8	status;
	__le16	handle;
} __packed;

struct hci_ev_io_capa_request {
	bdaddr_t bdaddr;
} __packed;

struct hci_ev_io_capa_reply {
	bdaddr_t bdaddr;
	__u8     capability;
	__u8     oob_data;
	__u8     authentication;
} __packed;

struct hci_ev_user_confirm_req {
	bdaddr_t	bdaddr;
	__le32		passkey;
} __packed;

struct hci_ev_user_passkey_req {
	bdaddr_t	bdaddr;
} __packed;

struct hci_ev_remote_oob_data_request {
	bdaddr_t bdaddr;
} __packed;

struct hci_ev_simple_pair_complete {
	__u8     status;
	bdaddr_t bdaddr;
} __packed;

struct hci_ev_user_passkey_notify {
	bdaddr_t	bdaddr;
	__le32		passkey;
} __packed;

struct hci_ev_keypress_notify {
	bdaddr_t	bdaddr;
	__u8		type;
} __packed;

struct hci_ev_remote_host_features {
	bdaddr_t bdaddr;
	__u8     features[8];
} __packed;

#include <net/bluetooth/hci_cmd_defs.h>
#include <net/bluetooth/hci_event_defs.h>

#define hci_opcode_ogf(op) ((op) >> 10)

#endif
