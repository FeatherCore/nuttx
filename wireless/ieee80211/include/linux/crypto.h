#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_CRYPTO_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_CRYPTO_H

#include <linux/cfg80211_compat.h>

#define CRYPTO_ALG_ASYNC 0

struct crypto_cipher
{
  int dummy;
};

static inline bool crypto_memneq(const void *a, const void *b, size_t size)
{
  return memcmp(a, b, size) != 0;
}

#endif
