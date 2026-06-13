/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/list.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_LIST_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_LIST_H

#include_next <linux/list.h>

#ifdef list_next_entry
#undef list_next_entry
#endif
#define list_next_entry(pos, member) \
  list_entry((pos)->member.next, typeof(*(pos)), member)

#ifndef list_for_each_entry_from
#define list_for_each_entry_from(pos, head, member) \
  for (; &pos->member != (head); \
       pos = list_next_entry(pos, member))
#endif

#endif
