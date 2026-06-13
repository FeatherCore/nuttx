/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/net/sock.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_SOCK_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_SOCK_H

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/uio.h>
#include <linux/ethtool.h>
#include <linux/slab.h>
#include <linux/refcount.h>
#include <linux/bitops.h>
#include <linux/atomic.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/sched/signal.h>

#include <stdbool.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <net/if.h>

#define msghdr linux_bt_msghdr

typedef unsigned int __poll_t;
typedef void poll_table;

struct module;
struct cred;
struct pid;
struct socket;
struct vm_area_struct;

typedef void *sockptr_t;

struct msghdr
{
  struct iov_iter msg_iter;
  unsigned int msg_flags;
  void *msg_name;
  int msg_namelen;
  size_t msg_controllen;
};

struct lock_class_key
{
  int unused;
};

struct proto_accept_arg
{
  int flags;
  int err;
  int is_empty;
  bool kern;
};

struct proto
{
  const char *name;
  struct module *owner;
  size_t obj_size;
};

struct net
{
  int unused;
  void *proc_net;
};

extern struct net init_net;

struct sock
{
  struct hlist_node sk_node;
  spinlock_t sk_peer_lock;
  wait_queue_head_t sk_sleep;
  struct sk_buff_head sk_receive_queue;
  struct sk_buff_head sk_error_queue;
  struct sk_buff_head sk_write_queue;
  int sk_protocol;
  int sk_state;
  int sk_err;
  int sk_shutdown;
  int sk_sndbuf;
  int sk_rcvbuf;
  atomic_t sk_rmem_alloc;
  int sk_priority;
  int sk_type;
  int sk_max_ack_backlog;
  int sk_ack_backlog;
  long sk_sndtimeo;
  long sk_lingertime;
  unsigned long sk_flags;
  struct socket *sk_socket;
  atomic_t sk_tskey;
  struct pid *sk_peer_pid;
  const struct cred *sk_peer_cred;
  refcount_t sk_refcnt;
  void (*sk_destruct)(struct sock *sk);
  void (*sk_state_change)(struct sock *sk);
  void (*sk_data_ready)(struct sock *sk);
  void (*sk_write_space)(struct sock *sk);
  void (*sk_error_report)(struct sock *sk);
};

struct bt_sock
{
  struct sock sk;
  struct list_head accept_q;
  struct sock *parent;
  unsigned long flags;
  void (*skb_msg_name)(struct sk_buff *skb, void *msg_name,
                       int *msg_namelen);
  void (*skb_put_cmsg)(struct sk_buff *skb, struct msghdr *msg,
                       struct sock *sk);
};

struct proto_ops
{
  int family;
  struct module *owner;
  int (*release)(struct socket *sock);
  int (*bind)(struct socket *sock, struct sockaddr *addr, int addr_len);
  int (*getname)(struct socket *sock, struct sockaddr *addr,
                 int peer);
  int (*sendmsg)(struct socket *sock, struct msghdr *msg, size_t len);
  int (*recvmsg)(struct socket *sock, struct msghdr *msg, size_t len,
                 int flags);
  int (*ioctl)(struct socket *sock, unsigned int cmd, unsigned long arg);
  __poll_t (*poll)(struct file *file, struct socket *sock,
                   poll_table *wait);
  int (*listen)(struct socket *sock, int len);
  int (*shutdown)(struct socket *sock, int flags);
  int (*setsockopt)(struct socket *sock, int level, int optname,
                    sockptr_t optval, unsigned int optlen);
  int (*getsockopt)(struct socket *sock, int level, int optname,
                    char *optval, int *optlen);
  int (*connect)(struct socket *sock, struct sockaddr *addr,
                 int addr_len, int flags);
  int (*socketpair)(struct socket *sock1, struct socket *sock2);
  int (*accept)(struct socket *sock, struct socket *newsock,
                struct proto_accept_arg *arg);
  int (*mmap)(struct file *file, struct socket *sock,
              struct vm_area_struct *vma);
  int (*gettstamp)(struct socket *sock, void *userstamp,
                   bool timeval, bool time32);
};

