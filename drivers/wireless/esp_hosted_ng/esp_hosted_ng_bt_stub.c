/****************************************************************************
 * drivers/wireless/esp_hosted_ng/esp_hosted_ng_bt_stub.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <nuttx/mutex.h>

#include <linux/skbuff.h>

#include <nuttx/wireless/bluetooth/bt_driver.h>

#include "esp.h"
#include "esp_api.h"
#include "esp_if.h"
#include "esp_hosted_ng_proto.h"
#include "adapter.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ESP_HCI_COMMAND_PKT 0x01
#define ESP_HCI_ACLDATA_PKT 0x02
#define ESP_HCI_SCODATA_PKT 0x03
#define ESP_HCI_EVENT_PKT   0x04
#define ESP_HCI_ISODATA_PKT 0x05

#define ESP_HCI_RX_DRAIN_POLLS 25
#define ESP_HCI_RX_DRAIN_USEC  2000
#define ESP_HCI_TRACE_LIMIT     16

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct esp_hosted_ng_btdev_s
{
  struct bt_driver_s driver;
  FAR struct esp_adapter *adapter;
  mutex_t lock;
  bool registered;
  bool opened;
  unsigned int cmd_tx;
  unsigned int acl_tx;
  unsigned int iso_tx;
  unsigned int evt_rx;
  unsigned int acl_rx;
  unsigned int iso_rx;
  unsigned int err_tx;
  unsigned int err_rx;
  unsigned int trace_tx;
  unsigned int trace_rx;
  size_t byte_tx;
  size_t byte_rx;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct esp_hosted_ng_btdev_s g_esp_btdev =
{
  .lock = NXMUTEX_INITIALIZER,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR struct esp_hosted_ng_btdev_s *
esp_hosted_ng_from_driver(FAR struct bt_driver_s *driver)
{
  return (FAR struct esp_hosted_ng_btdev_s *)driver;
}

static int esp_hosted_ng_bt_packet_from_type(enum bt_buf_type_e type,
                                             FAR uint8_t *pkt_type)
{
  switch (type)
    {
      case BT_CMD:
        *pkt_type = ESP_HCI_COMMAND_PKT;
        return OK;

      case BT_ACL_OUT:
        *pkt_type = ESP_HCI_ACLDATA_PKT;
        return OK;

      case BT_ISO_OUT:
        *pkt_type = ESP_HCI_ISODATA_PKT;
        return OK;

      default:
        return -EINVAL;
    }
}

static int esp_hosted_ng_bt_type_from_packet(uint8_t pkt_type,
                                             FAR enum bt_buf_type_e *type)
{
  switch (pkt_type)
    {
      case ESP_HCI_EVENT_PKT:
        *type = BT_EVT;
        return OK;

      case ESP_HCI_ACLDATA_PKT:
        *type = BT_ACL_IN;
        return OK;

      case ESP_HCI_ISODATA_PKT:
        *type = BT_ISO_IN;
        return OK;

      default:
        return -EINVAL;
    }
}

static void esp_hosted_ng_bt_count_tx(FAR struct esp_hosted_ng_btdev_s *priv,
                                      uint8_t pkt_type, size_t len)
{
  switch (pkt_type)
    {
      case ESP_HCI_COMMAND_PKT:
        priv->cmd_tx++;
        break;

      case ESP_HCI_ACLDATA_PKT:
        priv->acl_tx++;
        break;

      case ESP_HCI_ISODATA_PKT:
        priv->iso_tx++;
        break;

      default:
        break;
    }

  priv->byte_tx += len;
}

static void esp_hosted_ng_bt_count_rx(FAR struct esp_hosted_ng_btdev_s *priv,
                                      uint8_t pkt_type, size_t len)
{
  switch (pkt_type)
    {
      case ESP_HCI_EVENT_PKT:
        priv->evt_rx++;
        break;

      case ESP_HCI_ACLDATA_PKT:
        priv->acl_rx++;
        break;

      case ESP_HCI_ISODATA_PKT:
        priv->iso_rx++;
        break;

      default:
        break;
    }

  priv->byte_rx += len;
}

static void esp_hosted_ng_bt_trace(FAR struct esp_hosted_ng_btdev_s *priv,
                                   FAR const char *dir, uint8_t pkt_type,
                                   FAR const uint8_t *data, size_t len)
{
  uint8_t b0 = len > 0 ? data[0] : 0;
  uint8_t b1 = len > 1 ? data[1] : 0;
  uint8_t b2 = len > 2 ? data[2] : 0;
  uint8_t b3 = len > 3 ? data[3] : 0;
  uint8_t b4 = len > 4 ? data[4] : 0;
  uint8_t b5 = len > 5 ? data[5] : 0;
  uint16_t opcode = 0;

  if (data == NULL)
    {
      return;
    }

  if (dir[0] == 'T')
    {
      if (priv->trace_tx++ >= ESP_HCI_TRACE_LIMIT)
        {
          return;
        }

      if (pkt_type == ESP_HCI_COMMAND_PKT && len >= 2)
        {
          opcode = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
        }
    }
  else
    {
      if (priv->trace_rx++ >= ESP_HCI_TRACE_LIMIT)
        {
          return;
        }

      if (pkt_type == ESP_HCI_EVENT_PKT && len >= 5 &&
          data[0] == 0x0e)
        {
          opcode = (uint16_t)data[3] | ((uint16_t)data[4] << 8);
        }
    }

  syslog(LOG_INFO,
         "esp_hosted_ng: HCI %s pkt=0x%02x len=%zu opcode=0x%04x "
         "data=%02x %02x %02x %02x %02x %02x\n",
         dir, pkt_type, len, opcode, b0, b1, b2, b3, b4, b5);
}

static void esp_hosted_ng_bt_drain_rx(FAR struct esp_adapter *adapter)
{
  if (adapter == NULL)
    {
      return;
    }

  esp_hosted_ng_poll_rx(adapter, ESP_HCI_RX_DRAIN_POLLS,
                        ESP_HCI_RX_DRAIN_USEC);
}

static int esp_hosted_ng_bt_open(FAR struct bt_driver_s *driver)
{
  FAR struct esp_hosted_ng_btdev_s *priv;
  int ret;

  if (driver == NULL)
    {
      return -EINVAL;
    }

  priv = esp_hosted_ng_from_driver(driver);
  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  priv->opened = true;
  nxmutex_unlock(&priv->lock);
  return OK;
}

static void esp_hosted_ng_bt_close(FAR struct bt_driver_s *driver)
{
  FAR struct esp_hosted_ng_btdev_s *priv;

  if (driver == NULL)
    {
      return;
    }

  priv = esp_hosted_ng_from_driver(driver);
  nxmutex_lock(&priv->lock);
  priv->opened = false;
  nxmutex_unlock(&priv->lock);
}

static int esp_hosted_ng_bt_send(FAR struct bt_driver_s *driver,
                                 enum bt_buf_type_e type,
                                 FAR void *data, size_t len)
{
  FAR struct esp_hosted_ng_btdev_s *priv;
  FAR struct esp_adapter *adapter;
  FAR struct sk_buff *skb;
  FAR struct esp_hosted_ng_payload_header_s *header;
  uint8_t pkt_type;
  size_t offset;
  size_t total_len;
  int ret;

  if (driver == NULL || data == NULL || len == 0)
    {
      return -EINVAL;
    }

  ret = esp_hosted_ng_bt_packet_from_type(type, &pkt_type);
  if (ret < 0)
    {
      return ret;
    }

  priv = esp_hosted_ng_from_driver(driver);

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  adapter = priv->adapter;
  if (adapter == NULL || adapter->if_ops == NULL ||
      adapter->if_ops->write == NULL)
    {
      priv->err_tx++;
      nxmutex_unlock(&priv->lock);
      return -ENODEV;
    }

  offset = sizeof(struct esp_hosted_ng_payload_header_s);
  total_len = len + sizeof(struct esp_hosted_ng_payload_header_s);

  /* Match upstream ESP-Hosted SPI HCI framing: payload is aligned and the
   * H4 packet type is stored in the byte immediately before payload
   * (offset - 1).  The ESP firmware passes payload to process_hci_rx_pkt(),
   * which does payload-- to recover that H4 byte before handing the packet
   * to VHCI.
   */

  offset += SKB_DATA_ADDR_ALIGNMENT -
            (total_len % SKB_DATA_ADDR_ALIGNMENT);

  if (len + offset > ESP_HOSTED_NG_SPI_BUF_SIZE)
    {
      priv->err_tx++;
      nxmutex_unlock(&priv->lock);
      return -EMSGSIZE;
    }

  skb = esp_if_alloc_skb(adapter, len + offset);
  if (skb == NULL)
    {
      priv->err_tx++;
      nxmutex_unlock(&priv->lock);
      return -ENOMEM;
    }

  memset(skb_put(skb, len + offset), 0, len + offset);
  header = (FAR struct esp_hosted_ng_payload_header_s *)skb->data;
  header->if_type = ESP_HOSTED_NG_HCI_IF;
  header->if_num = 0;
  header->packet_type = ESP_HOSTED_NG_PACKET_TYPE_DATA;
  header->len = len;
  header->offset = offset;
  skb->data[offset - 1] = pkt_type;
  memcpy(skb->data + offset, data, len);

  if (adapter->capabilities & ESP_CHECKSUM_ENABLED)
    {
      header->checksum = compute_checksum(skb->data, len + offset);
    }

  esp_hosted_ng_bt_trace(priv, "TX", pkt_type, data, len);

  ret = adapter->if_ops->write(adapter, skb);
  if (ret < 0)
    {
      priv->err_tx++;
      nxmutex_unlock(&priv->lock);
      return ret;
    }

  esp_hosted_ng_bt_count_tx(priv, pkt_type, len);
  nxmutex_unlock(&priv->lock);

  esp_hosted_ng_bt_drain_rx(adapter);
  return OK;
}

