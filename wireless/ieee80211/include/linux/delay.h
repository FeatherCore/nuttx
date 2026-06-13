#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_DELAY_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_DELAY_H

#include <nuttx/signal.h>

#include <linux/cfg80211_compat.h>

static inline void udelay(unsigned long usecs)
{
  nxsig_usleep(usecs);
}

static inline void mdelay(unsigned long msecs)
{
  nxsig_usleep(msecs * 1000);
}

static inline void msleep(unsigned int msecs)
{
  nxsig_usleep(msecs * 1000);
}

#endif
