/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/skbuff.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SKBUFF_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SKBUFF_H

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>

#include <string.h>

#ifndef CHECKSUM_NONE
#  define CHECKSUM_NONE 0
#endif

#ifndef NET_SKB_PAD
#  define NET_SKB_PAD 32
#endif

struct sock;
struct net_device;
struct sk_buff;
struct udphdr;

struct skb_shared_info
{
  unsigned int tx_flags;
  uint32_t tskey;
  struct sk_buff *frag_list;
  struct skb_shared_hwtstamps
  {
    int64_t hwtstamp;
  } hwtstamps;
};

struct sk_buff
{
  unsigned int len;
  unsigned int size;
  unsigned int truesize;
  unsigned char *head;
  unsigned char *data;
  unsigned char cb[64];
  struct sk_buff *next;
  struct sock *sk;
  struct net_device *dev;
  __be16 protocol;
  unsigned int data_len;
  unsigned int priority;
  uint8_t pkt_type;
  uint8_t pkt_status;
  uint16_t pkt_seqnum;
  int ip_summed;
  int64_t tstamp;
  uint8_t tstamp_type;
  unsigned char *mac_header;
  unsigned char *network_header;
  unsigned char *transport_header;
  struct skb_shared_info shinfo;
};

struct sk_buff_head
{
  struct sk_buff *head;
  struct sk_buff *tail;
  unsigned int qlen;
  spinlock_t lock;
};

struct sk_buff *linux_bt_compat_skb_recv_datagram(void *sk,
                                                  unsigned int flags,
                                                  int *err);
int linux_bt_compat_skb_copy_datagram_msg(const struct sk_buff *skb,
                                          int offset, void *msg, int len);

static inline struct sk_buff *alloc_skb(unsigned int len, gfp_t priority)
{
  struct sk_buff *skb;

  (void)priority;
  skb = kzalloc(sizeof(*skb), GFP_KERNEL);
  if (skb == NULL)
    {
      return NULL;
    }

  skb->head = kzalloc(len, GFP_KERNEL);
  if (skb->head == NULL)
    {
      kfree(skb);
      return NULL;
    }

  skb->data = skb->head;
  skb->size = len;
  skb->truesize = sizeof(*skb) + len;
  skb->len = 0;
  skb->mac_header = skb->data;
  skb->network_header = skb->data;
  skb->transport_header = skb->data;
  return skb;
}

static inline void skb_reserve(struct sk_buff *skb, unsigned int len)
{
  if (skb != NULL && len <= skb->size)
    {
      skb->data += len;
      skb->size -= len;
      skb->network_header = skb->data;
      skb->transport_header = skb->data;
    }
}

static inline void *skb_put(struct sk_buff *skb, unsigned int len)
{
  void *ptr;

  if (skb == NULL || skb->len + len > skb->size)
    {
      return NULL;
    }

  ptr = skb->data + skb->len;
  skb->len += len;
  return ptr;
}

static inline void *__skb_put(struct sk_buff *skb, unsigned int len)
{
  return skb_put(skb, len);
}

static inline void *skb_push(struct sk_buff *skb, unsigned int len)
{
  if (skb == NULL || skb->data < skb->head + len)
    {
      return NULL;
    }

  skb->data -= len;
  skb->len += len;
  return skb->data;
}

static inline void skb_put_data(struct sk_buff *skb, const void *data,
                                unsigned int len)
{
  void *dst = skb_put(skb, len);

  if (dst != NULL)
    {
      memcpy(dst, data, len);
    }
}

static inline void __skb_put_data(struct sk_buff *skb, const void *data,
                                  unsigned int len)
{
  skb_put_data(skb, data, len);
}

static inline void skb_put_u8(struct sk_buff *skb, uint8_t value)
{
  uint8_t *dst = skb_put(skb, 1);

  if (dst != NULL)
    {
      *dst = value;
    }
}

static inline void *skb_pull(struct sk_buff *skb, unsigned int len)
{
  if (skb != NULL && len <= skb->len)
    {
      skb->data += len;
      skb->len -= len;
      return skb->data;
    }

  return NULL;
}

