/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/net/bluetooth/hci_cmd_defs.h
 *
 * Linux HCI command ABI definitions imported from Linux 6.17 hci.h.
 * Keep numeric values and layouts aligned with upstream Linux.
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_CMD_DEFS_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_CMD_DEFS_H

#ifndef __HCI_H
#define __HCI_H
#endif

#ifndef HCI_MAX_SCO_SIZE
#define HCI_MAX_SCO_SIZE	255
#endif

#ifndef HCI_MAX_ISO_SIZE
#define HCI_MAX_ISO_SIZE	251
#endif

#ifndef HCI_MAX_EVENT_SIZE
#define HCI_MAX_EVENT_SIZE	260
#endif

#ifndef HCI_MAX_CPB_DATA_SIZE
#define HCI_MAX_CPB_DATA_SIZE	252
#endif

#ifndef HCI_DEV_REG
#define HCI_DEV_REG			1
#endif

#ifndef HCI_DEV_UNREG
#define HCI_DEV_UNREG			2
#endif

#ifndef HCI_DEV_UP
#define HCI_DEV_UP			3
#endif

#ifndef HCI_DEV_DOWN
#define HCI_DEV_DOWN			4
#endif

#ifndef HCI_DEV_SUSPEND
#define HCI_DEV_SUSPEND			5
#endif

#ifndef HCI_DEV_RESUME
#define HCI_DEV_RESUME			6
#endif

#ifndef HCI_DEV_OPEN
#define HCI_DEV_OPEN			7
#endif

#ifndef HCI_DEV_CLOSE
#define HCI_DEV_CLOSE			8
#endif

#ifndef HCI_DEV_SETUP
#define HCI_DEV_SETUP			9
#endif

#ifndef HCI_NOTIFY_CONN_ADD
#define HCI_NOTIFY_CONN_ADD		1
#endif

#ifndef HCI_NOTIFY_CONN_DEL
#define HCI_NOTIFY_CONN_DEL		2
#endif

#ifndef HCI_NOTIFY_VOICE_SETTING
#define HCI_NOTIFY_VOICE_SETTING	3
#endif

#ifndef HCI_NOTIFY_ENABLE_SCO_CVSD
#define HCI_NOTIFY_ENABLE_SCO_CVSD	4
#endif

#ifndef HCI_NOTIFY_ENABLE_SCO_TRANSP
#define HCI_NOTIFY_ENABLE_SCO_TRANSP	5
#endif

#ifndef HCI_NOTIFY_DISABLE_SCO
#define HCI_NOTIFY_DISABLE_SCO		6
#endif

#ifndef HCI_USB
#define HCI_USB		1
#endif

#ifndef HCI_PCCARD
#define HCI_PCCARD	2
#endif

#ifndef HCI_UART
#define HCI_UART	3
#endif

#ifndef HCI_RS232
#define HCI_RS232	4
#endif

#ifndef HCI_PCI
#define HCI_PCI		5
#endif

#ifndef HCI_SDIO
#define HCI_SDIO	6
#endif

#ifndef HCI_SPI
#define HCI_SPI		7
#endif

#ifndef HCI_I2C
#define HCI_I2C		8
#endif

#ifndef HCI_SMD
#define HCI_SMD		9
#endif

#ifndef HCI_VIRTIO
#define HCI_VIRTIO	10
#endif

#ifndef HCI_IPC
#define HCI_IPC		11
#endif

#ifndef HCI_PAIRING_TIMEOUT
#define HCI_PAIRING_TIMEOUT	msecs_to_jiffies(60000)	/* 60 seconds */
#endif

#ifndef HCI_INIT_TIMEOUT
#define HCI_INIT_TIMEOUT	msecs_to_jiffies(10000)	/* 10 seconds */
#endif

#ifndef HCI_NCMD_TIMEOUT
#define HCI_NCMD_TIMEOUT	msecs_to_jiffies(4000)	/* 4 seconds */
#endif

#ifndef HCI_ACL_CONN_TIMEOUT
#define HCI_ACL_CONN_TIMEOUT	msecs_to_jiffies(20000)	/* 20 seconds */
#endif

#ifndef ACL_START_NO_FLUSH
#define ACL_START_NO_FLUSH	0x00
#endif

#ifndef ACL_COMPLETE
#define ACL_COMPLETE		0x03
#endif

#ifndef ACL_ACTIVE_BCAST
#define ACL_ACTIVE_BCAST	0x04
#endif

#ifndef ACL_PICO_BCAST
#define ACL_PICO_BCAST		0x08
#endif

#ifndef ISO_TS
#define ISO_TS			0x01
#endif

#ifndef INVALID_LINK
#define INVALID_LINK	0xff
#endif

#ifndef LMP_ENCRYPT
#define LMP_ENCRYPT	0x04
#endif

#ifndef LMP_SOFFSET
#define LMP_SOFFSET	0x08
#endif

#ifndef LMP_TACCURACY
#define LMP_TACCURACY	0x10
#endif

#ifndef LMP_RSWITCH
#define LMP_RSWITCH	0x20
#endif

#ifndef LMP_HOLD
#define LMP_HOLD	0x40
#endif

#ifndef LMP_SNIFF
#define LMP_SNIFF	0x80
#endif

#ifndef LMP_PARK
#define LMP_PARK	0x01
#endif

#ifndef LMP_RSSI
#define LMP_RSSI	0x02
#endif

#ifndef LMP_QUALITY
#define LMP_QUALITY	0x04
#endif

#ifndef LMP_SCO
#define LMP_SCO		0x08
#endif

#ifndef LMP_ULAW
#define LMP_ULAW	0x40
#endif

#ifndef LMP_ALAW
#define LMP_ALAW	0x80
#endif

#ifndef LMP_CVSD
#define LMP_CVSD	0x01
#endif

#ifndef LMP_PSCHEME
#define LMP_PSCHEME	0x02
#endif

#ifndef LMP_PCONTROL
#define LMP_PCONTROL	0x04
#endif

#ifndef LMP_TRANSPARENT
#define LMP_TRANSPARENT	0x08
#endif

#ifndef LMP_EDR_2M
#define LMP_EDR_2M		0x02
#endif

#ifndef LMP_EDR_3M
#define LMP_EDR_3M		0x04
#endif

#ifndef LMP_RSSI_INQ
#define LMP_RSSI_INQ	0x40
#endif

#ifndef LMP_ESCO
#define LMP_ESCO	0x80
#endif

#ifndef LMP_NO_BREDR
#define LMP_NO_BREDR	0x20
#endif

#ifndef LMP_LE
#define LMP_LE		0x40
#endif

#ifndef LMP_EDR_3SLOT
#define LMP_EDR_3SLOT	0x80
#endif

#ifndef LMP_EDR_5SLOT
#define LMP_EDR_5SLOT	0x01
#endif

#ifndef LMP_SNIFF_SUBR
#define LMP_SNIFF_SUBR	0x02
#endif

#ifndef LMP_PAUSE_ENC
#define LMP_PAUSE_ENC	0x04
#endif

#ifndef LMP_EXT_INQ
#define LMP_EXT_INQ	0x01
#endif

#ifndef LMP_SIMUL_LE_BR
#define LMP_SIMUL_LE_BR	0x02
#endif

#ifndef LMP_SIMPLE_PAIR
#define LMP_SIMPLE_PAIR	0x08
#endif

