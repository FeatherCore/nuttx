#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_MODULE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_MODULE_H
#include <linux/export.h>
#include <linux/init.h>
#define MODULE_AUTHOR(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_VERSION(x)
#define MODULE_LICENSE(x)
#define MODULE_ALIAS(x)
#define MODULE_ALIAS_MISCDEV(x)
#define THIS_MODULE NULL
#define module_param(name, type, perm)
#define MODULE_PARM_DESC(name, desc)
#define module_init(fn) \
  int linux_bt_compat_module_init_##fn(void) { return fn(); }
#define module_exit(fn) \
  void linux_bt_compat_module_exit_##fn(void) { fn(); }
#define subsys_initcall(fn) \
  int linux_bt_compat_subsys_initcall_##fn(void) { return fn(); }
#define MODULE_ALIAS_NETPROTO(proto)
#define try_module_get(module) 1
#define __module_get(module) do { (void)(module); } while (0)
#define module_put(module) do { (void)(module); } while (0)
#define request_module(fmt, ...) do { (void)(fmt); } while (0)
#endif
