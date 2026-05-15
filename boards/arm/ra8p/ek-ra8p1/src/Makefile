############################################################################
# boards/arm/ra8p/ek-ra8p1/src/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

include $(TOPDIR)/Make.defs

CSRCS = ra8p1_autoleds.c ra8p1_bringup.c ra8p1_boot.c ra8p1_heap.c
CSRCS += ra8p1_extmem.c ra8p1_ospi.c ra8p1_pins.c ra8p1_sdram.c

ifeq ($(CONFIG_BOARDCTL_BOOT_IMAGE),y)
CSRCS += ra8p1_boot_image.c
endif

include $(TOPDIR)/boards/Board.mk