#ifndef LMP_ERR_DATA_REPORTING
#define LMP_ERR_DATA_REPORTING 0x20
#endif

#ifndef LMP_NO_FLUSH
#define LMP_NO_FLUSH	0x40
#endif

#ifndef LMP_LSTO
#define LMP_LSTO	0x01
#endif

#ifndef LMP_INQ_TX_PWR
#define LMP_INQ_TX_PWR	0x02
#endif

#ifndef LMP_EXTFEATURES
#define LMP_EXTFEATURES	0x80
#endif

#ifndef LMP_CPB_CENTRAL
#define LMP_CPB_CENTRAL		0x01
#endif

#ifndef LMP_CPB_PERIPHERAL
#define LMP_CPB_PERIPHERAL	0x02
#endif

#ifndef LMP_SYNC_TRAIN
#define LMP_SYNC_TRAIN		0x04
#endif

#ifndef LMP_SYNC_SCAN
#define LMP_SYNC_SCAN		0x08
#endif

#ifndef LMP_SC
#define LMP_SC		0x01
#endif

#ifndef LMP_PING
#define LMP_PING	0x02
#endif

#ifndef HCI_LE_ENCRYPTION
#define HCI_LE_ENCRYPTION		0x01
#endif

#ifndef HCI_LE_CONN_PARAM_REQ_PROC
#define HCI_LE_CONN_PARAM_REQ_PROC	0x02
#endif

#ifndef HCI_LE_PERIPHERAL_FEATURES
#define HCI_LE_PERIPHERAL_FEATURES	0x08
#endif

#ifndef HCI_LE_PING
#define HCI_LE_PING			0x10
#endif

#ifndef HCI_LE_DATA_LEN_EXT
#define HCI_LE_DATA_LEN_EXT		0x20
#endif

#ifndef HCI_LE_LL_PRIVACY
#define HCI_LE_LL_PRIVACY		0x40
#endif

#ifndef HCI_LE_EXT_SCAN_POLICY
#define HCI_LE_EXT_SCAN_POLICY		0x80
#endif

#ifndef HCI_LE_PHY_2M
#define HCI_LE_PHY_2M			0x01
#endif

#ifndef HCI_LE_PHY_CODED
#define HCI_LE_PHY_CODED		0x08
#endif

#ifndef HCI_LE_EXT_ADV
#define HCI_LE_EXT_ADV			0x10
#endif

#ifndef HCI_LE_PERIODIC_ADV
#define HCI_LE_PERIODIC_ADV		0x20
#endif

#ifndef HCI_LE_CHAN_SEL_ALG2
#define HCI_LE_CHAN_SEL_ALG2		0x40
#endif

#ifndef HCI_LE_CIS_CENTRAL
#define HCI_LE_CIS_CENTRAL		0x10
#endif

#ifndef HCI_LE_CIS_PERIPHERAL
#define HCI_LE_CIS_PERIPHERAL		0x20
#endif

#ifndef HCI_LE_ISO_BROADCASTER
#define HCI_LE_ISO_BROADCASTER		0x40
#endif

#ifndef HCI_LE_ISO_SYNC_RECEIVER
#define HCI_LE_ISO_SYNC_RECEIVER	0x80
#endif

#ifndef HCI_CM_HOLD
#define HCI_CM_HOLD	0x0001
#endif

#ifndef HCI_CM_PARK
#define HCI_CM_PARK	0x0003
#endif

#ifndef HCI_LP_RSWITCH
#define HCI_LP_RSWITCH	0x0001
#endif

#ifndef HCI_LP_HOLD
#define HCI_LP_HOLD	0x0002
#endif

#ifndef HCI_LP_PARK
#define HCI_LP_PARK	0x0008
#endif

#ifndef HCI_ERROR_PIN_OR_KEY_MISSING
#define HCI_ERROR_PIN_OR_KEY_MISSING	0x06
#endif

#ifndef HCI_ERROR_MEMORY_EXCEEDED
#define HCI_ERROR_MEMORY_EXCEEDED	0x07
#endif

#ifndef HCI_ERROR_CONNECTION_TIMEOUT
#define HCI_ERROR_CONNECTION_TIMEOUT	0x08
#endif

#ifndef HCI_ERROR_REJ_LIMITED_RESOURCES
#define HCI_ERROR_REJ_LIMITED_RESOURCES	0x0d
#endif

#ifndef HCI_ERROR_REMOTE_LOW_RESOURCES
#define HCI_ERROR_REMOTE_LOW_RESOURCES	0x14
#endif

#ifndef HCI_ERROR_REMOTE_POWER_OFF
#define HCI_ERROR_REMOTE_POWER_OFF	0x15
#endif

#ifndef HCI_ERROR_PAIRING_NOT_ALLOWED
#define HCI_ERROR_PAIRING_NOT_ALLOWED	0x18
#endif

#ifndef HCI_ERROR_UNSUPPORTED_REMOTE_FEATURE
#define HCI_ERROR_UNSUPPORTED_REMOTE_FEATURE	0x1a
#endif

#ifndef HCI_ERROR_INVALID_LL_PARAMS
#define HCI_ERROR_INVALID_LL_PARAMS	0x1e
#endif

#ifndef HCI_ERROR_CANCELLED_BY_HOST
#define HCI_ERROR_CANCELLED_BY_HOST	0x44
#endif

#ifndef HCI_FLOW_CTL_MODE_PACKET_BASED
#define HCI_FLOW_CTL_MODE_PACKET_BASED	0x00
#endif

#ifndef HCI_FLOW_CTL_MODE_BLOCK_BASED
#define HCI_FLOW_CTL_MODE_BLOCK_BASED	0x01
#endif

#ifndef EIR_FLAGS
#define EIR_FLAGS		0x01 /* flags */
#endif

#ifndef EIR_UUID16_SOME
#define EIR_UUID16_SOME		0x02 /* 16-bit UUID, more available */
#endif

#ifndef EIR_UUID16_ALL
#define EIR_UUID16_ALL		0x03 /* 16-bit UUID, all listed */
#endif

#ifndef EIR_UUID32_SOME
#define EIR_UUID32_SOME		0x04 /* 32-bit UUID, more available */
#endif

#ifndef EIR_UUID32_ALL
#define EIR_UUID32_ALL		0x05 /* 32-bit UUID, all listed */
#endif

#ifndef EIR_UUID128_SOME
#define EIR_UUID128_SOME	0x06 /* 128-bit UUID, more available */
#endif

#ifndef EIR_UUID128_ALL
#define EIR_UUID128_ALL		0x07 /* 128-bit UUID, all listed */
#endif

#ifndef EIR_NAME_SHORT
#define EIR_NAME_SHORT		0x08 /* shortened local name */
#endif

#ifndef EIR_NAME_COMPLETE
#define EIR_NAME_COMPLETE	0x09 /* complete local name */
#endif

#ifndef EIR_TX_POWER
#define EIR_TX_POWER		0x0A /* transmit power level */
#endif

#ifndef EIR_CLASS_OF_DEV
#define EIR_CLASS_OF_DEV	0x0D /* Class of Device */
#endif

#ifndef EIR_SSP_HASH_C192
#define EIR_SSP_HASH_C192	0x0E /* Simple Pairing Hash C-192 */
#endif

