#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_ATOMIC_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_ATOMIC_H

#include <nuttx/atomic.h>

#include <linux/types.h>

#ifndef ATOMIC_INIT
#  define ATOMIC_INIT(i) (i)
#endif

#ifndef atomic_inc
#  define atomic_inc(v) \
    do { atomic_fetch_add((v), 1); } while (0)
#endif

#ifndef atomic_dec
#  define atomic_dec(v) \
    do { atomic_fetch_sub((v), 1); } while (0)
#endif

#ifndef atomic_dec_and_test
#  define atomic_dec_and_test(v) (atomic_fetch_sub((v), 1) == 1)
#endif

#ifndef atomic_inc_return
#  define atomic_inc_return(v) (atomic_fetch_add((v), 1) + 1)
#endif

#ifndef atomic_add_return
#  define atomic_add_return(i, v) (atomic_fetch_add((v), (i)) + (i))
#endif

#endif
