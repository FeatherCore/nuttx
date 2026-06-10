/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __WIRELESS_IEEE80211_INCLUDE_NET_BLUETOOTH_HCI_CORE_H
#define __WIRELESS_IEEE80211_INCLUDE_NET_BLUETOOTH_HCI_CORE_H

#include <net/bluetooth/bluetooth.h>

struct hci_dev_stats
{
  unsigned int err_rx;
};

struct hci_dev
{
  struct hci_dev_stats stat;
};

static inline int hci_recv_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
  (void)hdev;
  dev_kfree_skb_any(skb);
  return 0;
}

#endif /* __WIRELESS_IEEE80211_INCLUDE_NET_BLUETOOTH_HCI_CORE_H */
