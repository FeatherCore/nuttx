#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_LOCKDEP_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_LOCKDEP_H
#include <linux/cfg80211_compat.h>
#define lockdep_assert_held(lock) do { (void)(lock); } while (0)
#endif
