#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_SOCK_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_HCI_SOCK_H

#include <linux/types.h>
#include <linux/init.h>
#include <linux/unaligned.h>

#include <net/bluetooth/hci.h>

#ifndef HCI_DATA_DIR
#define HCI_DATA_DIR    1
#endif
#ifndef HCI_FILTER
#define HCI_FILTER      2
#endif
#ifndef HCI_TIME_STAMP
#define HCI_TIME_STAMP  3
#endif

#ifndef HCI_CMSG_DIR
#define HCI_CMSG_DIR    0x01
#endif
#ifndef HCI_CMSG_TSTAMP
#define HCI_CMSG_TSTAMP 0x02
#endif

#ifndef HCI_DEV_NONE
#define HCI_DEV_NONE 0xffff
#endif

#ifndef HCI_CHANNEL_RAW
#define HCI_CHANNEL_RAW     0
#endif
#ifndef HCI_CHANNEL_USER
#define HCI_CHANNEL_USER    1
#endif
#ifndef HCI_CHANNEL_MONITOR
#define HCI_CHANNEL_MONITOR 2
#endif
#ifndef HCI_CHANNEL_CONTROL
#define HCI_CHANNEL_CONTROL 3
#endif
#ifndef HCI_CHANNEL_LOGGING
#define HCI_CHANNEL_LOGGING 4
#endif

struct hci_ufilter
{
  __u32 type_mask;
  __u32 event_mask[2];
  __le16 opcode;
};

#define HCI_FLT_TYPE_BITS  31
#define HCI_FLT_EVENT_BITS 63
#define HCI_FLT_OGF_BITS   63
#define HCI_FLT_OCF_BITS   127

#ifndef HCIINQUIRY
#define HCIINQUIRY 0x800448f0
#endif

enum
{
  HCI_SOCK_TRUSTED,
  HCI_MGMT_INDEX_EVENTS,
  HCI_MGMT_UNCONF_INDEX_EVENTS,
  HCI_MGMT_EXT_INDEX_EVENTS,
  HCI_MGMT_OPTION_EVENTS,
  HCI_MGMT_SETTING_EVENTS,
  HCI_MGMT_DEV_CLASS_EVENTS,
  HCI_MGMT_LOCAL_NAME_EVENTS,
  HCI_MGMT_OOB_DATA_EVENTS,
  HCI_MGMT_EXP_FEATURE_EVENTS,
  HCI_MGMT_EXT_INFO_EVENTS,
};

#endif