static inline void *skb_pull_rcsum(struct sk_buff *skb, unsigned int len)
{
  return skb_pull(skb, len);
}

static inline unsigned char *skb_network_header(const struct sk_buff *skb)
{
  return skb != NULL && skb->network_header != NULL ? skb->network_header :
         skb != NULL ? skb->data : NULL;
}

static inline void skb_reset_network_header(struct sk_buff *skb)
{
  if (skb != NULL)
    {
      skb->network_header = skb->data;
    }
}

static inline void skb_set_transport_header(struct sk_buff *skb,
                                            unsigned int offset)
{
  if (skb != NULL)
    {
      skb->transport_header = skb_network_header(skb) + offset;
    }
}

static inline void skb_queue_head_init(struct sk_buff_head *list)
{
  list->head = NULL;
  list->tail = NULL;
  list->qlen = 0;
  spin_lock_init(&list->lock);
}

static inline void __skb_queue_head_init(struct sk_buff_head *list)
{
  skb_queue_head_init(list);
}

static inline int skb_queue_empty(const struct sk_buff_head *list)
{
  return list == NULL || list->qlen == 0;
}

static inline int skb_queue_empty_lockless(const struct sk_buff_head *list)
{
  return skb_queue_empty(list);
}

static inline struct sk_buff *skb_peek(const struct sk_buff_head *list)
{
  return list != NULL ? list->head : NULL;
}

static inline struct sk_buff *skb_peek_tail(const struct sk_buff_head *list)
{
  return list != NULL ? list->tail : NULL;
}

static inline void skb_queue_tail(struct sk_buff_head *list,
                                  struct sk_buff *skb)
{
  skb->next = NULL;
  if (list->tail != NULL)
    {
      list->tail->next = skb;
    }
  else
    {
      list->head = skb;
    }

  list->tail = skb;
  list->qlen++;
}

static inline void skb_queue_head(struct sk_buff_head *list,
                                  struct sk_buff *skb)
{
  skb->next = list->head;
  list->head = skb;
  if (list->tail == NULL)
    {
      list->tail = skb;
    }

  list->qlen++;
}

static inline struct sk_buff *skb_dequeue(struct sk_buff_head *list)
{
  struct sk_buff *skb;

  if (skb_queue_empty(list))
    {
      return NULL;
    }

  skb = list->head;
  list->head = skb->next;
  if (list->head == NULL)
    {
      list->tail = NULL;
    }

  skb->next = NULL;
  list->qlen--;
  return skb;
}

static inline void skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
  struct sk_buff *prev = NULL;
  struct sk_buff *iter;

  if (skb == NULL || list == NULL)
    {
      return;
    }

  for (iter = list->head; iter != NULL; prev = iter, iter = iter->next)
    {
      if (iter != skb)
        {
          continue;
        }

      if (prev != NULL)
        {
          prev->next = iter->next;
        }
      else
        {
          list->head = iter->next;
        }

      if (list->tail == iter)
        {
          list->tail = prev;
        }

      iter->next = NULL;
      if (list->qlen > 0)
        {
          list->qlen--;
        }

      return;
    }
}

static inline void kfree_skb(struct sk_buff *skb)
{
  if (skb != NULL)
    {
      kfree(skb->head);
      kfree(skb);
    }
}

static inline void __skb_queue_purge(struct sk_buff_head *list)
{
  struct sk_buff *skb;

  if (list == NULL)
    {
      return;
    }

  while ((skb = skb_dequeue(list)) != NULL)
    {
      kfree_skb(skb);
    }
}

static inline void skb_queue_purge(struct sk_buff_head *list)
{
  struct sk_buff *skb;

  while ((skb = skb_dequeue(list)) != NULL)
    {
      kfree_skb(skb);
    }
}

static inline unsigned int skb_queue_len(const struct sk_buff_head *list)
{
  return list != NULL ? list->qlen : 0;
}

static inline struct skb_shared_info *skb_shinfo(struct sk_buff *skb)
{
  return skb != NULL ? &skb->shinfo : NULL;
}

