#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_U64_STATS_SYNC_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_U64_STATS_SYNC_H

#include <linux/cfg80211_compat.h>

struct u64_stats_sync
{
  int dummy;
};

typedef struct
{
  u64 v;
} u64_stats_t;

static inline void u64_stats_init(struct u64_stats_sync *syncp)
{
  syncp->dummy = 0;
}

static inline void u64_stats_update_begin(struct u64_stats_sync *syncp)
{
  (void)syncp;
}

static inline void u64_stats_update_end(struct u64_stats_sync *syncp)
{
  (void)syncp;
}

static inline unsigned int u64_stats_fetch_begin(const struct u64_stats_sync *syncp)
{
  (void)syncp;
  return 0;
}

static inline bool u64_stats_fetch_retry(const struct u64_stats_sync *syncp,
                                         unsigned int start)
{
  (void)syncp;
  (void)start;
  return false;
}

static inline u64 u64_stats_read(const u64_stats_t *p)
{
  return p->v;
}

static inline void u64_stats_set(u64_stats_t *p, u64 val)
{
  p->v = val;
}

static inline void u64_stats_add(u64_stats_t *p, u64 val)
{
  p->v += val;
}

static inline void u64_stats_inc(u64_stats_t *p)
{
  p->v++;
}

#endif
