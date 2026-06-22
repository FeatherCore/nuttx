#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_CFG80211_COMPAT_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_CFG80211_COMPAT_H

#include <nuttx/config.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/endian.h>

#include <nuttx/kmalloc.h>
#include <nuttx/clock.h>
#include <nuttx/irq.h>
#include <nuttx/wqueue.h>

#ifdef CONFIG_WL_NUTTX_HWSIM_DEBUG
#  define nuttx_hwsim_debugf(...) printf(__VA_ARGS__)
#else
#  define nuttx_hwsim_debugf(...) do { } while (0)
#endif

#ifdef CONFIG_WL_NUTTX_HWSIM_AMPDU_PROOF
#  define nuttx_hwsim_ampdu_prooff(...) printf(__VA_ARGS__)
#else
#  define nuttx_hwsim_ampdu_prooff(...) do { } while (0)
#endif

#ifdef CONFIG_WL_NUTTX_HWSIM_AMSDU_PROOF
#  define nuttx_hwsim_amsdu_prooff(...) printf(__VA_ARGS__)
#else
#  define nuttx_hwsim_amsdu_prooff(...) do { } while (0)
#endif

#ifdef CONFIG_WL_NUTTX_HWSIM_DCM_PROOF
#  define nuttx_hwsim_dcm_prooff(...) printf(__VA_ARGS__)
#else
#  define nuttx_hwsim_dcm_prooff(...) do { } while (0)
#endif

#ifdef CONFIG_WL_NUTTX_HWSIM_PS_PROOF
#  define nuttx_hwsim_ps_prooff(...) printf(__VA_ARGS__)
#else
#  define nuttx_hwsim_ps_prooff(...) do { } while (0)
#endif

#define NUTTX_HWSIM_MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define NUTTX_HWSIM_MAC_ARG(addr) \
  (addr)[0], (addr)[1], (addr)[2], (addr)[3], (addr)[4], (addr)[5]

#undef list_add_tail
#undef list_entry
#undef list_first_entry
#undef list_last_entry
#undef list_next_entry
#undef list_prepare_entry
#undef list_prev_entry

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
typedef unsigned short __le16;
typedef unsigned int __le32;
typedef unsigned long long __le64;
typedef unsigned short __be16;
typedef unsigned int __be32;
typedef unsigned long long __be64;
typedef unsigned short __sum16;
typedef unsigned int __wsum;
typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;
typedef unsigned long long __u64;
typedef signed char __s8;
typedef signed short __s16;
typedef signed int __s32;
typedef signed long long __s64;
typedef int gfp_t;
typedef long long ktime_t;

#ifndef BIT
#  define BIT(n) (1UL << (n))
#endif

#ifndef BIT_ULL
#  define BIT_ULL(n) (1ULL << (n))
#endif

#ifndef GENMASK
#  define GENMASK(h, l) \
    (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (sizeof(long) * 8 - 1 - (h))))
#endif

#ifndef GENMASK_ULL
#  define GENMASK_ULL(h, l) \
    (((~0ULL) - (1ULL << (l)) + 1) & (~0ULL >> (sizeof(long long) * 8 - 1 - (h))))
#endif

#ifndef BITS_PER_LONG
#  define BITS_PER_LONG (sizeof(unsigned long) * 8)
#endif

#ifndef BITS_PER_BYTE
#  define BITS_PER_BYTE 8
#endif

#ifndef BITS_TO_LONGS
#  define BITS_TO_LONGS(nr) (((nr) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#endif

#ifndef __WIRELESS_IEEE80211_BITOPS_DEFINED
#define __WIRELESS_IEEE80211_BITOPS_DEFINED
static inline bool test_bit(unsigned int nr, const unsigned long *addr)
{
  return !!(addr[nr / BITS_PER_LONG] & BIT(nr % BITS_PER_LONG));
}

static inline void set_bit(unsigned int nr, unsigned long *addr)
{
  addr[nr / BITS_PER_LONG] |= BIT(nr % BITS_PER_LONG);
}

static inline void __set_bit(unsigned int nr, unsigned long *addr)
{
  set_bit(nr, addr);
}

static inline void clear_bit(unsigned int nr, unsigned long *addr)
{
  addr[nr / BITS_PER_LONG] &= ~BIT(nr % BITS_PER_LONG);
}

static inline void __clear_bit(unsigned int nr, unsigned long *addr)
{
  clear_bit(nr, addr);
}

static inline bool test_and_clear_bit(unsigned int nr, unsigned long *addr)
{
  bool old = test_bit(nr, addr);

  clear_bit(nr, addr);
  return old;
}

static inline bool test_and_set_bit(unsigned int nr, unsigned long *addr)
{
  bool old = test_bit(nr, addr);

  set_bit(nr, addr);
  return old;
}
#endif

#ifndef __bf_shf
#  define __bf_shf(x) (__builtin_ffsll(x) - 1)
#endif

#ifndef u32_get_bits
#  define u32_get_bits(value, mask) (((u32)(value) & (u32)(mask)) >> __bf_shf(mask))
#endif

#ifndef u8_get_bits
#  define u8_get_bits(value, mask) (((u8)(value) & (u8)(mask)) >> __bf_shf(mask))
#endif

#ifndef u16_get_bits
#  define u16_get_bits(value, mask) (((u16)(value) & (u16)(mask)) >> __bf_shf(mask))
#endif

#ifndef u8_encode_bits
#  define u8_encode_bits(value, mask) ((u8)(((u8)(value) << __bf_shf(mask)) & (u8)(mask)))
#endif

#ifndef u16_encode_bits
#  define u16_encode_bits(value, mask) ((u16)(((u16)(value) << __bf_shf(mask)) & (u16)(mask)))
#endif

#ifndef u32_encode_bits
#  define u32_encode_bits(value, mask) ((u32)(((u32)(value) << __bf_shf(mask)) & (u32)(mask)))
#endif

#ifndef u8_replace_bits
#  define u8_replace_bits(old, val, mask) \
    (((u8)(old) & ~(u8)(mask)) | u8_encode_bits(val, mask))
#endif

#ifndef __WIRELESS_IEEE80211_BYTEORDER_HELPERS_DEFINED
#define __WIRELESS_IEEE80211_BYTEORDER_HELPERS_DEFINED
static inline u16 swab16(u16 value)
{
  return (value << 8) | (value >> 8);
}

static inline u32 rol32(u32 word, unsigned int shift)
{
  return (word << (shift & 31)) | (word >> ((32 - shift) & 31));
}

static inline u32 ror32(u32 word, unsigned int shift)
{
  return (word >> (shift & 31)) | (word << ((32 - shift) & 31));
}

static inline u16 ror16(u16 word, unsigned int shift)
{
  return (word >> (shift & 15)) | (word << ((16 - shift) & 15));
}

static inline u64 le64_to_cpu(__le64 value)
{
  return value;
}

static inline __le64 cpu_to_le64(u64 value)
{
  return value;
}

#define put_unaligned(value, p) \
  do \
    { \
      typeof(value) __tmp = (value); \
      memcpy((p), &__tmp, sizeof(__tmp)); \
    } \
  while (0)

static inline void put_unaligned_be16(u16 value, void *p)
{
  u8 *b = p;

  b[0] = value >> 8;
  b[1] = value;
}

static inline void put_unaligned_le32(u32 value, void *p)
{
  u8 *b = p;

  b[0] = value;
  b[1] = value >> 8;
  b[2] = value >> 16;
  b[3] = value >> 24;
}

static inline void put_unaligned_le64(u64 value, void *p)
{
  u8 *b = p;
  int i;

  for (i = 0; i < 8; i++)
    {
      b[i] = value >> (i * 8);
    }
}
#endif

#ifndef le16_encode_bits
#  define le16_encode_bits(value, mask) ((__le16)u16_encode_bits(value, mask))
#endif

#ifndef le32_encode_bits
#  define le32_encode_bits(value, mask) ((__le32)u32_encode_bits(value, mask))
#endif

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef array_size
#  define array_size(size, count) ((size_t)(size) * (size_t)(count))
#endif

#ifndef size_add
#  define size_add(a, b) ((size_t)(a) + (size_t)(b))
#endif

#ifndef typecheck
#  define typecheck(type, x) 1
#endif

#ifndef WRITE_ONCE
#  define WRITE_ONCE(x, val) ((x) = (val))
#endif

#ifndef READ_ONCE
#  define READ_ONCE(x) (x)
#endif

#ifndef __WIRELESS_IEEE80211_BITMAP_HELPERS_DEFINED
#define __WIRELESS_IEEE80211_BITMAP_HELPERS_DEFINED
static inline void bitmap_copy(unsigned long *dst, const unsigned long *src,
                               unsigned int nbits)
{
  memcpy(dst, src, BITS_TO_LONGS(nbits) * sizeof(unsigned long));
}

static inline bool bitmap_subset(const unsigned long *src1,
                                 const unsigned long *src2,
                                 unsigned int nbits)
{
  unsigned int i;

  for (i = 0; i < BITS_TO_LONGS(nbits); i++)
    {
      if ((src1[i] & ~src2[i]) != 0)
        {
          return false;
        }
    }

  return true;
}

static inline bool bitmap_empty(const unsigned long *src, unsigned int nbits)
{
  unsigned int i;

  for (i = 0; i < BITS_TO_LONGS(nbits); i++)
    {
      if (src[i] != 0)
        {
          return false;
        }
    }

  return true;
}
#endif

#ifndef IS_ENABLED
#  define __ARG_PLACEHOLDER_1 0,
#  define __take_second_arg(__ignored, val, ...) val
#  define __is_defined(x) ___is_defined(x)
#  define ___is_defined(val) ____is_defined(__ARG_PLACEHOLDER_##val)
#  define ____is_defined(arg1_or_junk) __take_second_arg(arg1_or_junk 1, 0)
#  define IS_ENABLED(option) __is_defined(option)
#endif

#ifndef IS_REACHABLE
#  define IS_REACHABLE(option) IS_ENABLED(option)
#endif

#define CONFIG_CFG80211 1
#define CONFIG_MAC80211_STA_HASH_MAX_SIZE 0
#define CONFIG_MAC80211_RC_DEFAULT "minstrel_ht"

