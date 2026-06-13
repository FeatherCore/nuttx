#ifndef __WIRELESS_IEEE80211_INCLUDE_CRYPTO_UTILS_H
#define __WIRELESS_IEEE80211_INCLUDE_CRYPTO_UTILS_H

#include <linux/cfg80211_compat.h>

static inline void crypto_xor(u8 *dst, const u8 *src, unsigned int size)
{
  while (size--)
    {
      *dst++ ^= *src++;
    }
}

#endif
