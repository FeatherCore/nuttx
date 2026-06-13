#ifndef __WIRELESS_LINUX_COMPAT_INCLUDE_LINUX_LIST_H
#define __WIRELESS_LINUX_COMPAT_INCLUDE_LINUX_LIST_H

#include <linux/kernel.h>

#ifndef __WIRELESS_LINUX_COMPAT_LIST_TYPES_DEFINED
#define __WIRELESS_LINUX_COMPAT_LIST_TYPES_DEFINED
struct list_head
{
  struct list_head *next;
  struct list_head *prev;
};

struct hlist_node
{
  struct hlist_node *next;
  struct hlist_node **pprev;
};

struct hlist_head
{
  struct hlist_node *first;
};
#endif

#ifndef __WIRELESS_LINUX_COMPAT_LIST_HELPERS_DEFINED
#define __WIRELESS_LINUX_COMPAT_LIST_HELPERS_DEFINED

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)
#define HLIST_HEAD_INIT { NULL }
#define HLIST_HEAD(name) struct hlist_head name = HLIST_HEAD_INIT

static inline void INIT_LIST_HEAD(struct list_head *list)
{
  list->next = list;
  list->prev = list;
}

static inline void __list_add(struct list_head *newent,
                              struct list_head *prev,
                              struct list_head *next)
{
  next->prev = newent;
  newent->next = next;
  newent->prev = prev;
  prev->next = newent;
}

static inline void list_add(struct list_head *newent, struct list_head *head)
{
  __list_add(newent, head, head->next);
}

static inline void list_add_tail(struct list_head *newent,
                                 struct list_head *head)
{
  __list_add(newent, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
  next->prev = prev;
  prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
  __list_del(entry->prev, entry->next);
  entry->next = NULL;
  entry->prev = NULL;
}

static inline void list_del_init(struct list_head *entry)
{
  list_del(entry);
  INIT_LIST_HEAD(entry);
}

static inline int list_empty(const struct list_head *head)
{
  return head->next == head;
}

static inline void list_move_tail(struct list_head *entry,
                                  struct list_head *head)
{
  list_del(entry);
  list_add_tail(entry, head);
}

static inline void list_splice_tail(struct list_head *list,
                                    struct list_head *head)
{
  if (!list_empty(list))
    {
      struct list_head *first = list->next;
      struct list_head *last = list->prev;

      first->prev = head->prev;
      head->prev->next = first;
      last->next = head;
      head->prev = last;
    }
}

#define list_add_rcu(newent, head) list_add(newent, head)
#define list_add_tail_rcu(newent, head) list_add_tail(newent, head)
#define list_del_rcu(entry) list_del(entry)

static inline void INIT_HLIST_HEAD(struct hlist_head *h)
{
  h->first = NULL;
}

static inline void INIT_HLIST_NODE(struct hlist_node *h)
{
  h->next = NULL;
  h->pprev = NULL;
}

static inline int hlist_empty(const struct hlist_head *h)
{
  return !h->first;
}

static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
  n->next = h->first;
  if (h->first)
    {
      h->first->pprev = &n->next;
    }

  h->first = n;
  n->pprev = &h->first;
}

static inline void hlist_del(struct hlist_node *n)
{
  if (n->pprev)
    {
      *n->pprev = n->next;
      if (n->next)
        {
          n->next->pprev = n->pprev;
        }
    }

  INIT_HLIST_NODE(n);
}

static inline void hlist_del_init(struct hlist_node *n)
{
  hlist_del(n);
}

#define hlist_add_head_rcu(n, h) hlist_add_head(n, h)
#define hlist_del_rcu(n) hlist_del(n)
#define hlist_entry(ptr, type, member) container_of(ptr, type, member)
#define hlist_for_each_entry(pos, head, member) \
  for (pos = (head)->first ? hlist_entry((head)->first, typeof(*pos), member) : NULL; \
       pos; \
       pos = pos->member.next ? hlist_entry(pos->member.next, typeof(*pos), member) : NULL)
#define hlist_for_each_entry_rcu(pos, head, member, args...) \
  hlist_for_each_entry(pos, head, member)
#define hlist_for_each_entry_safe(pos, n, head, member) \
  for (pos = (head)->first ? hlist_entry((head)->first, typeof(*pos), member) : NULL, \
       n = pos ? pos->member.next : NULL; \
       pos; \
       pos = n ? hlist_entry(n, typeof(*pos), member) : NULL, \
       n = pos ? pos->member.next : NULL)

static inline void list_splice_tail_init(struct list_head *list,
                                         struct list_head *head)
{
  if (!list_empty(list))
    {
      list_splice_tail(list, head);
      INIT_LIST_HEAD(list);
    }
}

static inline void list_splice_init(struct list_head *list,
                                    struct list_head *head)
{
  if (!list_empty(list))
    {
      struct list_head *first = list->next;
      struct list_head *last = list->prev;

      first->prev = head;
      last->next = head->next;
      head->next->prev = last;
      head->next = first;
      INIT_LIST_HEAD(list);
    }
}

#define list_entry(ptr, type, member) container_of(ptr, type, member)
#define list_first_entry(ptr, type, member) \
  list_entry((ptr)->next, type, member)
#define list_first_entry_or_null(ptr, type, member) \
  (list_empty(ptr) ? NULL : list_first_entry(ptr, type, member))
#define list_first_or_null_rcu(ptr, type, member) \
  list_first_entry_or_null(ptr, type, member)
#define list_last_entry(ptr, type, member) \
  list_entry((ptr)->prev, type, member)
#define list_prepare_entry(pos, head, member) \
  ((pos) ? (pos) : list_entry(head, typeof(*pos), member))
#define list_next_entry(pos, member) \
  list_entry((pos)->member.next, typeof(*(pos)), member)
#define list_for_each_entry(pos, head, member) \
  for (pos = list_entry((head)->next, typeof(*pos), member); \
       &pos->member != (head); \
       pos = list_entry(pos->member.next, typeof(*pos), member))
#define list_for_each_entry_continue(pos, head, member) \
  for (pos = list_next_entry(pos, member); \
       &pos->member != (head); \
       pos = list_next_entry(pos, member))
#define list_for_each_entry_continue_reverse(pos, head, member) \
  for (pos = list_entry((pos)->member.prev, typeof(*pos), member); \
       &pos->member != (head); \
       pos = list_entry(pos->member.prev, typeof(*pos), member))
#define list_for_each_entry_safe(pos, n, head, member) \
  for (pos = list_entry((head)->next, typeof(*pos), member), \
       n = list_entry(pos->member.next, typeof(*pos), member); \
       &pos->member != (head); \
       pos = n, n = list_entry(n->member.next, typeof(*n), member))
#define list_for_each_entry_rcu(pos, head, member, args...) \
  list_for_each_entry(pos, head, member)

#endif
#endif
