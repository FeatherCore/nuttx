#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_VERIFICATION_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_VERIFICATION_H
#include <linux/cfg80211_compat.h>
#define VERIFY_USE_SECONDARY_KEYRING 0
static inline int verify_pkcs7_signature(const void *data, unsigned long len,
                                         const void *raw_pkcs7,
                                         size_t pkcs7_len,
                                         const void *trusted_keys,
                                         int usage, int (*view_content)(void *,
                                         size_t, size_t, void *),
                                         void *ctx)
{
  (void)data;
  (void)len;
  (void)raw_pkcs7;
  (void)pkcs7_len;
  (void)trusted_keys;
  (void)usage;
  (void)view_content;
  (void)ctx;
  return -ENOKEY;
}
#endif
