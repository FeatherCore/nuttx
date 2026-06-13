/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __WIRELESS_IEEE80211_INCLUDE_NET_BLUETOOTH_BLUETOOTH_H
#define __WIRELESS_IEEE80211_INCLUDE_NET_BLUETOOTH_BLUETOOTH_H

#include <linux/skbuff.h>

struct bt_skb_cb
{
  u8 pkt_type;
};

static inline struct bt_skb_cb *bt_cb(struct sk_buff *skb)
{
  return (struct bt_skb_cb *)skb->cb;
}

#ifndef HCI_BREDR
#  define HCI_BREDR 0
#endif

#ifndef HCI_PRIMARY
#  define HCI_PRIMARY HCI_BREDR
#endif

#ifndef hci_skb_pkt_type
#  define hci_skb_pkt_type(skb) (bt_cb(skb)->pkt_type)
#endif

#endif /* __WIRELESS_IEEE80211_INCLUDE_NET_BLUETOOTH_BLUETOOTH_H */