struct socket
{
  struct sock *sk;
  struct file *file;
  int type;
  int state;
  const struct proto_ops *ops;
};

struct net_proto_family
{
  struct module *owner;
  int family;
  int (*create)(struct net *net, struct socket *sock, int protocol,
                int kern);
};

struct l2cap_chan;

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_SCM_CREDS
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_SCM_CREDS
struct scm_creds
{
  unsigned int pid;
  unsigned int uid;
  unsigned int gid;
};
#endif

struct scm_cookie
{
  struct scm_creds creds;
};

typedef int64_t ktime_t;

struct __kernel_old_timeval
{
  long tv_sec;
  long tv_usec;
};

#define SOCK_ZAPPED 0
#define SOCK_SELECT_ERR_QUEUE 1
#define SOCKWQ_ASYNC_WAITDATA 2
#define SOCKWQ_ASYNC_NOSPACE 3
#define SOCK_DEAD 4
#define SOCK_LINGER 5
#define SINGLE_DEPTH_NESTING 1
#define RCV_SHUTDOWN 1
#define SEND_SHUTDOWN 2
#define SHUTDOWN_MASK (RCV_SHUTDOWN | SEND_SHUTDOWN)

#ifndef SOF_TIMESTAMPING_TX_HARDWARE
#  define SOF_TIMESTAMPING_TX_HARDWARE      BIT(0)
#endif
#ifndef SOF_TIMESTAMPING_TX_SOFTWARE
#  define SOF_TIMESTAMPING_TX_SOFTWARE      BIT(1)
#endif
#ifndef SOF_TIMESTAMPING_RX_SOFTWARE
#  define SOF_TIMESTAMPING_RX_SOFTWARE      BIT(3)
#endif
#ifndef SOF_TIMESTAMPING_SOFTWARE
#  define SOF_TIMESTAMPING_SOFTWARE         BIT(4)
#endif
#ifndef SOF_TIMESTAMPING_TX_COMPLETION
#  define SOF_TIMESTAMPING_TX_COMPLETION    BIT(18)
#endif
#ifndef SOF_TIMESTAMPING_OPT_ID
#  define SOF_TIMESTAMPING_OPT_ID           BIT(7)
#endif
#ifndef SOF_TIMESTAMPING_TX_RECORD_MASK
#  define SOF_TIMESTAMPING_TX_RECORD_MASK \
    (SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_TX_SOFTWARE)
#endif

#ifndef SOCKCM_FLAG_TS_OPT_ID
#  define SOCKCM_FLAG_TS_OPT_ID             BIT(0)
#endif

#ifndef SCM_TSTAMP_SND
#  define SCM_TSTAMP_SND                    0
#endif
#ifndef SCM_TSTAMP_COMPLETION
#  define SCM_TSTAMP_COMPLETION             1
#endif

struct sockcm_cookie
{
  uint32_t tsflags;
  uint32_t ts_opt_id;
};

static inline void sock_tx_timestamp(struct sock *sk,
                                     const struct sockcm_cookie *sockc,
                                     unsigned int *tx_flags)
{
  (void)sk;
  if (sockc != NULL && tx_flags != NULL &&
      (sockc->tsflags & SOF_TIMESTAMPING_TX_SOFTWARE) != 0)
    {
      *tx_flags |= SKBTX_SW_TSTAMP;
    }
}

static inline void hci_sockcm_init(struct sockcm_cookie *sockc,
                                   struct sock *sk)
{
  (void)sk;
  if (sockc != NULL)
    {
      memset(sockc, 0, sizeof(*sockc));
    }
}

static inline int sock_cmsg_send(struct sock *sk, struct msghdr *msg,
                                 struct sockcm_cookie *sockc)
{
  (void)sk;
  (void)msg;
  (void)sockc;
  return 0;
}

static inline int sock_recv_errqueue(struct sock *sk, struct msghdr *msg,
                                     size_t len, int level, int type)
{
  (void)sk;
  (void)msg;
  (void)len;
  (void)level;
  (void)type;
  return -EAGAIN;
}

