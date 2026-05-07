##############################################################################
# boards/arm/stm32u5/stm32u5x9j-dk/src/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
##############################################################################

-include $(TOPDIR)/Make.defs

ASRCS =
CSRCS = stm32_boot.c stm32_bringup.c stm32_userleds.c stm32_extmem.c

ifeq ($(CONFIG_ARCH_BUTTONS),y)
CSRCS += stm32_buttons.c
endif

ifeq ($(CONFIG_STM32U5X9J_DK_I2C_PROBES),y)
CSRCS += stm32_i2c_probes.c
endif

ifeq ($(CONFIG_STM32U5X9J_DK_TOUCH),y)
CSRCS += stm32_touchscreen.c
endif

ifeq ($(CONFIG_STM32U5X9J_DK_LCD),y)
CSRCS += stm32_lcd.c
endif

ifeq ($(CONFIG_STM32U5X9J_DK_EMMC),y)
CSRCS += stm32_emmc.c
endif

ifeq ($(CONFIG_ARCH_BOARD_STM32U5_CUSTOM_CLOCKCONFIG),y)
CSRCS += stm32_clockconfig.c
endif

ifeq ($(CONFIG_USBDEV),y)
CSRCS += stm32_usb.c
endif

include $(TOPDIR)/boards/Board.mk