#ifndef EIR_SSP_RAND_R192
#define EIR_SSP_RAND_R192	0x0F /* Simple Pairing Randomizer R-192 */
#endif

#ifndef EIR_DEVICE_ID
#define EIR_DEVICE_ID		0x10 /* device ID */
#endif

#ifndef EIR_APPEARANCE
#define EIR_APPEARANCE		0x19 /* Device appearance */
#endif

#ifndef EIR_SERVICE_DATA
#define EIR_SERVICE_DATA	0x16 /* Service Data */
#endif

#ifndef EIR_LE_BDADDR
#define EIR_LE_BDADDR		0x1B /* LE Bluetooth device address */
#endif

#ifndef EIR_LE_ROLE
#define EIR_LE_ROLE		0x1C /* LE role */
#endif

#ifndef EIR_SSP_HASH_C256
#define EIR_SSP_HASH_C256	0x1D /* Simple Pairing Hash C-256 */
#endif

#ifndef EIR_SSP_RAND_R256
#define EIR_SSP_RAND_R256	0x1E /* Simple Pairing Rand R-256 */
#endif

#ifndef EIR_LE_SC_CONFIRM
#define EIR_LE_SC_CONFIRM	0x22 /* LE SC Confirmation Value */
#endif

#ifndef EIR_LE_SC_RANDOM
#define EIR_LE_SC_RANDOM	0x23 /* LE SC Random Value */
#endif

#ifndef LE_AD_LIMITED
#define LE_AD_LIMITED		0x01 /* Limited Discoverable */
#endif

#ifndef LE_AD_GENERAL
#define LE_AD_GENERAL		0x02 /* General Discoverable */
#endif

#ifndef LE_AD_NO_BREDR
#define LE_AD_NO_BREDR		0x04 /* BR/EDR not supported */
#endif

#ifndef LE_AD_SIM_LE_BREDR_CTRL
#define LE_AD_SIM_LE_BREDR_CTRL	0x08 /* Simultaneous LE & BR/EDR Controller */
#endif

#ifndef LE_AD_SIM_LE_BREDR_HOST
#define LE_AD_SIM_LE_BREDR_HOST	0x10 /* Simultaneous LE & BR/EDR Host */
#endif

#ifndef HCI_OP_INQUIRY
#define HCI_OP_INQUIRY			0x0401
#endif

struct hci_cp_inquiry {
	__u8     lap[3];
	__u8     length;
	__u8     num_rsp;
} __packed;

#ifndef HCI_OP_INQUIRY_CANCEL
#define HCI_OP_INQUIRY_CANCEL		0x0402
#endif

#ifndef HCI_OP_PERIODIC_INQ
#define HCI_OP_PERIODIC_INQ		0x0403
#endif

#ifndef HCI_OP_EXIT_PERIODIC_INQ
#define HCI_OP_EXIT_PERIODIC_INQ	0x0404
#endif

#ifndef HCI_OP_CREATE_CONN
#define HCI_OP_CREATE_CONN		0x0405
#endif

struct hci_cp_create_conn {
	bdaddr_t bdaddr;
	__le16   pkt_type;
	__u8     pscan_rep_mode;
	__u8     pscan_mode;
	__le16   clock_offset;
	__u8     role_switch;
} __packed;

#ifndef HCI_OP_DISCONNECT
#define HCI_OP_DISCONNECT		0x0406
#endif

struct hci_cp_disconnect {
	__le16   handle;
	__u8     reason;
} __packed;

#ifndef HCI_OP_CREATE_CONN_CANCEL
#define HCI_OP_CREATE_CONN_CANCEL	0x0408
#endif

struct hci_cp_create_conn_cancel {
	bdaddr_t bdaddr;
} __packed;

#ifndef HCI_OP_ACCEPT_CONN_REQ
#define HCI_OP_ACCEPT_CONN_REQ		0x0409
#endif

struct hci_cp_accept_conn_req {
	bdaddr_t bdaddr;
	__u8     role;
} __packed;

#ifndef HCI_OP_REJECT_CONN_REQ
#define HCI_OP_REJECT_CONN_REQ		0x040a
#endif

struct hci_cp_reject_conn_req {
	bdaddr_t bdaddr;
	__u8     reason;
} __packed;

#ifndef HCI_OP_LINK_KEY_REPLY
#define HCI_OP_LINK_KEY_REPLY		0x040b
#endif

struct hci_cp_link_key_reply {
	bdaddr_t bdaddr;
	__u8     link_key[HCI_LINK_KEY_SIZE];
} __packed;

#ifndef HCI_OP_LINK_KEY_NEG_REPLY
#define HCI_OP_LINK_KEY_NEG_REPLY	0x040c
#endif

struct hci_cp_link_key_neg_reply {
	bdaddr_t bdaddr;
} __packed;

#ifndef HCI_OP_PIN_CODE_REPLY
#define HCI_OP_PIN_CODE_REPLY		0x040d
#endif

struct hci_cp_pin_code_reply {
	bdaddr_t bdaddr;
	__u8     pin_len;
	__u8     pin_code[16];
} __packed;

struct hci_rp_pin_code_reply {
	__u8     status;
	bdaddr_t bdaddr;
} __packed;

#ifndef HCI_OP_PIN_CODE_NEG_REPLY
#define HCI_OP_PIN_CODE_NEG_REPLY	0x040e
#endif

struct hci_cp_pin_code_neg_reply {
	bdaddr_t bdaddr;
} __packed;

struct hci_rp_pin_code_neg_reply {
	__u8     status;
	bdaddr_t bdaddr;
} __packed;

#ifndef HCI_OP_CHANGE_CONN_PTYPE
#define HCI_OP_CHANGE_CONN_PTYPE	0x040f
#endif

struct hci_cp_change_conn_ptype {
	__le16   handle;
	__le16   pkt_type;
} __packed;

#ifndef HCI_OP_CHANGE_CONN_LINK_KEY
#define HCI_OP_CHANGE_CONN_LINK_KEY	0x0415
#endif

struct hci_cp_change_conn_link_key {
	__le16   handle;
} __packed;

#ifndef HCI_OP_REMOTE_NAME_REQ
#define HCI_OP_REMOTE_NAME_REQ		0x0419
#endif

struct hci_cp_remote_name_req {
	bdaddr_t bdaddr;
	__u8     pscan_rep_mode;
	__u8     pscan_mode;
	__le16   clock_offset;
} __packed;

struct hci_cp_remote_name_req_cancel {
	bdaddr_t bdaddr;
} __packed;

#ifndef HCI_OP_READ_REMOTE_FEATURES
#define HCI_OP_READ_REMOTE_FEATURES	0x041b
#endif

struct hci_cp_read_remote_features {
	__le16   handle;
} __packed;

#ifndef HCI_OP_READ_REMOTE_EXT_FEATURES
#define HCI_OP_READ_REMOTE_EXT_FEATURES	0x041c
#endif

struct hci_cp_read_remote_ext_features {
	__le16   handle;
	__u8     page;
} __packed;

#ifndef HCI_OP_READ_REMOTE_VERSION
#define HCI_OP_READ_REMOTE_VERSION	0x041d
#endif

struct hci_cp_read_remote_version {
	__le16   handle;
} __packed;

#ifndef HCI_OP_ACCEPT_SYNC_CONN_REQ
#define HCI_OP_ACCEPT_SYNC_CONN_REQ	0x0429
#endif