static inline struct net *sock_net(const struct sock *sk)
{
  (void)sk;
  return &init_net;
}

static inline void security_sk_clone(const struct sock *sk,
                                     struct sock *newsk)
{
  (void)sk;
  (void)newsk;
}

static inline int sk_acceptq_is_full(const struct sock *sk)
{
  return sk != NULL && sk->sk_ack_backlog >= sk->sk_max_ack_backlog;
}

static inline int sk_filter(struct sock *sk, struct sk_buff *skb)
{
  (void)sk;
  (void)skb;
  return 0;
}

static inline int __sock_queue_rcv_skb(struct sock *sk, struct sk_buff *skb)
{
  if (sk == NULL || skb == NULL)
    {
      return -EINVAL;
    }

  skb_queue_tail(&sk->sk_receive_queue, skb);
  atomic_inc(&sk->sk_rmem_alloc);
  return 0;
}

#ifndef SS_UNCONNECTED
#  define SS_UNCONNECTED 1
#endif
#ifndef SS_CONNECTED
#  define SS_CONNECTED 2
#endif

#ifndef SOL_BLUETOOTH
#define SOL_BLUETOOTH 274
#endif

#ifndef MSG_CMSG_COMPAT
#  define MSG_CMSG_COMPAT 0
#endif

#ifndef MSG_ERRQUEUE
#  define MSG_ERRQUEUE 0x2000
#endif

#ifndef MSG_TRUNC
#  define MSG_TRUNC 0x20
#endif

#ifndef EPOLLRDNORM
#  define EPOLLRDNORM POLLRDNORM
#endif
#ifndef EPOLLWRNORM
#  define EPOLLWRNORM POLLWRNORM
#endif
#ifndef EPOLLERR
#  define EPOLLERR POLLERR
#endif
#ifndef EPOLLHUP
#  define EPOLLHUP POLLHUP
#endif
#ifndef EPOLLPRI
#  define EPOLLPRI POLLPRI
#endif
#ifndef EPOLLRDHUP
#  define EPOLLRDHUP POLLHUP
#endif
#ifndef EPOLLWRBAND
#  define EPOLLWRBAND POLLWRBAND
#endif
#ifndef EPOLLIN
#  define EPOLLIN POLLIN
#endif
#ifndef EPOLLOUT
#  define EPOLLOUT POLLOUT
#endif

static inline int sock_allow_reclassification(struct sock *sk)
{
  (void)sk;
  return 1;
}

static inline void sock_lock_init_class_and_name(struct sock *sk,
                                                 const char *slock_name,
                                                 struct lock_class_key *slock_key,
                                                 const char *lock_name,
                                                 struct lock_class_key *lock_key)
{
  (void)sk;
  (void)slock_name;
  (void)slock_key;
  (void)lock_name;
  (void)lock_key;
}

static inline struct sock *sk_alloc(struct net *net, int family, int priority,
                                    struct proto *prot, int kern)
{
  struct sock *sk;

  (void)net;
  (void)family;
  (void)priority;
  (void)prot;
  (void)kern;

  sk = kzalloc(prot != NULL && prot->obj_size >= sizeof(*sk) ?
               prot->obj_size : sizeof(*sk), GFP_KERNEL);
  if (sk != NULL)
    {
      INIT_HLIST_NODE(&sk->sk_node);
      spin_lock_init(&sk->sk_peer_lock);
      init_waitqueue_head(&sk->sk_sleep);
      skb_queue_head_init(&sk->sk_receive_queue);
      skb_queue_head_init(&sk->sk_error_queue);
      skb_queue_head_init(&sk->sk_write_queue);
      sk->sk_sndbuf = 4096;
      sk->sk_rcvbuf = 4096;
      sk->sk_sndtimeo = 0;
      sk->sk_lingertime = 0;
      atomic_set(&sk->sk_rmem_alloc, 0);
      refcount_set(&sk->sk_refcnt, 1);
    }

  return sk;
}

static inline void sock_init_data(struct socket *sock, struct sock *sk)
{
  if (sock != NULL)
    {
      sock->sk = sk;
      if (sk != NULL)
        {
          sk->sk_socket = sock;
          sk->sk_type = sock->type;
        }
    }
}

