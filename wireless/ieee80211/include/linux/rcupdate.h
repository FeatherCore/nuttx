#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_RCUPDATE_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_RCUPDATE_H
#include <linux/cfg80211_compat.h>
#define rcu_read_lock()
#define rcu_read_unlock()
#define rcu_dereference(p) (p)
#define rcu_access_pointer(p) (p)
#define rcu_dereference_protected(p, c) (p)
#define rcu_assign_pointer(p, v) do { (p) = (v); } while (0)
#define RCU_INIT_POINTER(p, v) do { (p) = (v); } while (0)
#define synchronize_rcu()
#define synchronize_net() synchronize_rcu()
struct rcu_head
{
  void (*func)(struct rcu_head *head);
};
static inline void call_rcu(struct rcu_head *head,
                            void (*func)(struct rcu_head *head))
{
  if (func)
    {
      func(head);
    }
}
#define kfree_rcu(ptr, member) kfree(ptr)
#endif
