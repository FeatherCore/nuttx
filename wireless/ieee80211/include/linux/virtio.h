#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_VIRTIO_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_VIRTIO_H

#include <linux/cfg80211_compat.h>

struct virtqueue;
struct virtio_device;

struct virtqueue_info
{
  const char *name;
  void (*callback)(struct virtqueue *vq);
};

#endif