#ifndef DIV_ROUND_UP
#  define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#endif

#ifndef container_of
#  define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#ifndef min
#  define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#  define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef swap
#  define swap(a, b) do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)
#endif

#ifndef min_t
#  define min_t(type, a, b) ((type)min((type)(a), (type)(b)))
#endif

#ifndef max_t
#  define max_t(type, a, b) ((type)max((type)(a), (type)(b)))
#endif

#ifndef round_up
#  define round_up(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#endif

#ifndef ALIGN
#  define ALIGN(x, a) ((((x) + ((a) - 1)) / (a)) * (a))
#endif

#ifndef for_each_set_bit
#  define for_each_set_bit(bit, addr, size) \
    for ((bit) = 0; (bit) < (size); (bit)++) \
      if (!(*(addr) & BIT(bit))) { } else
#endif

#ifndef __aligned
#  define __aligned(x) __attribute__((aligned(x)))
#endif

#ifndef __packed
#  define __packed __attribute__((packed))
#endif

#ifndef __user
#  define __user
#endif

#ifndef __rcu
#  define __rcu
#endif

#ifndef __counted_by
#  define __counted_by(member)
#endif

#ifndef __percpu
#  define __percpu
#endif

#ifndef __force
#  define __force
#endif

#ifndef __bitwise
#  define __bitwise
#endif

#ifndef __iftd
#  define __iftd
#endif

#ifndef __acquires
#  define __acquires(x)
#endif

#ifndef __releases
#  define __releases(x)
#endif

#ifndef __acquire
#  define __acquire(x)
#endif

#ifndef __release
#  define __release(x)
#endif

#ifndef __read_mostly
#  define __read_mostly
#endif

#ifndef __maybe_unused
#  define __maybe_unused __attribute__((unused))
#endif

#ifndef __must_check
#  define __must_check __attribute__((warn_unused_result))
#endif

#ifndef __pure
#  define __pure
#endif

#ifndef __init
#  define __init
#endif

#ifndef __exit
#  define __exit
#endif

#ifndef __net_exit
#  define __net_exit
#endif

#ifndef __net_init
#  define __net_init
#endif

#ifndef __attribute_const__
#  define __attribute_const__ __attribute__((const))
#endif

#ifndef fallthrough
#  define fallthrough do { } while (0)
#endif

#ifndef WARN_ON
#  define WARN_ON(cond) ({ bool __ret = !!(cond); __ret; })
#endif

#ifndef WARN_ON_ONCE
#  define WARN_ON_ONCE(cond) WARN_ON(cond)
#endif

#ifndef WARN
#  define WARN(cond, fmt, args...) WARN_ON(cond)
#endif

#ifndef WARN_ONCE
#  define WARN_ONCE(cond, fmt, args...) WARN_ON(cond)
#endif

#ifndef struct_group
#  define struct_group(NAME, MEMBERS...) \
    union { struct { MEMBERS }; struct { MEMBERS } NAME; }
#endif

#ifndef __struct_group
#  define __struct_group(TAG, NAME, ATTRS, MEMBERS...) \
    union { struct { MEMBERS } ATTRS; struct TAG { MEMBERS } ATTRS NAME; }
#endif

#ifndef DECLARE_FLEX_ARRAY
#  define DECLARE_FLEX_ARRAY(TYPE, NAME) TYPE NAME[0]
#endif

#ifndef DEFINE_RAW_FLEX
#  define DEFINE_RAW_FLEX(TYPE, NAME, MEMBER, COUNT) \
    unsigned char NAME##_raw_flex[sizeof(TYPE) + (COUNT)] = { 0 }; \
    TYPE *NAME = (TYPE *)NAME##_raw_flex
#endif

#ifndef offsetofend
#  define offsetofend(TYPE, MEMBER) \
    (offsetof(TYPE, MEMBER) + sizeof(((TYPE *)0)->MEMBER))
#endif

#ifndef sizeof_field
#  define sizeof_field(TYPE, MEMBER) sizeof(((TYPE *)0)->MEMBER)
#endif

#ifndef memset_after
#  define memset_after(obj, val, member) \
    memset((char *)(obj) + offsetofend(typeof(*(obj)), member), val, \
           sizeof(*(obj)) - offsetofend(typeof(*(obj)), member))
#endif

#ifndef struct_size
#  define struct_size(ptr, member, count) \
    (sizeof(*(ptr)) + sizeof((ptr)->member[0]) * (count))
#endif

#ifndef flex_array_size
#  define flex_array_size(ptr, member, count) \
    (sizeof((ptr)->member[0]) * (count))
#endif

#ifndef roundup
#  define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#endif

#ifndef EXPORT_SYMBOL
#  define EXPORT_SYMBOL(sym)
#endif

#ifndef EXPORT_SYMBOL_GPL
#  define EXPORT_SYMBOL_GPL(sym)
#endif

#ifndef DEFINE_GUARD
#  define DEFINE_GUARD(name, type, lock, unlock)
#endif

#define cfg80211_compat_guard_noop(...) do { } while (0)
#define guard(name) cfg80211_compat_guard_noop
#define scoped_guard(name, ptr) for (int __sg_once = 1; __sg_once; __sg_once = 0)

#ifndef alloc_hooks
#  define alloc_hooks(expr) (expr)
#endif

#ifndef MODULE_LICENSE
#  define MODULE_LICENSE(x)
#endif

#ifndef MODULE_DESCRIPTION
#  define MODULE_DESCRIPTION(x)
#endif

#ifndef MODULE_AUTHOR
#  define MODULE_AUTHOR(x)
#endif

#define GFP_KERNEL 0
#define GFP_ATOMIC 0

#ifndef U32_MAX
#  define U32_MAX UINT32_MAX
#endif

#ifndef U8_MAX
#  define U8_MAX UINT8_MAX
#endif

#ifndef S32_MIN
#  define S32_MIN INT32_MIN
#endif

#ifndef S32_MAX
#  define S32_MAX INT32_MAX
#endif

#ifndef IS_ALIGNED
#  define IS_ALIGNED(x, a) (((x) & ((typeof(x))(a) - 1)) == 0)
#endif

#ifndef __ro_after_init
#  define __ro_after_init
#endif

#ifndef __free
#  define __free(fn)
#endif

#ifndef VLAN_N_VID
#  define VLAN_N_VID 4096
#endif

#ifndef NLMSG_DEFAULT_SIZE
#  define NLMSG_DEFAULT_SIZE 8192
#endif

#ifndef NLMSG_GOODSIZE
#  define NLMSG_GOODSIZE NLMSG_DEFAULT_SIZE
#endif

#ifndef NET_NAME_USER
#  define NET_NAME_USER 3
#endif

#ifndef NET_NAME_ENUM
#  define NET_NAME_ENUM 1
#endif

#ifndef NET_NAME_UNKNOWN
#  define NET_NAME_UNKNOWN 0
#endif

#ifndef KERN_DEBUG
#  define KERN_DEBUG ""
#endif

#ifndef KERN_INFO
#  define KERN_INFO ""
#endif

#ifndef PAGE_SIZE
#  define PAGE_SIZE 4096
#endif

#define CHECKSUM_NONE 0
#define CHECKSUM_UNNECESSARY 1
#define CHECKSUM_COMPLETE 2
#define CHECKSUM_PARTIAL 3
#define PACKET_OTHERHOST 3

typedef int netdev_tx_t;
#define NETDEV_TX_OK 0
#define NETDEV_TX_BUSY 1
#define ETH_FRAME_LEN 1514
#define __WIRELESS_IEEE80211_SKBUFF_COMPAT_INLINE 1

#ifndef __WIRELESS_IEEE80211_IDA_DEFINED
#define __WIRELESS_IEEE80211_IDA_DEFINED
struct ida
{
  int next;
};

#define IDA_INIT(name) { 0 }
#define DEFINE_IDA(name) struct ida name = IDA_INIT(name)

static inline int ida_alloc(struct ida *ida, gfp_t gfp)
{
  (void)gfp;
  return ida->next++;
}

static inline void ida_free(struct ida *ida, unsigned int id)
{
  (void)ida;
  (void)id;
}
#endif

struct static_key_false
{
  struct
  {
    int dummy;
  } key;
  bool enabled;
};

#define DEFINE_STATIC_KEY_FALSE(name) struct static_key_false name = { 0 }
#define DECLARE_STATIC_KEY_FALSE(name) extern struct static_key_false name
static inline bool static_branch_unlikely(struct static_key_false *key)
{
  return key && key->enabled;
}
static inline bool static_key_false(void *key)
{
  (void)key;
  return false;
}
static inline void static_branch_enable(struct static_key_false *key)
{
  if (key)
    {
      key->enabled = true;
    }
}
static inline void static_branch_disable(struct static_key_false *key)
{
  if (key)
    {
      key->enabled = false;
    }
}

struct netlink_ext_ack
{
  const char *_msg;
};

typedef u64 netdev_features_t;
#define NETIF_F_IP_CSUM BIT_ULL(0)
#define NETIF_F_IPV6_CSUM BIT_ULL(1)
#define NETIF_F_HW_CSUM BIT_ULL(2)
#define NETIF_F_SG BIT_ULL(3)
#define NETIF_F_HIGHDMA BIT_ULL(4)
#define NETIF_F_GSO_SOFTWARE BIT_ULL(5)
#define NETIF_F_HW_TC BIT_ULL(6)
#define NETIF_F_RXCSUM BIT_ULL(7)

struct netdev_hw_addr_list
{
  int count;
};

enum tc_setup_type
{
  TC_SETUP_UNSPEC = 0,
};

struct net_device_path_ctx
{
  struct net_device *dev;
  const u8 *daddr;
};

struct net_device_path
{
  int dummy;
};

struct napi_struct
{
  int dummy;
};

struct ethtool_drvinfo
{
  char driver[32];
  char version[32];
  char fw_version[32];
  char bus_info[32];
};

static inline size_t strscpy(char *dest, const char *src, size_t count)
{
  size_t len;

  if (!count)
    {
      return 0;
    }

  len = strlen(src);
  if (len >= count)
    {
      len = count - 1;
    }

  memcpy(dest, src, len);
  dest[len] = '\0';
  return len;
}

