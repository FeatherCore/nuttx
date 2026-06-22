/****************************************************************************
 * drivers/wireless/esp_hosted_ng/esp_hosted_ng_linux_if.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <nuttx/clock.h>
#include <nuttx/kthread.h>
#include <nuttx/mutex.h>

#include <linux/skbuff.h>

#include "adapter.h"
#include "esp.h"
#include "esp_api.h"
#include "esp_cfg80211.h"
#include "esp_if.h"

#include "esp_hosted_ng_transport.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct esp_hosted_ng_transport_s g_esp_transport;
static struct sk_buff_head g_esp_rxq;
static mutex_t g_esp_if_lock = NXMUTEX_INITIALIZER;
static uint8_t g_esp_txbuf[ESP_HOSTED_NG_SPI_BUF_SIZE];
static uint8_t g_esp_rxbuf[ESP_HOSTED_NG_SPI_BUF_SIZE];
static FAR struct esp_adapter *g_esp_rx_adapter;
static pid_t g_esp_rx_pid = -1;
static volatile bool g_esp_rx_stop;
static unsigned int g_esp_trace_rx;
static unsigned int g_esp_trace_invalid;
volatile u8 host_sleep;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static unsigned long esp_linux_if_msec(void)
{
  return (unsigned long)TICK2MSEC(clock_systime_ticks());
}

static int esp_linux_if_exchange(FAR struct esp_adapter *adapter,
                                 FAR const uint8_t *txbuf, size_t txlen);
static int esp_linux_if_rx_pump(int argc, FAR char *argv[]);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void esp_hosted_ng_board_spi_ready(void)
{
  FAR struct esp_adapter *adapter = esp_get_adapter();

  if (adapter != NULL && adapter->if_rx_workqueue != NULL)
    {
      esp_process_new_packet_intr(adapter);
    }
}

int esp_hosted_ng_poll_rx(FAR struct esp_adapter *adapter, int attempts,
                          unsigned int delay_us)
{
  int i;
  int ret = OK;

  if (adapter == NULL)
    {
      return -EINVAL;
    }

  if (attempts <= 0)
    {
      attempts = 1;
    }

  for (i = 0; i < attempts; i++)
    {
      int cur;

      cur = esp_linux_if_exchange(adapter, NULL, 0);
      if (cur < 0 && ret == OK)
        {
          ret = cur;
        }

      esp_process_pending_rx(adapter);

      if (delay_us > 0)
        {
          usleep(delay_us);
        }
    }

  return ret;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool esp_linux_if_valid_rx(FAR const uint8_t *buf, size_t buflen,
                                  FAR size_t *pktlen)
{
  FAR const struct esp_payload_header *header;
  uint16_t len;
  uint16_t offset;

  if (buf == NULL || buflen < sizeof(*header))
    {
      return false;
    }

  header = (FAR const struct esp_payload_header *)buf;
  if (header->if_type >= ESP_MAX_IF)
    {
      if (g_esp_trace_invalid++ < 16)
        {
          syslog(LOG_INFO,
                 "esp_hosted_ng: invalid SPI RX if=%u pkt=%u len=%u "
                 "off=%u raw=%02x %02x %02x %02x %02x %02x\n",
                 header->if_type, header->packet_type,
                 le16_to_cpu(header->len), le16_to_cpu(header->offset),
                 buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
        }

      return false;
    }

  len = le16_to_cpu(header->len);
  offset = le16_to_cpu(header->offset);
  if (len == 0 || !ESP_OFFSET_VALID(offset) || len + offset > buflen)
    {
      if (g_esp_trace_invalid++ < 16)
        {
          syslog(LOG_INFO,
                 "esp_hosted_ng: invalid SPI RX if=%u pkt=%u len=%u "
                 "off=%u raw=%02x %02x %02x %02x %02x %02x\n",
                 header->if_type, header->packet_type, len, offset,
                 buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
        }

      return false;
    }

  *pktlen = len + offset;
  return true;
}

static FAR struct sk_buff *esp_linux_if_alloc_skb(u32 len)
{
  FAR struct sk_buff *skb;
  u32 alloc_len;
  uint8_t offset;

  alloc_len = max(len, (u32)ESP_HOSTED_NG_SPI_BUF_SIZE) +
              INTERFACE_HEADER_PADDING;
  skb = netdev_alloc_skb(NULL, alloc_len);
  if (skb != NULL)
    {
      offset = ((uintptr_t)skb->data) & (SKB_DATA_ADDR_ALIGNMENT - 1);
      if (offset != 0)
        {
          skb_reserve(skb, INTERFACE_HEADER_PADDING - offset);
        }
    }

  return skb;
}

static int esp_linux_if_queue_rx(FAR struct esp_adapter *adapter,
                                 FAR const uint8_t *rxbuf, size_t buflen)
{
  FAR struct sk_buff *rxskb;
  size_t pktlen;

  (void)adapter;

  if (!esp_linux_if_valid_rx(rxbuf, buflen, &pktlen))
    {
      return -EAGAIN;
    }

  rxskb = esp_linux_if_alloc_skb(pktlen);
  if (rxskb == NULL)
    {
      return -ENOMEM;
    }

  if (g_esp_trace_rx++ < 24)
    {
      FAR const struct esp_payload_header *header =
        (FAR const struct esp_payload_header *)rxbuf;
      uint16_t len = le16_to_cpu(header->len);
      uint16_t offset = le16_to_cpu(header->offset);
      uint8_t hci = offset > 0 ? rxbuf[offset - 1] : 0;
      uint8_t p0 = len > 0 ? rxbuf[offset] : 0;
      uint8_t p1 = len > 1 ? rxbuf[offset + 1] : 0;
      uint8_t p2 = len > 2 ? rxbuf[offset + 2] : 0;
      uint8_t p3 = len > 3 ? rxbuf[offset + 3] : 0;

      syslog(LOG_INFO,
             "esp_hosted_ng: SPI RX if=%u pkt=%u len=%u off=%u hci=0x%02x "
             "payload=%02x %02x %02x %02x\n",
             header->if_type, header->packet_type, len, offset, hci,
             p0, p1, p2, p3);
    }

  memcpy(skb_put(rxskb, pktlen), rxbuf, pktlen);
  skb_queue_tail(&g_esp_rxq, rxskb);
  return OK;
}

static int esp_linux_if_exchange(FAR struct esp_adapter *adapter,
                                 FAR const uint8_t *txbuf, size_t txlen)
{
  unsigned long start;
  unsigned long locked;
  unsigned long transport_done;
  unsigned long enqueue_done;
  unsigned long unlocked;
  unsigned long done;
  bool wake_rx = false;
  int ret;
  int rxret = -EAGAIN;

  start = esp_linux_if_msec();

  ret = nxmutex_lock(&g_esp_if_lock);
  if (ret < 0)
    {
      return ret;
    }

  locked = esp_linux_if_msec();

  memset(g_esp_txbuf, 0, sizeof(g_esp_txbuf));
  memset(g_esp_rxbuf, 0, sizeof(g_esp_rxbuf));
  if (txbuf != NULL && txlen > 0)
    {
      if (txlen > sizeof(g_esp_txbuf))
        {
          nxmutex_unlock(&g_esp_if_lock);
          return -EMSGSIZE;
        }

      memcpy(g_esp_txbuf, txbuf, txlen);
    }

  ret = esp_hosted_ng_transport_exchange(&g_esp_transport, g_esp_txbuf,
                                         g_esp_rxbuf, sizeof(g_esp_rxbuf));
  transport_done = esp_linux_if_msec();
  if (ret == OK)
    {
      rxret = esp_linux_if_queue_rx(adapter, g_esp_rxbuf, sizeof(g_esp_rxbuf));
      wake_rx = (rxret == OK);
    }

  enqueue_done = esp_linux_if_msec();
  nxmutex_unlock(&g_esp_if_lock);
  unlocked = esp_linux_if_msec();
  if (wake_rx)
    {
      esp_process_new_packet_intr(adapter);
    }

  done = esp_linux_if_msec();
  if (done - start >= 100)
    {
      syslog(LOG_WARNING,
             "esp_hosted_ng: if exchange slow txlen=%zu "
             "lock_ms=%lu transport_ms=%lu enqueue_ms=%lu unlock_ms=%lu "
             "wake_ms=%lu total_ms=%lu ret=%d rxret=%d hs=%d dr=%d\n",
             txlen, locked - start, transport_done - locked,
             enqueue_done - transport_done, unlocked - enqueue_done,
             done - unlocked, done - start, ret, rxret,
             esp_hosted_ng_board_handshake_ready(),
             esp_hosted_ng_board_data_ready());
    }

  return ret;
}

static FAR struct sk_buff *esp_linux_if_read(FAR struct esp_adapter *adapter)
{
  FAR struct sk_buff *skb;
  bool poll_spi;

  skb = skb_dequeue(&g_esp_rxq);
  if (skb != NULL)
    {
      return skb;
    }

  poll_spi = esp_hosted_ng_board_data_ready();

  if (adapter != NULL)
    {
      /* ESP-Hosted raises HS when the slave side can take part in the next
       * SPI transaction.  Async events may be pending even when DR is not
       * sampled high, so RX work triggered by HS must also pull one frame.
       */

      poll_spi = poll_spi || esp_hosted_ng_board_handshake_ready();
    }

  if (poll_spi)
    {
      esp_linux_if_exchange(adapter, NULL, 0);
      skb = skb_dequeue(&g_esp_rxq);
    }

  return skb;
}

