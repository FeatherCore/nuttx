#ifndef __WIRELESS_IEEE80211_INCLUDE_CRYPTO_ARC4_H
#define __WIRELESS_IEEE80211_INCLUDE_CRYPTO_ARC4_H

#include <linux/cfg80211_compat.h>

struct arc4_ctx
{
  u8 state[256];
};

static inline void arc4_setkey(struct arc4_ctx *ctx, const u8 *key,
                               unsigned int keylen)
{
  (void)ctx;
  (void)key;
  (void)keylen;
}

static inline void arc4_crypt(struct arc4_ctx *ctx, u8 *out, const u8 *in,
                              unsigned int len)
{
  (void)ctx;
  if (out != in)
    {
      memcpy(out, in, len);
    }
}

#endif
