#ifndef __WIRELESS_IEEE80211_INCLUDE_NET_DROPREASON_H
#define __WIRELESS_IEEE80211_INCLUDE_NET_DROPREASON_H

#include <linux/cfg80211_compat.h>

enum skb_drop_reason
{
  SKB_NOT_DROPPED_YET = 0,
  SKB_CONSUMED = 1,
  SKB_DROP_REASON_NOT_SPECIFIED = 2,
};

#define SKB_DROP_REASON_SUBSYS_SHIFT 16
#define SKB_DROP_REASON_SUBSYS_MASK 0xffff0000U
#define SKB_DROP_REASON_SUBSYS_MAC80211_UNUSABLE 1

struct drop_reason_list
{
  const char * const *reasons;
  size_t n_reasons;
};

static inline void drop_reasons_register_subsys(unsigned int subsys,
                                                struct drop_reason_list *list)
{
  (void)subsys;
  (void)list;
}

static inline void drop_reasons_unregister_subsys(unsigned int subsys)
{
  (void)subsys;
}

#endif
