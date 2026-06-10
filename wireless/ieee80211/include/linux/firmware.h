#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_FIRMWARE_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_FIRMWARE_H
#include <linux/cfg80211_compat.h>
struct firmware
{
  size_t size;
  const u8 *data;
};
static inline int request_firmware(const struct firmware **fw,
                                   const char *name,
                                   struct device *device)
{
  (void)name;
  (void)device;
  *fw = NULL;
  return -ENOENT;
}
static inline int request_firmware_nowait(struct module *module, bool uevent,
                                          const char *name,
                                          struct device *device, gfp_t gfp,
                                          void *context,
                                          void (*cont)(const struct firmware *,
                                                       void *))
{
  (void)module;
  (void)uevent;
  (void)name;
  (void)device;
  (void)gfp;
  if (cont)
    {
      cont(NULL, context);
    }

  return 0;
}
static inline void release_firmware(const struct firmware *fw)
{
  (void)fw;
}
#endif