#define NL_SET_ERR_MSG(extack, msg) do { \
  if (extack) \
    { \
      (extack)->_msg = (msg); \
    } \
} while (0)

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

#define __WIRELESS_LINUX_COMPAT_LIST_TYPES_DEFINED 1

struct skb_shared_hwtstamps
{
  ktime_t hwtstamp;
};

struct skb_shared_info;

struct sk_buff
{
  struct list_head list;
  struct sk_buff *next;
  struct sk_buff *prev;
  unsigned char *head;
  unsigned char *data;
  unsigned char *tail;
  unsigned char *end;
  unsigned int len;
  unsigned int data_len;
  unsigned int truesize;
  unsigned int priority;
  unsigned int queue_mapping;
  unsigned int mark;
  unsigned int ip_summed;
  unsigned int mac_header;
  unsigned int network_header;
  unsigned int transport_header;
  unsigned int inner_transport_header;
  bool mac_header_set;
  bool head_frag;
  bool encapsulation;
  bool wifi_acked_valid;
  bool wifi_acked;
  __be16 protocol;
  struct net_device *dev;
  struct sock *sk;
  unsigned int pkt_type;
  void (*destructor)(struct sk_buff *skb);
  struct skb_shared_hwtstamps hwtstamps;
  struct skb_shared_info *shinfo;
  unsigned char cb[64];
};

struct sk_buff_head
{
  struct list_head list;
  struct sk_buff *next;
  struct sk_buff *prev;
  unsigned int qlen;
  spinlock_t lock;
};

struct page
{
  unsigned char data[PAGE_SIZE];
};

typedef struct
{
  struct page *page;
  void *data;
  unsigned int size;
} skb_frag_t;

struct skb_shared_info
{
  int nr_frags;
  unsigned int gso_size;
  skb_frag_t frags[8];
  struct sk_buff *frag_list;
};

#define NET_SKB_PAD 32

static inline unsigned char *skb_tail_pointer(const struct sk_buff *skb)
{
  return skb->tail;
}

static inline unsigned int skb_tailroom(const struct sk_buff *skb)
{
  return skb->end - skb->tail;
}

static inline unsigned char *skb_put(struct sk_buff *skb, unsigned int len)
{
  unsigned char *tmp = skb->tail;
  skb->tail += len;
  skb->len += len;
  return tmp;
}

static inline void *skb_put_data(struct sk_buff *skb, const void *data,
                                 unsigned int len)
{
  void *tmp = skb_put(skb, len);

  memcpy(tmp, data, len);
  return tmp;
}

static inline void skb_copy_from_linear_data(const struct sk_buff *skb,
                                             void *to, unsigned int len)
{
  memcpy(to, skb->data, len);
}

static inline void *skb_put_zero(struct sk_buff *skb, unsigned int len)
{
  void *tmp = skb_put(skb, len);

  memset(tmp, 0, len);
  return tmp;
}

static inline u8 *skb_put_u8(struct sk_buff *skb, u8 value)
{
  u8 *tmp = skb_put(skb, sizeof(value));

  *tmp = value;
  return tmp;
}

static inline unsigned char *skb_push(struct sk_buff *skb, unsigned int len)
{
  skb->data -= len;
  skb->len += len;
  return skb->data;
}

static inline unsigned char *skb_pull(struct sk_buff *skb, unsigned int len)
{
  if (skb->len < len)
    {
      return NULL;
    }

  skb->data += len;
  skb->len -= len;
  return skb->data;
}

static inline unsigned int skb_headroom(const struct sk_buff *skb)
{
  return skb->data - skb->head;
}

static inline bool skb_is_nonlinear(const struct sk_buff *skb)
{
  (void)skb;
  return false;
}

static inline int skb_network_offset(const struct sk_buff *skb)
{
  (void)skb;
  return 0;
}

static inline unsigned char *skb_network_header(const struct sk_buff *skb)
{
  return skb->head + skb->network_header;
}

static inline unsigned char *skb_mac_header(const struct sk_buff *skb)
{
  return skb->head + skb->mac_header;
}

static inline unsigned int skb_headlen(const struct sk_buff *skb)
{
  return skb->len - skb->data_len;
}

static inline void skb_set_tail_pointer(struct sk_buff *skb,
                                        const int offset)
{
  skb->tail = skb->data + offset;
}

static inline void skb_reset_mac_header(struct sk_buff *skb)
{
  skb->mac_header = skb->data - skb->head;
  skb->mac_header_set = true;
}

static inline void skb_reset_network_header(struct sk_buff *skb)
{
  skb->network_header = skb->data - skb->head;
}

static inline void skb_set_network_header(struct sk_buff *skb, const int offset)
{
  skb->network_header = skb->data - skb->head + offset;
}

static inline void skb_set_mac_header(struct sk_buff *skb, const int offset)
{
  skb->mac_header = skb->data - skb->head + offset;
  skb->mac_header_set = true;
}

static inline void skb_set_transport_header(struct sk_buff *skb,
                                            const int offset)
{
  skb->transport_header = skb->data - skb->head + offset;
}

static inline void skb_set_inner_transport_header(struct sk_buff *skb,
                                                  const int offset)
{
  skb->inner_transport_header = skb->data - skb->head + offset;
}

static inline bool skb_mac_header_was_set(const struct sk_buff *skb)
{
  return skb->mac_header_set;
}

static inline struct skb_shared_info *skb_shinfo(struct sk_buff *skb)
{
  return skb->shinfo;
}

static inline struct skb_shared_hwtstamps *skb_hwtstamps(struct sk_buff *skb)
{
  return &skb->hwtstamps;
}

static inline s64 ktime_to_ns(ktime_t kt)
{
  return kt;
}

static inline void skb_reserve(struct sk_buff *skb, unsigned int len)
{
  skb->data += len;
  skb->tail += len;
}

static inline int skb_copy_bits(const struct sk_buff *skb, int offset,
                                void *to, int len)
{
  unsigned int linear;
  unsigned int copied = 0;
  struct sk_buff *frag;

  if (offset < 0 || (unsigned int)(offset + len) > skb->len)
    {
      return -EINVAL;
    }

  linear = skb_headlen(skb);
  if ((unsigned int)offset < linear)
    {
      unsigned int copy = linear - offset;

      if (copy > (unsigned int)len)
        {
          copy = len;
        }

      memcpy(to, skb->data + offset, copy);
      copied += copy;
      offset = 0;
    }
  else
    {
      offset -= linear;
    }

  if (copied == (unsigned int)len)
    {
      return 0;
    }

  if (skb->shinfo == NULL)
    {
      return -EINVAL;
    }

  for (frag = skb->shinfo->frag_list; frag != NULL; frag = frag->next)
    {
      if ((unsigned int)offset < frag->len)
        {
          unsigned int copy = frag->len - offset;

          if (copy > (unsigned int)len - copied)
            {
              copy = len - copied;
            }

          memcpy((unsigned char *)to + copied, frag->data + offset, copy);
          copied += copy;
          offset = 0;

          if (copied == (unsigned int)len)
            {
              return 0;
            }
        }
      else
        {
          offset -= frag->len;
        }
    }

  if (copied != (unsigned int)len)
    {
      return -EINVAL;
    }

  return 0;
}

static inline bool pskb_may_pull(struct sk_buff *skb, unsigned int len)
{
  return skb->len >= len;
}

static inline unsigned char *pskb_pull(struct sk_buff *skb, unsigned int len)
{
  if (skb->len < len)
    {
      return NULL;
    }

  skb->data += len;
  skb->len -= len;
  return skb->data;
}

static inline struct sk_buff *dev_alloc_skb(unsigned int len)
{
  struct sk_buff *skb = kmm_zalloc(sizeof(*skb));
  if (!skb)
    {
      return NULL;
    }

  skb->head = kmm_zalloc(len);
  if (!skb->head)
    {
      kmm_free(skb);
      return NULL;
    }

  skb->shinfo = kmm_zalloc(sizeof(*skb->shinfo));
  if (!skb->shinfo)
    {
      kmm_free(skb->head);
      kmm_free(skb);
      return NULL;
    }

  skb->data = skb->head;
  skb->tail = skb->head;
  skb->end = skb->head + len;
  return skb;
}

static inline struct sk_buff *__dev_alloc_skb(unsigned int len, gfp_t gfp)
{
  (void)gfp;
  return dev_alloc_skb(len);
}

static inline struct sk_buff *alloc_skb(unsigned int size, gfp_t priority)
{
  return __dev_alloc_skb(size, priority);
}

static inline struct sk_buff *netdev_alloc_skb(struct net_device *dev,
                                               unsigned int len)
{
  struct sk_buff *skb = dev_alloc_skb(len);

  if (skb != NULL)
    {
      skb->dev = dev;
    }

  return skb;
}

static inline void dev_kfree_skb(struct sk_buff *skb)
{
  if (skb)
    {
      if (skb->shinfo)
        {
          struct sk_buff *frag = skb->shinfo->frag_list;

          while (frag)
            {
              struct sk_buff *next = frag->next;

              dev_kfree_skb(frag);
              frag = next;
            }

          kmm_free(skb->shinfo);
        }

      kmm_free(skb->head);
      kmm_free(skb);
    }
}

static inline void dev_kfree_skb_any(struct sk_buff *skb)
{
  dev_kfree_skb(skb);
}

static inline void dev_kfree_skb_irq(struct sk_buff *skb)
{
  dev_kfree_skb(skb);
}

static inline void consume_skb(struct sk_buff *skb)
{
  dev_kfree_skb(skb);
}

