#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_BUG_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_BUG_H
#include <linux/cfg80211_compat.h>
#define BUG() DEBUGPANIC()
#define BUG_ON(cond) do { if (cond) { DEBUGPANIC(); } } while (0)
#define WARN_ON(cond) ({ bool __ret = !!(cond); __ret; })
#define WARN_ON_ONCE(cond) WARN_ON(cond)
#define WARN(cond, fmt, args...) WARN_ON(cond)
#endif
