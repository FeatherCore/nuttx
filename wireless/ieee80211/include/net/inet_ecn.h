#ifndef __WIRELESS_IEEE80211_INCLUDE_NET_INET_ECN_H
#define __WIRELESS_IEEE80211_INCLUDE_NET_INET_ECN_H

#include <linux/cfg80211_compat.h>

static inline int INET_ECN_set_ce(struct sk_buff *skb)
{
  (void)skb;
  return 0;
}

#endif
