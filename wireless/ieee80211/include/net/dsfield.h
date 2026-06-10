#ifndef __WIRELESS_IEEE80211_INCLUDE_NET_DSFIELD_H
#define __WIRELESS_IEEE80211_INCLUDE_NET_DSFIELD_H
static inline unsigned int ipv4_get_dsfield(const void *iph)
{
  (void)iph;
  return 0;
}
static inline unsigned int ipv6_get_dsfield(const void *ip6h)
{
  (void)ip6h;
  return 0;
}
#endif
