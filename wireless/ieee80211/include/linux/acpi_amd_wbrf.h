#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_ACPI_AMD_WBRF_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_ACPI_AMD_WBRF_H

#include <linux/cfg80211_compat.h>

#define WBRF_RECORD_ADD 1
#define WBRF_RECORD_REMOVE 2

struct wbrf_range
{
  u64 start;
  u64 end;
};

struct wbrf_ranges_in_out
{
  struct wbrf_range band_list[2];
  u32 num_of_ranges;
};

static inline bool acpi_amd_wbrf_supported_producer(struct device *dev)
{
  (void)dev;
  return false;
}

static inline int acpi_amd_wbrf_add_remove(struct device *dev, int action,
                                           struct wbrf_ranges_in_out *ranges)
{
  (void)dev;
  (void)action;
  (void)ranges;
  return 0;
}

#endif
