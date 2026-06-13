/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/crypto/algapi.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_CRYPTO_ALGAPI_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_CRYPTO_ALGAPI_H

#include <string.h>

static inline int crypto_memneq(const void *a, const void *b, size_t size)
{
  return memcmp(a, b, size) != 0;
}

#endif