static inline int skb_linearize(struct sk_buff *skb)
{
  struct sk_buff *frag;
  unsigned int headroom;
  unsigned char *new_head;
  unsigned char *new_data;

  if (skb == NULL || skb->shinfo == NULL)
    {
      return -EINVAL;
    }

  if (skb->data_len == 0 && skb->shinfo->frag_list == NULL &&
      skb->shinfo->nr_frags == 0)
    {
      return 0;
    }

  headroom = skb_headroom(skb);
  new_head = kmm_zalloc(headroom + skb->len);
  if (new_head == NULL)
    {
      return -ENOMEM;
    }

  new_data = new_head + headroom;
  if (skb_copy_bits(skb, 0, new_data, skb->len) < 0)
    {
      kmm_free(new_head);
      return -EINVAL;
    }

  kmm_free(skb->head);
  skb->head = new_head;
  skb->data = new_data;
  skb->tail = skb->data + skb->len;
  skb->end = skb->tail;
  skb->data_len = 0;
  skb->shinfo->nr_frags = 0;

  frag = skb->shinfo->frag_list;
  skb->shinfo->frag_list = NULL;
  while (frag)
    {
      struct sk_buff *next = frag->next;

      dev_kfree_skb(frag);
      frag = next;
    }

  return 0;
}

static inline int pskb_trim(struct sk_buff *skb, unsigned int len)
{
  if (len > skb->len)
    {
      return -EINVAL;
    }

  skb->len = len;
  skb->tail = skb->data + len;
  return 0;
}

#define __pskb_trim(skb, len) pskb_trim(skb, len)

static inline void skb_trim(struct sk_buff *skb, unsigned int len)
{
  (void)pskb_trim(skb, len);
}

static inline int pskb_expand_head(struct sk_buff *skb, int nhead, int ntail,
                                   gfp_t gfp)
{
  unsigned int old_headroom;
  unsigned int new_size;
  unsigned char *new_head;

  (void)gfp;

  old_headroom = skb_headroom(skb);
  new_size = nhead + skb->len + ntail;
  new_head = kmm_zalloc(new_size);
  if (new_head == NULL)
    {
      return -ENOMEM;
    }

  if (skb_copy_bits(skb, 0, new_head + nhead, skb->len) < 0)
    {
      kmm_free(new_head);
      return -EINVAL;
    }

  kmm_free(skb->head);
  skb->head = new_head;
  skb->data = new_head + nhead;
  skb->tail = skb->data + skb->len;
  skb->end = new_head + new_size;
  skb->data_len = 0;

  if (skb->shinfo)
    {
      struct sk_buff *frag = skb->shinfo->frag_list;

      skb->shinfo->frag_list = NULL;
      skb->shinfo->nr_frags = 0;
      while (frag)
        {
          struct sk_buff *next = frag->next;

          dev_kfree_skb(frag);
          frag = next;
        }
    }

  if (skb->mac_header >= old_headroom)
    {
      skb->mac_header = nhead + skb->mac_header - old_headroom;
    }

  if (skb->network_header >= old_headroom)
    {
      skb->network_header = nhead + skb->network_header - old_headroom;
    }

  return 0;
}

static inline struct sk_buff *skb_copy(const struct sk_buff *skb, gfp_t gfp)
{
  struct sk_buff *copy;

  (void)gfp;

  copy = dev_alloc_skb(skb->len + skb_headroom(skb));
  if (copy == NULL)
    {
      return NULL;
    }

  skb_reserve(copy, skb_headroom(skb));
  if (skb_copy_bits(skb, 0, skb_put(copy, skb->len), skb->len) < 0)
    {
      dev_kfree_skb(copy);
      return NULL;
    }

  copy->protocol = skb->protocol;
  copy->dev = skb->dev;
  copy->queue_mapping = skb->queue_mapping;
  return copy;
}

static inline struct sk_buff *skb_copy_expand(const struct sk_buff *skb,
                                              int newheadroom,
                                              int newtailroom,
                                              gfp_t gfp)
{
  struct sk_buff *copy;

  (void)gfp;
  copy = dev_alloc_skb(newheadroom + skb->len + newtailroom);
  if (copy == NULL)
    {
      return NULL;
    }

  skb_reserve(copy, newheadroom);
  if (skb_copy_bits(skb, 0, skb_put(copy, skb->len), skb->len) < 0)
    {
      dev_kfree_skb(copy);
      return NULL;
    }

  copy->protocol = skb->protocol;
  copy->dev = skb->dev;
  copy->queue_mapping = skb->queue_mapping;
  return copy;
}

static inline struct sk_buff *skb_clone(struct sk_buff *skb, gfp_t gfp)
{
  return skb_copy(skb, gfp);
}

static inline struct sk_buff *skb_clone_sk(struct sk_buff *skb)
{
  return skb_clone(skb, GFP_ATOMIC);
}

static inline bool skb_cloned(const struct sk_buff *skb)
{
  (void)skb;
  return false;
}

static inline bool skb_clone_writable(const struct sk_buff *skb,
                                      unsigned int len)
{
  return skb->len >= len;
}

static inline struct sk_buff *skb_share_check(struct sk_buff *skb,
                                              gfp_t gfp)
{
  (void)gfp;
  return skb;
}

static inline int skb_ensure_writable(struct sk_buff *skb, unsigned int len)
{
  return skb->len >= len ? 0 : -ENOMEM;
}

static inline bool skb_needs_linearize(struct sk_buff *skb,
                                       netdev_features_t features)
{
  (void)skb;
  (void)features;
  return false;
}

static inline int __skb_linearize(struct sk_buff *skb)
{
  return skb_linearize(skb);
}

static inline int skb_checksum_start_offset(const struct sk_buff *skb)
{
  (void)skb;
  return 0;
}

static inline int skb_csum_hwoffload_help(struct sk_buff *skb,
                                          netdev_features_t features)
{
  (void)skb;
  (void)features;
  return 0;
}

static inline int skb_checksum_help(struct sk_buff *skb)
{
  (void)skb;
  return 0;
}

static inline void skb_postpull_rcsum(struct sk_buff *skb,
                                      const void *start,
                                      unsigned int len)
{
  (void)skb;
  (void)start;
  (void)len;
}

static inline void skb_copy_queue_mapping(struct sk_buff *to,
                                          const struct sk_buff *from)
{
  to->queue_mapping = from->queue_mapping;
}

static inline void __skb_queue_purge(struct sk_buff_head *list)
{
  (void)list;
}

static inline void get_page(struct page *page)
{
  (void)page;
}

static inline struct page *virt_to_head_page(const void *addr)
{
  (void)addr;
  return NULL;
}

static inline void *page_address(struct page *page)
{
  (void)page;
  return NULL;
}

static inline struct page *skb_frag_page(const skb_frag_t *frag)
{
  return frag->page;
}

static inline void *skb_frag_address(const skb_frag_t *frag)
{
  return frag->data;
}

static inline unsigned int skb_frag_size(const skb_frag_t *frag)
{
  return frag->size;
}

static inline void skb_add_rx_frag(struct sk_buff *skb, int i,
                                   struct page *page, int off,
                                   int len, int size)
{
  (void)skb;
  (void)i;
  (void)page;
  (void)off;
  (void)len;
  (void)size;
}

static inline bool skb_vlan_tag_present(const struct sk_buff *skb)
{
  (void)skb;
  return false;
}

static inline u16 skb_vlan_tag_get(const struct sk_buff *skb)
{
  (void)skb;
  return 0;
}

static inline void *skb_header_pointer(const struct sk_buff *skb, int offset,
                                       int len, void *buffer)
{
  if ((unsigned int)(offset + len) > skb->len)
    {
      return NULL;
    }

  memcpy(buffer, skb->data + offset, len);
  return buffer;
}

#ifndef NUTTX_WIFI_USERSPACE_PORT
static inline bool ether_addr_equal(const u8 *addr1, const u8 *addr2)
{
  return memcmp(addr1, addr2, 6) == 0;
}
#endif

#ifndef likely
#  define likely(x) (x)
#endif

#ifndef unlikely
#  define unlikely(x) (x)
#endif

#undef htons
#define htons(x) ((__be16)htobe16(x))
#undef ntohs
#define ntohs(x) be16toh(x)
#undef htonl
#define htonl(x) ((__be32)htobe32(x))
#undef ntohl
#define ntohl(x) be32toh(x)
#define cpu_to_le16(x) ((__le16)(x))
#define le16_to_cpu(x) ((u16)(x))
#define __le16_to_cpu(x) le16_to_cpu(x)
#define cpu_to_le32(x) ((__le32)(x))
#define le32_to_cpu(x) ((u32)(x))
#define __le32_to_cpu(x) le32_to_cpu(x)
#define cpu_to_be16(x) ((__be16)htobe16(x))
#define be16_to_cpu(x) be16toh(x)
#define cpu_to_be32(x) ((__be32)htobe32(x))
#define be32_to_cpu(x) be32toh(x)
#define be16_to_cpup(p) be16_to_cpu(*(p))
#define be32_to_cpup(p) be32_to_cpu(*(p))

#define pr_debug(fmt, args...) do { } while (0)
#define pr_info(fmt, args...)  do { } while (0)
#define pr_err(fmt, args...)   do { } while (0)
#define pr_warn(fmt, args...)  do { } while (0)
#define pr_err_ratelimited(fmt, args...) pr_err(fmt, ##args)
#define pr_warn_ratelimited(fmt, args...) pr_warn(fmt, ##args)

static inline int scnprintf(char *buf, size_t size, const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start(ap, fmt);
  ret = vsnprintf(buf, size, fmt, ap);
  va_end(ap);

  if (ret < 0)
    {
      return ret;
    }

  return ret >= (int)size ? (int)size - 1 : ret;
}

static inline int vscnprintf(char *buf, size_t size, const char *fmt,
                             va_list ap)
{
  int ret;

  ret = vsnprintf(buf, size, fmt, ap);
  if (ret < 0)
    {
      return ret;
    }

  return ret >= (int)size ? (int)size - 1 : ret;
}

struct netlink_callback
{
  struct sk_buff *skb;
  struct nlmsghdr *nlh;
  void *data;
  long args[8];
  unsigned int min_dump_alloc;
  unsigned int portid;
  unsigned int seq;
  unsigned int prev_seq;
  struct netlink_ext_ack *extack;
};

typedef bool (*netlink_filter_fn)(struct sk_buff *skb, void *data);

#ifdef __WIRELESS_IEEE80211_EXTERNAL_STRUCT_SOCK
struct sock;
#else
struct sock
{
  int dummy;
};
#endif

static inline struct net *sock_net(const struct sock *sk)
{
  (void)sk;
  extern struct net init_net;
  return &init_net;
}