static inline struct skb_shared_hwtstamps *skb_hwtstamps(struct sk_buff *skb)
{
  return skb != NULL ? &skb->shinfo.hwtstamps : NULL;
}

static inline struct sk_buff *skb_clone(struct sk_buff *skb, gfp_t priority)
{
  struct sk_buff *clone;

  (void)priority;
  if (skb == NULL)
    {
      return NULL;
    }

  clone = alloc_skb(skb->len, GFP_KERNEL);
  if (clone == NULL)
    {
      return NULL;
    }

  skb_put_data(clone, skb->data, skb->len);
  memcpy(clone->cb, skb->cb, sizeof(clone->cb));
  clone->data_len = skb->data_len;
  clone->truesize = skb->truesize;
  clone->priority = skb->priority;
  clone->pkt_type = skb->pkt_type;
  clone->pkt_status = skb->pkt_status;
  clone->pkt_seqnum = skb->pkt_seqnum;
  clone->tstamp = skb->tstamp;
  clone->sk = skb->sk;
  clone->dev = skb->dev;
  clone->protocol = skb->protocol;
  clone->mac_header = clone->data;
  clone->transport_header = clone->data;
  clone->shinfo = skb->shinfo;
  return clone;
}

static inline struct sk_buff *__pskb_copy_fclone(struct sk_buff *skb,
                                                 int headroom,
                                                 gfp_t priority,
                                                 bool fclone)
{
  struct sk_buff *copy;

  (void)fclone;
  if (skb == NULL)
    {
      return NULL;
    }

  copy = alloc_skb((unsigned int)headroom + skb->len, priority);
  if (copy == NULL)
    {
      return NULL;
    }

  if (headroom > 0)
    {
      skb_reserve(copy, (unsigned int)headroom);
    }

  skb_put_data(copy, skb->data, skb->len);
  memcpy(copy->cb, skb->cb, sizeof(copy->cb));
  copy->data_len = skb->data_len;
  copy->truesize = copy->size + sizeof(*copy);
  copy->priority = skb->priority;
  copy->pkt_type = skb->pkt_type;
  copy->pkt_status = skb->pkt_status;
  copy->pkt_seqnum = skb->pkt_seqnum;
  copy->tstamp = skb->tstamp;
  copy->sk = skb->sk;
  copy->dev = skb->dev;
  copy->protocol = skb->protocol;
  copy->mac_header = copy->data;
  copy->transport_header = copy->data;
  copy->shinfo = skb->shinfo;
  return copy;
}

static inline struct sk_buff *skb_clone_sk(struct sk_buff *skb)
{
  return skb_clone(skb, GFP_KERNEL);
}

static inline struct sk_buff *skb_get(struct sk_buff *skb)
{
  return skb_clone(skb, GFP_KERNEL);
}

static inline bool skb_queue_is_last(const struct sk_buff_head *list,
                                     const struct sk_buff *skb)
{
  return list != NULL && skb != NULL && list->tail == skb;
}

static inline struct sk_buff *skb_queue_next(const struct sk_buff_head *list,
                                             const struct sk_buff *skb)
{
  (void)list;
  return skb != NULL ? skb->next : NULL;
}

static inline void skb_queue_splice_tail(struct sk_buff_head *list,
                                         struct sk_buff_head *head)
{
  struct sk_buff *skb;

  if (list == NULL || head == NULL || skb_queue_empty(list))
    {
      return;
    }

  while ((skb = skb_dequeue(list)) != NULL)
    {
      skb_queue_tail(head, skb);
    }
}

static inline void skb_queue_splice_tail_init(struct sk_buff_head *list,
                                              struct sk_buff_head *head)
{
  skb_queue_splice_tail(list, head);
  if (list != NULL)
    {
      skb_queue_head_init(list);
    }
}

static inline int skb_cloned(const struct sk_buff *skb)
{
  (void)skb;
  return 0;
}

static inline struct sk_buff *skb_copy(const struct sk_buff *skb,
                                       gfp_t priority)
{
  return skb != NULL ? skb_clone((struct sk_buff *)skb, priority) : NULL;
}

static inline unsigned char *skb_tail_pointer(const struct sk_buff *skb)
{
  return skb != NULL ? skb->data + skb->len : NULL;
}

