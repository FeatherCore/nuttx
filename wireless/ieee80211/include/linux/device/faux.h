#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_DEVICE_FAUX_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_DEVICE_FAUX_H
#include <linux/device.h>
struct faux_device
{
  struct device dev;
};
static inline struct faux_device *faux_device_create(const char *name,
                                                     struct device *parent,
                                                     void *drvdata)
{
  static struct faux_device faux;

  (void)name;
  faux.dev.parent = parent;
  faux.dev.driver_data = drvdata;

  return &faux;
}
static inline void faux_device_destroy(struct faux_device *faux)
{
  (void)faux;
}
#endif
