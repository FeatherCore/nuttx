#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_BITOPS_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_BITOPS_H

#ifndef BITS_PER_LONG
#  define BITS_PER_LONG (sizeof(unsigned long) * 8)
#endif

#ifndef BITS_TO_LONGS
#  define BITS_TO_LONGS(nr) (((nr) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#endif

#ifndef DECLARE_BITMAP
#  define DECLARE_BITMAP(name, bits) unsigned long name[BITS_TO_LONGS(bits)]
#endif

static inline void set_bit(unsigned int nr, volatile unsigned long *addr)
{
  addr[nr / BITS_PER_LONG] |= 1UL << (nr % BITS_PER_LONG);
}

static inline void clear_bit(unsigned int nr, volatile unsigned long *addr)
{
  addr[nr / BITS_PER_LONG] &= ~(1UL << (nr % BITS_PER_LONG));
}

static inline int test_bit(unsigned int nr, const volatile unsigned long *addr)
{
  return (addr[nr / BITS_PER_LONG] & (1UL << (nr % BITS_PER_LONG))) != 0;
}

static inline int test_and_set_bit(unsigned int nr,
                                   volatile unsigned long *addr)
{
  int old = test_bit(nr, addr);

  set_bit(nr, addr);
  return old;
}

static inline int test_and_clear_bit(unsigned int nr,
                                     volatile unsigned long *addr)
{
  int old = test_bit(nr, addr);

  clear_bit(nr, addr);
  return old;
}

static inline void smp_mb__after_atomic(void)
{
}

#endif
