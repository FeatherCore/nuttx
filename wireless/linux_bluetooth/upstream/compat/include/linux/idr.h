#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_IDR_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_IDR_H

#include <linux/slab.h>

#include <limits.h>

struct idr { int unused; };
struct ida { int unused; };
#define DEFINE_IDR(name) struct idr name = { 0 }
#define DEFINE_IDA(name) struct ida name = { 0 }
static inline void idr_init(struct idr *idr) { (void)idr; }
static inline void idr_destroy(struct idr *idr) { (void)idr; }
static inline bool idr_is_empty(struct idr *idr) { (void)idr; return true; }
static inline void ida_init(struct ida *ida) { (void)ida; }
static inline void ida_destroy(struct ida *ida) { (void)ida; }
static inline int ida_alloc_max(struct ida *ida, unsigned int max, gfp_t gfp)
{
  (void)ida;
  (void)max;
  (void)gfp;
  return 0;
}

static inline int ida_alloc_range(struct ida *ida, unsigned int min,
                                  unsigned int max, gfp_t gfp)
{
  (void)ida;
  (void)max;
  (void)gfp;
  return (int)min;
}

static inline int ida_alloc_min(struct ida *ida, unsigned int min, gfp_t gfp)
{
  return ida_alloc_range(ida, min, UINT_MAX, gfp);
}

static inline void ida_free(struct ida *ida, unsigned int id)
{
  (void)ida;
  (void)id;
}

#define idr_for_each_entry(idr, entry, id) \
  for (entry = NULL, id = 0; entry != NULL; entry = NULL)

#endif
