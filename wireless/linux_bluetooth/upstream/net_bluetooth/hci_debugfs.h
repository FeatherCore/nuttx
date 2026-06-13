/****************************************************************************
 * wireless/linux_bluetooth/upstream/net_bluetooth/hci_debugfs.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_UPSTREAM_HCI_DEBUGFS_H
#define __WIRELESS_LINUX_BLUETOOTH_UPSTREAM_HCI_DEBUGFS_H

#include <net/bluetooth/hci_core.h>

static inline void hci_debugfs_create_basic(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void hci_debugfs_create_common(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void hci_debugfs_create_bredr(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void hci_debugfs_create_le(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void hci_debugfs_create_conn(struct hci_conn *conn)
{
  (void)conn;
}

#endif

