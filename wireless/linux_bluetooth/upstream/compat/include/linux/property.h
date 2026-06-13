/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/property.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_PROPERTY_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_PROPERTY_H

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct device;
struct fwnode_handle;

static inline bool device_property_read_bool(const struct device *dev,
                                             const char *propname)
{
  (void)dev;
  (void)propname;
  return false;
}

static inline struct fwnode_handle *dev_fwnode(const struct device *dev)
{
  (void)dev;
  return NULL;
}

static inline int fwnode_property_read_u8_array(
                                const struct fwnode_handle *fwnode,
                                const char *propname, uint8_t *val,
                                size_t nval)
{
  (void)fwnode;
  (void)propname;
  (void)val;
  (void)nval;
  return -ENOENT;
}

#endif
