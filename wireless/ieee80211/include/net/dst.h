#ifndef __WIRELESS_IEEE80211_INCLUDE_NET_DST_H
#define __WIRELESS_IEEE80211_INCLUDE_NET_DST_H

#include <linux/skbuff.h>

static inline void skb_dst_drop(struct sk_buff *skb)
{
  (void)skb;
}

#endif /* __WIRELESS_IEEE80211_INCLUDE_NET_DST_H */
