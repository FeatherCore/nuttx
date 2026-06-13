#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_PLATFORM_DEVICE_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_PLATFORM_DEVICE_H

#include <linux/device.h>

struct platform_device
{
  struct device dev;
};

struct platform_driver
{
  struct
  {
    const char *name;
  } driver;
};

static inline int platform_driver_register(struct platform_driver *driver)
{
  (void)driver;
  return 0;
}

static inline void platform_driver_unregister(struct platform_driver *driver)
{
  (void)driver;
}

#endif /* __WIRELESS_IEEE80211_INCLUDE_LINUX_PLATFORM_DEVICE_H */
