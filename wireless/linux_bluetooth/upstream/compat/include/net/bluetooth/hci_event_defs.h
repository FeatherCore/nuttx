/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/net/bluetooth/hci_event_defs.h
 *
 * Linux HCI event ABI definitions imported from Linux 6.17 hci.h.
 * Keep numeric values and layouts aligned with upstream Linux.
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_EVENT_DEFS_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_EVENT_DEFS_H

#ifndef HCI_EV_QOS_SETUP_COMPLETE
#define HCI_EV_QOS_SETUP_COMPLETE	0x0d
#endif

struct hci_qos {
	__u8     service_type;
	__u32    token_rate;
	__u32    peak_bandwidth;
	__u32    latency;
	__u32    delay_variation;
} __packed;

struct hci_ev_qos_setup_complete {
	__u8     status;
	__le16   handle;
	struct   hci_qos qos;
} __packed;

#ifndef HCI_EV_SYNC_CONN_CHANGED
#define HCI_EV_SYNC_CONN_CHANGED	0x2d
#endif

struct hci_ev_sync_conn_changed {
	__u8     status;
	__le16   handle;
	__u8     tx_interval;
	__u8     retrans_window;
	__le16   rx_pkt_len;
	__le16   tx_pkt_len;
} __packed;

#ifndef HCI_EV_SNIFF_SUBRATE
#define HCI_EV_SNIFF_SUBRATE		0x2e
#endif

struct hci_ev_sniff_subrate {
	__u8     status;
	__le16   handle;
	__le16   max_tx_latency;
	__le16   max_rx_latency;
	__le16   max_remote_timeout;
	__le16   max_local_timeout;
} __packed;

#ifndef HCI_KEYPRESS_STARTED
#define HCI_KEYPRESS_STARTED		0
#endif

#ifndef HCI_KEYPRESS_ENTERED
#define HCI_KEYPRESS_ENTERED		1
#endif

#ifndef HCI_KEYPRESS_ERASED
#define HCI_KEYPRESS_ERASED		2
#endif

#ifndef HCI_KEYPRESS_CLEARED
#define HCI_KEYPRESS_CLEARED		3
#endif

#ifndef HCI_KEYPRESS_COMPLETED
#define HCI_KEYPRESS_COMPLETED		4
#endif

#ifndef HCI_EV_PHY_LINK_COMPLETE
#define HCI_EV_PHY_LINK_COMPLETE	0x40
#endif

struct hci_ev_phy_link_complete {
	__u8     status;
	__u8     phy_handle;
} __packed;

#ifndef HCI_EV_CHANNEL_SELECTED
#define HCI_EV_CHANNEL_SELECTED		0x41
#endif

struct hci_ev_channel_selected {
	__u8     phy_handle;
} __packed;

#ifndef HCI_EV_DISCONN_PHY_LINK_COMPLETE
#define HCI_EV_DISCONN_PHY_LINK_COMPLETE	0x42
#endif

struct hci_ev_disconn_phy_link_complete {
	__u8     status;
	__u8     phy_handle;
	__u8     reason;
} __packed;

#ifndef HCI_EV_LOGICAL_LINK_COMPLETE
#define HCI_EV_LOGICAL_LINK_COMPLETE		0x45
#endif

struct hci_ev_logical_link_complete {
	__u8     status;
	__le16   handle;
	__u8     phy_handle;
	__u8     flow_spec_id;
} __packed;

#ifndef HCI_EV_DISCONN_LOGICAL_LINK_COMPLETE
#define HCI_EV_DISCONN_LOGICAL_LINK_COMPLETE	0x46
#endif

struct hci_ev_disconn_logical_link_complete {
	__u8     status;
	__le16   handle;
	__u8     reason;
} __packed;

#ifndef HCI_EV_NUM_COMP_BLOCKS
#define HCI_EV_NUM_COMP_BLOCKS		0x48
#endif

struct hci_comp_blocks_info {
	__le16   handle;
	__le16   pkts;
	__le16   blocks;
} __packed;

struct hci_ev_num_comp_blocks {
	__le16   num_blocks;
	__u8     num_hndl;
	struct hci_comp_blocks_info handles[];
} __packed;

#ifndef HCI_EV_SYNC_TRAIN_COMPLETE
#define HCI_EV_SYNC_TRAIN_COMPLETE	0x4F
#endif

struct hci_ev_sync_train_complete {
	__u8	status;
} __packed;

#ifndef HCI_EV_PERIPHERAL_PAGE_RESP_TIMEOUT
#define HCI_EV_PERIPHERAL_PAGE_RESP_TIMEOUT	0x54
#endif

#ifndef LE_ADV_IND
#define LE_ADV_IND		0x00
#endif

#ifndef LE_ADV_DIRECT_IND
#define LE_ADV_DIRECT_IND	0x01
#endif

#ifndef LE_ADV_SCAN_IND
#define LE_ADV_SCAN_IND		0x02
#endif

#ifndef LE_ADV_NONCONN_IND
#define LE_ADV_NONCONN_IND	0x03
#endif

#ifndef LE_ADV_SCAN_RSP
#define LE_ADV_SCAN_RSP		0x04
#endif

#ifndef LE_ADV_INVALID
#define LE_ADV_INVALID		0x05
#endif

#ifndef LE_LEGACY_ADV_IND
#define LE_LEGACY_ADV_IND		0x0013
#endif

#ifndef LE_LEGACY_ADV_DIRECT_IND
#define LE_LEGACY_ADV_DIRECT_IND 	0x0015
#endif

