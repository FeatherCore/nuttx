/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __LINUX_BT_COMPAT_CRYPTO_KPP_H
#define __LINUX_BT_COMPAT_CRYPTO_KPP_H

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/types.h>

struct crypto_kpp
{
  uint8_t private_key[32];
  uint8_t public_key[64];
  bool has_private_key;
  bool has_public_key;
};

static inline struct crypto_kpp *crypto_alloc_kpp(const char *alg_name,
                                                  u32 type, u32 mask)
{
  struct crypto_kpp *tfm;

  if (alg_name == NULL || strcmp(alg_name, "ecdh-nist-p256") != 0)
    {
      return ERR_PTR(-ENOENT);
    }

  tfm = kzalloc(sizeof(*tfm), GFP_KERNEL);
  return tfm != NULL ? tfm : ERR_PTR(-ENOMEM);
}

static inline void crypto_free_kpp(struct crypto_kpp *tfm)
{
  if (!IS_ERR_OR_NULL(tfm))
    {
      kfree_sensitive(tfm);
    }
}

#endif /* __LINUX_BT_COMPAT_CRYPTO_KPP_H */