static inline void sock_hold(struct sock *sk)
{
  if (sk != NULL)
    {
      refcount_inc(&sk->sk_refcnt);
    }
}

static inline void sock_put(struct sock *sk)
{
  if (sk != NULL && refcount_dec_and_test(&sk->sk_refcnt))
    {
      if (sk->sk_destruct != NULL)
        {
          sk->sk_destruct(sk);
        }

      skb_queue_purge(&sk->sk_receive_queue);
      skb_queue_purge(&sk->sk_error_queue);
      skb_queue_purge(&sk->sk_write_queue);
      kfree(sk);
    }
}

static inline void sk_free(struct sock *sk)
{
  sock_put(sk);
}

static inline void lock_sock_nested(struct sock *sk, int subclass)
{
  (void)sk;
  (void)subclass;
}

static inline void lock_sock(struct sock *sk)
{
  lock_sock_nested(sk, 0);
}

static inline void release_sock(struct sock *sk)
{
  (void)sk;
}

static inline void sock_orphan(struct sock *sk)
{
  (void)sk;
}

static inline int sock_queue_rcv_skb(struct sock *sk, struct sk_buff *skb)
{
  if (sk == NULL || skb == NULL)
    {
      return -EINVAL;
    }

  skb_queue_tail(&sk->sk_receive_queue, skb);
  if (sk->sk_data_ready != NULL)
    {
      sk->sk_data_ready(sk);
    }

  return 0;
}

static inline void bh_lock_sock_nested(struct sock *sk)
{
  (void)sk;
}

static inline void bh_unlock_sock(struct sock *sk)
{
  (void)sk;
}

static inline void sk_add_node(struct sock *sk, struct hlist_head *list)
{
  hlist_add_head(&sk->sk_node, list);
}

static inline void sk_del_node_init(struct sock *sk)
{
  hlist_del_init(&sk->sk_node);
}

#define sk_for_each(__sk, list) \
  hlist_for_each_entry(__sk, list, sk_node)

#define sk_entry(ptr) hlist_entry(ptr, struct sock, sk_node)

static inline void sk_acceptq_added(struct sock *sk)
{
  (void)sk;
}

static inline void sk_acceptq_removed(struct sock *sk)
{
  (void)sk;
}

static inline void sock_graft(struct sock *sk, struct socket *parent)
{
  if (parent != NULL)
    {
      parent->sk = sk;
    }
}

static inline wait_queue_head_t *sk_sleep(struct sock *sk)
{
  return &sk->sk_sleep;
}

static inline int sock_error(struct sock *sk)
{
  int err = sk != NULL ? sk->sk_err : 0;

  if (sk != NULL)
    {
      sk->sk_err = 0;
    }

  return err;
}

static inline unsigned long sock_rcvlowat(struct sock *sk, int waitall,
                                          unsigned long len)
{
  (void)sk;
  return waitall ? len : 1;
}

static inline long sock_rcvtimeo(struct sock *sk, int noblock)
{
  (void)sk;
  return noblock ? 0 : 1;
}

static inline long sock_sndtimeo(struct sock *sk, int noblock)
{
  (void)sk;
  return noblock ? 0 : 1;
}

static inline int sock_intr_errno(long timeo)
{
  return timeo ? -ERESTARTSYS : -EINTR;
}

static inline void sock_skb_cb_check_size(size_t size)
{
  (void)size;
}

static inline int sock_writeable(struct sock *sk)
{
  (void)sk;
  return 1;
}

static inline int sock_flag(struct sock *sk, int flag)
{
  return sk != NULL ? test_bit(flag, &sk->sk_flags) : 0;
}

static inline void sock_set_flag(struct sock *sk, int flag)
{
  if (sk != NULL)
    {
      set_bit(flag, &sk->sk_flags);
    }
}

static inline void sock_reset_flag(struct sock *sk, int flag)
{
  if (sk != NULL)
    {
      clear_bit(flag, &sk->sk_flags);
    }
}

