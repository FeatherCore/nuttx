#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_NETLINK_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_NETLINK_H
#include_next <linux/netlink.h>
#include <linux/cfg80211_compat.h>
#define NETLINK_CTX_SIZE 48
struct netlink_skb_parms
{
  u32 portid;
  u32 dst_group;
  u32 flags;
  u8 ctx[NETLINK_CTX_SIZE];
};
#define NETLINK_CB(skb) (*(struct netlink_skb_parms *)&((skb)->cb))
static inline int netlink_set_err(struct sock *ssk, u32 portid,
                                  u32 group, int code)
{
  (void)ssk;
  (void)portid;
  (void)group;
  return code;
}
static inline bool netlink_has_listeners(struct sock *ssk, unsigned int group)
{
  (void)ssk;
  (void)group;
  return false;
}
#endif
