/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/device.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_DEVICE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_DEVICE_H

struct device_type
{
  const char *name;
};

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_DEVICE_STRUCT_DEFINED
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_DEVICE_STRUCT_DEFINED
struct device
{
  const struct device_type *type;
  void *parent;
  void *kobj;
  char name[16];
};
#endif

#endif
