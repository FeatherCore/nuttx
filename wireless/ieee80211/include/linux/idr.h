#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_IDR_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_IDR_H

#include <linux/cfg80211_compat.h>

struct idr_entry
{
  int idr_id;
  void *ptr;
  struct idr_entry *next;
};

struct idr
{
  struct idr_entry *head;
};

#define IDR_INIT(name) { NULL }
#define DEFINE_IDR(name) struct idr name = IDR_INIT(name)

#ifndef __WIRELESS_IEEE80211_IDA_DEFINED
#define __WIRELESS_IEEE80211_IDA_DEFINED
struct ida
{
  int next;
};

#define IDA_INIT(name) { 0 }
#define DEFINE_IDA(name) struct ida name = IDA_INIT(name)
#endif

#define idr_for_each_entry(idr, entry, id)                            \
  for (struct idr_entry *__idr_iter = (idr)->head, *__idr_next = NULL; \
       __idr_iter != NULL &&                                          \
       ((__idr_next = __idr_iter->next),                              \
        (id) = __idr_iter->idr_id,                                    \
        (entry) = __idr_iter->ptr, 1);                                \
       __idr_iter = __idr_next)

static inline void idr_init(struct idr *idr)
{
  idr->head = NULL;
}

static inline void idr_destroy(struct idr *idr)
{
  struct idr_entry *entry;
  struct idr_entry *next;

  for (entry = idr->head; entry != NULL; entry = next)
    {
      next = entry->next;
      kfree(entry);
    }

  idr->head = NULL;
}

static inline struct idr_entry *idr_find_entry(const struct idr *idr,
                                               int id)
{
  struct idr_entry *entry;

  for (entry = idr->head; entry != NULL; entry = entry->next)
    {
      if (entry->idr_id == id)
        {
          return entry;
        }
    }

  return NULL;
}

static inline int idr_alloc(struct idr *idr, void *ptr, int start, int end,
                            gfp_t gfp)
{
  struct idr_entry *entry;
  int id;

  (void)gfp;

  if (start < 0)
    {
      start = 0;
    }

  for (id = start; end <= 0 || id < end; id++)
    {
      if (idr_find_entry(idr, id) == NULL)
        {
          break;
        }
    }

  if (end > 0 && id >= end)
    {
      return -ENOSPC;
    }

  entry = kzalloc(sizeof(*entry), gfp);
  if (entry == NULL)
    {
      return -ENOMEM;
    }

  entry->idr_id = id;
  entry->ptr = ptr;
  entry->next = idr->head;
  idr->head = entry;

  return id;
}

static inline void *idr_find(const struct idr *idr, unsigned long id)
{
  struct idr_entry *entry;

  entry = idr_find_entry(idr, (int)id);
  return entry != NULL ? entry->ptr : NULL;
}

static inline void *idr_remove(struct idr *idr, unsigned long id)
{
  struct idr_entry **prev;
  struct idr_entry *entry;
  void *ptr;

  for (prev = &idr->head; *prev != NULL; prev = &(*prev)->next)
    {
      entry = *prev;
      if (entry->idr_id == (int)id)
        {
          ptr = entry->ptr;
          *prev = entry->next;
          kfree(entry);
          return ptr;
        }
    }

  return NULL;
}

static inline int idr_for_each(struct idr *idr,
                               int (*fn)(int id, void *p, void *data),
                               void *data)
{
  struct idr_entry *entry;
  struct idr_entry *next;
  int ret;

  for (entry = idr->head; entry != NULL; entry = next)
    {
      next = entry->next;
      ret = fn(entry->idr_id, entry->ptr, data);
      if (ret != 0)
        {
          return ret;
        }
    }

  return 0;
}

#endif