struct hci_cp_accept_sync_conn_req {
	bdaddr_t bdaddr;
	__le32   tx_bandwidth;
	__le32   rx_bandwidth;
	__le16   max_latency;
	__le16   content_format;
	__u8     retrans_effort;
	__le16   pkt_type;
} __packed;

#ifndef HCI_OP_REJECT_SYNC_CONN_REQ
#define HCI_OP_REJECT_SYNC_CONN_REQ	0x042a
#endif

struct hci_cp_reject_sync_conn_req {
	bdaddr_t bdaddr;
	__u8     reason;
} __packed;

#ifndef HCI_OP_IO_CAPABILITY_REPLY
#define HCI_OP_IO_CAPABILITY_REPLY	0x042b
#endif

struct hci_cp_io_capability_reply {
	bdaddr_t bdaddr;
	__u8     capability;
	__u8     oob_data;
	__u8     authentication;
} __packed;

struct hci_cp_user_confirm_reply {
	bdaddr_t bdaddr;
} __packed;

struct hci_rp_user_confirm_reply {
	__u8     status;
	bdaddr_t bdaddr;
} __packed;

#ifndef HCI_OP_USER_CONFIRM_NEG_REPLY
#define HCI_OP_USER_CONFIRM_NEG_REPLY	0x042d
#endif

#ifndef HCI_OP_USER_PASSKEY_REPLY
#define HCI_OP_USER_PASSKEY_REPLY		0x042e
#endif

struct hci_cp_user_passkey_reply {
	bdaddr_t bdaddr;
	__le32	passkey;
} __packed;

#ifndef HCI_OP_USER_PASSKEY_NEG_REPLY
#define HCI_OP_USER_PASSKEY_NEG_REPLY	0x042f
#endif

#ifndef HCI_OP_REMOTE_OOB_DATA_REPLY
#define HCI_OP_REMOTE_OOB_DATA_REPLY	0x0430
#endif

struct hci_cp_remote_oob_data_reply {
	bdaddr_t bdaddr;
	__u8     hash[16];
	__u8     rand[16];
} __packed;

#ifndef HCI_OP_REMOTE_OOB_DATA_NEG_REPLY
#define HCI_OP_REMOTE_OOB_DATA_NEG_REPLY	0x0433
#endif

struct hci_cp_remote_oob_data_neg_reply {
	bdaddr_t bdaddr;
} __packed;

#ifndef HCI_OP_IO_CAPABILITY_NEG_REPLY
#define HCI_OP_IO_CAPABILITY_NEG_REPLY	0x0434
#endif

struct hci_cp_io_capability_neg_reply {
	bdaddr_t bdaddr;
	__u8     reason;
} __packed;

struct hci_rp_logical_link_cancel {
	__u8     status;
	__u8     phy_handle;
	__u8     flow_spec_id;
} __packed;

#ifndef HCI_OP_SET_CPB
#define HCI_OP_SET_CPB			0x0441
#endif

struct hci_cp_set_cpb {
	__u8	enable;
	__u8	lt_addr;
	__u8	lpo_allowed;
	__le16	packet_type;
	__le16	interval_min;
	__le16	interval_max;
	__le16	cpb_sv_tout;
} __packed;

struct hci_rp_set_cpb {
	__u8	status;
	__u8	lt_addr;
	__le16	interval;
} __packed;

#ifndef HCI_OP_START_SYNC_TRAIN
#define HCI_OP_START_SYNC_TRAIN		0x0443
#endif

#ifndef HCI_OP_REMOTE_OOB_EXT_DATA_REPLY
#define HCI_OP_REMOTE_OOB_EXT_DATA_REPLY	0x0445
#endif

struct hci_cp_remote_oob_ext_data_reply {
	bdaddr_t bdaddr;
	__u8     hash192[16];
	__u8     rand192[16];
	__u8     hash256[16];
	__u8     rand256[16];
} __packed;

struct hci_cp_role_discovery {
	__le16   handle;
} __packed;

struct hci_cp_read_link_policy {
	__le16   handle;
} __packed;

struct hci_cp_write_link_policy {
	__le16   handle;
	__le16   policy;
} __packed;

struct hci_cp_write_def_link_policy {
	__le16   policy;
} __packed;

#ifndef HCI_OP_SET_EVENT_MASK
#define HCI_OP_SET_EVENT_MASK		0x0c01
#endif

#ifndef HCI_SET_EVENT_FLT_SIZE
#define HCI_SET_EVENT_FLT_SIZE		9
#endif

#ifndef HCI_CONN_SETUP_ALLOW_ALL
#define HCI_CONN_SETUP_ALLOW_ALL	0x00
#endif

#ifndef HCI_CONN_SETUP_ALLOW_CLASS
#define HCI_CONN_SETUP_ALLOW_CLASS	0x01
#endif

#ifndef HCI_CONN_SETUP_ALLOW_BDADDR
#define HCI_CONN_SETUP_ALLOW_BDADDR	0x02
#endif

#ifndef HCI_CONN_SETUP_AUTO_OFF
#define HCI_CONN_SETUP_AUTO_OFF		0x01
#endif

#ifndef HCI_CONN_SETUP_AUTO_ON
#define HCI_CONN_SETUP_AUTO_ON		0x02
#endif

#ifndef HCI_CONN_SETUP_AUTO_ON_WITH_RS
#define HCI_CONN_SETUP_AUTO_ON_WITH_RS	0x03
#endif

#ifndef HCI_OP_WRITE_CA_TIMEOUT
#define HCI_OP_WRITE_CA_TIMEOUT		0x0c16
#endif

#ifndef HCI_OP_WRITE_PG_TIMEOUT
#define HCI_OP_WRITE_PG_TIMEOUT		0x0c18
#endif

#ifndef HCI_OP_READ_AUTH_ENABLE
#define HCI_OP_READ_AUTH_ENABLE		0x0c1f
#endif

#ifndef HCI_OP_READ_ENCRYPT_MODE
#define HCI_OP_READ_ENCRYPT_MODE	0x0c21
#endif

#ifndef HCI_OP_HOST_BUFFER_SIZE
#define HCI_OP_HOST_BUFFER_SIZE		0x0c33
#endif

struct hci_cp_host_buffer_size {
	__le16   acl_mtu;
	__u8     sco_mtu;
	__le16   acl_max_pkt;
	__le16   sco_max_pkt;
} __packed;

#ifndef HCI_OP_READ_CURRENT_IAC_LAP
#define HCI_OP_READ_CURRENT_IAC_LAP	0x0c39
#endif

#ifndef HCI_OP_WRITE_CURRENT_IAC_LAP
#define HCI_OP_WRITE_CURRENT_IAC_LAP	0x0c3a
#endif

#ifndef HCI_OP_WRITE_INQUIRY_MODE
#define HCI_OP_WRITE_INQUIRY_MODE	0x0c45
#endif

#ifndef HCI_OP_WRITE_EIR
#define HCI_OP_WRITE_EIR		0x0c52
#endif

struct hci_cp_write_eir {
	__u8	fec;
	__u8	data[HCI_MAX_EIR_LENGTH];
} __packed;

#ifndef HCI_OP_READ_SSP_MODE
#define HCI_OP_READ_SSP_MODE		0x0c55
#endif

