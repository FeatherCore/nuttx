#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_DEVICE_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_DEVICE_H
#include <linux/cfg80211_compat.h>
struct device
{
  struct device *parent;
  struct class *class;
  const struct device_type *type;
  const struct device_driver *driver;
  void *platform_data;
  struct
  {
    int dummy;
  } kobj;
  void *driver_data;
  char name[64];
};
struct class
{
  const char *name;
};
struct device_driver
{
  const char *name;
};
struct device_type
{
  const char *name;
};
static inline const char *dev_name(const struct device *dev)
{
  return dev != NULL && dev->name[0] != '\0' ? dev->name : "nuttx";
}
static inline int dev_set_name(struct device *dev, const char *fmt, ...)
{
  va_list ap;

  if (dev == NULL)
    {
      return -EINVAL;
    }

  va_start(ap, fmt);
  vsnprintf(dev->name, sizeof(dev->name), fmt, ap);
  va_end(ap);
  return 0;
}
static inline void device_initialize(struct device *dev)
{
  if (dev != NULL)
    {
      memset(dev, 0, sizeof(*dev));
    }
}
static inline void device_enable_async_suspend(struct device *dev)
{
  (void)dev;
}
static inline int device_add(struct device *dev)
{
  (void)dev;
  return 0;
}
static inline void device_del(struct device *dev)
{
  (void)dev;
}
static inline void put_device(struct device *dev)
{
  (void)dev;
}
static inline struct class *class_create(const char *name)
{
  struct class *class;

  class = kmm_zalloc(sizeof(*class));
  if (class != NULL)
    {
      class->name = name;
    }

  return class;
}
static inline void class_destroy(struct class *class)
{
  kmm_free(class);
}
static inline struct device *device_create(struct class *class,
                                           struct device *parent,
                                           int devt, void *drvdata,
                                           const char *fmt, ...)
{
  struct device *dev;
  va_list ap;

  (void)devt;

  dev = kmm_zalloc(sizeof(*dev));
  if (dev == NULL)
    {
      return NULL;
    }

  dev->class = class;
  dev->parent = parent;
  dev->driver_data = drvdata;

  va_start(ap, fmt);
  vsnprintf(dev->name, sizeof(dev->name), fmt, ap);
  va_end(ap);
  return dev;
}
static inline int device_bind_driver(struct device *dev)
{
  (void)dev;
  return 0;
}
static inline void device_release_driver(struct device *dev)
{
  (void)dev;
}
static inline void device_unregister(struct device *dev)
{
  kmm_free(dev);
}
static inline int device_rename(struct device *dev, const char *new_name)
{
  (void)dev;
  (void)new_name;
  return 0;
}
static inline void dev_set_uevent_suppress(struct device *dev, bool suppress)
{
  (void)dev;
  (void)suppress;
}
static inline int sysfs_create_link(void *kobj, void *target,
                                    const char *name)
{
  (void)kobj;
  (void)target;
  (void)name;
  return 0;
}
static inline void sysfs_remove_link(void *kobj, const char *name)
{
  (void)kobj;
  (void)name;
}
#endif