static inline bool lockdep_is_held(const void *lock)
{
  (void)lock;
  return true;
}

#define rcu_dereference_check(p, c) (p)
#define rcu_dereference_raw(p) (p)

static inline void local_bh_disable(void)
{
}

static inline void local_bh_enable(void)
{
}

#define BUILD_BUG_ON(cond) ((void)sizeof(char[1 - 2 * !!(cond)]))
#define BUILD_BUG_ON_NOT_POWER_OF_2(n) BUILD_BUG_ON(((n) == 0) || ((n) & ((n) - 1)))
#define do_div(n, base) ((n) = (n) / (base))

struct netlink_notify
{
  int protocol;
  u32 portid;
  struct net *net;
};

#define NETLINK_URELEASE 1
#define NOTIFY_DONE 0
#define NOTIFY_OK 1

#ifndef __WIRELESS_IEEE80211_NOTIFIER_BLOCK_DEFINED
#define __WIRELESS_IEEE80211_NOTIFIER_BLOCK_DEFINED
struct notifier_block
{
  int (*notifier_call)(struct notifier_block *nb,
                       unsigned long action,
                       void *data);
};
#endif

struct net;
extern struct net init_net;
#define __WIRELESS_IEEE80211_POSSIBLE_NET_T_DEFINED
typedef struct net *possible_net_t;
static inline struct net *read_pnet(const possible_net_t *pnet)
{
  return *pnet;
}
static inline void write_pnet(possible_net_t *pnet, struct net *net)
{
  *pnet = net;
}

static inline bool net_eq(const struct net *net1, const struct net *net2)
{
  return net1 == net2;
}

static inline struct net *get_net_ns_by_pid(pid_t pid)
{
  (void)pid;
  return &init_net;
}

static inline struct net *get_net_ns_by_fd(int fd)
{
  (void)fd;
  return &init_net;
}

static inline void put_net(struct net *net)
{
  (void)net;
}

typedef long long time64_t;

struct timespec64
{
  time64_t tv_sec;
  long tv_nsec;
};
typedef int wait_queue_head_t;

#ifndef HZ
#  define HZ TICK_PER_SEC
#endif

extern unsigned long jiffies;

static inline void cfg80211_compat_refresh_jiffies(void)
{
  jiffies = (unsigned long)clock_systime_ticks();
}

#define time_after(a, b) ((long)((b) - (a)) < 0)
#define time_before(a, b) time_after(b, a)
#define time_after_eq(a, b) ((long)((a) - (b)) >= 0)
#define time_before_eq(a, b) time_after_eq(b, a)
#define time_is_after_jiffies(a) time_after(a, jiffies)
#define time_is_before_jiffies(a) time_before(a, jiffies)

#ifndef __WIRELESS_IEEE80211_JIFFIES_HELPERS_DEFINED
#define __WIRELESS_IEEE80211_JIFFIES_HELPERS_DEFINED
static inline unsigned long msecs_to_jiffies(unsigned int m)
{
  return (unsigned long)m * HZ / 1000;
}

static inline unsigned long secs_to_jiffies(unsigned int s)
{
  return msecs_to_jiffies(s * 1000U);
}

static inline unsigned long usecs_to_jiffies(unsigned int u)
{
  return (unsigned long)u * HZ / 1000000;
}

static inline unsigned int jiffies_to_msecs(unsigned long j)
{
  return (unsigned int)(j * 1000 / HZ);
}

static inline unsigned long round_jiffies_up(unsigned long j)
{
  return j;
}

static inline unsigned long round_jiffies(unsigned long j)
{
  return j;
}
#endif

#ifndef __WIRELESS_IEEE80211_KTIME_HELPERS_DEFINED
#define __WIRELESS_IEEE80211_KTIME_HELPERS_DEFINED
static inline u64 ktime_get_ns(void)
{
  return 0;
}

static inline ktime_t ktime_get_boottime(void)
{
  return 0;
}

static inline u64 ktime_get_boottime_ns(void)
{
  return 0;
}

static inline time64_t ktime_get_seconds(void)
{
  return 0;
}

static inline void ktime_get_real_ts64(struct timespec64 *ts)
{
  struct timespec now;

  if (ts == NULL)
    {
      return;
    }

  if (clock_gettime(CLOCK_REALTIME, &now) == 0)
    {
      ts->tv_sec = now.tv_sec;
      ts->tv_nsec = now.tv_nsec;
    }
  else
    {
      ts->tv_sec = 0;
      ts->tv_nsec = 0;
    }
}

static inline ktime_t us_to_ktime(u64 usecs)
{
  return (ktime_t)usecs * 1000;
}

static inline ktime_t ms_to_ktime(u64 msecs)
{
  return (ktime_t)msecs * 1000000;
}
#endif

struct srcu_struct
{
  int dummy;
};

#ifndef ATOMIC_INIT
#define ATOMIC_INIT(i) (i)
#endif

enum hrtimer_restart
{
  HRTIMER_NORESTART = 0,
  HRTIMER_RESTART = 1,
};

struct timer_list
{
  void (*function)(struct timer_list *timer);
  unsigned long expires;
  bool active;
};

#define TIMER_DEFERRABLE 0

static inline int timer_pending(const struct timer_list *timer)
{
  return timer->active;
}

static inline int mod_timer(struct timer_list *timer, unsigned long expires)
{
  cfg80211_compat_refresh_jiffies();
  timer->expires = expires;
  timer->active = true;
  return 0;
}

static inline int timer_delete_sync(struct timer_list *timer)
{
  timer->active = false;
  return 0;
}

static inline void timer_setup(struct timer_list *timer,
                               void (*function)(struct timer_list *),
                               unsigned int flags)
{
  (void)flags;
  timer->function = function;
  timer->active = false;
}

struct hrtimer
{
  enum hrtimer_restart (*function)(struct hrtimer *timer);
};

#define HRTIMER_MODE_REL 0
#define HRTIMER_MODE_ABS 1
#define HRTIMER_MODE_REL_SOFT HRTIMER_MODE_REL
#define HRTIMER_MODE_ABS_SOFT HRTIMER_MODE_ABS
#ifndef CLOCK_MONOTONIC
#  define CLOCK_MONOTONIC 0
#endif

static inline void hrtimer_setup(struct hrtimer *timer,
                                 enum hrtimer_restart (*function)
                                   (struct hrtimer *),
                                 int clock_id, int mode)
{
  (void)clock_id;
  (void)mode;
  timer->function = function;
}

static inline void spin_lock_init(spinlock_t *lock)
{
  *lock = 0;
}

static inline void spin_lock_bh(spinlock_t *lock)
{
  *lock = enter_critical_section();
}

static inline void spin_unlock_bh(spinlock_t *lock)
{
  leave_critical_section((irqstate_t)*lock);
}

static inline void spin_lock(spinlock_t *lock)
{
  *lock = enter_critical_section();
}

static inline void spin_unlock(spinlock_t *lock)
{
  leave_critical_section((irqstate_t)*lock);
}

static inline void spin_lock_irq(spinlock_t *lock)
{
  *lock = enter_critical_section();
}

static inline void spin_unlock_irq(spinlock_t *lock)
{
  leave_critical_section((irqstate_t)*lock);
}

static inline irqstate_t cfg80211_compat_spin_lock_irqsave(spinlock_t *lock)
{
  (void)lock;
  return enter_critical_section();
}

static inline void cfg80211_compat_spin_unlock_irqrestore(spinlock_t *lock,
                                                          irqstate_t flags)
{
  (void)lock;
  leave_critical_section(flags);
}

#define spin_lock_irqsave(lock, flags) \
  do \
    { \
      (flags) = cfg80211_compat_spin_lock_irqsave(lock); \
    } \
  while (0)

#define spin_unlock_irqrestore(lock, flags) \
  cfg80211_compat_spin_unlock_irqrestore(lock, (irqstate_t)(flags))

static inline void init_waitqueue_head(wait_queue_head_t *head)
{
  *head = 0;
}

static inline void wake_up(wait_queue_head_t *head)
{
  (void)head;
}

#ifndef atomic_read
static inline int atomic_read(const atomic_t *v)
{
  return *v;
}
#endif

#ifndef atomic_set
static inline void atomic_set(atomic_t *v, int i)
{
  *v = i;
}
#endif

static inline void atomic_inc(atomic_t *v)
{
  ++(*v);
}

static inline int atomic_inc_return(atomic_t *v)
{
  return ++(*v);
}

static inline int atomic_dec_return(atomic_t *v)
{
  return --(*v);
}

static inline void atomic_dec(atomic_t *v)
{
  --(*v);
}

static inline void atomic_add(int i, atomic_t *v)
{
  *v += i;
}

static inline void atomic_sub(int i, atomic_t *v)
{
  *v -= i;
}

static inline int atomic_sub_return(int i, atomic_t *v)
{
  *v -= i;
  return *v;
}

static inline long long atomic64_inc_return(atomic64_t *v)
{
  return ++(*v);
}

static inline void might_sleep(void)
{
}

struct work_struct
{
  void (*func)(struct work_struct *work);
  struct work_s nuttx_work;
};

struct delayed_work
{
  struct work_struct work;
  bool pending;
};

struct workqueue_struct
{
  int dummy;
};

#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_TYPES_DEFINED 1

static struct workqueue_struct cfg80211_compat_system_wq;
#define system_dfl_wq (&cfg80211_compat_system_wq)
#define system_power_efficient_wq (&cfg80211_compat_system_wq)
#define system_freezable_wq (&cfg80211_compat_system_wq)
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_GLOBALS_DEFINED 1

#define WQ_MEM_RECLAIM 0
#define WQ_HIGHPRI 0
#define WQ_UNBOUND 0
#define DECLARE_WORK(name, fn) \
  struct work_struct name = { .func = (void (*)(struct work_struct *))(fn) }
#define DECLARE_DELAYED_WORK(name, fn) \
  struct delayed_work name = { .work = { .func = (void (*)(struct work_struct *))(fn) } }

#ifdef LPWORK
#  define CFG80211_COMPAT_WORKQ LPWORK
#elif defined(HPWORK)
#  define CFG80211_COMPAT_WORKQ HPWORK
#else
#  define CFG80211_COMPAT_WORKQ USRWORK
#endif

