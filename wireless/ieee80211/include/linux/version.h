/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_VERSION_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_VERSION_H

#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))

/*
 * Keep imported Linux drivers on their modern code paths.  This does not
 * describe the NuttX kernel version; it only selects compatible prototypes.
 */

#define LINUX_VERSION_CODE KERNEL_VERSION(6, 12, 0)

#endif /* __WIRELESS_IEEE80211_INCLUDE_LINUX_VERSION_H */
