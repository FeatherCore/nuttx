/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/rfkill.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_RFKILL_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_RFKILL_H

#include <linux/types.h>

struct device;

struct rfkill
{
  const char *name;
  void *data;
  const struct rfkill_ops *ops;
  bool blocked;
};

enum rfkill_type
{
  RFKILL_TYPE_BLUETOOTH,
};

struct rfkill_ops
{
  int (*set_block)(void *data, bool blocked);
};

static inline struct rfkill *rfkill_alloc(const char *name,
                                          struct device *parent,
                                          enum rfkill_type type,
                                          const struct rfkill_ops *ops,
                                          void *data)
{
  static struct rfkill rfkill;

  (void)parent;
  (void)type;
  rfkill.name = name;
  rfkill.ops = ops;
  rfkill.data = data;
  rfkill.blocked = false;
  return &rfkill;
}

static inline int rfkill_register(struct rfkill *rfkill)
{
  (void)rfkill;
  return 0;
}

static inline void rfkill_unregister(struct rfkill *rfkill)
{
  (void)rfkill;
}

static inline void rfkill_destroy(struct rfkill *rfkill)
{
  (void)rfkill;
}

static inline bool rfkill_blocked(struct rfkill *rfkill)
{
  return rfkill != NULL ? rfkill->blocked : false;
}

#endif
