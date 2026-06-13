/****************************************************************************
 * wireless/linux_bluetooth/upstream/net_bluetooth/aosp.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_UPSTREAM_AOSP_H
#define __WIRELESS_LINUX_BLUETOOTH_UPSTREAM_AOSP_H

#include <net/bluetooth/hci_core.h>

static inline void aosp_do_open(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void aosp_do_close(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline bool aosp_has_quality_report(struct hci_dev *hdev)
{
  (void)hdev;
  return false;
}

static inline int aosp_set_quality_report(struct hci_dev *hdev, bool enable)
{
  (void)hdev;
  (void)enable;
  return -EOPNOTSUPP;
}

#endif
