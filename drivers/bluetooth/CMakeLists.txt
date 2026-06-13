# ##############################################################################
# drivers/bluetooth/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
# ##############################################################################

if(CONFIG_NET_LINUX_BLUETOOTH)
  target_include_directories(
    drivers
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/../../wireless/linux_bluetooth/upstream/compat/include
      ${CMAKE_CURRENT_LIST_DIR}/../../wireless/linux_compat/include
      ${CMAKE_CURRENT_LIST_DIR}/../../wireless/ieee80211/include)
  target_sources(drivers PRIVATE linux_bt_upstream_vhci.c)
endif()

if(CONFIG_DRIVERS_BLUETOOTH_LINUX)
  target_include_directories(
    drivers
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/../../wireless/linux_bluetooth/upstream/compat/include
      ${CMAKE_CURRENT_LIST_DIR}/../../wireless/linux_compat/include
      ${CMAKE_CURRENT_LIST_DIR}/../../wireless/ieee80211/include)

  if(CONFIG_DRIVERS_BLUETOOTH_LINUX_HCI_VHCI)
    target_sources(
      drivers
      PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../../wireless/linux_bluetooth/upstream/drivers_bluetooth/hci_vhci.c)
  endif()
endif()
