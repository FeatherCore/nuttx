#ifndef __WIRELESS_IEEE80211_INCLUDE_CRYPTO_SKCIPHER_H
#define __WIRELESS_IEEE80211_INCLUDE_CRYPTO_SKCIPHER_H

#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <crypto/rijndael.h>

struct crypto_skcipher
{
  rijndael_ctx aes;
  bool key_set;
};

struct skcipher_request
{
  struct crypto_skcipher *tfm;
  struct scatterlist *src;
  struct scatterlist *dst;
  unsigned int cryptlen;
  u8 iv[16];
};

static inline struct crypto_skcipher *crypto_alloc_skcipher(const char *alg,
                                                            u32 type,
                                                            u32 mask)
{
  (void)alg;
  (void)type;
  (void)mask;
  return kzalloc(sizeof(struct crypto_skcipher), GFP_KERNEL);
}

static inline void crypto_free_skcipher(struct crypto_skcipher *tfm)
{
  kfree(tfm);
}

static inline int crypto_skcipher_setkey(struct crypto_skcipher *tfm,
                                         const u8 *key,
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

static inline struct skcipher_request *
skcipher_request_alloc(struct crypto_skcipher *tfm, gfp_t gfp)
{
  struct skcipher_request *req;

  (void)gfp;
  req = kzalloc(sizeof(*req), GFP_KERNEL);
  if (req)
    {
      req->tfm = tfm;
    }

  return req;
}

static inline void skcipher_request_free(struct skcipher_request *req)
{
  kfree(req);
}

static inline void skcipher_request_set_crypt(struct skcipher_request *req,
                                              struct scatterlist *src,
                                              struct scatterlist *dst,
                                              unsigned int cryptlen,
                                              u8 *iv)
{
  req->src = src;
  req->dst = dst;
  req->cryptlen = cryptlen;
  memcpy(req->iv, iv, sizeof(req->iv));
}

static inline int crypto_skcipher_ctr_crypt(struct skcipher_request *req)
{
  u8 counter[16];
  u8 stream[16];
  const u8 *src;
  u8 *dst;
  unsigned int left;

  if (!req || !req->tfm || !req->tfm->key_set ||
      !req->src || !req->dst ||
      req->src->length < req->cryptlen ||
      req->dst->length < req->cryptlen)
    {
      return -EINVAL;
    }

  memcpy(counter, req->iv, sizeof(counter));
  src = req->src->buf;
  dst = req->dst->buf;
  left = req->cryptlen;

  while (left > 0)
    {
      unsigned int todo = left < sizeof(stream) ? left : sizeof(stream);
      unsigned int i;

      rijndael_encrypt(&req->tfm->aes, counter, stream);
      for (i = 0; i < todo; i++)
        {
          dst[i] = src[i] ^ stream[i];
        }

      src += todo;
      dst += todo;
      left -= todo;

      for (i = sizeof(counter); i > 0; i--)
        {
          counter[i - 1]++;
          if (counter[i - 1])
            {
              break;
            }
        }
    }

  memset(stream, 0, sizeof(stream));
  memset(counter, 0, sizeof(counter));
  return 0;
}

static inline int crypto_skcipher_encrypt(struct skcipher_request *req)
{
  return crypto_skcipher_ctr_crypt(req);
}

static inline int crypto_skcipher_decrypt(struct skcipher_request *req)
{
  return crypto_skcipher_ctr_crypt(req);
}

static inline int crypto_skcipher_ivsize(struct crypto_skcipher *tfm)
{
  (void)tfm;
  return 16;
}

static inline unsigned int crypto_skcipher_blocksize(struct crypto_skcipher *tfm)
{
  (void)tfm;
  return 1;
}

static inline unsigned int crypto_skcipher_alignmask(struct crypto_skcipher *tfm)
{
  (void)tfm;
  return 0;
}

#endif
