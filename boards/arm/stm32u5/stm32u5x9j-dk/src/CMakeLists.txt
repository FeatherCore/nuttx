# ##############################################################################
# boards/arm/stm32u5/stm32u5x9j-dk/src/CMakeLists.txt
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

set(SRCS stm32_boot.c stm32_bringup.c stm32_userleds.c stm32_extmem.c
         stm32u5x9j_xspi.c)

if(CONFIG_BUILD_PROTECTED)
  list(APPEND SRCS stm32u5x9j_heap.c)
endif()

if(CONFIG_ARCH_BUTTONS)
  list(APPEND SRCS stm32_buttons.c)
endif()

if(CONFIG_STM32U5X9J_DK_I2C_PROBES)
  list(APPEND SRCS stm32_i2c_probes.c)
endif()

if(CONFIG_STM32U5X9J_DK_TOUCH)
  list(APPEND SRCS stm32_touchscreen.c)
endif()

if(CONFIG_STM32U5X9J_DK_LCD)
  list(APPEND SRCS stm32_lcd.c)
endif()

if(CONFIG_STM32U5X9J_DK_EMMC)
  list(APPEND SRCS stm32_emmc.c)
endif()

if(CONFIG_ARCH_BOARD_STM32U5_CUSTOM_CLOCKCONFIG)
  list(APPEND SRCS stm32_clockconfig.c)
endif()

target_sources(board PRIVATE ${SRCS})

if(CONFIG_BUILD_PROTECTED)
  set_property(
    GLOBAL PROPERTY LD_SCRIPT "${NUTTX_BOARD_DIR}/scripts/memory.ld"
                    "${NUTTX_BOARD_DIR}/scripts/kernel-space.ld")
  set_property(
    GLOBAL PROPERTY LD_SCRIPT_USER "${NUTTX_BOARD_DIR}/scripts/memory.ld"
                    "${NUTTX_BOARD_DIR}/scripts/user-space.ld")
else()
  set_property(GLOBAL PROPERTY LD_SCRIPT "${NUTTX_BOARD_DIR}/scripts/flash.ld")
endif()
