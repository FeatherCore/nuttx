/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/unaligned.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_UNALIGNED_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_UNALIGNED_H

#include_next <linux/unaligned.h>

#ifndef put_unaligned_le32
static inline void put_unaligned_le32(__u32 value, void *p)
{
  __u8 *dst = (__u8 *)p;

  dst[0] = (__u8)value;
  dst[1] = (__u8)(value >> 8);
  dst[2] = (__u8)(value >> 16);
  dst[3] = (__u8)(value >> 24);
}
#endif

#ifndef put_unaligned
#define put_unaligned(value, ptr) do { *(ptr) = (value); } while (0)
#endif

#ifndef get_unaligned_le24
static inline __u32 get_unaligned_le24(const void *p)
{
  const __u8 *src = (const __u8 *)p;

  return (__u32)src[0] | ((__u32)src[1] << 8) |
         ((__u32)src[2] << 16);
}
#endif

#ifndef get_unaligned
#define get_unaligned(ptr) (*(ptr))
#endif

#endif