static int esp_linux_if_write(FAR struct esp_adapter *adapter,
                              FAR struct sk_buff *skb)
{
  int ret;

  if (adapter == NULL || skb == NULL || skb->data == NULL || skb->len == 0)
    {
      dev_kfree_skb(skb);
      return -EINVAL;
    }

  ret = esp_linux_if_exchange(adapter, skb->data, skb->len);
  dev_kfree_skb(skb);
  return ret;
}

static int esp_linux_if_rx_pump(int argc, FAR char *argv[])
{
  (void)argc;
  (void)argv;

  while (!g_esp_rx_stop)
    {
      FAR struct esp_adapter *adapter = g_esp_rx_adapter;

      if (adapter != NULL)
        {
          esp_hosted_ng_poll_rx(adapter, 1, 0);
        }

      usleep(5000);
    }

  return OK;
}

static int esp_linux_if_start_rx_pump(FAR struct esp_adapter *adapter)
{
  if (g_esp_rx_pid > 0)
    {
      return OK;
    }

  g_esp_rx_adapter = adapter;
  g_esp_rx_stop = false;
  g_esp_rx_pid = kthread_create("esp_rx", 120, 2048,
                                esp_linux_if_rx_pump, NULL);
  if (g_esp_rx_pid < 0)
    {
      int ret = g_esp_rx_pid;

      g_esp_rx_pid = -1;
      g_esp_rx_adapter = NULL;
      g_esp_rx_stop = true;
      syslog(LOG_ERR, "esp_hosted_ng: RX pump start failed: %d\n", ret);
      return ret;
    }

  syslog(LOG_INFO, "esp_hosted_ng: SPI RX pump started pid=%d\n",
         g_esp_rx_pid);
  return OK;
}

