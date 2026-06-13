#ifndef __WIRELESS_IEEE80211_INCLUDE_NET_GSO_H
#define __WIRELESS_IEEE80211_INCLUDE_NET_GSO_H

#include <linux/cfg80211_compat.h>

static inline bool skb_is_gso(const struct sk_buff *skb)
{
  (void)skb;
  return false;
}

static inline struct sk_buff *skb_gso_segment(struct sk_buff *skb,
                                              netdev_features_t features)
{
  (void)features;
  return skb;
}

#endif
