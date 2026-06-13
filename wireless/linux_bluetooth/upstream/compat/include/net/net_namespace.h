/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/net/net_namespace.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_NET_NAMESPACE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_NET_NAMESPACE_H

#include <net/sock.h>

typedef struct net *possible_net_t;

static inline void *net_generic(struct net *net, unsigned int id)
{
  (void)net;
  (void)id;
  return NULL;
}

#define for_each_net_rcu(net) \
  for ((net) = &init_net; (net) != NULL; (net) = NULL)

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_NET_NAMESPACE_H */