static void esp_linux_if_stop_rx_pump(void)
{
  if (g_esp_rx_pid > 0)
    {
      pid_t pid = g_esp_rx_pid;

      g_esp_rx_stop = true;
      g_esp_rx_pid = -1;
      kthread_delete(pid);
    }

  g_esp_rx_adapter = NULL;
}

static int esp_linux_if_init(FAR struct esp_adapter *adapter)
{
  int ret;

  if (adapter == NULL)
    {
      return -EINVAL;
    }

  skb_queue_head_init(&g_esp_rxq);

  ret = esp_hosted_ng_transport_init(&g_esp_transport);
  if (ret < 0)
    {
      return ret;
    }

  adapter->if_context = &g_esp_transport;
  atomic_set(&adapter->state, ESP_CONTEXT_READY);
  ret = esp_linux_if_start_rx_pump(adapter);
  if (ret < 0)
    {
      adapter->if_context = NULL;
      atomic_set(&adapter->state, ESP_CONTEXT_DISABLED);
      esp_hosted_ng_transport_deinit(&g_esp_transport);
      return ret;
    }

  return OK;
}

static int esp_linux_if_deinit(FAR struct esp_adapter *adapter)
{
  esp_linux_if_stop_rx_pump();

  if (adapter != NULL)
    {
      adapter->if_context = NULL;
      atomic_set(&adapter->state, ESP_CONTEXT_DISABLED);
    }

  skb_queue_purge(&g_esp_rxq);
  esp_hosted_ng_transport_deinit(&g_esp_transport);
  return OK;
}

