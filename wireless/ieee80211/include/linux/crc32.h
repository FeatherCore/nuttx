#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_CRC32_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_CRC32_H
#include <linux/cfg80211_compat.h>
static inline u32 crc32_le(u32 crc, const unsigned char *p, size_t len)
{
  while (len--)
    {
      crc ^= *p++;
      for (int i = 0; i < 8; i++)
        {
          crc = (crc >> 1) ^ (0xedb88320U & -(crc & 1));
        }
    }

  return crc;
}
#endif
