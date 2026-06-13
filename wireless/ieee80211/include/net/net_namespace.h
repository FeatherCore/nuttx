#ifndef __WIRELESS_IEEE80211_INCLUDE_NET_NET_NAMESPACE_H
#define __WIRELESS_IEEE80211_INCLUDE_NET_NET_NAMESPACE_H
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include <nuttx/kmalloc.h>

struct net
{
  void *genl_sock;
  void *generic;
};
extern struct net init_net;
#ifndef __WIRELESS_IEEE80211_POSSIBLE_NET_T_DEFINED
#define __WIRELESS_IEEE80211_POSSIBLE_NET_T_DEFINED
typedef struct net *possible_net_t;
#endif
struct pernet_operations
{
  int (*init)(struct net *net);
  void (*exit)(struct net *net);
  unsigned int *id;
  size_t size;
};

#define for_each_net_rcu(net) \
  for ((net) = &init_net; (net) != NULL; (net) = NULL)

static inline void *net_generic(struct net *net, unsigned int id)
{
  (void)id;
  return net->generic;
}

static inline int register_pernet_device(struct pernet_operations *ops)
{
  if (ops != NULL && ops->size > 0)
    {
      init_net.generic = kmm_zalloc(ops->size);
      if (init_net.generic == NULL)
        {
          return -ENOMEM;
        }
    }

  if (ops != NULL && ops->id != NULL)
    {
      *ops->id = 0;
    }

  if (ops != NULL && ops->init != NULL)
    {
      return ops->init(&init_net);
    }

  return 0;
}
static inline void unregister_pernet_device(struct pernet_operations *ops)
{
  if (ops != NULL && ops->exit != NULL)
    {
      ops->exit(&init_net);
    }

  if (ops != NULL && ops->size > 0)
    {
      kmm_free(init_net.generic);
      init_net.generic = NULL;
    }
}
#endif
