#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_KTIME_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_KTIME_H

#include <linux/cfg80211_compat.h>

#ifndef __WIRELESS_IEEE80211_KTIME_HELPERS_DEFINED
#define __WIRELESS_IEEE80211_KTIME_HELPERS_DEFINED
static inline u64 ktime_get_ns(void)
{
  return 0;
}

static inline ktime_t ktime_get_boottime(void)
{
  return 0;
}

static inline ktime_t us_to_ktime(u64 usecs)
{
  return (ktime_t)usecs * 1000;
}
#endif

#endif
