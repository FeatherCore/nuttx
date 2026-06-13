/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __LINUX_BT_UPSTREAM_NET_BLUETOOTH_ECDH_HELPER_H
#define __LINUX_BT_UPSTREAM_NET_BLUETOOTH_ECDH_HELPER_H

#include <errno.h>
#include <string.h>

#include <crypto/kpp.h>
#include <linux/types.h>

int ecc_make_key_uncomp(uint8_t publickey_x[32],
                        uint8_t publickey_y[32],
                        uint8_t privatekey[32]);
int ecdh_shared_secret(const uint8_t publickey[33],
                       const uint8_t privatekey[32],
                       uint8_t secret[32]);

static inline int set_ecdh_privkey(struct crypto_kpp *tfm,
                                   const uint8_t private_key[32])
{
  if (tfm == NULL || private_key == NULL)
    {
      return -EINVAL;
    }

  memcpy(tfm->private_key, private_key, sizeof(tfm->private_key));
  tfm->has_private_key = true;
  tfm->has_public_key = false;
  return 0;
}

static inline int generate_ecdh_keys(struct crypto_kpp *tfm,
                                     uint8_t public_key[64])
{
  if (tfm == NULL || public_key == NULL)
    {
      return -EINVAL;
    }

  if (!ecc_make_key_uncomp(tfm->public_key, tfm->public_key + 32,
                           tfm->private_key))
    {
      return -EIO;
    }

  memcpy(public_key, tfm->public_key, sizeof(tfm->public_key));
  tfm->has_private_key = true;
  tfm->has_public_key = true;
  return 0;
}

static inline int generate_ecdh_public_key(struct crypto_kpp *tfm,
                                           uint8_t public_key[64])
{
  if (tfm == NULL || public_key == NULL)
    {
      return -EINVAL;
    }

  if (!tfm->has_public_key)
    {
      return -EOPNOTSUPP;
    }

  memcpy(public_key, tfm->public_key, sizeof(tfm->public_key));
  return 0;
}

static inline int compute_ecdh_secret(struct crypto_kpp *tfm,
                                      const uint8_t remote_public_key[64],
                                      uint8_t secret[32])
{
  uint8_t public_key[33];

  if (tfm == NULL || remote_public_key == NULL || secret == NULL ||
      !tfm->has_private_key)
    {
      return -EINVAL;
    }

  public_key[0] = 0x04;
  memcpy(public_key + 1, remote_public_key, 32);

  if (!ecdh_shared_secret(public_key, tfm->private_key, secret))
    {
      return -EIO;
    }

  return 0;
}

#endif /* __LINUX_BT_UPSTREAM_NET_BLUETOOTH_ECDH_HELPER_H */