static struct esp_if_ops g_esp_linux_if_ops =
{
  .init      = esp_linux_if_init,
  .read      = esp_linux_if_read,
  .write     = esp_linux_if_write,
  .alloc_skb = esp_linux_if_alloc_skb,
  .deinit    = esp_linux_if_deinit,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int esp_init_interface_layer(FAR struct esp_adapter *adapter, u32 speed)
{
  (void)speed;

  if (adapter == NULL)
    {
      return -EINVAL;
    }

  adapter->if_type = ESP_IF_TYPE_SPI;
  adapter->if_ops = &g_esp_linux_if_ops;
  return adapter->if_ops->init(adapter);
}

int esp_validate_chipset(FAR struct esp_adapter *adapter, u8 chipset)
{
  if (adapter == NULL)
    {
      return -EINVAL;
    }

  if (!esp_is_valid_hardware_id(chipset))
    {
      adapter->chipset = ESP_FIRMWARE_CHIP_UNRECOGNIZED;
      syslog(LOG_ERR, "esp_hosted_ng: unsupported chipset id=%02x\n",
             chipset);
      return -ENODEV;
    }

  adapter->chipset = chipset;
  syslog(LOG_INFO, "esp_hosted_ng: chipset=%s id=%02x over SPI\n",
         esp_get_hardware_name(chipset), chipset);
  return OK;
}

int esp_adjust_spi_clock(FAR struct esp_adapter *adapter, u8 spi_clk_mhz)
{
  uint32_t frequency;

  (void)adapter;

  if (spi_clk_mhz == 0)
    {
      return OK;
    }

  frequency = (uint32_t)spi_clk_mhz * 1000000u;
  g_esp_transport.frequency = frequency;
  syslog(LOG_INFO, "esp_hosted_ng: SPI clock adjusted to %" PRIu32 " Hz\n",
         frequency);
  return OK;
}

int generate_slave_intr(FAR void *context, u8 data)
{
  FAR struct esp_hosted_ng_transport_s *transport = context;

  (void)data;

  if (transport == NULL)
    {
      transport = &g_esp_transport;
    }

  return esp_hosted_ng_transport_ready(transport) ? OK : -ENODEV;
}

int esp_deinit_module(FAR struct esp_adapter *adapter)
{
  uint8_t iface_idx;

  if (adapter == NULL)
    {
      return -EINVAL;
    }

  for (iface_idx = 0; iface_idx < ESP_MAX_INTERFACE; iface_idx++)
    {
      if (adapter->priv[iface_idx] != NULL)
        {
          esp_mark_scan_done_and_disconnect(adapter->priv[iface_idx], true);
        }
    }

  esp_remove_card(adapter);
  return OK;
}

void esp_deinit_interface_layer(void)
{
  FAR struct esp_adapter *adapter = esp_get_adapter();

  if (adapter != NULL && adapter->if_ops != NULL &&
      adapter->if_ops->deinit != NULL)
    {
      adapter->if_ops->deinit(adapter);
    }
}

void test_raw_tp_cleanup(void)
{
}

int debugfs_init(void)
{
  return OK;
}

void debugfs_exit(void)
{
}
