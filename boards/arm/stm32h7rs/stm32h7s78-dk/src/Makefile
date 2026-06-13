############################################################################
# boards/arm/stm32h7rs/stm32h7s78-dk/src/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

include $(TOPDIR)/Make.defs

CSRCS = stm32_bringup.c stm32_autoleds.c stm32_heap.c

ifeq ($(CONFIG_STM32H7RS_XSPI),y)
CSRCS += stm32_extmem.c stm32_xspi.c
endif

ifeq ($(CONFIG_STM32H7RS_SPI4),y)
CSRCS += stm32_spi.c
endif

ifeq ($(CONFIG_STM32H7S78_DK_LCD),y)
CSRCS += stm32_lcd.c
endif

ifeq ($(CONFIG_STM32H7S78_DK_GT911),y)
CSRCS += stm32_touchscreen.c
endif

ifeq ($(CONFIG_STM32H7S78_DK_AW8697),y)
CSRCS += stm32_aw8697.c
endif

ifeq ($(CONFIG_STM32H7S78_DK_BME280),y)
CSRCS += stm32_bme280.c
endif

ifeq ($(CONFIG_STM32H7S78_DK_MAX30102),y)
CSRCS += stm32_max30102.c
endif

ifeq ($(CONFIG_STM32H7S78_DK_LSM6DSOX),y)
CSRCS += stm32_lsm6dsox.c
endif

ifeq ($(CONFIG_BOARDCTL_BOOT_IMAGE),y)
CSRCS += stm32_boot_image.c
endif

ifeq ($(CONFIG_ETC_ROMFS),y)
RCRAWS = etc/init.d/rc.sysinit etc/init.d/rcS etc/wpa_supplicant.conf
endif

include $(TOPDIR)/boards/Board.mk
