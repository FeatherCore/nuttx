#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_MISCDEVICE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_MISCDEVICE_H

#include <linux/fs.h>

#define MISC_DYNAMIC_MINOR 255
#define VHCI_MINOR 224

struct miscdevice
{
  int minor;
  const char *name;
  const struct file_operations *fops;
};

int misc_register(struct miscdevice *misc);
void misc_deregister(struct miscdevice *misc);

#define module_misc_device(misc) \
  int linux_bt_compat_module_misc_register_##misc(void) \
  { \
    return misc_register(&(misc)); \
  }

#endif