static inline void sk_set_bit(int nr, struct sock *sk)
{
  (void)nr;
  (void)sk;
}

static inline void sk_clear_bit(int nr, struct sock *sk)
{
  (void)nr;
  (void)sk;
}

static inline unsigned int sk_rmem_alloc_get(struct sock *sk)
{
  return sk != NULL ? atomic_read(&sk->sk_rmem_alloc) : 0;
}

static inline unsigned int sk_wmem_alloc_get(struct sock *sk)
{
  (void)sk;
  return 0;
}

static inline unsigned int sk_uid(struct sock *sk)
{
  (void)sk;
  return 0;
}

static inline unsigned long sock_i_ino(struct sock *sk)
{
  return (unsigned long)(uintptr_t)sk;
}

static inline unsigned int from_kuid(void *ns, unsigned int kuid)
{
  (void)ns;
  return kuid;
}

static inline void *seq_user_ns(void *seq)
{
  (void)seq;
  return NULL;
}

static inline void sock_recv_cmsgs(void *msg, struct sock *sk,
                                   struct sk_buff *skb)
{
  (void)msg;
  (void)sk;
  (void)skb;
}

static inline void sock_recv_timestamp(void *msg, struct sock *sk,
                                       struct sk_buff *skb)
{
  (void)msg;
  (void)sk;
  (void)skb;
}

static inline void scm_recv(struct socket *sock, struct msghdr *msg,
                            struct scm_cookie *scm, int flags)
{
  (void)sock;
  (void)msg;
  (void)scm;
  (void)flags;
}

static inline int put_cmsg(void *msg, int level, int type, int len,
                           void *data)
{
  (void)msg;
  (void)level;
  (void)type;
  (void)len;
  (void)data;
  return 0;
}

static inline int kernel_sendmsg(struct socket *sock, struct msghdr *msg,
                                 struct kvec *vec, size_t num,
                                 size_t size)
{
  struct msghdr local;
  unsigned char *buf;
  size_t offset = 0;
  size_t i;
  int ret;

  if (sock == NULL || sock->ops == NULL || sock->ops->sendmsg == NULL)
    {
      return -EOPNOTSUPP;
    }

  if (size == 0)
    {
      return sock->ops->sendmsg(sock, msg, size);
    }

  if (vec == NULL || num == 0)
    {
      return -EINVAL;
    }

  buf = kmalloc(size, GFP_KERNEL);
  if (buf == NULL)
    {
      return -ENOMEM;
    }

  for (i = 0; i < num && offset < size; i++)
    {
      size_t copy = vec[i].iov_len;

      if (copy > size - offset)
        {
          copy = size - offset;
        }

      if (copy > 0 && vec[i].iov_base != NULL)
        {
          memcpy(buf + offset, vec[i].iov_base, copy);
        }

      offset += copy;
    }

  if (offset != size)
    {
      kfree(buf);
      return -EINVAL;
    }

  local = *msg;
  local.msg_iter.data = (const char *)buf;
  local.msg_iter.count = size;
  ret = sock->ops->sendmsg(sock, &local, size);
  kfree(buf);
  return ret;
}

static inline int get_user_ifreq(struct ifreq *ifr, void **data,
                                 void *useraddr)
{
  if (ifr == NULL || data == NULL || useraddr == NULL)
    {
      return -EFAULT;
    }

  memcpy(ifr, useraddr, sizeof(*ifr));
  *data = useraddr;
  return 0;
}

static inline int put_user_ifreq(struct ifreq *ifr, void *useraddr)
{
  if (ifr == NULL || useraddr == NULL)
    {
      return -EFAULT;
    }

  memcpy(useraddr, ifr, sizeof(*ifr));
  return 0;
}

#define put_user(value, ptr) ({ *(ptr) = (value); 0; })
#define get_user(value, ptr) ({ (value) = *(ptr); 0; })

#ifndef DECLARE_SOCKADDR
#define DECLARE_SOCKADDR(type, dst, src) type dst = (type)(src)
#endif

