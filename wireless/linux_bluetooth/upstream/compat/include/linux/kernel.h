/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/kernel.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_KERNEL_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_KERNEL_H

#include <linux/types.h>

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifndef HZ
#  define HZ 100
#endif

#ifndef ENOTSUPP
#  define ENOTSUPP ENOTSUP
#endif

#ifndef BIT
#  define BIT(n) (1UL << (n))
#endif

#ifndef U8_MAX
#  define U8_MAX UINT8_MAX
#endif

#ifndef U16_MAX
#  define U16_MAX UINT16_MAX
#endif

#ifndef PAGE_SIZE
#  define PAGE_SIZE 4096
#endif

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef array_size
#  define array_size(n, size) ((n) * (size))
#endif

#ifndef BUILD_BUG_ON
#  define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2 * !!(condition)]))
#endif

#ifndef UINT_PTR
#  define UINT_PTR(x) ((void *)(uintptr_t)(x))
#endif

#ifndef PTR_UINT
#  define PTR_UINT(x) ((unsigned int)(uintptr_t)(x))
#endif

#ifndef DEFINE_FLEX
#  define DEFINE_FLEX(type, name, member, count_member, max_count) \
    uint8_t name##_storage[sizeof(type) + \
      sizeof(((type *)0)->member[0]) * (max_count)] = { 0 }; \
    type *name = (type *)name##_storage; \
    name->count_member = (max_count)
#endif

#ifndef DEFINE_RAW_FLEX
#  define DEFINE_RAW_FLEX(type, name, member, count) \
    uint8_t name##_raw_flex_storage[sizeof(type) + \
      sizeof(((type *)0)->member[0]) * (count)] = { 0 }; \
    type *name = (type *)name##_raw_flex_storage
#endif

#ifndef __struct_size
#  define __struct_size(p) sizeof(*(p))
#endif

#ifndef __member_size
#  define __member_size(member) sizeof(member)
#endif

#ifndef container_of
#  define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#ifndef container_of_const
#  define container_of_const(ptr, type, member) \
    ((const type *)((const char *)(ptr) - offsetof(type, member)))
#endif

#ifndef lockdep_assert_held
#  define lockdep_assert_held(lock) do { (void)(lock); } while (0)
#endif

#ifndef min
#  define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#  define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min_t
#  define min_t(type, x, y) \
    ((type)(x) < (type)(y) ? (type)(x) : (type)(y))
#endif

#ifndef max_t
#  define max_t(type, x, y) \
    ((type)(x) > (type)(y) ? (type)(x) : (type)(y))
#endif

static inline void memcpy_and_pad(void *dest, size_t dest_len,
                                  const void *src, size_t count, int pad)
{
  size_t copy_len = count < dest_len ? count : dest_len;

  if (copy_len > 0)
    {
      memcpy(dest, src, copy_len);
    }

  if (copy_len < dest_len)
    {
      memset((uint8_t *)dest + copy_len, pad, dest_len - copy_len);
    }
}

#define __packed __attribute__((packed))
#define __maybe_unused __attribute__((unused))
#define __nonstring
#define __counted_by(member)
#define __acquires(x)
#define __releases(x)

#ifndef EBADRQC
#  define EBADRQC EINVAL
#endif

#ifndef EBADE
#  define EBADE EIO
#endif

#ifndef ENOIOCTLCMD
#  define ENOIOCTLCMD ENOTTY
#endif

#ifndef ERESTARTSYS
#  define ERESTARTSYS EINTR
#endif

struct va_format
{
  const char *fmt;
  va_list *va;
};

#define KERN_DEBUG ""

#define pr_info(fmt, ...) linux_bt_compat_vprint("info", fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...) linux_bt_compat_vprint("warn", fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...)  linux_bt_compat_vprint("err", fmt, ##__VA_ARGS__)
#define pr_warn_ratelimited(fmt, ...) pr_warn(fmt, ##__VA_ARGS__)
#define pr_err_ratelimited(fmt, ...)  pr_err(fmt, ##__VA_ARGS__)
#define pr_debug_ratelimited(fmt, ...) pr_debug(fmt, ##__VA_ARGS__)
#define printk(fmt, ...) linux_bt_compat_vprint("debug", fmt, ##__VA_ARGS__)

#ifndef likely
#  define likely(x) (x)
#endif

#ifndef unlikely
#  define unlikely(x) (x)
#endif

#ifndef __printf
#  define __printf(a, b) __attribute__((format(printf, a, b)))
#endif

#ifndef IS_ENABLED
#  define IS_ENABLED(option) 0
#endif

#ifndef pr_debug
#  define pr_debug(fmt, ...) linux_bt_compat_vprint("debug", fmt, ##__VA_ARGS__)
#endif