struct hci_rp_read_ssp_mode {
	__u8     status;
	__u8     mode;
} __packed;

#ifndef HCI_OP_READ_LOCAL_OOB_DATA
#define HCI_OP_READ_LOCAL_OOB_DATA		0x0c57
#endif

struct hci_rp_read_local_oob_data {
	__u8     status;
	__u8     hash[16];
	__u8     rand[16];
} __packed;

#ifndef HCI_OP_READ_INQ_RSP_TX_POWER
#define HCI_OP_READ_INQ_RSP_TX_POWER	0x0c58
#endif

struct hci_rp_read_inq_rsp_tx_power {
	__u8     status;
	__s8     tx_power;
} __packed;

#ifndef HCI_OP_READ_DEF_ERR_DATA_REPORTING
#define HCI_OP_READ_DEF_ERR_DATA_REPORTING	0x0c5a
#endif

struct hci_rp_read_def_err_data_reporting {
	__u8     status;
	__u8     err_data_reporting;
} __packed;

#ifndef HCI_OP_WRITE_DEF_ERR_DATA_REPORTING
#define HCI_OP_WRITE_DEF_ERR_DATA_REPORTING	0x0c5b
#endif

struct hci_cp_write_def_err_data_reporting {
	__u8     err_data_reporting;
} __packed;

#ifndef HCI_OP_SET_EVENT_MASK_PAGE_2
#define HCI_OP_SET_EVENT_MASK_PAGE_2	0x0c63
#endif

#ifndef HCI_OP_READ_LOCATION_DATA
#define HCI_OP_READ_LOCATION_DATA	0x0c64
#endif

#ifndef HCI_OP_READ_FLOW_CONTROL_MODE
#define HCI_OP_READ_FLOW_CONTROL_MODE	0x0c66
#endif

struct hci_rp_read_flow_control_mode {
	__u8     status;
	__u8     mode;
} __packed;

#ifndef HCI_OP_WRITE_LE_HOST_SUPPORTED
#define HCI_OP_WRITE_LE_HOST_SUPPORTED	0x0c6d
#endif

struct hci_cp_write_le_host_supported {
	__u8	le;
	__u8	simul;
} __packed;

#ifndef HCI_OP_SET_RESERVED_LT_ADDR
#define HCI_OP_SET_RESERVED_LT_ADDR	0x0c74
#endif

struct hci_cp_set_reserved_lt_addr {
	__u8	lt_addr;
} __packed;

struct hci_rp_set_reserved_lt_addr {
	__u8	status;
	__u8	lt_addr;
} __packed;

#ifndef HCI_OP_DELETE_RESERVED_LT_ADDR
#define HCI_OP_DELETE_RESERVED_LT_ADDR	0x0c75
#endif

struct hci_cp_delete_reserved_lt_addr {
	__u8	lt_addr;
} __packed;

struct hci_rp_delete_reserved_lt_addr {
	__u8	status;
	__u8	lt_addr;
} __packed;

#ifndef HCI_OP_SET_CPB_DATA
#define HCI_OP_SET_CPB_DATA		0x0c76
#endif

struct hci_cp_set_cpb_data {
	__u8	lt_addr;
	__u8	fragment;
	__u8	data_length;
	__u8	data[HCI_MAX_CPB_DATA_SIZE];
} __packed;

struct hci_rp_set_cpb_data {
	__u8	status;
	__u8	lt_addr;
} __packed;

#ifndef HCI_OP_READ_SYNC_TRAIN_PARAMS
#define HCI_OP_READ_SYNC_TRAIN_PARAMS	0x0c77
#endif

#ifndef HCI_OP_WRITE_SYNC_TRAIN_PARAMS
#define HCI_OP_WRITE_SYNC_TRAIN_PARAMS	0x0c78
#endif

struct hci_cp_write_sync_train_params {
	__le16	interval_min;
	__le16	interval_max;
	__le32	sync_train_tout;
	__u8	service_data;
} __packed;

struct hci_rp_write_sync_train_params {
	__u8	status;
	__le16	sync_train_int;
} __packed;

#ifndef HCI_OP_READ_SC_SUPPORT
#define HCI_OP_READ_SC_SUPPORT		0x0c79
#endif

struct hci_rp_read_sc_support {
	__u8	status;
	__u8	support;
} __packed;

#ifndef HCI_OP_READ_LOCAL_OOB_EXT_DATA
#define HCI_OP_READ_LOCAL_OOB_EXT_DATA	0x0c7d
#endif

struct hci_rp_read_local_oob_ext_data {
	__u8     status;
	__u8     hash192[16];
	__u8     rand192[16];
	__u8     hash256[16];
	__u8     rand256[16];
} __packed;

#ifndef HCI_OP_READ_DATA_BLOCK_SIZE
#define HCI_OP_READ_DATA_BLOCK_SIZE	0x100a
#endif

struct hci_rp_read_data_block_size {
	__u8     status;
	__le16   max_acl_len;
	__le16   block_len;
	__le16   num_blocks;
} __packed;

#ifndef HCI_OP_READ_LOCAL_CODECS
#define HCI_OP_READ_LOCAL_CODECS	0x100b
#endif

struct hci_std_codecs {
	__u8	num;
	__u8	codec[];
} __packed;

struct hci_vnd_codec {
	/* company id */
	__le16	cid;
	/* vendor codec id */
	__le16	vid;
} __packed;

struct hci_vnd_codecs {
	__u8	num;
	struct hci_vnd_codec codec[];
} __packed;

struct hci_rp_read_local_supported_codecs {
	__u8	status;
	struct hci_std_codecs std_codecs;
	struct hci_vnd_codecs vnd_codecs;
} __packed;

#ifndef HCI_OP_READ_LOCAL_CODECS_V2
#define HCI_OP_READ_LOCAL_CODECS_V2	0x100d
#endif

struct hci_std_codec_v2 {
	__u8	id;
	__u8	transport;
} __packed;

struct hci_std_codecs_v2 {
	__u8	num;
	struct hci_std_codec_v2 codec[];
} __packed;

struct hci_vnd_codec_v2 {
	__le16	cid;
	__le16	vid;
	__u8	transport;
} __packed;

struct hci_vnd_codecs_v2 {
	__u8	num;
	struct hci_vnd_codec_v2 codec[];
} __packed;

struct hci_rp_read_local_supported_codecs_v2 {
	__u8	status;
	struct hci_std_codecs_v2 std_codecs;
	struct hci_vnd_codecs_v2 vendor_codecs;
} __packed;

#ifndef HCI_OP_READ_LOCAL_CODEC_CAPS
#define HCI_OP_READ_LOCAL_CODEC_CAPS	0x100e
#endif

struct hci_op_read_local_codec_caps {
	__u8	id;
	__le16	cid;
	__le16	vid;
	__u8	transport;
	__u8	direction;
} __packed;

struct hci_codec_caps {
	__u8	len;
	__u8	data[];
} __packed;

struct hci_rp_read_local_codec_caps {
	__u8	status;
	__u8	num_caps;
} __packed;

#ifndef HCI_OP_READ_TX_POWER
#define HCI_OP_READ_TX_POWER		0x0c2d
#endif

struct hci_cp_read_tx_power {
	__le16   handle;
	__u8     type;
} __packed;