static inline void cfg80211_compat_work_trampoline(void *arg)
{
  struct work_struct *work = arg;

  if (work && work->func)
    {
      work->func(work);
    }
}

static inline struct workqueue_struct *
alloc_ordered_workqueue(const char *name, unsigned int flags, ...)
{
  (void)name;
  (void)flags;
  return system_dfl_wq;
}

static inline struct workqueue_struct *
alloc_workqueue(const char *name, unsigned int flags,
                int max_active, ...)
{
  (void)name;
  (void)flags;
  (void)max_active;
  return system_dfl_wq;
}

static inline struct workqueue_struct *
create_singlethread_workqueue(const char *name)
{
  return alloc_workqueue(name, 0, 1);
}

static inline void destroy_workqueue(struct workqueue_struct *wq)
{
  (void)wq;
}

static inline bool queue_work(struct workqueue_struct *wq,
                              struct work_struct *work)
{
  (void)wq;

  if (!work || !work->func)
    {
      return false;
    }

  cfg80211_compat_refresh_jiffies();
  return work_queue(CFG80211_COMPAT_WORKQ, &work->nuttx_work,
                    cfg80211_compat_work_trampoline, work, 0) == 0;
}

static inline bool queue_delayed_work(struct workqueue_struct *wq,
                                      struct delayed_work *dwork,
                                      unsigned long delay)
{
  (void)wq;

  if (!dwork || !dwork->work.func)
    {
      return false;
    }

  cfg80211_compat_refresh_jiffies();
  dwork->pending = work_queue(CFG80211_COMPAT_WORKQ,
                              &dwork->work.nuttx_work,
                              cfg80211_compat_work_trampoline,
                              &dwork->work,
                              (clock_t)delay) == 0;
  return dwork->pending;
}

static inline bool mod_delayed_work(struct workqueue_struct *wq,
                                    struct delayed_work *dwork,
                                    unsigned long delay)
{
  return queue_delayed_work(wq, dwork, delay);
}

static inline void flush_workqueue(struct workqueue_struct *wq)
{
  (void)wq;
}