#ifndef LE_LEGACY_ADV_SCAN_IND
#define LE_LEGACY_ADV_SCAN_IND		0x0012
#endif

#ifndef LE_LEGACY_NONCONN_IND
#define LE_LEGACY_NONCONN_IND		0x0010
#endif

#ifndef LE_LEGACY_SCAN_RSP_ADV
#define LE_LEGACY_SCAN_RSP_ADV		0x001b
#endif

#ifndef LE_LEGACY_SCAN_RSP_ADV_SCAN
#define LE_LEGACY_SCAN_RSP_ADV_SCAN	0x001a
#endif

#ifndef LE_EXT_ADV_NON_CONN_IND
#define LE_EXT_ADV_NON_CONN_IND		0x0000
#endif

#ifndef LE_EXT_ADV_CONN_IND
#define LE_EXT_ADV_CONN_IND		0x0001
#endif

#ifndef LE_EXT_ADV_SCAN_IND
#define LE_EXT_ADV_SCAN_IND		0x0002
#endif

#ifndef LE_EXT_ADV_DIRECT_IND
#define LE_EXT_ADV_DIRECT_IND		0x0004
#endif

#ifndef LE_EXT_ADV_SCAN_RSP
#define LE_EXT_ADV_SCAN_RSP		0x0008
#endif

#ifndef LE_EXT_ADV_LEGACY_PDU
#define LE_EXT_ADV_LEGACY_PDU		0x0010
#endif

#ifndef LE_EXT_ADV_DATA_STATUS_MASK
#define LE_EXT_ADV_DATA_STATUS_MASK	0x0060
#endif

#ifndef LE_EXT_ADV_EVT_TYPE_MASK
#define LE_EXT_ADV_EVT_TYPE_MASK	0x007f
#endif

#ifndef ADDR_LE_DEV_PUBLIC_RESOLVED
#define ADDR_LE_DEV_PUBLIC_RESOLVED	0x02
#endif

#ifndef ADDR_LE_DEV_RANDOM_RESOLVED
#define ADDR_LE_DEV_RANDOM_RESOLVED	0x03
#endif

#ifndef HCI_EV_LE_DATA_LEN_CHANGE
#define HCI_EV_LE_DATA_LEN_CHANGE	0x07
#endif

struct hci_ev_le_data_len_change {
	__le16	handle;
	__le16	tx_len;
	__le16	tx_time;
	__le16	rx_len;
	__le16	rx_time;
} __packed;

#ifndef LE_PA_DATA_COMPLETE
#define LE_PA_DATA_COMPLETE	0x00
#endif

#ifndef LE_PA_DATA_MORE_TO_COME
#define LE_PA_DATA_MORE_TO_COME	0x01
#endif

#ifndef LE_PA_DATA_TRUNCATED
#define LE_PA_DATA_TRUNCATED	0x02
#endif

#ifndef HCI_EV_STACK_INTERNAL
#define HCI_EV_STACK_INTERNAL	0xfd
#endif

struct hci_ev_stack_internal {
	__u16    type;
	__u8     data[];
} __packed;

#ifndef HCI_EV_SI_DEVICE
#define HCI_EV_SI_DEVICE	0x01
#endif

struct hci_ev_si_device {
	__u16    event;
	__u16    dev_id;
} __packed;

#ifndef HCI_EV_SI_SECURITY
#define HCI_EV_SI_SECURITY	0x02
#endif

struct hci_ev_si_security {
	__u16    event;
	__u16    proto;
	__u16    subproto;
	__u8     incoming;
} __packed;

#ifndef HCI_ISO_STATUS_VALID
#define HCI_ISO_STATUS_VALID	0x00
#endif

#ifndef HCI_ISO_STATUS_INVALID
#define HCI_ISO_STATUS_INVALID	0x01
#endif

#ifndef HCI_ISO_STATUS_NOP
#define HCI_ISO_STATUS_NOP	0x02
#endif

#ifndef HCI_ISO_DATA_HDR_SIZE
#define HCI_ISO_DATA_HDR_SIZE	4
#endif

struct hci_iso_data_hdr {
	__le16	sn;
	__le16	slen;
};

#ifndef HCI_ISO_TS_DATA_HDR_SIZE
#define HCI_ISO_TS_DATA_HDR_SIZE 8
#endif

struct hci_iso_ts_data_hdr {
	__le32	ts;
	__le16	sn;
	__le16	slen;
};

#ifndef hci_opcode_pack
#define hci_opcode_pack(ogf, ocf)	((__u16) ((ocf & 0x03ff)|(ogf << 10)))
#endif

#ifndef hci_opcode_ocf
#define hci_opcode_ocf(op)		(op & 0x03ff)
#endif

#ifndef hci_iso_flags_pb
#define hci_iso_flags_pb(f)		(f & 0x0003)
#endif

#ifndef hci_iso_flags_ts
#define hci_iso_flags_ts(f)		((f >> 2) & 0x0001)
#endif

#ifndef hci_iso_data_len_pack
#define hci_iso_data_len_pack(h, f)	((__u16) ((h) | ((f) << 14)))
#endif

#ifndef hci_iso_data_len
#define hci_iso_data_len(h)		((h) & 0x3fff)
#endif

#ifndef hci_iso_data_flags
#define hci_iso_data_flags(h)		((h) >> 14)
#endif

#ifndef HCI_TRANSPORT_SCO_ESCO
#define HCI_TRANSPORT_SCO_ESCO	0x01
#endif

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_EVENT_DEFS_H */
