#ifndef __WIRELESS_LINUX_COMPAT_INCLUDE_LINUX_UNALIGNED_H
#define __WIRELESS_LINUX_COMPAT_INCLUDE_LINUX_UNALIGNED_H

#include <linux/types.h>

#ifndef __WIRELESS_LINUX_COMPAT_UNALIGNED_HELPERS_DEFINED
#define __WIRELESS_LINUX_COMPAT_UNALIGNED_HELPERS_DEFINED
static inline u16 get_unaligned_le16(const void *p)
{
  const u8 *b = p;
  return (u16)b[0] | ((u16)b[1] << 8);
}

static inline u32 get_unaligned_le32(const void *p)
{
  const u8 *b = p;
  return (u32)b[0] | ((u32)b[1] << 8) |
         ((u32)b[2] << 16) | ((u32)b[3] << 24);
}

static inline u16 get_unaligned_be16(const void *p)
{
  const u8 *b = p;
  return ((u16)b[0] << 8) | (u16)b[1];
}

static inline void put_unaligned_le16(u16 val, void *p)
{
  u8 *b = p;
  b[0] = val & 0xff;
  b[1] = val >> 8;
}

#ifndef __le16_to_cpu
static inline u16 __le16_to_cpu(__le16 val)
{
  return val;
}
#endif

#ifndef __le32_to_cpu
static inline u32 __le32_to_cpu(__le32 val)
{
  return val;
}
#endif

#ifndef cpu_to_le16
static inline __le16 cpu_to_le16(u16 val)
{
  return val;
}
#endif

#ifndef cpu_to_le32
static inline __le32 cpu_to_le32(u32 val)
{
  return val;
}
#endif
#endif

#endif
