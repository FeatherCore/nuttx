############################################################################
# boards/arm/stm32h7rs/stm32h7s78-dk/src/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

include $(TOPDIR)/Make.defs

CSRCS = stm32h7rs_bringup.c stm32h7rs_autoleds.c stm32h7s78_heap.c

ifeq ($(CONFIG_STM32H7RS_XSPI),y)
CSRCS += stm32h7rs_extmem.c stm32h7s78_xspi.c
endif

ifeq ($(CONFIG_STM32H7S78_DK_LCD),y)
CSRCS += stm32h7s78_lcd.c
endif

ifeq ($(CONFIG_STM32H7S78_DK_GT911),y)
CSRCS += stm32h7s78_touch.c
endif

ifeq ($(CONFIG_BOARDCTL_BOOT_IMAGE),y)
CSRCS += stm32h7rs_boot_image.c
endif

include $(TOPDIR)/boards/Board.mk
