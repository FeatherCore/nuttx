/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/rcupdate.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_RCUPDATE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_RCUPDATE_H

#include <linux/slab.h>

struct rcu_head
{
  int unused;
};

#define rcu_read_lock() do { } while (0)
#define rcu_read_unlock() do { } while (0)
#define synchronize_rcu() do { } while (0)
#define kfree_rcu(ptr, member) kfree(ptr)

#endif
