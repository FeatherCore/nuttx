/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __LINUX_BT_COMPAT_LINUX_CRC16_H
#define __LINUX_BT_COMPAT_LINUX_CRC16_H

#include <linux/types.h>

uint16_t crc16ibmpart(const uint8_t *src, size_t len, uint16_t crc16val);

static inline u16 crc16(u16 crc, const u8 *p, size_t len)
{
  return crc16ibmpart(p, len, crc);
}

#endif /* __LINUX_BT_COMPAT_LINUX_CRC16_H */
