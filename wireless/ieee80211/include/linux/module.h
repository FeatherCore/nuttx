#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_MODULE_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_MODULE_H
#include <linux/cfg80211_compat.h>
struct module
{
  int dummy;
};
#define THIS_MODULE NULL
#define MODULE_ALIAS(x)
#define MODULE_ALIAS_NET_PF_PROTO_NAME(pf, proto, name)
#define module_param(name, type, perm)
#define MODULE_PARM_DESC(name, desc)
#define module_init(fn)
#define module_exit(fn) \
  static void (*__module_exit_##fn)(void) __maybe_unused = fn
#define MODULE_DEVICE_TABLE(type, name)
#define MODULE_LICENSE(x)
#define MODULE_AUTHOR(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_VERSION(x)
#endif
