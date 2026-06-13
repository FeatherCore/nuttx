#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_JHASH_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_JHASH_H

#include <linux/cfg80211_compat.h>

static inline u32 jhash(const void *key, u32 length, u32 initval)
{
  const u8 *data = key;
  u32 hash = initval;

  while (length--)
    {
      hash = hash * 33 + *data++;
    }

  return hash;
}

static inline u32 jhash_1word(u32 a, u32 initval)
{
  return jhash(&a, sizeof(a), initval);
}

static inline u32 jhash_2words(u32 a, u32 b, u32 initval)
{
  u32 words[2] = { a, b };
  return jhash(words, sizeof(words), initval);
}

#endif
