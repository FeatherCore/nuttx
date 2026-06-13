#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_BITMAP_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_BITMAP_H

#include <linux/cfg80211_compat.h>

#define DECLARE_BITMAP(name, bits) unsigned long name[BITS_TO_LONGS(bits)]

static inline void bitmap_zero(unsigned long *dst, unsigned int nbits)
{
  memset(dst, 0, BITS_TO_LONGS(nbits) * sizeof(unsigned long));
}

static inline void bitmap_fill(unsigned long *dst, unsigned int nbits)
{
  memset(dst, 0xff, BITS_TO_LONGS(nbits) * sizeof(unsigned long));
}

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
#endif

#endif
