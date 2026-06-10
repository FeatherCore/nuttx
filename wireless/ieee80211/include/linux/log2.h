#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_LOG2_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_LOG2_H

#include <linux/cfg80211_compat.h>

static inline int ilog2(unsigned long n)
{
  int log = -1;

  while (n)
    {
      n >>= 1;
      log++;
    }

  return log;
}

#endif
