/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __LINUX_BT_COMPAT_CRYPTO_HASH_H
#define __LINUX_BT_COMPAT_CRYPTO_HASH_H

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/types.h>

#ifndef __LINUX_BT_COMPAT_CRYPTO_AES_CTX_DEFINED
#define __LINUX_BT_COMPAT_CRYPTO_AES_CTX_DEFINED
struct crypto_aes_ctx
{
  uint32_t sk[60];
  uint32_t sk_exp[120];
  unsigned int num_rounds;
};
#endif

typedef struct linux_bt_compat_aes_cmac_ctx
{
  struct crypto_aes_ctx aesctx;
  uint8_t X[16];
  uint8_t m_last[16];
  unsigned int m_n;
} AES_CMAC_CTX;

void aes_cmac_init(AES_CMAC_CTX *ctx);
void aes_cmac_setkey(AES_CMAC_CTX *ctx, const uint8_t *key);
void aes_cmac_update(AES_CMAC_CTX *ctx, const uint8_t *data,
                     unsigned int len);
void aes_cmac_final(uint8_t *mac, AES_CMAC_CTX *ctx);

struct crypto_shash
{
  uint8_t key[16];
  unsigned int key_len;
  bool keyed;
};

static inline struct crypto_shash *crypto_alloc_shash(const char *alg_name,
                                                      u32 type, u32 mask)
{
  struct crypto_shash *tfm;

  if (alg_name == NULL || strcmp(alg_name, "cmac(aes)") != 0)
    {
      return ERR_PTR(-ENOENT);
    }

  tfm = kzalloc(sizeof(*tfm), GFP_KERNEL);
  return tfm != NULL ? tfm : ERR_PTR(-ENOMEM);
}

static inline void crypto_free_shash(struct crypto_shash *tfm)
{
  if (!IS_ERR_OR_NULL(tfm))
    {
      kfree_sensitive(tfm);
    }
}

static inline int crypto_shash_setkey(struct crypto_shash *tfm,
                                      const uint8_t *key,
                                      unsigned int key_len)
{
  if (tfm == NULL || key == NULL || key_len != sizeof(tfm->key))
    {
      return -EINVAL;
    }

  memcpy(tfm->key, key, sizeof(tfm->key));
  tfm->key_len = key_len;
  tfm->keyed = true;
  return 0;
}

static inline int crypto_shash_tfm_digest(struct crypto_shash *tfm,
                                          const uint8_t *data,
                                          unsigned int len,
                                          uint8_t *out)
{
  AES_CMAC_CTX ctx;

  if (tfm == NULL || data == NULL || out == NULL || !tfm->keyed)
    {
      return -EINVAL;
    }

  aes_cmac_init(&ctx);
  aes_cmac_setkey(&ctx, tfm->key);
  aes_cmac_update(&ctx, data, len);
  aes_cmac_final(out, &ctx);
  memset(&ctx, 0, sizeof(ctx));
  return 0;
}

#endif /* __LINUX_BT_COMPAT_CRYPTO_HASH_H */
