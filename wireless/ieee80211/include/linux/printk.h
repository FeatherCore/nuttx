#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_PRINTK_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_PRINTK_H

#include <linux/cfg80211_compat.h>

#ifndef DUMP_PREFIX_ADDRESS
#  define DUMP_PREFIX_ADDRESS 0
#endif

#ifndef KBUILD_MODNAME
#  define KBUILD_MODNAME "nuttx"
#endif

#ifndef KERN_ERR
#  define KERN_ERR ""
#endif

#ifndef KERN_WARNING
#  define KERN_WARNING ""
#endif

#ifndef KERN_INFO
#  define KERN_INFO ""
#endif

#ifndef KERN_DEBUG
#  define KERN_DEBUG ""
#endif

static inline void print_hex_dump(const char *level, const char *prefix_str,
                                  int prefix_type, int rowsize,
                                  int groupsize, const void *buf,
                                  size_t len, bool ascii)
{
  (void)level;
  (void)prefix_type;
  (void)rowsize;
  (void)groupsize;
  (void)buf;
  (void)len;
  (void)ascii;
  if (prefix_str != NULL)
    {
      printf("%s\n", prefix_str);
    }
}

#endif /* __WIRELESS_IEEE80211_INCLUDE_LINUX_PRINTK_H */
