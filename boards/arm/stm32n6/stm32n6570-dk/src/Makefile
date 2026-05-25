############################################################################
# boards/arm/stm32n6/stm32n6570-dk/src/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

include $(TOPDIR)/Make.defs

CSRCS = stm32_bringup.c stm32_autoleds.c stm32_heap.c

ifeq ($(CONFIG_STM32N6_XSPI),y)
CSRCS += stm32_extmem.c stm32_xspi.c
endif

ifeq ($(CONFIG_BOARDCTL_BOOT_IMAGE),y)
CSRCS += stm32_boot_image.c
endif

ifeq ($(CONFIG_STM32N6570_DK_LCD),y)
CSRCS += stm32_lcd.c
endif

ifeq ($(CONFIG_STM32N6570_DK_GT911),y)
CSRCS += stm32_touchscreen.c
endif

include $(TOPDIR)/boards/Board.mk