#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_CORE_HELPERS_DEFINED 1

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_SCHEDULE_WORK_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_SCHEDULE_WORK_DEFINED 1
static inline bool schedule_work(struct work_struct *work)
{
  return queue_work(system_dfl_wq, work);
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_SCHEDULE_DELAYED_WORK_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_SCHEDULE_DELAYED_WORK_DEFINED 1
static inline bool schedule_delayed_work(struct delayed_work *dwork,
                                         unsigned long delay)
{
  return queue_delayed_work(system_dfl_wq, dwork, delay);
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_WORK_SYNC_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_WORK_SYNC_DEFINED 1
static inline bool cancel_work_sync(struct work_struct *work)
{
  if (work == NULL)
    {
      return false;
    }

  return work_cancel_sync(CFG80211_COMPAT_WORKQ, &work->nuttx_work) == 0;
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_DELAYED_WORK_SYNC_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_DELAYED_WORK_SYNC_DEFINED 1
static inline bool cancel_delayed_work_sync(struct delayed_work *dwork)
{
  if (dwork == NULL)
    {
      return false;
    }

  dwork->pending = false;
  return work_cancel_sync(CFG80211_COMPAT_WORKQ,
                          &dwork->work.nuttx_work) == 0;
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_DELAYED_WORK_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_DELAYED_WORK_DEFINED 1
static inline bool cancel_delayed_work(struct delayed_work *dwork)
{
  if (dwork == NULL)
    {
      return false;
    }

  dwork->pending = false;
  return work_cancel(CFG80211_COMPAT_WORKQ, &dwork->work.nuttx_work) == 0;
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_DELAYED_WORK_PENDING_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_DELAYED_WORK_PENDING_DEFINED 1
static inline bool delayed_work_pending(struct delayed_work *dwork)
{
  return dwork != NULL && !work_available(&dwork->work.nuttx_work);
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_FLUSH_DELAYED_WORK_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_FLUSH_DELAYED_WORK_DEFINED 1
static inline bool flush_delayed_work(struct delayed_work *dwork)
{
  if (delayed_work_pending(dwork))
    {
      dwork->pending = false;
      cfg80211_compat_work_trampoline(&dwork->work);
      return true;
    }

  return false;
}
#endif

#define list_first_entry_or_null(ptr, type, member) \
  (list_empty(ptr) ? NULL : list_first_entry(ptr, type, member))
#define list_first_or_null_rcu(ptr, type, member) \
  list_first_entry_or_null(ptr, type, member)
#define list_last_entry(ptr, type, member) \
  list_entry((ptr)->prev, type, member)
#define list_for_each_entry_continue_reverse(pos, head, member) \
  for (pos = list_entry((pos)->member.prev, typeof(*pos), member); \
       &pos->member != (head); \
       pos = list_entry(pos->member.prev, typeof(*pos), member))

static inline void *kzalloc(size_t size, int flags)
{
  (void)flags;
  return kmm_zalloc(size);
}

static inline void *kmalloc(size_t size, int flags)
{
  (void)flags;
  return kmm_malloc(size);
}

static inline void *kcalloc(size_t n, size_t size, int flags)
{
  (void)flags;
  return kmm_calloc(n, size);
}

static inline void *kmemdup(const void *src, size_t len, gfp_t gfp)
{
  void *dst;

  (void)gfp;
  dst = kmm_malloc(len);
  if (dst)
    {
      memcpy(dst, src, len);
    }

  return dst;
}

static inline void kfree(const void *p)
{
  kmm_free((FAR void *)p);
}

static inline void kfree_sensitive(const void *p)
{
  kfree(p);
}

#define DEFINE_SPINLOCK(name) spinlock_t name = 0
#define kmalloc_obj(obj, args...) kmalloc(sizeof(obj), GFP_KERNEL)
#define kzalloc_obj(obj, args...) kzalloc(sizeof(obj), GFP_KERNEL)
#define kzalloc_objs(obj, count, args...) \
  kcalloc(count, sizeof(obj), GFP_KERNEL)
#define kzalloc_flex(obj, member, count, args...) \
  kzalloc(sizeof(obj) + sizeof((obj).member[0]) * (count), GFP_KERNEL)
#define MODULE_FIRMWARE(name)
#define late_initcall(fn)

#define timer_container_of(var, callback_timer, timer_field) \
  container_of(callback_timer, typeof(*var), timer_field)

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
      struct list_head *first = list->next;
      struct list_head *last = list->prev;

      first->prev = head->prev;
      head->prev->next = first;
      last->next = head;
      head->prev = last;
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
#define list_for_each_entry_safe(pos, n, head, member) \
  for (pos = list_entry((head)->next, typeof(*pos), member), \
       n = list_entry(pos->member.next, typeof(*pos), member); \
       &pos->member != (head); \
       pos = n, n = list_entry(n->member.next, typeof(*n), member))
#define list_for_each_entry_rcu(pos, head, member, args...) \
  list_for_each_entry(pos, head, member)

#define __WIRELESS_LINUX_COMPAT_LIST_HELPERS_DEFINED 1

#define rhl_for_each_entry_rcu(pos, tmp, list, member) \
  for (tmp = (list), pos = tmp ? container_of(tmp, typeof(*pos), member) : NULL; \
       pos; tmp = tmp ? tmp->next : NULL, \
       pos = tmp ? container_of(tmp, typeof(*pos), member) : NULL)

#define alloc_percpu_gfp(type, gfp) ((type *)kzalloc(sizeof(type), gfp))
#define free_percpu(ptr) kfree(ptr)
#define for_each_possible_cpu(cpu) for ((cpu) = 0; (cpu) < 1; (cpu)++)
#define per_cpu_ptr(ptr, cpu) (ptr)
#define this_cpu_ptr(ptr) (ptr)

static inline void __skb_queue_head_init(struct sk_buff_head *list)
{
  INIT_LIST_HEAD(&list->list);
  list->next = NULL;
  list->prev = NULL;
  list->qlen = 0;
  spin_lock_init(&list->lock);
}

#define skb_queue_head_init(list) __skb_queue_head_init(list)

static inline unsigned int skb_queue_len(const struct sk_buff_head *list)
{
  return list->qlen;
}

static inline bool skb_queue_empty(const struct sk_buff_head *list)
{
  return list_empty(&list->list);
}

static inline struct sk_buff *skb_peek(struct sk_buff_head *list)
{
  if (list_empty(&list->list))
    {
      return NULL;
    }

  return list_entry(list->list.next, struct sk_buff, list);
}

static inline struct sk_buff *__skb_peek(struct sk_buff_head *list)
{
  return skb_peek(list);
}

static inline struct sk_buff *skb_peek_tail(struct sk_buff_head *list)
{
  if (list_empty(&list->list))
    {
      return NULL;
    }

  return list_entry(list->list.prev, struct sk_buff, list);
}

static inline bool skb_queue_is_last(const struct sk_buff_head *list,
                                     const struct sk_buff *skb)
{
  return skb->list.next == &list->list;
}

static inline struct sk_buff *skb_queue_next(const struct sk_buff_head *list,
                                             const struct sk_buff *skb)
{
  if (skb_queue_is_last(list, skb))
    {
      return NULL;
    }

  return list_entry(skb->list.next, struct sk_buff, list);
}

static inline void __skb_queue_tail(struct sk_buff_head *list,
                                    struct sk_buff *skb)
{
  list_add_tail(&skb->list, &list->list);
  list->qlen++;
}

static inline void skb_queue_tail(struct sk_buff_head *list,
                                  struct sk_buff *skb)
{
  spin_lock_bh(&list->lock);
  __skb_queue_tail(list, skb);
  spin_unlock_bh(&list->lock);
}

static inline void skb_queue_head(struct sk_buff_head *list,
                                  struct sk_buff *skb)
{
  spin_lock_bh(&list->lock);
  list_add(&skb->list, &list->list);
  list->qlen++;
  spin_unlock_bh(&list->lock);
}

static inline struct sk_buff *__skb_dequeue(struct sk_buff_head *list)
{
  struct sk_buff *skb;

  if (list_empty(&list->list))
    {
      return NULL;
    }

  skb = list_entry(list->list.next, struct sk_buff, list);
  list_del(&skb->list);
  list->qlen--;
  return skb;
}

static inline struct sk_buff *skb_dequeue(struct sk_buff_head *list)
{
  struct sk_buff *skb;

  spin_lock_bh(&list->lock);
  skb = __skb_dequeue(list);
  spin_unlock_bh(&list->lock);
  return skb;
}

static inline void __skb_unlink(struct sk_buff *skb, struct sk_buff_head *list)
{
  list_del(&skb->list);
  if (list->qlen)
    {
      list->qlen--;
    }
}

static inline void skb_list_del_init(struct sk_buff *skb)
{
  list_del_init(&skb->list);
  skb->next = NULL;
  skb->prev = NULL;
}

static inline void skb_queue_splice_tail(struct sk_buff_head *list,
                                         struct sk_buff_head *head)
{
  if (!skb_queue_empty(list))
    {
      list_splice_tail(&list->list, &head->list);
      head->qlen += list->qlen;
    }
}

static inline void skb_queue_splice(struct sk_buff_head *list,
                                    struct sk_buff_head *head)
{
  if (!skb_queue_empty(list))
    {
      list_splice_init(&list->list, &head->list);
      head->qlen += list->qlen;
      list->qlen = 0;
    }
}

static inline void skb_queue_splice_init(struct sk_buff_head *list,
                                         struct sk_buff_head *head)
{
  if (!skb_queue_empty(list))
    {
      list_splice_init(&list->list, &head->list);
      head->qlen += list->qlen;
      list->qlen = 0;
    }
}

static inline void skb_queue_splice_tail_init(struct sk_buff_head *list,
                                              struct sk_buff_head *head)
{
  if (!skb_queue_empty(list))
    {
      list_splice_tail_init(&list->list, &head->list);
      head->qlen += list->qlen;
      list->qlen = 0;
    }
}

#undef __skb_queue_purge
static inline void skb_queue_purge(struct sk_buff_head *list)
{
  struct sk_buff *skb;

  while ((skb = skb_dequeue(list)) != NULL)
    {
      dev_kfree_skb(skb);
    }
}

#define __skb_queue_purge(list) skb_queue_purge(list)
#define skb_queue_walk(queue, skb) \
  list_for_each_entry(skb, &(queue)->list, list)
#define skb_queue_walk_safe(queue, skb, tmp) \
  list_for_each_entry_safe(skb, tmp, &(queue)->list, list)
#define skb_list_walk_safe(first, skb, next_skb) \
  for ((skb) = (first), (next_skb) = (skb) ? (skb)->next : NULL; \
       (skb); \
       (skb) = (next_skb), (next_skb) = (skb) ? (skb)->next : NULL)

static inline void skb_mark_not_on_list(struct sk_buff *skb)
{
  skb->next = NULL;
  skb->prev = NULL;
}

static inline unsigned int skb_get_queue_mapping(const struct sk_buff *skb)
{
  return skb->queue_mapping;
}

static inline void skb_set_queue_mapping(struct sk_buff *skb,
                                         unsigned int queue_mapping)
{
  skb->queue_mapping = queue_mapping;
}

static inline bool skb_has_frag_list(const struct sk_buff *skb)
{
  return skb != NULL && skb->shinfo != NULL &&
         skb->shinfo->frag_list != NULL;
}

static inline void kfree_skb_list(struct sk_buff *segs)
{
  while (segs)
    {
      struct sk_buff *next = segs->next;
      dev_kfree_skb(segs);
      segs = next;
    }
}

struct rb_node
{
  struct rb_node *rb_left;
  struct rb_node *rb_right;
  struct rb_node *rb_parent;
};

struct rb_root
{
  struct rb_node *rb_node;
};

#define RB_ROOT { NULL }
#define rb_entry(ptr, type, member) container_of(ptr, type, member)

static inline void rb_link_node(struct rb_node *node, struct rb_node *parent,
                                struct rb_node **link)
{
  node->rb_parent = parent;
  node->rb_left = NULL;
  node->rb_right = NULL;
  *link = node;
}

static inline void rb_insert_color(struct rb_node *node, struct rb_root *root)
{
  (void)node;
  (void)root;
}

static inline void rb_erase(struct rb_node *node, struct rb_root *root)
{
  struct rb_node *parent = node->rb_parent;
  struct rb_node *child = node->rb_left ? node->rb_left : node->rb_right;

  if (parent == NULL)
    {
      root->rb_node = child;
    }
  else if (parent->rb_left == node)
    {
      parent->rb_left = child;
    }
  else if (parent->rb_right == node)
    {
      parent->rb_right = child;
    }

  if (child != NULL)
    {
      child->rb_parent = parent;
    }

  node->rb_left = NULL;
  node->rb_right = NULL;
  node->rb_parent = NULL;
}

static inline unsigned int hweight8(u8 value)
{
  return __builtin_popcount((unsigned int)value);
}

static inline unsigned int hweight16(u16 value)
{
  return __builtin_popcount((unsigned int)value);
}

static inline unsigned int hweight32(u32 value)
{
  return __builtin_popcount(value);
}

/*
 * Late-stage compatibility helpers used by imported cfg80211/mac80211/hwsim
 * sources. These are intentionally minimal NuttX shims and should be replaced
 * by real integrations as the simulation path matures.
 */

static inline ktime_t ktime_get_real(void)
{
  return 0;
}

static inline u64 ktime_to_us(ktime_t kt)
{
  return (u64)kt / 1000;
}

static inline ktime_t ktime_set(long secs, unsigned long nsecs)
{
  return (ktime_t)secs * 1000000000LL + nsecs;
}

static inline ktime_t ns_to_ktime(u64 nsecs)
{
  return (ktime_t)nsecs;
}

static inline unsigned long round_jiffies_relative(unsigned long j)
{
  return j;
}

static inline u8 get_random_u8(void)
{
  static u32 seed = 0x87654321;

  seed = seed * 1103515245 + 12345;
  return seed >> 24;
}

static inline u16 get_random_u16(void)
{
  u16 value = get_random_u8();

  return (value << 8) | get_random_u8();
}

static inline u64 get_unaligned_be64(const void *p)
{
  const u8 *b = p;
  int i;
  u64 value = 0;

  for (i = 0; i < 8; i++)
    {
      value = (value << 8) | b[i];
    }

  return value;
}

static inline u64 get_unaligned_le64(const void *p)
{
  const u8 *b = p;
  int i;
  u64 value = 0;

  for (i = 7; i >= 0; i--)
    {
      value = (value << 8) | b[i];
    }

  return value;
}

static inline void put_unaligned_be64(u64 value, void *p)
{
  u8 *b = p;
  int i;

  for (i = 7; i >= 0; i--)
    {
      b[i] = value;
      value >>= 8;
    }
}

static inline struct page *alloc_page(gfp_t gfp)
{
  (void)gfp;
  return kmm_zalloc(sizeof(struct page));
}

static inline void __free_page(struct page *page)
{
  kmm_free(page);
}

static inline int hrtimer_cancel(struct hrtimer *timer)
{
  (void)timer;
  return 0;
}

static inline bool hrtimer_active(const struct hrtimer *timer)
{
  (void)timer;
  return false;
}

static inline void hrtimer_start(struct hrtimer *timer, ktime_t time,
                                 int mode)
{
  (void)timer;
  (void)time;
  (void)mode;
}

static inline void hrtimer_start_range_ns(struct hrtimer *timer,
                                          ktime_t time, u64 range,
                                          int mode)
{
  (void)range;
  hrtimer_start(timer, time, mode);
}

static inline u64 hrtimer_forward_now(struct hrtimer *timer,
                                      ktime_t interval)
{
  (void)timer;
  (void)interval;
  return 0;
}

static inline void flush_work(struct work_struct *work)
{
  (void)work;
}
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_FLUSH_WORK_DEFINED 1

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_DELAYED_WORK_SYNC_DEFINED
static inline bool cancel_delayed_work_sync(struct delayed_work *dwork)
{
  if (!dwork)
    {
      return false;
    }

  return work_cancel_sync(CFG80211_COMPAT_WORKQ,
                          &dwork->work.nuttx_work) == 0;
}
#  define __WIRELESS_IEEE80211_WORKQUEUE_CANCEL_COMPAT 1
#  define __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_DELAYED_WORK_SYNC_DEFINED 1
#endif

#ifndef INIT_WORK
#  define INIT_WORK(work, fn) do { \
    memset((work), 0, sizeof(*(work))); \
    (work)->func = (void (*)(struct work_struct *))(fn); \
  } while (0)
#endif
#ifndef INIT_DELAYED_WORK
#  define INIT_DELAYED_WORK(dwork, fn) INIT_WORK(&(dwork)->work, fn)
#endif
#ifndef to_delayed_work
#  define to_delayed_work(work) container_of(work, struct delayed_work, work)
#endif

#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_INIT_HELPERS_DEFINED 1

static inline long schedule_timeout_interruptible(long timeout)
{
  return timeout;
}

static inline int netif_rx(struct sk_buff *skb)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP
  extern int linux_bt_bnep_netif_rx(struct sk_buff *skb);

  if (skb != NULL && skb->dev != NULL &&
      ((skb->dev->name[0] == 'b' && skb->dev->name[1] == 'n' &&
        skb->dev->name[2] == 'e' && skb->dev->name[3] == 'p') ||
       (skb->dev->name[0] == 'b' && skb->dev->name[1] == 't' &&
        skb->dev->name[2] == 'n')))
    {
      return linux_bt_bnep_netif_rx(skb);
    }
#endif

  extern int ieee80211_linux_netif_rx(struct sk_buff *skb);

  return ieee80211_linux_netif_rx(skb);
}

#define __WIRELESS_LINUX_COMPAT_NETIF_RX_DEFINED 1

static inline void skb_orphan(struct sk_buff *skb)
{
  skb->sk = NULL;
}

static inline void skb_ext_reset(struct sk_buff *skb)
{
  (void)skb;
}

static inline void nf_reset_ct(struct sk_buff *skb)
{
  (void)skb;
}

static inline int skb_cow_head(struct sk_buff *skb, unsigned int headroom)
{
  if (skb_headroom(skb) >= headroom)
    {
      return 0;
    }

  return pskb_expand_head(skb, headroom, skb_tailroom(skb), GFP_ATOMIC);
}

static inline void skb_reset_transport_header(struct sk_buff *skb)
{
  skb->transport_header = skb->data - skb->head;
}

static inline void skb_complete_wifi_ack(struct sk_buff *skb, bool acked)
{
  if (skb != NULL)
    {
      skb->wifi_acked_valid = true;
      skb->wifi_acked = acked;
    }
}

struct ethhdr;
struct iphdr;
struct ipv6hdr;

static inline struct ethhdr *eth_hdr(const struct sk_buff *skb)
{
  return (struct ethhdr *)skb->data;
}

static inline struct iphdr *ip_hdr(const struct sk_buff *skb)
{
  return (struct iphdr *)skb->data;
}

static inline struct ipv6hdr *ipv6_hdr(const struct sk_buff *skb)
{
  return (struct ipv6hdr *)skb->data;
}

static inline void *kmemdup_array(const void *src, size_t element_size,
                                  size_t count, gfp_t gfp)
{
  return kmemdup(src, element_size * count, gfp);
}

static inline void __hw_addr_init(struct netdev_hw_addr_list *list)
{
  list->count = 0;
}

static inline bool qdisc_all_tx_empty(const struct net_device *dev)
{
  (void)dev;
  return true;
}

static inline int find_first_zero_bit(const unsigned long *addr,
                                      unsigned int size)
{
  unsigned int i;

  for (i = 0; i < size; i++)
    {
      if (!test_bit(i, addr))
        {
          return i;
        }
    }

  return size;
}

static inline int printk_ratelimit(void)
{
  return 0;
}

static inline int rcu_read_lock_held(void)
{
  return 1;
}

static inline char *kstrndup(const char *s, size_t max, gfp_t gfp)
{
  size_t len;
  char *dst;

  (void)gfp;
  len = strnlen(s, max);
  dst = kmm_malloc(len + 1);
  if (dst == NULL)
    {
      return NULL;
    }

  memcpy(dst, s, len);
  dst[len] = '\0';
  return dst;
}

static inline void list_move(struct list_head *entry,
                             struct list_head *head)
{
  list_del(entry);
  list_add(entry, head);
}

#define tasklet_disable(tasklet) do { (void)(tasklet); } while (0)
#define tasklet_enable(tasklet) do { (void)(tasklet); } while (0)
#define wait_event(wq, condition) do { (void)(wq); } while (!(condition) && 0)
#define net_dbg_ratelimited(fmt, args...) do { } while (0)
#define dev_err(dev, fmt, args...) do { (void)(dev); } while (0)
#define printk(fmt, args...) printf(fmt, ##args)
#define fs_initcall(fn)
#ifndef smp_mb
#  define smp_mb() barrier()
#endif

static inline bool hrtimer_is_queued(const struct hrtimer *timer)
{
  return hrtimer_active(timer);
}

static inline unsigned int hweight64(u64 value)
{
  return __builtin_popcountll(value);
}

static inline unsigned long __ffs(unsigned long word)
{
  return __builtin_ctzl(word);
}

static inline unsigned long __ffs64(u64 word)
{
  return __builtin_ctzll(word);
}

static inline int fls64(u64 word)
{
  return word ? 64 - __builtin_clzll(word) : 0;
}

#ifndef BITMAP_FROM_U64
#  define BITMAP_FROM_U64(value) ((unsigned long)(value))
#endif

#define get_unaligned(ptr) \
  ({ typeof(*(ptr)) __tmp; memcpy(&__tmp, (ptr), sizeof(__tmp)); __tmp; })

#ifndef FIELD_MAX
#  define FIELD_MAX(mask) ((typeof(mask))((mask) >> __bf_shf(mask)))
#endif

#ifndef FIELD_FIT
#  define FIELD_FIT(mask, val) (!(((typeof(mask))(val) << __bf_shf(mask)) & ~(mask)))
#endif

static inline void get_random_bytes(void *buf, int len)
{
  static u32 seed = 0x12345678;
  u8 *out = buf;
  int i;

  for (i = 0; i < len; i++)
    {
      seed = seed * 1103515245 + 12345;
      out[i] = seed >> 24;
    }
}

static inline void memzero_explicit(void *s, size_t count)
{
  memset(s, 0, count);
  __asm__ __volatile__("" : : : "memory");
}

static inline unsigned int jiffies_delta_to_msecs(long delta)
{
  return jiffies_to_msecs((unsigned long)delta);
}

#define smp_mb() barrier()

#define kvzalloc_objs(obj, count, args...) \
  kmm_calloc((count), sizeof(obj))

static inline void *bitmap_zalloc(unsigned int nbits, gfp_t gfp)
{
  (void)gfp;
  return kmm_zalloc(BITS_TO_LONGS(nbits) * sizeof(unsigned long));
}

static inline void bitmap_free(const unsigned long *bitmap)
{
  kmm_free((FAR void *)bitmap);
}

static inline void kvfree(const void *addr)
{
  kmm_free((FAR void *)addr);
}

struct kmem_cache
{
  size_t size;
};

static inline struct kmem_cache *
kmem_cache_create(const char *name, size_t size, size_t align,
                  unsigned long flags, void (*ctor)(void *))
{
  struct kmem_cache *cache;

  (void)name;
  (void)align;
  (void)flags;
  (void)ctor;

  cache = kmm_zalloc(sizeof(*cache));
  if (cache != NULL)
    {
      cache->size = size;
    }

  return cache;
}

static inline void kmem_cache_destroy(struct kmem_cache *cache)
{
  kmm_free(cache);
}

static inline void *kmem_cache_alloc(struct kmem_cache *cache, gfp_t gfp)
{
  (void)gfp;
  return cache != NULL ? kmm_zalloc(cache->size) : NULL;
}

static inline void kmem_cache_free(struct kmem_cache *cache, void *obj)
{
  (void)cache;
  kmm_free(obj);
}

static inline u32 reciprocal_scale(u32 val, u32 ep_ro)
{
  return ep_ro ? ((u64)val * ep_ro) >> 32 : 0;
}

static inline u32 skb_get_hash(const struct sk_buff *skb)
{
  return skb != NULL ? skb->mark : 0;
}

static inline u8 skb_get_dsfield(const struct sk_buff *skb)
{
  (void)skb;
  return 0;
}

static inline int skb_get_kcov_handle(struct sk_buff *skb)
{
  (void)skb;
  return 0;
}

static inline void kcov_remote_start_common(int handle)
{
  (void)handle;
}

static inline void kcov_remote_stop(void)
{
}

static inline bool sk_requests_wifi_status(struct sock *sk)
{
  (void)sk;
  return false;
}

static inline void sk_pacing_shift_update(struct sock *sk, int val)
{
  (void)sk;
  (void)val;
}

static inline int atomic_add_unless(atomic_t *v, int a, int u)
{
  if (atomic_read(v) == u)
    {
      return 0;
    }

  atomic_add(a, v);
  return 1;
}

static inline void timer_shutdown_sync(struct timer_list *timer)
{
  timer_delete_sync(timer);
}

static inline int net_ratelimit(void)
{
  return 0;
}

static inline int softirq_count(void)
{
  return 0;
}

static inline int netif_receive_skb(struct sk_buff *skb)
{
  extern int ieee80211_linux_netif_rx(struct sk_buff *skb);

  return ieee80211_linux_netif_rx(skb);
}

static inline void netif_receive_skb_list(struct list_head *head)
{
  struct sk_buff *skb;
  struct sk_buff *tmp;

  list_for_each_entry_safe(skb, tmp, head, list)
    {
      list_del_init(&skb->list);
      netif_receive_skb(skb);
    }

  INIT_LIST_HEAD(head);
}

static inline int napi_gro_receive(struct napi_struct *napi,
                                   struct sk_buff *skb)
{
  (void)napi;
  return netif_receive_skb(skb);
}

static inline void dev_sw_netstats_rx_add(struct net_device *dev,
                                          unsigned int len)
{
  (void)dev;
  (void)len;
}

static inline void dev_sw_netstats_tx_add(struct net_device *dev,
                                          unsigned int packets,
                                          unsigned int len)
{
  (void)dev;
  (void)packets;
  (void)len;
}

static inline __be16 eth_type_trans(struct sk_buff *skb,
                                    struct net_device *dev)
{
  uint16_t proto;

  (void)dev;

  if (skb == NULL || skb->len < 14)
    {
      return 0;
    }

  proto = ((uint16_t)skb->data[12] << 8) | skb->data[13];
  return cpu_to_be16(proto);
}

static inline int dev_queue_xmit(struct sk_buff *skb)
{
  extern int ieee80211_linux_dev_queue_xmit(struct sk_buff *skb);

  return ieee80211_linux_dev_queue_xmit(skb);
}

#define kfree_skb_reason(skb, reason) dev_kfree_skb(skb)

static inline void kernel_param_lock(void *mod)
{
  (void)mod;
}

static inline void kernel_param_unlock(void *mod)
{
  (void)mod;
}

#define dev_warn(dev, fmt, args...) do { (void)(dev); } while (0)
#define dev_info(dev, fmt, args...) do { (void)(dev); } while (0)
#define dev_dbg(dev, fmt, args...) do { (void)(dev); } while (0)
#define dev_printk(level, dev, fmt, args...) do { (void)(dev); } while (0)
#define barrier() __asm__ __volatile__("" : : : "memory")
#define kfree_skb(skb) dev_kfree_skb(skb)
#define ERR_CAST(ptr) ((void *)(ptr))

static inline u32 crc32_be(u32 crc, const unsigned char *p, size_t len)
{
  size_t i;
  int bit;

  crc = ~crc;
  for (i = 0; i < len; i++)
    {
      crc ^= (u32)p[i] << 24;
      for (bit = 0; bit < 8; bit++)
        {
          crc = (crc & 0x80000000) ? (crc << 1) ^ 0x04c11db7 : crc << 1;
        }
    }

  return ~crc;
}

#endif /* __WIRELESS_IEEE80211_INCLUDE_LINUX_CFG80211_COMPAT_H */
