/****************************************************************************
 * drivers/wireless/esp_hosted_ng/esp_hosted_ng_transport.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __DRIVERS_WIRELESS_ESP_HOSTED_NG_TRANSPORT_H
#define __DRIVERS_WIRELESS_ESP_HOSTED_NG_TRANSPORT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <nuttx/spi/spi.h>

#include "esp_hosted_ng_proto.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct esp_hosted_ng_transport_s
{
  FAR struct spi_dev_s *spi;
  uint32_t frequency;
  uint32_t devid;
  uint8_t mode;
  bool initialized;
  bool datapath_open;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR struct spi_dev_s *esp_hosted_ng_spibus_initialize(int bus);
int esp_hosted_ng_board_initialize(void);
void esp_hosted_ng_board_reset(bool reset);
bool esp_hosted_ng_board_handshake_ready(void);
bool esp_hosted_ng_board_data_ready(void);
void esp_hosted_ng_board_spi_ready(void);

int esp_hosted_ng_transport_init(FAR struct esp_hosted_ng_transport_s *priv);
void esp_hosted_ng_transport_deinit(FAR struct esp_hosted_ng_transport_s *priv);
bool esp_hosted_ng_transport_ready(
  FAR const struct esp_hosted_ng_transport_s *priv);
int esp_hosted_ng_transport_send(FAR struct esp_hosted_ng_transport_s *priv,
                                 uint8_t if_type, uint8_t packet_type,
                                 FAR const uint8_t *payload,
                                 size_t payload_len);
int esp_hosted_ng_transport_exchange(FAR struct esp_hosted_ng_transport_s *priv,
                                     FAR const uint8_t *txbuf,
                                     FAR uint8_t *rxbuf, size_t buflen);

#endif /* __DRIVERS_WIRELESS_ESP_HOSTED_NG_TRANSPORT_H */
