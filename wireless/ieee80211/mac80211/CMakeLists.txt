# ##############################################################################
# wireless/ieee80211/mac80211/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_WIRELESS_IEEE80211_MAC80211_LINUX)
  set(
    SRCS
    main.c
    status.c
    driver-ops.c
    sta_info.c
    wep.c
    aead_api.c
    wpa.c
    scan.c
    offchannel.c
    ht.c
    agg-tx.c
    agg-rx.c
    vht.c
    he.c
    s1g.c
    ibss.c
    iface.c
    link.c
    rate.c
    rc80211_minstrel_ht.c
    michael.c
    tkip.c
    aes_cmac.c
    aes_gmac.c
    fils_aead.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../crypto/rijndael.c
    cfg.c
    ethtool.c
    rx.c
    spectmgmt.c
    tx.c
    key.c
    util.c
    parse.c
    wme.c
    chan.c
    trace.c
    mlme.c
    tdls.c
    ocb.c
    airtime.c
    eht.c
    uhr.c
    wbrf.c
    mesh.c
    mesh_pathtbl.c
    mesh_plink.c
    mesh_hwmp.c
    mesh_sync.c
    mesh_ps.c)

  target_sources(wireless PRIVATE ${SRCS})
  target_compile_definitions(wireless PRIVATE CONFIG_MAC80211=1
                                              CONFIG_MAC80211_MESH=1
                                              CONFIG_MAC80211_RC_MINSTREL=1)
  target_include_directories(
    wireless
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}
            ${CMAKE_CURRENT_LIST_DIR}/../include
            ${CMAKE_CURRENT_LIST_DIR}/../include/uapi)
endif()
