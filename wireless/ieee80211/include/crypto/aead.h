#ifndef __WIRELESS_IEEE80211_INCLUDE_CRYPTO_AEAD_H
#define __WIRELESS_IEEE80211_INCLUDE_CRYPTO_AEAD_H

#include <linux/crypto.h>
#include <linux/scatterlist.h>

struct crypto_aead
{
  unsigned int authsize;
};

struct aead_request
{
  struct crypto_aead *tfm;
};

static inline struct crypto_aead *crypto_alloc_aead(const char *alg,
                                                    u32 type, u32 mask)
{
  (void)alg;
  (void)type;
  (void)mask;
  return kzalloc(sizeof(struct crypto_aead), GFP_KERNEL);
}

static inline void crypto_free_aead(struct crypto_aead *tfm)
{
  kfree(tfm);
}

static inline int crypto_aead_setkey(struct crypto_aead *tfm, const u8 *key,
                                     unsigned int keylen)
{
  (void)tfm;
  (void)key;
  (void)keylen;
  return 0;
}

static inline int crypto_aead_setauthsize(struct crypto_aead *tfm,
                                          unsigned int authsize)
{
  tfm->authsize = authsize;
  return 0;
}

static inline unsigned int crypto_aead_authsize(struct crypto_aead *tfm)
{
  return tfm ? tfm->authsize : 0;
}

static inline unsigned int crypto_aead_reqsize(struct crypto_aead *tfm)
{
  (void)tfm;
  return 0;
}

static inline void aead_request_set_tfm(struct aead_request *req,
                                        struct crypto_aead *tfm)
{
  req->tfm = tfm;
}

static inline void aead_request_set_crypt(struct aead_request *req,
                                          struct scatterlist *src,
                                          struct scatterlist *dst,
                                          unsigned int cryptlen,
                                          u8 *iv)
{
  (void)req;
  (void)src;
  (void)dst;
  (void)cryptlen;
  (void)iv;
}

static inline void aead_request_set_ad(struct aead_request *req,
                                       unsigned int assoclen)
{
  (void)req;
  (void)assoclen;
}

static inline int crypto_aead_encrypt(struct aead_request *req)
{
  (void)req;
  return 0;
}

static inline int crypto_aead_decrypt(struct aead_request *req)
{
  (void)req;
  return 0;
}

#endif
