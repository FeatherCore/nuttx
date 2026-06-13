/****************************************************************************
 * wireless/linux_bluetooth/upstream/net_bluetooth/leds.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_UPSTREAM_LEDS_H
#define __WIRELESS_LINUX_BLUETOOTH_UPSTREAM_LEDS_H

#include <net/bluetooth/hci_core.h>

static inline void hci_leds_init(struct hci_dev *hdev)
{
  (void)hdev;
}

static inline void hci_leds_update_powered(struct hci_dev *hdev,
                                           bool powered)
{
  (void)hdev;
  (void)powered;
}

static inline int bt_leds_init(void)
{
  return 0;
}

static inline void bt_leds_cleanup(void)
{
}

#endif
