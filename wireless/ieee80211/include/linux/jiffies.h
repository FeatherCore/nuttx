#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_JIFFIES_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_JIFFIES_H
#include <linux/cfg80211_compat.h>
#ifndef __WIRELESS_IEEE80211_JIFFIES_HELPERS_DEFINED
#define __WIRELESS_IEEE80211_JIFFIES_HELPERS_DEFINED
static inline unsigned long msecs_to_jiffies(unsigned int m)
{
  return (unsigned long)m * HZ / 1000;
}
static inline unsigned long usecs_to_jiffies(unsigned int u)
{
  return (unsigned long)u * HZ / 1000000;
}
static inline unsigned int jiffies_to_msecs(unsigned long j)
{
  return (unsigned int)(j * 1000 / HZ);
}
static inline unsigned long round_jiffies_up(unsigned long j)
{
  return j;
}

static inline unsigned long round_jiffies(unsigned long j)
{
  return j;
}
#endif
#endif
