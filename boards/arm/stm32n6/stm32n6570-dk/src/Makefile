############################################################################
# boards/arm/stm32n6/stm32n6570-dk/src/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

include $(TOPDIR)/Make.defs

CSRCS = stm32n6_bringup.c stm32n6_autoleds.c stm32n6570_heap.c

ifeq ($(CONFIG_STM32N6_XSPI),y)
CSRCS += stm32n6_extmem.c stm32n6570_xspi.c
endif

ifeq ($(CONFIG_BOARDCTL_BOOT_IMAGE),y)
CSRCS += stm32n6_boot_image.c
endif

include $(TOPDIR)/boards/Board.mk