#ifndef WARN_ON
#  define WARN_ON(condition) ({ int __ret = !!(condition); __ret; })
#endif

#ifndef WARN_ON_ONCE
#  define WARN_ON_ONCE(condition) WARN_ON(condition)
#endif

#ifndef WARN_ONCE
#  define WARN_ONCE(condition, fmt, ...) WARN_ON(condition)
#endif

#ifndef BUG_ON
#  define BUG_ON(condition) do { if (condition) { abort(); } } while (0)
#endif

#ifndef BUG
#  define BUG() abort()
#endif

#ifndef flex_array_size
#  define flex_array_size(p, member, count) (sizeof(*(p)->member) * (count))
#endif

#ifndef struct_size
#  define struct_size(p, member, count) \
    (offsetof(typeof(*(p)), member) + sizeof(*(p)->member) * (count))
#endif

#ifndef fallthrough
#  define fallthrough do { } while (0);
#endif

#ifndef jiffies
#  define jiffies ((unsigned long)time(NULL))
#endif

static inline int64_t us_to_ktime(uint32_t usec)
{
  return (int64_t)usec * 1000;
}

#ifndef time_after
#  define time_after(a, b) ((long)((b) - (a)) < 0)
#endif

#ifndef READ_ONCE
#  define READ_ONCE(x) (x)
#endif

#ifndef WRITE_ONCE
#  define WRITE_ONCE(x, val) ({ (x) = (val); (val); })
#endif

#ifndef no_printk
#  define no_printk(fmt, ...) ({ 0; })
#endif

#ifndef le16_to_cpu
#  define le16_to_cpu(v) ((__u16)(v))
#endif

#ifndef le32_to_cpu
#  define le32_to_cpu(v) ((__u32)(v))
#endif

#ifndef le64_to_cpu
#  define le64_to_cpu(v) ((__u64)(v))
#endif

#ifndef cpu_to_le16
#  define cpu_to_le16(v) ((__le16)(v))
#endif

#ifndef cpu_to_le32
#  define cpu_to_le32(v) ((__le32)(v))
#endif

#ifndef cpu_to_le64
#  define cpu_to_le64(v) ((__le64)(v))
#endif

#ifndef __cpu_to_le16
#  define __cpu_to_le16(v) cpu_to_le16(v)
#endif

#ifndef __cpu_to_le32
#  define __cpu_to_le32(v) cpu_to_le32(v)
#endif

#ifndef __cpu_to_le64
#  define __cpu_to_le64(v) cpu_to_le64(v)
#endif

#ifndef DIV_ROUND_UP
#  define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#endif

#ifndef DIV_ROUND_CLOSEST
#  define DIV_ROUND_CLOSEST(x, divisor) \
    ({ typeof(x) __x = (x); typeof(divisor) __d = (divisor); \
       (__x + (__d / 2)) / __d; })
#endif

static inline unsigned int jiffies_to_msecs(unsigned long j)
{
  return (unsigned int)(j * 1000);
}

static inline void memzero_explicit(void *s, size_t count)
{
  if (s != NULL)
    {
      memset(s, 0, count);
    }
}

static inline ssize_t strscpy(char *dest, const char *src, size_t count)
{
  size_t len;

  if (dest == NULL || src == NULL || count == 0)
    {
      return -E2BIG;
    }

  len = strnlen(src, count);
  if (len >= count)
    {
      memcpy(dest, src, count - 1);
      dest[count - 1] = '\0';
      return -E2BIG;
    }

  memcpy(dest, src, len + 1);
  return (ssize_t)len;
}

static inline void get_random_bytes(void *buf, int len)
{
  if (buf != NULL && len > 0)
    {
      arc4random_buf(buf, (size_t)len);
    }
}

static inline uint32_t get_random_u32_inclusive(uint32_t floor,
                                                uint32_t ceil)
{
  uint32_t value;

  if (floor >= ceil)
    {
      return floor;
    }

  get_random_bytes(&value, sizeof(value));
  return floor + value % (ceil - floor + 1);
}

static inline void linux_bt_compat_vprint(const char *level,
                                          const char *fmt, ...)
{
  va_list ap;
  const struct va_format *vaf;

  (void)level;

  va_start(ap, fmt);
  if (fmt != NULL && !strcmp(fmt, "%pV"))
    {
      vaf = va_arg(ap, const struct va_format *);
      if (vaf != NULL && vaf->fmt != NULL && vaf->va != NULL)
        {
          vprintf(vaf->fmt, *vaf->va);
          printf("\n");
        }
    }
  else if (fmt != NULL)
    {
      vprintf(fmt, ap);
      printf("\n");
    }

  va_end(ap);
}

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_KERNEL_H */
