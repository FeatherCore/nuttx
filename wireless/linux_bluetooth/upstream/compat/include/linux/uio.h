/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_UIO_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_UIO_H

#include <linux/fs.h>
#include <linux/types.h>

#ifndef ITER_SOURCE
#define ITER_SOURCE 1
#endif

#ifndef ITER_DEST
#define ITER_DEST 0
#endif

struct kvec
{
  void *iov_base;
  size_t iov_len;
};

static inline void iov_iter_kvec(struct iov_iter *iter,
                                 unsigned int direction,
                                 const struct kvec *kvec,
                                 unsigned long nr_segs,
                                 size_t count)
{
  (void)direction;
  (void)nr_segs;

  if (iter != NULL)
    {
      iter->data = kvec != NULL ? kvec[0].iov_base : NULL;
      iter->count = count;
    }
}

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_UIO_H */