static int esp_hosted_ng_bt_ioctl(FAR struct bt_driver_s *driver, int cmd,
                                  unsigned long arg)
{
  (void)driver;
  (void)cmd;
  (void)arg;
  return -ENOTTY;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int esp_init_bt(FAR struct esp_adapter *adapter)
{
  FAR struct esp_hosted_ng_btdev_s *priv = &g_esp_btdev;
  int ret;

  if (adapter == NULL)
    {
      return -EINVAL;
    }

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  if (priv->registered)
    {
      priv->adapter = adapter;
      nxmutex_unlock(&priv->lock);
      return OK;
    }

  memset(&priv->driver, 0, sizeof(priv->driver));
  priv->driver.head_reserve = sizeof(struct esp_hosted_ng_payload_header_s);
  priv->driver.open = esp_hosted_ng_bt_open;
  priv->driver.send = esp_hosted_ng_bt_send;
  priv->driver.close = esp_hosted_ng_bt_close;
  priv->driver.ioctl = esp_hosted_ng_bt_ioctl;
  priv->driver.priv = priv;
  priv->adapter = adapter;
  priv->registered = true;
  nxmutex_unlock(&priv->lock);

  ret = bt_driver_register(&priv->driver);
  if (ret < 0)
    {
      nxmutex_lock(&priv->lock);
      priv->adapter = NULL;
      priv->registered = false;
      nxmutex_unlock(&priv->lock);
      syslog(LOG_ERR, "esp_hosted_ng: BT HCI register failed: %d\n", ret);
      return ret;
    }

  syslog(LOG_INFO, "esp_hosted_ng: BT/BLE HCI registered over SPI as bnep0\n");
  return OK;
}

int esp_deinit_bt(FAR struct esp_adapter *adapter)
{
  FAR struct esp_hosted_ng_btdev_s *priv = &g_esp_btdev;

  (void)adapter;

  nxmutex_lock(&priv->lock);
  priv->opened = false;
  priv->adapter = NULL;
  nxmutex_unlock(&priv->lock);
  return OK;
}

int esp_hosted_ng_bt_rx(FAR struct esp_adapter *adapter, uint8_t pkt_type,
                        FAR uint8_t *data, size_t len)
{
  FAR struct esp_hosted_ng_btdev_s *priv = &g_esp_btdev;
  enum bt_buf_type_e type;
  int ret;

  (void)adapter;

  if (data == NULL || len == 0)
    {
      return -EINVAL;
    }

  ret = esp_hosted_ng_bt_type_from_packet(pkt_type, &type);
  if (ret < 0)
    {
      priv->err_rx++;
      return ret;
    }

  if (!priv->registered || priv->driver.receive == NULL)
    {
      priv->err_rx++;
      return -ENODEV;
    }

  esp_hosted_ng_bt_trace(priv, "RX", pkt_type, data, len);

  ret = bt_netdev_receive(&priv->driver, type, data, len);
  if (ret < 0)
    {
      priv->err_rx++;
      return ret;
    }

  esp_hosted_ng_bt_count_rx(priv, pkt_type, len);
  return OK;
}

void esp_hci_update_tx_counter(FAR struct hci_dev *hdev, uint8_t pkt_type,
                               size_t len)
{
  (void)hdev;
  esp_hosted_ng_bt_count_tx(&g_esp_btdev, pkt_type, len);
}

void esp_hci_update_rx_counter(FAR struct hci_dev *hdev, uint8_t pkt_type,
                               size_t len)
{
  (void)hdev;
  esp_hosted_ng_bt_count_rx(&g_esp_btdev, pkt_type, len);
}
