#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_SCATTERLIST_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_SCATTERLIST_H

#include <linux/cfg80211_compat.h>

struct scatterlist
{
  void *buf;
  unsigned int length;
};

static inline void sg_init_table(struct scatterlist *sg, unsigned int nents)
{
  memset(sg, 0, sizeof(*sg) * nents);
}

static inline void sg_set_buf(struct scatterlist *sg, const void *buf,
                              unsigned int buflen)
{
  sg->buf = (void *)buf;
  sg->length = buflen;
}

static inline void sg_init_one(struct scatterlist *sg, const void *buf,
                               unsigned int buflen)
{
  sg_set_buf(sg, buf, buflen);
}

#endif