struct hci_rp_read_tx_power {
	__u8     status;
	__le16   handle;
	__s8     tx_power;
} __packed;

#ifndef HCI_OP_WRITE_SYNC_FLOWCTL
#define HCI_OP_WRITE_SYNC_FLOWCTL	0x0c2f
#endif

struct hci_cp_write_sync_flowctl {
	__u8     enable;
} __packed;

#ifndef HCI_OP_READ_RSSI
#define HCI_OP_READ_RSSI		0x1405
#endif

struct hci_cp_read_rssi {
	__le16   handle;
} __packed;

struct hci_rp_read_rssi {
	__u8     status;
	__le16   handle;
	__s8     rssi;
} __packed;

#ifndef HCI_OP_READ_CLOCK
#define HCI_OP_READ_CLOCK		0x1407
#endif

#ifndef HCI_OP_GET_MWS_TRANSPORT_CONFIG
#define HCI_OP_GET_MWS_TRANSPORT_CONFIG	0x140c
#endif

#ifndef HCI_OP_ENABLE_DUT_MODE
#define HCI_OP_ENABLE_DUT_MODE		0x1803
#endif

#ifndef HCI_OP_WRITE_SSP_DEBUG_MODE
#define HCI_OP_WRITE_SSP_DEBUG_MODE	0x1804
#endif

#ifndef HCI_OP_LE_SET_EVENT_MASK
#define HCI_OP_LE_SET_EVENT_MASK	0x2001
#endif

struct hci_cp_le_set_event_mask {
	__u8     mask[8];
} __packed;

#ifndef HCI_OP_LE_READ_BUFFER_SIZE
#define HCI_OP_LE_READ_BUFFER_SIZE	0x2002
#endif

struct hci_rp_le_read_buffer_size {
	__u8     status;
	__le16   le_mtu;
	__u8     le_max_pkt;
} __packed;

#ifndef HCI_OP_LE_READ_LOCAL_FEATURES
#define HCI_OP_LE_READ_LOCAL_FEATURES	0x2003
#endif

struct hci_rp_le_read_local_features {
	__u8     status;
	__u8     features[8];
} __packed;

#ifndef HCI_OP_LE_SET_RANDOM_ADDR
#define HCI_OP_LE_SET_RANDOM_ADDR	0x2005
#endif

#ifndef HCI_OP_LE_SET_ADV_PARAM
#define HCI_OP_LE_SET_ADV_PARAM		0x2006
#endif

struct hci_cp_le_set_adv_param {
	__le16   min_interval;
	__le16   max_interval;
	__u8     type;
	__u8     own_address_type;
	__u8     direct_addr_type;
	bdaddr_t direct_addr;
	__u8     channel_map;
	__u8     filter_policy;
} __packed;

#ifndef HCI_OP_LE_READ_ADV_TX_POWER
#define HCI_OP_LE_READ_ADV_TX_POWER	0x2007
#endif

struct hci_rp_le_read_adv_tx_power {
	__u8	status;
	__s8	tx_power;
} __packed;

#ifndef HCI_OP_LE_SET_ADV_DATA
#define HCI_OP_LE_SET_ADV_DATA		0x2008
#endif

struct hci_cp_le_set_adv_data {
	__u8	length;
	__u8	data[HCI_MAX_AD_LENGTH];
} __packed;

#ifndef HCI_OP_LE_SET_SCAN_RSP_DATA
#define HCI_OP_LE_SET_SCAN_RSP_DATA	0x2009
#endif

struct hci_cp_le_set_scan_rsp_data {
	__u8	length;
	__u8	data[HCI_MAX_AD_LENGTH];
} __packed;

#ifndef HCI_OP_LE_SET_SCAN_PARAM
#define HCI_OP_LE_SET_SCAN_PARAM	0x200b
#endif

struct hci_cp_le_set_scan_param {
	__u8    type;
	__le16  interval;
	__le16  window;
	__u8    own_address_type;
	__u8    filter_policy;
} __packed;

#ifndef LE_SCAN_DISABLE
#define LE_SCAN_DISABLE			0x00
#endif

#ifndef LE_SCAN_ENABLE
#define LE_SCAN_ENABLE			0x01
#endif

#ifndef LE_SCAN_FILTER_DUP_DISABLE
#define LE_SCAN_FILTER_DUP_DISABLE	0x00
#endif

#ifndef LE_SCAN_FILTER_DUP_ENABLE
#define LE_SCAN_FILTER_DUP_ENABLE	0x01
#endif

#ifndef HCI_OP_LE_SET_SCAN_ENABLE
#define HCI_OP_LE_SET_SCAN_ENABLE	0x200c
#endif

struct hci_cp_le_set_scan_enable {
	__u8     enable;
	__u8     filter_dup;
} __packed;

#ifndef HCI_LE_USE_PEER_ADDR
#define HCI_LE_USE_PEER_ADDR		0x00
#endif

#ifndef HCI_LE_USE_ACCEPT_LIST
#define HCI_LE_USE_ACCEPT_LIST		0x01
#endif

#ifndef HCI_OP_LE_CREATE_CONN
#define HCI_OP_LE_CREATE_CONN		0x200d
#endif

struct hci_cp_le_create_conn {
	__le16   scan_interval;
	__le16   scan_window;
	__u8     filter_policy;
	__u8     peer_addr_type;
	bdaddr_t peer_addr;
	__u8     own_address_type;
	__le16   conn_interval_min;
	__le16   conn_interval_max;
	__le16   conn_latency;
	__le16   supervision_timeout;
	__le16   min_ce_len;
	__le16   max_ce_len;
} __packed;

#ifndef HCI_OP_LE_CREATE_CONN_CANCEL
#define HCI_OP_LE_CREATE_CONN_CANCEL	0x200e
#endif

#ifndef HCI_OP_LE_READ_ACCEPT_LIST_SIZE
#define HCI_OP_LE_READ_ACCEPT_LIST_SIZE	0x200f
#endif

struct hci_rp_le_read_accept_list_size {
	__u8	status;
	__u8	size;
} __packed;

#ifndef HCI_OP_LE_CLEAR_ACCEPT_LIST
#define HCI_OP_LE_CLEAR_ACCEPT_LIST	0x2010
#endif

#ifndef HCI_OP_LE_ADD_TO_ACCEPT_LIST
#define HCI_OP_LE_ADD_TO_ACCEPT_LIST	0x2011
#endif

struct hci_cp_le_add_to_accept_list {
	__u8     bdaddr_type;
	bdaddr_t bdaddr;
} __packed;

#ifndef HCI_OP_LE_DEL_FROM_ACCEPT_LIST
#define HCI_OP_LE_DEL_FROM_ACCEPT_LIST	0x2012
#endif

struct hci_cp_le_del_from_accept_list {
	__u8     bdaddr_type;
	bdaddr_t bdaddr;
} __packed;

#ifndef HCI_OP_LE_READ_REMOTE_FEATURES
#define HCI_OP_LE_READ_REMOTE_FEATURES	0x2016
#endif

struct hci_cp_le_read_remote_features {
	__le16	 handle;
} __packed;

#ifndef HCI_OP_LE_LTK_REPLY
#define HCI_OP_LE_LTK_REPLY		0x201a
#endif

struct hci_cp_le_ltk_reply {
	__le16	handle;
	__u8	ltk[16];
} __packed;

