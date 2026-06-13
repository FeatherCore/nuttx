/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __LINUX_BT_COMPAT_LINUX_SCATTERLIST_H
#define __LINUX_BT_COMPAT_LINUX_SCATTERLIST_H

#include <linux/types.h>

struct scatterlist
{
  void *address;
  unsigned int length;
};

static inline void sg_init_one(struct scatterlist *sg, const void *buf,
                               unsigned int buflen)
{
  sg->address = (void *)buf;
  sg->length = buflen;
}

static inline void *sg_virt(struct scatterlist *sg)
{
  return sg->address;
}

#endif /* __LINUX_BT_COMPAT_LINUX_SCATTERLIST_H */
