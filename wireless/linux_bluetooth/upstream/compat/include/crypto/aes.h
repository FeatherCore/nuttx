/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __LINUX_BT_COMPAT_CRYPTO_AES_H
#define __LINUX_BT_COMPAT_CRYPTO_AES_H

#include <linux/types.h>

#define AES_BLOCK_SIZE 16
#define AES_MIN_KEY_SIZE 16
#define AES_MAX_KEY_SIZE 32
#define AES_KEYSIZE_128 16
#define AES_KEYSIZE_192 24
#define AES_KEYSIZE_256 32
#define AES_MAXROUNDS 14

#ifndef __LINUX_BT_COMPAT_CRYPTO_AES_CTX_DEFINED
#define __LINUX_BT_COMPAT_CRYPTO_AES_CTX_DEFINED
struct crypto_aes_ctx
{
  uint32_t sk[60];
  uint32_t sk_exp[120];
  unsigned int num_rounds;
};
#endif

int aes_setkey(struct crypto_aes_ctx *ctx, const uint8_t *key, int len);
void aes_encrypt(struct crypto_aes_ctx *ctx, const uint8_t *src,
                 uint8_t *dst);

static inline int aes_expandkey(struct crypto_aes_ctx *ctx,
                                const uint8_t *key, unsigned int key_len)
{
  return aes_setkey(ctx, key, (int)key_len);
}

#endif /* __LINUX_BT_COMPAT_CRYPTO_AES_H */
