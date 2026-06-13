#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_RHASHTABLE_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_RHASHTABLE_H

#include <linux/cfg80211_compat.h>

struct rhash_head
{
  struct rhash_head *next;
};

struct rhlist_head
{
  struct rhash_head rhead;
  struct rhlist_head *next;
};

struct rhashtable_params
{
  unsigned int nelem_hint;
  unsigned int max_size;
  size_t head_offset;
  size_t key_offset;
  size_t key_len;
  bool automatic_shrinking;
  u32 (*hashfn)(const void *data, u32 len, u32 seed);
};

struct rhashtable
{
  struct rhash_head *head;
  struct rhashtable_params params;
  atomic_t nelems;
};

struct rhltable
{
  struct rhashtable ht;
};

#define rht_dereference(p, ht) (p)
#define rht_dereference_protected(p, ht, cond) (p)

static inline int rhashtable_init(struct rhashtable *ht,
                                  const struct rhashtable_params *params)
{
  ht->head = NULL;
  if (params != NULL)
    {
      ht->params = *params;
    }
  else
    {
      memset(&ht->params, 0, sizeof(ht->params));
    }

  atomic_set(&ht->nelems, 0);
  return 0;
}

static inline void rhashtable_destroy(struct rhashtable *ht)
{
  ht->head = NULL;
  atomic_set(&ht->nelems, 0);
}

static inline void *rhashtable_obj(const struct rhash_head *head,
                                   const struct rhashtable_params params)
{
  return (void *)((uintptr_t)head - params.head_offset);
}

static inline bool rhashtable_key_equal(const struct rhash_head *head,
                                        const void *key,
                                        const struct rhashtable_params params)
{
  const void *obj_key;

  obj_key = (const void *)((uintptr_t)rhashtable_obj(head, params) +
                           params.key_offset);
  return memcmp(obj_key, key, params.key_len) == 0;
}

static inline int rhashtable_insert_fast(struct rhashtable *ht,
                                         struct rhash_head *obj,
                                         const struct rhashtable_params params)
{
  (void)params;
  obj->next = ht->head;
  ht->head = obj;
  atomic_inc(&ht->nelems);
  return 0;
}

static inline int rhashtable_remove_fast(struct rhashtable *ht,
                                         struct rhash_head *obj,
                                         const struct rhashtable_params params)
{
  struct rhash_head **cur = &ht->head;

  (void)params;
  while (*cur != NULL)
    {
      if (*cur == obj)
        {
          *cur = obj->next;
          obj->next = NULL;
          atomic_dec(&ht->nelems);
          return 0;
        }

      cur = &(*cur)->next;
    }

  return -ENOENT;
}

static inline void *rhashtable_lookup_fast(struct rhashtable *ht,
                                           const void *key,
                                           const struct rhashtable_params params)
{
  struct rhash_head *cur;

  for (cur = ht->head; cur != NULL; cur = cur->next)
    {
      if (rhashtable_key_equal(cur, key, params))
        {
          return rhashtable_obj(cur, params);
        }
    }

  return NULL;
}

#define rhashtable_lookup(ht, key, params) \
  rhashtable_lookup_fast(ht, key, params)

static inline void *
rhashtable_lookup_get_insert_fast(struct rhashtable *ht,
                                  struct rhash_head *obj,
                                  const struct rhashtable_params params)
{
  void *obj_ptr;
  const void *key;
  void *old;

  obj_ptr = rhashtable_obj(obj, params);
  key = (const void *)((uintptr_t)obj_ptr + params.key_offset);
  old = rhashtable_lookup_fast(ht, key, params);
  if (old != NULL)
    {
      return old;
    }

  rhashtable_insert_fast(ht, obj, params);
  return NULL;
}

static inline int
rhashtable_lookup_insert_fast(struct rhashtable *ht,
                              struct rhash_head *obj,
                              const struct rhashtable_params params)
{
  void *obj_ptr;
  const void *key;

  obj_ptr = rhashtable_obj(obj, params);
  key = (const void *)((uintptr_t)obj_ptr + params.key_offset);
  if (rhashtable_lookup_fast(ht, key, params) != NULL)
    {
      return -EEXIST;
    }

  return rhashtable_insert_fast(ht, obj, params);
}

static inline int
rhashtable_replace_fast(struct rhashtable *ht,
                        struct rhash_head *obj_old,
                        struct rhash_head *obj_new,
                        const struct rhashtable_params params)
{
  struct rhash_head **cur = &ht->head;

  (void)params;
  while (*cur != NULL)
    {
      if (*cur == obj_old)
        {
          obj_new->next = obj_old->next;
          *cur = obj_new;
          obj_old->next = NULL;
          return 0;
        }

      cur = &(*cur)->next;
    }

  return -ENOENT;
}

static inline void
rhashtable_free_and_destroy(struct rhashtable *ht,
                            void (*free_fn)(void *ptr, void *arg),
                            void *arg)
{
  struct rhash_head *cur = ht->head;

  while (cur != NULL)
    {
      struct rhash_head *next = cur->next;

      if (free_fn != NULL)
        {
          free_fn(rhashtable_obj(cur, ht->params), arg);
        }

      cur = next;
    }

  rhashtable_destroy(ht);
}

static inline int rhltable_init(struct rhltable *hlt,
                                const struct rhashtable_params *params)
{
  return rhashtable_init(&hlt->ht, params);
}

static inline void rhltable_destroy(struct rhltable *hlt)
{
  rhashtable_destroy(&hlt->ht);
}

static inline int rhltable_insert(struct rhltable *hlt,
                                  struct rhlist_head *list,
                                  const struct rhashtable_params params)
{
  list->next = NULL;
  return rhashtable_insert_fast(&hlt->ht, &list->rhead, params);
}

static inline int rhltable_remove(struct rhltable *hlt,
                                  struct rhlist_head *list,
                                  const struct rhashtable_params params)
{
  return rhashtable_remove_fast(&hlt->ht, &list->rhead, params);
}

static inline struct rhlist_head *rhltable_lookup(struct rhltable *hlt,
                                                  const void *key,
                                                  const struct rhashtable_params params)
{
  struct rhash_head *cur;
  struct rhlist_head *first = NULL;
  struct rhlist_head **tail = &first;

  for (cur = hlt->ht.head; cur != NULL; cur = cur->next)
    {
      if (rhashtable_key_equal(cur, key, params))
        {
          struct rhlist_head *entry;

          entry = (struct rhlist_head *)cur;
          *tail = entry;
          tail = &entry->next;
        }
    }

  *tail = NULL;
  return first;
}

#endif
