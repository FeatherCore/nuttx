#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_BITFIELD_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_BITFIELD_H
#include <linux/cfg80211_compat.h>
#ifndef __bf_shf
#  define __bf_shf(x) (__builtin_ffsll(x) - 1)
#endif
#ifndef FIELD_PREP
#  define FIELD_PREP(mask, val) (((typeof(mask))(val) << __bf_shf(mask)) & (mask))
#endif
#ifndef FIELD_PREP_CONST
#  define FIELD_PREP_CONST(mask, val) (((val) << (__builtin_ffsll(mask) - 1)) & (mask))
#endif
#ifndef FIELD_GET
#  define FIELD_GET(mask, reg) (((reg) & (mask)) >> __bf_shf(mask))
#endif
#ifndef FIELD_MAX
#  define FIELD_MAX(mask) ((typeof(mask))((mask) >> __bf_shf(mask)))
#endif
#ifndef FIELD_FIT
#  define FIELD_FIT(mask, val) (!(((typeof(mask))(val) << __bf_shf(mask)) & ~(mask)))
#endif
#ifndef u8_get_bits
#  define u8_get_bits(value, field) FIELD_GET(field, value)
#endif
#ifndef u16_get_bits
#  define u16_get_bits(value, field) FIELD_GET(field, value)
#endif
#ifndef le16_get_bits
#  define le16_get_bits(value, field) FIELD_GET(field, value)
#endif
#ifndef le32_get_bits
#  define le32_get_bits(value, field) FIELD_GET(field, value)
#endif
#endif
