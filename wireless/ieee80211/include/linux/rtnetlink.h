#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_RTNETLINK_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_RTNETLINK_H
#include <linux/cfg80211_compat.h>
struct rtgenmsg
{
  unsigned char rtgen_family;
};

struct ifinfomsg
{
  unsigned char ifi_family;
  unsigned char __ifi_pad;
  unsigned short ifi_type;
  int ifi_index;
  unsigned int ifi_flags;
  unsigned int ifi_change;
};

static inline void rtnl_lock(void)
{
}
static inline void rtnl_unlock(void)
{
}
static inline bool rtnl_is_locked(void)
{
  return true;
}
#define ASSERT_RTNL() do { } while (0)
#define rcu_dereference_rtnl(p) (p)
#endif
