/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/net/mac802154.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_MAC802154_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_MAC802154_H

#include <linux/types.h>

#ifndef IEEE802154_PAN_ID_BROADCAST
#  define IEEE802154_PAN_ID_BROADCAST 0xffff
#endif
#ifndef IEEE802154_ADDR_SHORT_UNSPEC
#  define IEEE802154_ADDR_SHORT_UNSPEC 0xfffe
#endif
#ifndef IEEE802154_SHORT_ADDR_LEN
#  define IEEE802154_SHORT_ADDR_LEN 2
#endif

enum ieee802154_addr_mode
{
  IEEE802154_ADDR_NONE = 0,
  IEEE802154_ADDR_SHORT = 2,
  IEEE802154_ADDR_LONG = 3,
};

struct ieee802154_addr
{
  enum ieee802154_addr_mode mode;
  __le16 pan_id;
  union
  {
    __le16 short_addr;
    __le64 extended_addr;
  };
};

struct wpan_dev
{
  __le16 short_addr;
  __le16 pan_id;
};

static inline void ieee802154_le16_to_be16(void *dst, const void *src)
{
  const __u8 *s = (const __u8 *)src;
  __u8 *d = (__u8 *)dst;

  d[0] = s[1];
  d[1] = s[0];
}

static inline void ieee802154_be16_to_le16(void *dst, const void *src)
{
  ieee802154_le16_to_be16(dst, src);
}

static inline void ieee802154_le64_to_be64(void *dst, const void *src)
{
  const __u8 *s = (const __u8 *)src;
  __u8 *d = (__u8 *)dst;
  int i;

  for (i = 0; i < 8; i++)
    {
      d[i] = s[7 - i];
    }
}

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_MAC802154_H */
