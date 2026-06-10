# ##############################################################################
# drivers/wireless/esp_hosted_ng/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# ##############################################################################

if(CONFIG_WL_ESP_HOSTED_NG)
  target_sources(drivers PRIVATE esp_hosted_ng_netdev.c esp_hosted_ng_transport.c)
endif()