static inline unsigned int skb_headroom(const struct sk_buff *skb)
{
  return skb != NULL && skb->data >= skb->head ?
         (unsigned int)(skb->data - skb->head) : 0;
}

static inline unsigned int skb_tailroom(const struct sk_buff *skb)
{
  if (skb == NULL || skb->data < skb->head)
    {
      return 0;
    }

  return skb->size > skb->len ? skb->size - skb->len : 0;
}

static inline void skb_trim(struct sk_buff *skb, unsigned int len)
{
  if (skb != NULL && len <= skb->len)
    {
      skb->len = len;
    }
}

static inline bool skb_has_frag_list(const struct sk_buff *skb)
{
  return skb != NULL && skb->shinfo.frag_list != NULL;
}

static inline bool pskb_may_pull(struct sk_buff *skb, unsigned int len)
{
  return skb != NULL && skb->len >= len;
}

static inline int skb_linearize(struct sk_buff *skb)
{
  (void)skb;
  return 0;
}

static inline void skb_copy_from_linear_data(const struct sk_buff *skb,
                                             void *to,
                                             unsigned int len)
{
  if (skb != NULL && to != NULL && len <= skb->len)
    {
      memcpy(to, skb->data, len);
    }
}

static inline void skb_copy_to_linear_data(struct sk_buff *skb,
                                           const void *from,
                                           unsigned int len)
{
  if (skb != NULL && from != NULL && len <= skb->len)
    {
      memcpy(skb->data, from, len);
    }
}

static inline int skb_cow(struct sk_buff *skb, unsigned int headroom)
{
  unsigned int old_mac_off;
  unsigned int old_network_off;
  unsigned int old_transport_off;
  unsigned int used_headroom;
  unsigned int new_size;
  unsigned char *new_head;
  unsigned char *new_data;

  if (skb == NULL)
    {
      return -EINVAL;
    }

  used_headroom = (unsigned int)(skb->data - skb->head);
  if (used_headroom >= headroom)
    {
      return 0;
    }

  old_mac_off = skb->mac_header != NULL ?
                (unsigned int)(skb->mac_header - skb->head) : used_headroom;
  old_network_off = skb->network_header != NULL ?
                    (unsigned int)(skb->network_header - skb->head) :
                    used_headroom;
  old_transport_off = skb->transport_header != NULL ?
                      (unsigned int)(skb->transport_header - skb->head) :
                      used_headroom;

  new_size = headroom + skb->len;
  new_head = kzalloc(new_size, GFP_KERNEL);
  if (new_head == NULL)
    {
      return -ENOMEM;
    }

  new_data = new_head + headroom;
  memcpy(new_data, skb->data, skb->len);
  kfree(skb->head);

  skb->head = new_head;
  skb->data = new_data;
  skb->size = new_size;
  skb->truesize = sizeof(*skb) + new_size;
  skb->mac_header = new_head + old_mac_off + (headroom - used_headroom);
  skb->network_header = new_head + old_network_off +
                        (headroom - used_headroom);
  skb->transport_header = new_head + old_transport_off +
                          (headroom - used_headroom);
  return 0;
}

static inline void __skb_tstamp_tx(struct sk_buff *skb, void *hwtstamps,
                                   void *skb_shared_hwtstamps,
                                   struct sock *sk, int tstype)
{
  (void)skb;
  (void)hwtstamps;
  (void)skb_shared_hwtstamps;
  (void)sk;
  (void)tstype;
}

static inline unsigned int skb_headlen(const struct sk_buff *skb)
{
  return skb != NULL ? skb->len - skb->data_len : 0;
}

static inline void __skb_pull(struct sk_buff *skb, unsigned int len)
{
  skb_pull(skb, len);
}

static inline void skb_reset_transport_header(struct sk_buff *skb)
{
  if (skb != NULL)
    {
      skb->transport_header = skb->data;
    }
}

static inline void skb_reset_mac_header(struct sk_buff *skb)
{
  if (skb != NULL)
    {
      skb->mac_header = skb->data;
    }
}

