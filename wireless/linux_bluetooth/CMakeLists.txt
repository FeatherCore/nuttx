# ##############################################################################
# wireless/linux_bluetooth/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
# ##############################################################################

if(CONFIG_NET_LINUX_BLUETOOTH)
  target_include_directories(
    wireless
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/upstream/compat/include
      ${CMAKE_CURRENT_LIST_DIR}/../linux_compat/include
      ${CMAKE_CURRENT_LIST_DIR}/../ieee80211/include)
  target_sources(wireless PRIVATE linux_bt_core.c)
  target_sources(wireless PRIVATE linux_bt_upstream_hci.c)
  target_sources(wireless PRIVATE linux_bt_upstream_net.c)
endif()

if(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_SOURCES)
  if(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_LIB)
    target_sources(wireless PRIVATE upstream/net_bluetooth/lib.c)
  endif()
  if(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_EIR)
    target_sources(wireless PRIVATE upstream/net_bluetooth/eir.c)
  endif()
  if(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CODEC)
    target_sources(wireless PRIVATE upstream/net_bluetooth/hci_codec.c)
  endif()
  if(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF)
    target_sources(wireless PRIVATE upstream/net_bluetooth/af_bluetooth.c)
  endif()
  if(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CORE)
    target_sources(wireless PRIVATE upstream/net_bluetooth/hci_core.c)
    target_sources(wireless PRIVATE upstream/net_bluetooth/hci_conn.c)
    target_sources(wireless PRIVATE upstream/net_bluetooth/hci_event.c)
    target_sources(wireless PRIVATE upstream/net_bluetooth/hci_sync.c)
  endif()
  if(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_SOCK)
    target_sources(wireless PRIVATE upstream/net_bluetooth/hci_sock.c)
  endif()
  if(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_MGMT)
    target_sources(wireless PRIVATE upstream/net_bluetooth/mgmt.c)
    target_sources(wireless PRIVATE upstream/net_bluetooth/mgmt_config.c)
    target_sources(wireless PRIVATE upstream/net_bluetooth/mgmt_util.c)
  endif()
  if(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_L2CAP)
    target_sources(wireless PRIVATE upstream/net_bluetooth/l2cap_core.c)
    target_sources(wireless PRIVATE upstream/net_bluetooth/l2cap_sock.c)
  endif()
  if(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_SMP)
    target_sources(wireless PRIVATE upstream/net_bluetooth/smp.c)
  endif()
  if(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_ISO)
    target_sources(wireless PRIVATE upstream/net_bluetooth/iso.c)
  endif()
endif()
