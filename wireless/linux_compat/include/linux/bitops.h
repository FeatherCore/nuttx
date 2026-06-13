#ifndef __WIRELESS_LINUX_COMPAT_INCLUDE_LINUX_BITOPS_H
#define __WIRELESS_LINUX_COMPAT_INCLUDE_LINUX_BITOPS_H

#include <linux/kernel.h>

#ifndef __WIRELESS_IEEE80211_BITOPS_DEFINED
#define __WIRELESS_IEEE80211_BITOPS_DEFINED
static inline bool test_bit(unsigned int nr, const unsigned long *addr)
{
  return !!(addr[nr / BITS_PER_LONG] & BIT(nr % BITS_PER_LONG));
}

static inline void set_bit(unsigned int nr, unsigned long *addr)
{
  addr[nr / BITS_PER_LONG] |= BIT(nr % BITS_PER_LONG);
}

static inline void __set_bit(unsigned int nr, unsigned long *addr)
{
  set_bit(nr, addr);
}

static inline void clear_bit(unsigned int nr, unsigned long *addr)
{
  addr[nr / BITS_PER_LONG] &= ~BIT(nr % BITS_PER_LONG);
}

static inline void __clear_bit(unsigned int nr, unsigned long *addr)
{
  clear_bit(nr, addr);
}

static inline bool test_and_clear_bit(unsigned int nr, unsigned long *addr)
{
  bool old = test_bit(nr, addr);

  clear_bit(nr, addr);
  return old;
}

static inline bool test_and_set_bit(unsigned int nr, unsigned long *addr)
{
  bool old = test_bit(nr, addr);

  set_bit(nr, addr);
  return old;
}
#endif

#endif
