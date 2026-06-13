/****************************************************************************
 * drivers/wireless/esp_hosted_ng/esp_hosted_ng_proto.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __DRIVERS_WIRELESS_ESP_HOSTED_NG_PROTO_H
#define __DRIVERS_WIRELESS_ESP_HOSTED_NG_PROTO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/compiler.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ESP_HOSTED_NG_SPI_BUF_SIZE 1600
#define ESP_HOSTED_NG_MAX_SSID_LEN 32
#define ESP_HOSTED_NG_MAX_PASSPHRASE_LEN 64
#define ESP_HOSTED_NG_MAX_ALIGN_PADDING 4

#define ESP_HOSTED_NG_OFFSET_VALID(o) \
  ((o) >= sizeof(struct esp_hosted_ng_payload_header_s) && \
   (o) <= sizeof(struct esp_hosted_ng_payload_header_s) + \
          ESP_HOSTED_NG_MAX_ALIGN_PADDING)

/****************************************************************************
 * Public Types
 ****************************************************************************/

begin_packed_struct struct esp_hosted_ng_payload_header_s
{
  uint8_t if_type:4;
  uint8_t if_num:4;
  uint8_t flags;
  uint8_t packet_type;
  uint8_t reserved1;
  uint16_t len;
  uint16_t offset;
  uint16_t checksum;
  uint8_t reserved2;
  union
  {
    uint8_t reserved3;
    uint8_t hci_pkt_type;
    uint8_t priv_pkt_type;
  };
} end_packed_struct;

enum esp_hosted_ng_interface_type_e
{
  ESP_HOSTED_NG_STA_IF,
  ESP_HOSTED_NG_AP_IF,
  ESP_HOSTED_NG_HCI_IF,
  ESP_HOSTED_NG_INTERNAL_IF,
  ESP_HOSTED_NG_TEST_IF,
  ESP_HOSTED_NG_MAX_IF,
};

enum esp_hosted_ng_packet_type_e
{
  ESP_HOSTED_NG_PACKET_TYPE_DATA,
  ESP_HOSTED_NG_PACKET_TYPE_COMMAND_REQUEST,
  ESP_HOSTED_NG_PACKET_TYPE_COMMAND_RESPONSE,
  ESP_HOSTED_NG_PACKET_TYPE_EVENT,
  ESP_HOSTED_NG_PACKET_TYPE_EAPOL,
};

enum esp_hosted_ng_command_code_e
{
  ESP_HOSTED_NG_CMD_INIT_INTERFACE = 1,
  ESP_HOSTED_NG_CMD_SET_MAC = 2,
  ESP_HOSTED_NG_CMD_GET_MAC = 3,
  ESP_HOSTED_NG_CMD_SCAN_REQUEST = 4,
  ESP_HOSTED_NG_CMD_STA_CONNECT = 5,
  ESP_HOSTED_NG_CMD_DISCONNECT = 6,
  ESP_HOSTED_NG_CMD_DEINIT_INTERFACE = 7,
  ESP_HOSTED_NG_CMD_SET_MODE = 22,
};

enum esp_hosted_ng_firmware_chip_e
{
  ESP_HOSTED_NG_FIRMWARE_CHIP_UNRECOGNIZED = 0xff,
  ESP_HOSTED_NG_FIRMWARE_CHIP_ESP32C5 = 0x0e,
};

begin_packed_struct struct esp_hosted_ng_command_header_s
{
  uint8_t cmd_code;
  uint8_t cmd_status;
  uint16_t len;
  uint16_t seq_num;
  uint8_t reserved1;
  uint8_t reserved2;
} end_packed_struct;

begin_packed_struct struct esp_hosted_ng_sta_connect_cmd_s
{
  struct esp_hosted_ng_command_header_s header;
  uint8_t bssid[6];
  uint16_t assoc_flags;
  char ssid[ESP_HOSTED_NG_MAX_SSID_LEN + 1];
  uint8_t channel;
  uint8_t is_auth_open;
  uint8_t assoc_ie_len;
  uint8_t passphrase_len;
  char passphrase[ESP_HOSTED_NG_MAX_PASSPHRASE_LEN];
} end_packed_struct;

#endif /* __DRIVERS_WIRELESS_ESP_HOSTED_NG_PROTO_H */
