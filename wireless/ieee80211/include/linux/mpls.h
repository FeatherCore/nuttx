#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_MPLS_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_MPLS_H
#include <linux/cfg80211_compat.h>
struct mpls_label
{
  __be32 entry;
};
#define MPLS_LS_TC_MASK 0x00000e00
#define MPLS_LS_TC_SHIFT 9
#endif