struct hci_rp_le_ltk_reply {
	__u8	status;
	__le16	handle;
} __packed;

#ifndef HCI_OP_LE_LTK_NEG_REPLY
#define HCI_OP_LE_LTK_NEG_REPLY		0x201b
#endif

struct hci_cp_le_ltk_neg_reply {
	__le16	handle;
} __packed;

struct hci_rp_le_ltk_neg_reply {
	__u8	status;
	__le16	handle;
} __packed;

#ifndef HCI_OP_LE_READ_SUPPORTED_STATES
#define HCI_OP_LE_READ_SUPPORTED_STATES	0x201c
#endif

struct hci_rp_le_read_supported_states {
	__u8	status;
	__u8	le_states[8];
} __packed;

#ifndef HCI_OP_LE_CONN_PARAM_REQ_REPLY
#define HCI_OP_LE_CONN_PARAM_REQ_REPLY	0x2020
#endif

struct hci_cp_le_conn_param_req_reply {
	__le16	handle;
	__le16	interval_min;
	__le16	interval_max;
	__le16	latency;
	__le16	timeout;
	__le16	min_ce_len;
	__le16	max_ce_len;
} __packed;

#ifndef HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY
#define HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY	0x2021
#endif

struct hci_cp_le_conn_param_req_neg_reply {
	__le16	handle;
	__u8	reason;
} __packed;

#ifndef HCI_OP_LE_SET_DATA_LEN
#define HCI_OP_LE_SET_DATA_LEN		0x2022
#endif

struct hci_cp_le_set_data_len {
	__le16	handle;
	__le16	tx_len;
	__le16	tx_time;
} __packed;

struct hci_rp_le_set_data_len {
	__u8	status;
	__le16	handle;
} __packed;

#ifndef HCI_OP_LE_READ_DEF_DATA_LEN
#define HCI_OP_LE_READ_DEF_DATA_LEN	0x2023
#endif

struct hci_rp_le_read_def_data_len {
	__u8	status;
	__le16	tx_len;
	__le16	tx_time;
} __packed;

#ifndef HCI_OP_LE_WRITE_DEF_DATA_LEN
#define HCI_OP_LE_WRITE_DEF_DATA_LEN	0x2024
#endif

struct hci_cp_le_write_def_data_len {
	__le16	tx_len;
	__le16	tx_time;
} __packed;

#ifndef HCI_OP_LE_ADD_TO_RESOLV_LIST
#define HCI_OP_LE_ADD_TO_RESOLV_LIST	0x2027
#endif

struct hci_cp_le_add_to_resolv_list {
	__u8	 bdaddr_type;
	bdaddr_t bdaddr;
	__u8	 peer_irk[16];
	__u8	 local_irk[16];
} __packed;

#ifndef HCI_OP_LE_DEL_FROM_RESOLV_LIST
#define HCI_OP_LE_DEL_FROM_RESOLV_LIST	0x2028
#endif

struct hci_cp_le_del_from_resolv_list {
	__u8	 bdaddr_type;
	bdaddr_t bdaddr;
} __packed;

#ifndef HCI_OP_LE_CLEAR_RESOLV_LIST
#define HCI_OP_LE_CLEAR_RESOLV_LIST	0x2029
#endif

#ifndef HCI_OP_LE_READ_RESOLV_LIST_SIZE
#define HCI_OP_LE_READ_RESOLV_LIST_SIZE	0x202a
#endif

struct hci_rp_le_read_resolv_list_size {
	__u8	status;
	__u8	size;
} __packed;

#ifndef HCI_OP_LE_SET_ADDR_RESOLV_ENABLE
#define HCI_OP_LE_SET_ADDR_RESOLV_ENABLE 0x202d
#endif

#ifndef HCI_OP_LE_SET_RPA_TIMEOUT
#define HCI_OP_LE_SET_RPA_TIMEOUT	0x202e
#endif

#ifndef HCI_OP_LE_READ_MAX_DATA_LEN
#define HCI_OP_LE_READ_MAX_DATA_LEN	0x202f
#endif

struct hci_rp_le_read_max_data_len {
	__u8	status;
	__le16	tx_len;
	__le16	tx_time;
	__le16	rx_len;
	__le16	rx_time;
} __packed;

#ifndef HCI_OP_LE_SET_DEFAULT_PHY
#define HCI_OP_LE_SET_DEFAULT_PHY	0x2031
#endif

struct hci_cp_le_set_default_phy {
	__u8    all_phys;
	__u8    tx_phys;
	__u8    rx_phys;
} __packed;

#ifndef HCI_OP_LE_SET_EXT_SCAN_PARAMS
#define HCI_OP_LE_SET_EXT_SCAN_PARAMS   0x2041
#endif

struct hci_cp_le_set_ext_scan_params {
	__u8    own_addr_type;
	__u8    filter_policy;
	__u8    scanning_phys;
	__u8    data[];
} __packed;

#ifndef LE_SCAN_PHY_1M
#define LE_SCAN_PHY_1M		0x01
#endif

#ifndef LE_SCAN_PHY_2M
#define LE_SCAN_PHY_2M		0x02
#endif

#ifndef LE_SCAN_PHY_CODED
#define LE_SCAN_PHY_CODED	0x04
#endif

struct hci_cp_le_scan_phy_params {
	__u8    type;
	__le16  interval;
	__le16  window;
} __packed;

#ifndef HCI_OP_LE_SET_EXT_SCAN_ENABLE
#define HCI_OP_LE_SET_EXT_SCAN_ENABLE   0x2042
#endif

struct hci_cp_le_set_ext_scan_enable {
	__u8    enable;
	__u8    filter_dup;
	__le16  duration;
	__le16  period;
} __packed;

#ifndef HCI_OP_LE_EXT_CREATE_CONN
#define HCI_OP_LE_EXT_CREATE_CONN    0x2043
#endif

struct hci_cp_le_ext_create_conn {
	__u8      filter_policy;
	__u8      own_addr_type;
	__u8      peer_addr_type;
	bdaddr_t  peer_addr;
	__u8      phys;
	__u8      data[];
} __packed;

struct hci_cp_le_ext_conn_param {
	__le16 scan_interval;
	__le16 scan_window;
	__le16 conn_interval_min;
	__le16 conn_interval_max;
	__le16 conn_latency;
	__le16 supervision_timeout;
	__le16 min_ce_len;
	__le16 max_ce_len;
} __packed;

#ifndef HCI_OP_LE_PA_CREATE_SYNC
#define HCI_OP_LE_PA_CREATE_SYNC	0x2044
#endif

struct hci_cp_le_pa_create_sync {
	__u8      options;
	__u8      sid;
	__u8      addr_type;
	bdaddr_t  addr;
	__le16    skip;
	__le16    sync_timeout;
	__u8      sync_cte_type;
} __packed;

#ifndef HCI_OP_LE_PA_CREATE_SYNC_CANCEL
#define HCI_OP_LE_PA_CREATE_SYNC_CANCEL	0x2045
#endif

#ifndef HCI_OP_LE_PA_TERM_SYNC
#define HCI_OP_LE_PA_TERM_SYNC		0x2046
#endif

struct hci_cp_le_pa_term_sync {
	__le16    handle;
} __packed;

#ifndef HCI_OP_LE_READ_NUM_SUPPORTED_ADV_SETS
#define HCI_OP_LE_READ_NUM_SUPPORTED_ADV_SETS	0x203b
#endif

