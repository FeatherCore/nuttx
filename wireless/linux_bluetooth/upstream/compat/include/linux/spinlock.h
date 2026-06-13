#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SPINLOCK_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SPINLOCK_H

#define spinlock_t linux_bt_spinlock_t
#define rwlock_t linux_bt_rwlock_t

typedef int linux_bt_spinlock_t;
typedef int linux_bt_rwlock_t;

#define DEFINE_SPINLOCK(name) spinlock_t name = 0
#define DEFINE_RWLOCK(name) rwlock_t name = 0
#define __RW_LOCK_UNLOCKED(name) 0
#define spin_lock_init(lock) do { *(lock) = 0; } while (0)
#define spin_lock(lock) do { (void)(lock); } while (0)
#define spin_unlock(lock) do { (void)(lock); } while (0)
#define spin_lock_bh(lock) spin_lock(lock)
#define spin_unlock_bh(lock) spin_unlock(lock)
#define spin_lock_irqsave(lock, flags) do { (void)(lock); (flags) = 0; } while (0)
#define spin_unlock_irqrestore(lock, flags) do { (void)(lock); (void)(flags); } while (0)

#define rwlock_init(lock) do { *(lock) = 0; } while (0)
#define read_lock(lock) do { (void)(lock); } while (0)
#define read_unlock(lock) do { (void)(lock); } while (0)
#define write_lock(lock) do { (void)(lock); } while (0)
#define write_unlock(lock) do { (void)(lock); } while (0)

#endif
