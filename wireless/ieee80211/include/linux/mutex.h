#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_MUTEX_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_MUTEX_H
#include <linux/cfg80211_compat.h>
#include <nuttx/mutex.h>
struct mutex
{
  mutex_t nx;
};
#define __MUTEX_INITIALIZER(name) { NXMUTEX_INITIALIZER }
#define DEFINE_MUTEX(name) struct mutex name = __MUTEX_INITIALIZER(name)
static inline void mutex_init(struct mutex *mutex)
{
  nxmutex_init(&mutex->nx);
}
static inline void mutex_lock(struct mutex *mutex)
{
  nxmutex_lock(&mutex->nx);
}
static inline void mutex_unlock(struct mutex *mutex)
{
  nxmutex_unlock(&mutex->nx);
}

static inline void mutex_destroy(struct mutex *mutex)
{
  nxmutex_destroy(&mutex->nx);
}

static inline int mutex_lock_interruptible(struct mutex *mutex)
{
  mutex_lock(mutex);
  return 0;
}
static inline bool mutex_is_locked(struct mutex *mutex)
{
  return nxmutex_is_hold(&mutex->nx);
}
#endif