static inline int copy_safe_from_sockptr(void *dst, size_t ksize,
                                         sockptr_t src, size_t usize)
{
  size_t ncopy;

  if (dst == NULL || src == NULL)
    {
      return -EFAULT;
    }

  ncopy = ksize < usize ? ksize : usize;
  memcpy(dst, src, ncopy);
  if (ncopy < ksize)
    {
      memset((char *)dst + ncopy, 0, ksize - ncopy);
    }

  return 0;
}

#ifndef CAP_NET_ADMIN
#define CAP_NET_ADMIN 12
#endif
#ifndef CAP_NET_BIND_SERVICE
#define CAP_NET_BIND_SERVICE 10
#endif
#ifndef CAP_NET_RAW
#define CAP_NET_RAW 13
#endif

static inline int capable(int cap)
{
  (void)cap;
  return 1;
}

static inline int sk_capable(const struct sock *sk, int cap)
{
  (void)sk;
  (void)cap;
  return 1;
}

int sock_register(const struct net_proto_family *ops);
void sock_unregister(int family);
int proto_register(struct proto *prot, int alloc_slab);
void proto_unregister(struct proto *prot);

static inline __poll_t datagram_poll(struct file *file,
                                     struct socket *sock,
                                     poll_table *wait)
{
  (void)file;
  (void)sock;
  (void)wait;
  return 0;
}

static inline int sock_no_listen(struct socket *sock, int len)
{
  (void)sock;
  (void)len;
  return -EOPNOTSUPP;
}

static inline int sock_no_bind(struct socket *sock, struct sockaddr *addr,
                               int addr_len)
{
  (void)sock;
  (void)addr;
  (void)addr_len;
  return -EOPNOTSUPP;
}

static inline int sock_no_getname(struct socket *sock, struct sockaddr *addr,
                                  int peer)
{
  (void)sock;
  (void)addr;
  (void)peer;
  return -EOPNOTSUPP;
}

static inline int sock_no_sendmsg(struct socket *sock, struct msghdr *msg,
                                  size_t len)
{
  (void)sock;
  (void)msg;
  (void)len;
  return -EOPNOTSUPP;
}

static inline int sock_no_recvmsg(struct socket *sock, struct msghdr *msg,
                                  size_t len, int flags)
{
  (void)sock;
  (void)msg;
  (void)len;
  (void)flags;
  return -EOPNOTSUPP;
}

static inline int sock_no_shutdown(struct socket *sock, int flags)
{
  (void)sock;
  (void)flags;
  return -EOPNOTSUPP;
}

static inline int sock_no_connect(struct socket *sock, struct sockaddr *addr,
                                  int addr_len, int flags)
{
  (void)sock;
  (void)addr;
  (void)addr_len;
  (void)flags;
  return -EOPNOTSUPP;
}

static inline int sock_no_socketpair(struct socket *sock1,
                                     struct socket *sock2)
{
  (void)sock1;
  (void)sock2;
  return -EOPNOTSUPP;
}

static inline int sock_no_accept(struct socket *sock,
                                 struct socket *newsock,
                                 struct proto_accept_arg *arg)
{
  (void)sock;
  (void)newsock;
  (void)arg;
  return -EOPNOTSUPP;
}

static inline int sock_gettstamp(struct socket *sock, void *userstamp,
                                 bool timeval, bool time32)
{
  (void)sock;
  (void)userstamp;
  (void)timeval;
  (void)time32;
  return -EOPNOTSUPP;
}

static inline int sock_no_mmap(struct file *file, struct socket *sock,
                               struct vm_area_struct *vma)
{
  (void)file;
  (void)sock;
  (void)vma;
  return -EOPNOTSUPP;
}

static inline struct sk_buff *sock_alloc_send_skb(struct sock *sk,
                                                  unsigned long len,
                                                  int nb, int *err)
{
  struct sk_buff *skb;

  (void)sk;
  (void)nb;
  skb = alloc_skb(len, GFP_KERNEL);
  if (skb == NULL && err != NULL)
    {
      *err = -ENOMEM;
    }
  else if (err != NULL)
    {
      *err = 0;
    }

  return skb;
}

#endif
