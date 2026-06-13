/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/crc32.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_CRC32_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_CRC32_H

#include <stdint.h>
#include <stddef.h>

static inline uint32_t crc32_be(uint32_t crc, const void *p, size_t len)
{
  const uint8_t *buf = (const uint8_t *)p;
  size_t i;

  while (len-- > 0)
    {
      crc ^= (uint32_t)(*buf++) << 24;

      for (i = 0; i < 8; i++)
        {
          if ((crc & 0x80000000u) != 0)
            {
              crc = (crc << 1) ^ 0x04c11db7u;
            }
          else
            {
              crc <<= 1;
            }
        }
    }

  return crc;
}

#endif
