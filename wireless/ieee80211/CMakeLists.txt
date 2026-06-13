# ##############################################################################
# wireless/ieee80211/CMakeLists.txt
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

if(CONFIG_WIRELESS_IEEE80211_CFG80211_LINUX
   OR CONFIG_WIRELESS_IEEE80211_MAC80211_LINUX)
  target_sources(wireless PRIVATE genetlink_bridge.c ieee80211_linux_init.c
                                  netdevice_compat.c)
  target_include_directories(
    wireless
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include
            ${CMAKE_CURRENT_LIST_DIR}/../linux_compat/include
            ${CMAKE_CURRENT_LIST_DIR}/include/uapi)
  target_compile_options(
    wireless
    PRIVATE -include
            ${CMAKE_CURRENT_LIST_DIR}/include/linux/cfg80211_compat.h)
endif()

add_subdirectory(cfg80211)
add_subdirectory(mac80211)
