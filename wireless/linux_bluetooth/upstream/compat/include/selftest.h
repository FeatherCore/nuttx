#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_SELFTEST_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_SELFTEST_H

static inline int bt_selftest(void)
{
  return 0;
}

static inline void bt_selftest_init(void)
{
}

#endif
