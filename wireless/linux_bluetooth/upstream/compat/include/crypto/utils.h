/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __LINUX_BT_COMPAT_CRYPTO_UTILS_H
#define __LINUX_BT_COMPAT_CRYPTO_UTILS_H

#include <string.h>

#include <linux/types.h>

static inline int crypto_memneq(const void *a, const void *b, size_t size)
{
  return memcmp(a, b, size) != 0;
}

static inline void crypto_xor(uint8_t *dst, const uint8_t *src,
                              unsigned int size)
{
  unsigned int i;

  for (i = 0; i < size; i++)
    {
      dst[i] ^= src[i];
    }
}

static inline void crypto_xor_cpy(uint8_t *dst, const uint8_t *src1,
                                  const uint8_t *src2, unsigned int size)
{
  unsigned int i;

  for (i = 0; i < size; i++)
    {
      dst[i] = src1[i] ^ src2[i];
    }
}

#endif /* __LINUX_BT_COMPAT_CRYPTO_UTILS_H */
