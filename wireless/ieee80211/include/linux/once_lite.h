#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_ONCE_LITE_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_ONCE_LITE_H

#include <linux/cfg80211_compat.h>

#define DO_ONCE_LITE(func, args...) func(args)

#endif
