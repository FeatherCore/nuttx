/****************************************************************************
 * wireless/linux_bluetooth/upstream/net_bluetooth/msft.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_UPSTREAM_MSFT_H
#define __WIRELESS_LINUX_BLUETOOTH_UPSTREAM_MSFT_H

#include <net/bluetooth/hci_core.h>

struct adv_monitor;
struct sk_buff;

static inline int msft_register(struct hci_dev *hdev)
{
  (void)hdev;
  return 0;
}

static inline void msft_release(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline bool msft_monitor_supported(struct hci_dev *hdev)
{
  (void)hdev;
  return false;
}

static inline bool msft_curve_validity(struct hci_dev *hdev)
{
  (void)hdev;
  return false;
}

static inline int msft_add_monitor_pattern(struct hci_dev *hdev,
                                           struct adv_monitor *monitor)
{
  (void)hdev;
  (void)monitor;
  return -EOPNOTSUPP;
}

static inline int msft_remove_monitor(struct hci_dev *hdev,
                                      struct adv_monitor *monitor)
{
  (void)hdev;
  (void)monitor;
  return -EOPNOTSUPP;
}

static inline void msft_do_open(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void msft_do_close(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void msft_suspend_sync(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void msft_resume_sync(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void msft_vendor_evt(struct hci_dev *hdev, void *data,
                                   struct sk_buff *skb)
{
  (void)hdev;
  (void)data;
  (void)skb;
}

#endif
