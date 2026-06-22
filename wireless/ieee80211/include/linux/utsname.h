#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_UTSNAME_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_UTSNAME_H

struct new_utsname
{
  char release[32];
  char machine[32];
};

static inline struct new_utsname *init_utsname(void)
{
  static struct new_utsname uts =
  {
    .release = "NuttX",
    .machine = "arm",
  };

  return &uts;
}

#endif
