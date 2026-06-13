#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_REFCOUNT_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_REFCOUNT_H

#include <linux/atomic.h>

typedef atomic_t refcount_t;
#define REFCOUNT_INIT(n) ATOMIC_INIT(n)
static inline void refcount_set(refcount_t *r, int n) { atomic_set(r, n); }
static inline void refcount_inc(refcount_t *r) { atomic_inc(r); }
static inline int refcount_dec_and_test(refcount_t *r) { return atomic_dec_and_test(r); }
static inline int refcount_read(const refcount_t *r) { return atomic_read(r); }

#endif
