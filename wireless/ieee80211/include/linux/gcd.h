#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_GCD_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_GCD_H
static inline unsigned long gcd(unsigned long a, unsigned long b)
{
  while (b)
    {
      unsigned long t = b;
      b = a % b;
      a = t;
    }

  return a;
}
#endif
