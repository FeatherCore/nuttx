#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_UNALIGNED_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_UNALIGNED_H
#include <linux/cfg80211_compat.h>
#ifndef __WIRELESS_IEEE80211_UNALIGNED_LE_GET_DEFINED
#define __WIRELESS_IEEE80211_UNALIGNED_LE_GET_DEFINED
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
#endif

#ifndef __WIRELESS_IEEE80211_UNALIGNED_PUT_LE16_DEFINED
#define __WIRELESS_IEEE80211_UNALIGNED_PUT_LE16_DEFINED
static inline void put_unaligned_le16(u16 v, void *p)
{
  u8 *b = p;
  b[0] = v;
  b[1] = v >> 8;
}
#endif
#endif
