/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __LINUX_BT_COMPAT_LINUX_KREF_H
#define __LINUX_BT_COMPAT_LINUX_KREF_H

#include <linux/atomic.h>

struct kref
{
  atomic_t refcount;
};

static inline void kref_init(struct kref *kref)
{
  atomic_set(&kref->refcount, 1);
}

static inline void kref_get(struct kref *kref)
{
  atomic_inc(&kref->refcount);
}

static inline unsigned int kref_read(const struct kref *kref)
{
  return (unsigned int)atomic_read(&kref->refcount);
}

static inline int kref_get_unless_zero(struct kref *kref)
{
  if (kref_read(kref) == 0)
    {
      return 0;
    }

  kref_get(kref);
  return 1;
}

static inline int kref_put(struct kref *kref, void (*release)(struct kref *kref))
{
  if (atomic_dec_and_test(&kref->refcount))
    {
      release(kref);
      return 1;
    }

  return 0;
}

#endif /* __LINUX_BT_COMPAT_LINUX_KREF_H */
