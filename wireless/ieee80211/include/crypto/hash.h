#ifndef __WIRELESS_IEEE80211_INCLUDE_CRYPTO_HASH_H
#define __WIRELESS_IEEE80211_INCLUDE_CRYPTO_HASH_H

#include <linux/crypto.h>
#include <crypto/aes.h>
#include <crypto/rijndael.h>

struct crypto_shash
{
  rijndael_ctx aes;
  bool key_set;
};

struct shash_desc
{
  struct crypto_shash *tfm;
  u8 state[AES_BLOCK_SIZE];
  u8 block[AES_BLOCK_SIZE];
  unsigned int block_len;
  bool initialized;
};

#define SHASH_DESC_ON_STACK(name, tfm_arg) struct shash_desc name##_desc; \
  struct shash_desc *name = &name##_desc

static inline struct crypto_shash *crypto_alloc_shash(const char *alg,
                                                      u32 type, u32 mask)
{
  (void)alg;
  (void)type;
  (void)mask;
  return kzalloc(sizeof(struct crypto_shash), GFP_KERNEL);
}

static inline void crypto_free_shash(struct crypto_shash *tfm)
{
  kfree(tfm);
}

static inline int crypto_shash_setkey(struct crypto_shash *tfm, const u8 *key,
                                      unsigned int keylen)
{
  if (!tfm || !key ||
      (keylen != 16 && keylen != 24 && keylen != 32))
    {
      return -EINVAL;
    }

  if (rijndael_set_key_enc_only(&tfm->aes, key, keylen * 8) < 0)
    {
      return -EINVAL;
    }

  tfm->key_set = true;
  return 0;
}

static inline void crypto_cmac_dbl(const u8 *in, u8 *out)
{
  unsigned int i;
  u8 carry = in[0] & 0x80;

  for (i = 0; i < AES_BLOCK_SIZE - 1; i++)
    {
      out[i] = (in[i] << 1) | (in[i + 1] >> 7);
    }

  out[AES_BLOCK_SIZE - 1] = in[AES_BLOCK_SIZE - 1] << 1;
  if (carry)
    {
      out[AES_BLOCK_SIZE - 1] ^= 0x87;
    }
}

static inline void crypto_cmac_process_block(struct shash_desc *desc,
                                             const u8 *block)
{
  u8 tmp[AES_BLOCK_SIZE];
  unsigned int i;

  for (i = 0; i < AES_BLOCK_SIZE; i++)
    {
      tmp[i] = desc->state[i] ^ block[i];
    }

  rijndael_encrypt(&desc->tfm->aes, tmp, desc->state);
  memset(tmp, 0, sizeof(tmp));
}

static inline int crypto_shash_init(struct shash_desc *desc)
{
  if (!desc || !desc->tfm || !desc->tfm->key_set)
    {
      return -EINVAL;
    }

  memset(desc->state, 0, sizeof(desc->state));
  memset(desc->block, 0, sizeof(desc->block));
  desc->block_len = 0;
  desc->initialized = true;
  return 0;
}

static inline int crypto_shash_update(struct shash_desc *desc,
                                      const u8 *data, unsigned int len)
{
  if (!desc || !desc->initialized || (!data && len))
    {
      return -EINVAL;
    }

  while (len > 0)
    {
      unsigned int copy_len;

      if (desc->block_len == AES_BLOCK_SIZE)
        {
          crypto_cmac_process_block(desc, desc->block);
          desc->block_len = 0;
        }

      copy_len = AES_BLOCK_SIZE - desc->block_len;
      if (copy_len > len)
        {
          copy_len = len;
        }

      memcpy(desc->block + desc->block_len, data, copy_len);
      desc->block_len += copy_len;
      data += copy_len;
      len -= copy_len;
    }

  return 0;
}

static inline int crypto_shash_finup(struct shash_desc *desc, const u8 *data,
                                     unsigned int len, u8 *out)
{
  u8 zero[AES_BLOCK_SIZE] = {};
  u8 l[AES_BLOCK_SIZE];
  u8 k1[AES_BLOCK_SIZE];
  u8 k2[AES_BLOCK_SIZE];
  u8 last[AES_BLOCK_SIZE];
  unsigned int i;
  int ret;

  if (!desc || !out)
    {
      return -EINVAL;
    }

  ret = crypto_shash_update(desc, data, len);
  if (ret)
    {
      return ret;
    }

  rijndael_encrypt(&desc->tfm->aes, zero, l);
  crypto_cmac_dbl(l, k1);
  crypto_cmac_dbl(k1, k2);

  if (desc->block_len == AES_BLOCK_SIZE)
    {
      for (i = 0; i < AES_BLOCK_SIZE; i++)
        {
          last[i] = desc->block[i] ^ k1[i];
        }
    }
  else
    {
      memset(last, 0, sizeof(last));
      memcpy(last, desc->block, desc->block_len);
      last[desc->block_len] = 0x80;
      for (i = 0; i < AES_BLOCK_SIZE; i++)
        {
          last[i] ^= k2[i];
        }
    }

  crypto_cmac_process_block(desc, last);
  memcpy(out, desc->state, AES_BLOCK_SIZE);

  memset(l, 0, sizeof(l));
  memset(k1, 0, sizeof(k1));
  memset(k2, 0, sizeof(k2));
  memset(last, 0, sizeof(last));
  memset(desc->state, 0, sizeof(desc->state));
  memset(desc->block, 0, sizeof(desc->block));
  desc->block_len = 0;
  desc->initialized = false;
  return 0;
}

static inline int crypto_shash_digest(struct shash_desc *desc, const u8 *data,
                                      unsigned int len, u8 *out)
{
  int ret;

  ret = crypto_shash_init(desc);
  if (ret)
    {
      return ret;
    }

  return crypto_shash_finup(desc, data, len, out);
}

#endif
