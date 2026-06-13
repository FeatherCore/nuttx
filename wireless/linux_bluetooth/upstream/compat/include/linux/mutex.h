#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_MUTEX_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_MUTEX_H

struct mutex { int locked; };

#define DEFINE_MUTEX(name) struct mutex name = { 0 }
static inline void mutex_init(struct mutex *mutex) { mutex->locked = 0; }
static inline void mutex_lock(struct mutex *mutex) { (void)mutex; }
static inline void mutex_lock_nested(struct mutex *mutex, int subclass)
{
  (void)subclass;
  mutex_lock(mutex);
}
static inline void mutex_unlock(struct mutex *mutex) { (void)mutex; }

#endif
