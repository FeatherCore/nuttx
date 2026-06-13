/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/net/addrconf.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_ADDRCONF_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_ADDRCONF_H

#include <net/ipv6.h>

struct inet6_dev;

static inline void addrconf_add_linklocal(struct inet6_dev *idev,
                                          const struct in6_addr *addr,
                                          int flags)
{
  (void)idev;
  (void)addr;
  (void)flags;
}

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_ADDRCONF_H */
