#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_WAIT_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_WAIT_H

#include <linux/cfg80211_compat.h>

#ifndef wait_event_interruptible_timeout
#  define wait_event_interruptible_timeout(wq, condition, timeout) \
    ({ \
      (void)(wq); \
      (void)(timeout); \
      (condition) ? 1 : 0; \
    })
#endif

#ifndef wake_up_interruptible
#  define wake_up_interruptible(head) wake_up(head)
#endif

#endif /* __WIRELESS_IEEE80211_INCLUDE_LINUX_WAIT_H */
