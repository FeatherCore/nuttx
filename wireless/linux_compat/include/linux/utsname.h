#ifndef __WIRELESS_LINUX_COMPAT_INCLUDE_LINUX_UTSNAME_H
#define __WIRELESS_LINUX_COMPAT_INCLUDE_LINUX_UTSNAME_H

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
    .machine = "sim",
  };

  return &uts;
}

#endif