static inline unsigned char *skb_mac_header(const struct sk_buff *skb)
{
  return skb != NULL && skb->mac_header != NULL ?
         skb->mac_header : (skb != NULL ? skb->data : NULL);
}

static inline unsigned char *skb_transport_header(const struct sk_buff *skb)
{
  return skb != NULL && skb->transport_header != NULL ?
         skb->transport_header : (skb != NULL ? skb->data : NULL);
}

static inline struct udphdr *udp_hdr(const struct sk_buff *skb)
{
  return (struct udphdr *)skb_transport_header(skb);
}

static inline void *__skb_pull_data(struct sk_buff *skb, unsigned int len)
{
  void *data;

  if (skb == NULL || skb->len < len)
    {
      return NULL;
    }

  data = skb->data;
  skb_pull(skb, len);
  return data;
}

#define skb_pull_data(skb, len) __skb_pull_data((skb), (len))
#define __skb_dequeue(list) skb_dequeue((list))
#define __skb_queue_head(list, skb) skb_queue_head((list), (skb))
#define __skb_queue_tail(list, skb) skb_queue_tail((list), (skb))

static inline void skb_orphan(struct sk_buff *skb)
{
  if (skb != NULL)
    {
      skb->sk = NULL;
    }
}

static inline void __net_timestamp(struct sk_buff *skb)
{
  if (skb != NULL)
    {
      skb->tstamp = 0;
      skb->tstamp_type = 0;
    }
}

static inline void skb_set_delivery_time(struct sk_buff *skb, int64_t tstamp,
                                         uint8_t tstamp_type)
{
  if (skb != NULL)
    {
      skb->tstamp = tstamp;
      skb->tstamp_type = tstamp_type;
    }
}

static inline int64_t skb_get_ktime(const struct sk_buff *skb)
{
  return skb != NULL ? skb->tstamp : 0;
}

static inline void skb_get_timestamp(const struct sk_buff *skb, void *tvp)
{
  struct linux_bt_compat_timeval
  {
    long tv_sec;
    long tv_usec;
  } *tv = tvp;

  (void)skb;
  if (tv != NULL)
    {
      tv->tv_sec = 0;
      tv->tv_usec = 0;
    }
}

static inline void dev_kfree_skb_irq(struct sk_buff *skb)
{
  kfree_skb(skb);
}

static inline uint64_t skb_get_kcov_handle(struct sk_buff *skb)
{
  (void)skb;
  return 0;
}

static inline void skb_tailroom_reserve(struct sk_buff *skb, unsigned int mtu,
                                        unsigned int needed_tailroom)
{
  (void)skb;
  (void)mtu;
  (void)needed_tailroom;
}

static inline int skb_copy_datagram_msg(const struct sk_buff *skb,
                                        int offset, void *msg,
                                        int len)
{
  return linux_bt_compat_skb_copy_datagram_msg(skb, offset, msg, len);
}

static inline struct sk_buff *skb_recv_datagram(void *sk, unsigned int flags,
                                                int *err)
{
  return linux_bt_compat_skb_recv_datagram(sk, flags, err);
}

static inline void skb_free_datagram(void *sk, struct sk_buff *skb)
{
  (void)sk;
  kfree_skb(skb);
}

#define skb_walk_frags(skb, iter) \
  for (iter = NULL; iter != NULL; iter = NULL)

#define skb_queue_walk(queue, skb) \
  for (skb = (queue)->head; skb != NULL; skb = skb->next)

#define skb_queue_walk_safe(queue, skb, tmp) \
  for (skb = (queue)->head; \
       skb != NULL && ((tmp) = skb->next, 1); \
       skb = (tmp))

#define skb_queue_walk_from(queue, skb) \
  for (; skb != NULL; skb = skb->next)

#define hci_skb_pkt_type(skb) ((skb)->pkt_type)
#define hci_skb_pkt_status(skb) ((skb)->pkt_status)
#define hci_skb_pkt_seqnum(skb) ((skb)->pkt_seqnum)

#define SKBTX_SW_TSTAMP             BIT(0)
#define SKBTX_COMPLETION_TSTAMP     BIT(1)

#endif
