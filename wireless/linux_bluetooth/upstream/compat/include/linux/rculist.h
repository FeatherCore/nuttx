#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_RCULIST_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_RCULIST_H
#include <linux/list.h>
#define list_for_each_entry_rcu(pos, head, member) list_for_each_entry(pos, head, member)
#endif
