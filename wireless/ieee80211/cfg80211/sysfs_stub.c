// SPDX-License-Identifier: GPL-2.0
/*
 * NuttX build stub for the Linux cfg80211 sysfs integration.
 */

#include <linux/device.h>

#include "sysfs.h"

struct class ieee80211_class =
{
  .name = "ieee80211",
};

int wiphy_sysfs_init(void)
{
  return 0;
}

void wiphy_sysfs_exit(void)
{
}
