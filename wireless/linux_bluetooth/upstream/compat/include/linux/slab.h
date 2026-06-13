/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/slab.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SLAB_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SLAB_H

#include <stdlib.h>
#include <string.h>

#define GFP_KERNEL 0
#define GFP_ATOMIC 0

static inline void *kmalloc(size_t size, int flags)
{
  (void)flags;
  return malloc(size);
}

static inline void *kzalloc(size_t size, int flags)
{
  void *ptr;

  (void)flags;
  ptr = malloc(size);
  if (ptr != NULL)
    {
      memset(ptr, 0, size);
    }

  return ptr;
}

static inline void *kmalloc_array(size_t n, size_t size, int flags)
{
  if (size != 0 && n > SIZE_MAX / size)
    {
      return NULL;
    }

  return kmalloc(n * size, flags);
}

static inline void *kvcalloc(size_t n, size_t size, int flags)
{
  if (size != 0 && n > SIZE_MAX / size)
    {
      return NULL;
    }

  return kzalloc(n * size, flags);
}

static inline void *kmemdup(const void *src, size_t len, int flags)
{
  void *dst;

  if (src == NULL)
    {
      return NULL;
    }

  dst = kmalloc(len, flags);
  if (dst != NULL)
    {
      memcpy(dst, src, len);
    }

  return dst;
}

static inline void kfree(const void *ptr)
{
  free((void *)ptr);
}

static inline void kvfree(const void *ptr)
{
  kfree(ptr);
}

static inline void kfree_sensitive(const void *ptr)
{
  kfree(ptr);
}

#endif