struct hci_rp_le_read_num_supported_adv_sets {
	__u8  status;
	__u8  num_of_sets;
} __packed;

#ifndef HCI_OP_LE_SET_EXT_ADV_PARAMS
#define HCI_OP_LE_SET_EXT_ADV_PARAMS		0x2036
#endif

struct hci_cp_le_set_ext_adv_params {
	__u8      handle;
	__le16    evt_properties;
	__u8      min_interval[3];
	__u8      max_interval[3];
	__u8      channel_map;
	__u8      own_addr_type;
	__u8      peer_addr_type;
	bdaddr_t  peer_addr;
	__u8      filter_policy;
	__u8      tx_power;
	__u8      primary_phy;
	__u8      secondary_max_skip;
	__u8      secondary_phy;
	__u8      sid;
	__u8      notif_enable;
} __packed;

#ifndef HCI_ADV_PHY_1M
#define HCI_ADV_PHY_1M		0X01
#endif

#ifndef HCI_ADV_PHY_2M
#define HCI_ADV_PHY_2M		0x02
#endif

#ifndef HCI_ADV_PHY_CODED
#define HCI_ADV_PHY_CODED	0x03
#endif

struct hci_rp_le_set_ext_adv_params {
	__u8  status;
	__u8  tx_power;
} __packed;

struct hci_cp_ext_adv_set {
	__u8  handle;
	__le16 duration;
	__u8  max_events;
} __packed;

#ifndef HCI_OP_LE_SET_EXT_ADV_DATA
#define HCI_OP_LE_SET_EXT_ADV_DATA		0x2037
#endif

struct hci_cp_le_set_ext_adv_data {
	__u8  handle;
	__u8  operation;
	__u8  frag_pref;
	__u8  length;
	__u8  data[] __counted_by(length);
} __packed;

#ifndef HCI_OP_LE_SET_EXT_SCAN_RSP_DATA
#define HCI_OP_LE_SET_EXT_SCAN_RSP_DATA		0x2038
#endif

struct hci_cp_le_set_ext_scan_rsp_data {
	__u8  handle;
	__u8  operation;
	__u8  frag_pref;
	__u8  length;
	__u8  data[] __counted_by(length);
} __packed;

#ifndef HCI_OP_LE_SET_PER_ADV_PARAMS
#define HCI_OP_LE_SET_PER_ADV_PARAMS		0x203e
#endif

struct hci_cp_le_set_per_adv_params {
	__u8      handle;
	__le16    min_interval;
	__le16    max_interval;
	__le16    periodic_properties;
} __packed;

#ifndef HCI_OP_LE_SET_PER_ADV_DATA
#define HCI_OP_LE_SET_PER_ADV_DATA		0x203f
#endif

struct hci_cp_le_set_per_adv_data {
	__u8  handle;
	__u8  operation;
	__u8  length;
	__u8  data[] __counted_by(length);
} __packed;

#ifndef HCI_OP_LE_SET_PER_ADV_ENABLE
#define HCI_OP_LE_SET_PER_ADV_ENABLE		0x2040
#endif

struct hci_cp_le_set_per_adv_enable {
	__u8  enable;
	__u8  handle;
} __packed;

#ifndef LE_SET_ADV_DATA_OP_COMPLETE
#define LE_SET_ADV_DATA_OP_COMPLETE	0x03
#endif

#ifndef LE_SET_ADV_DATA_NO_FRAG
#define LE_SET_ADV_DATA_NO_FRAG		0x01
#endif

#ifndef HCI_OP_LE_REMOVE_ADV_SET
#define HCI_OP_LE_REMOVE_ADV_SET	0x203c
#endif

#ifndef HCI_OP_LE_CLEAR_ADV_SETS
#define HCI_OP_LE_CLEAR_ADV_SETS	0x203d
#endif

#ifndef HCI_OP_LE_SET_ADV_SET_RAND_ADDR
#define HCI_OP_LE_SET_ADV_SET_RAND_ADDR	0x2035
#endif

struct hci_cp_le_set_adv_set_rand_addr {
	__u8  handle;
	bdaddr_t  bdaddr;
} __packed;

#ifndef HCI_OP_LE_READ_TRANSMIT_POWER
#define HCI_OP_LE_READ_TRANSMIT_POWER	0x204b
#endif

struct hci_rp_le_read_transmit_power {
	__u8  status;
	__s8  min_le_tx_power;
	__s8  max_le_tx_power;
} __packed;

#ifndef HCI_NETWORK_PRIVACY
#define HCI_NETWORK_PRIVACY		0x00
#endif

#ifndef HCI_DEVICE_PRIVACY
#define HCI_DEVICE_PRIVACY		0x01
#endif

#ifndef HCI_OP_LE_SET_PRIVACY_MODE
#define HCI_OP_LE_SET_PRIVACY_MODE	0x204e
#endif

struct hci_cp_le_set_privacy_mode {
	__u8  bdaddr_type;
	bdaddr_t  bdaddr;
	__u8  mode;
} __packed;

#ifndef HCI_OP_LE_READ_BUFFER_SIZE_V2
#define HCI_OP_LE_READ_BUFFER_SIZE_V2	0x2060
#endif

struct hci_rp_le_read_buffer_size_v2 {
	__u8    status;
	__le16  acl_mtu;
	__u8    acl_max_pkt;
	__le16  iso_mtu;
	__u8    iso_max_pkt;
} __packed;

#ifndef HCI_OP_LE_READ_ISO_TX_SYNC
#define HCI_OP_LE_READ_ISO_TX_SYNC		0x2061
#endif

struct hci_cp_le_read_iso_tx_sync {
	__le16  handle;
} __packed;

struct hci_rp_le_read_iso_tx_sync {
	__u8    status;
	__le16  handle;
	__le16  seq;
	__le32  imestamp;
	__u8    offset[3];
} __packed;

#ifndef HCI_OP_LE_CREATE_CIS
#define HCI_OP_LE_CREATE_CIS			0x2064
#endif

struct hci_cis {
	__le16  cis_handle;
	__le16  acl_handle;
} __packed;

struct hci_cp_le_create_cis {
	__u8    num_cis;
	struct hci_cis cis[] __counted_by(num_cis);
} __packed;

#ifndef HCI_OP_LE_REMOVE_CIG
#define HCI_OP_LE_REMOVE_CIG			0x2065
#endif

struct hci_cp_le_remove_cig {
	__u8    cig_id;
} __packed;

struct hci_bis {
	__u8    sdu_interval[3];
	__le16  sdu;
	__le16  latency;
	__u8    rtn;
	__u8    phy;
	__u8    packing;
	__u8    framing;
	__u8    encryption;
	__u8    bcode[16];
} __packed;

#ifndef HCI_OP_LE_TERM_BIG
#define HCI_OP_LE_TERM_BIG			0x206a
#endif

struct hci_cp_le_term_big {
	__u8    handle;
	__u8    reason;
} __packed;

#ifndef HCI_OP_LE_SET_HOST_FEATURE
#define HCI_OP_LE_SET_HOST_FEATURE		0x2074
#endif

struct hci_cp_le_set_host_feature {
	__u8     bit_number;
	__u8     bit_value;
} __packed;

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_CMD_DEFS_H */
