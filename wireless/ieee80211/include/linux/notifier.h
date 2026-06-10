#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_NOTIFIER_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_NOTIFIER_H
#include <linux/cfg80211_compat.h>
#ifndef __WIRELESS_IEEE80211_NOTIFIER_BLOCK_DEFINED
#define __WIRELESS_IEEE80211_NOTIFIER_BLOCK_DEFINED
struct notifier_block
{
  int (*notifier_call)(struct notifier_block *nb,
                       unsigned long action,
                       void *data);
};
#endif
#define NOTIFY_DONE 0
#define NOTIFY_OK 1
static inline int notifier_from_errno(int err)
{
  return err < 0 ? err : NOTIFY_OK;
}
#endif
