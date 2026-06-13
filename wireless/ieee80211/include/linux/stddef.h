/* SPDX-License-Identifier: Apache-2.0 */
#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_STDDEF_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_STDDEF_H

#include <stddef.h>

#ifndef offsetofend
#  define offsetofend(type, member) \
    (offsetof(type, member) + sizeof(((type *)0)->member))
#endif

#ifndef sizeof_field
#  define sizeof_field(type, member) sizeof(((type *)0)->member)
#endif

#endif /* __WIRELESS_IEEE80211_INCLUDE_LINUX_STDDEF_H */
