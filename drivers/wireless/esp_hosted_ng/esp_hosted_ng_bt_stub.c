/****************************************************************************
 * drivers/wireless/esp_hosted_ng/esp_hosted_ng_bt_stub.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#include <nuttx/config.h>

#include "esp.h"

int esp_init_bt(FAR struct esp_adapter *adapter)
{
  (void)adapter;
  return 0;
}

int esp_deinit_bt(FAR struct esp_adapter *adapter)
{
  (void)adapter;
  return 0;
}

void esp_hci_update_tx_counter(FAR struct hci_dev *hdev, u8 pkt_type,
                               size_t len)
{
  (void)hdev;
  (void)pkt_type;
  (void)len;
}

void esp_hci_update_rx_counter(FAR struct hci_dev *hdev, u8 pkt_type,
                               size_t len)
{
  (void)hdev;
  (void)pkt_type;
  (void)len;
}
