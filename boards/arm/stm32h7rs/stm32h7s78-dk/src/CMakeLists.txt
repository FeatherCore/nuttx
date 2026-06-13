# ##############################################################################
# boards/arm/stm32h7rs/stm32h7s78-dk/src/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# ##############################################################################

set(SRCS stm32_bringup.c stm32_autoleds.c stm32_heap.c)

if(CONFIG_STM32H7RS_XSPI)
  list(APPEND SRCS stm32_extmem.c stm32_xspi.c)
endif()

if(CONFIG_STM32H7RS_SPI4)
  list(APPEND SRCS stm32_spi.c)
endif()

if(CONFIG_BOARDCTL_BOOT_IMAGE)
  list(APPEND SRCS stm32_boot_image.c)
endif()

if(CONFIG_STM32H7S78_DK_LCD)
  list(APPEND SRCS stm32_lcd.c)
endif()

if(CONFIG_STM32H7S78_DK_GT911)
  list(APPEND SRCS stm32_touchscreen.c)
endif()

if(CONFIG_STM32H7S78_DK_AW8697)
  list(APPEND SRCS stm32_aw8697.c)
endif()

if(CONFIG_STM32H7S78_DK_BME280)
  list(APPEND SRCS stm32_bme280.c)
endif()

if(CONFIG_STM32H7S78_DK_MAX30102)
  list(APPEND SRCS stm32_max30102.c)
endif()

if(CONFIG_STM32H7S78_DK_LSM6DSOX)
  list(APPEND SRCS stm32_lsm6dsox.c)
endif()

target_sources(board PRIVATE ${SRCS})

if(CONFIG_ETC_ROMFS)
  nuttx_add_romfs(
    NAME
    etc
    MOUNTPOINT
    etc
    RCRAWS
    etc/init.d/rc.sysinit
    etc/init.d/rcS
    etc/wpa_supplicant.conf
    PATH
    ${CMAKE_CURRENT_BINARY_DIR}/etc)

  target_link_libraries(board PRIVATE romfs_etc)
endif()

if(CONFIG_BUILD_PROTECTED)
  set_property(
    GLOBAL PROPERTY LD_SCRIPT "${NUTTX_BOARD_DIR}/scripts/memory.ld"
                    "${NUTTX_BOARD_DIR}/scripts/kernel-space.ld")
  set_property(
    GLOBAL PROPERTY LD_SCRIPT_USER "${NUTTX_BOARD_DIR}/scripts/memory.ld"
                    "${NUTTX_BOARD_DIR}/scripts/user-space.ld")
elseif(CONFIG_STM32H7RS_APP_FORMAT_NXBOOT)
  if(CONFIG_NXBOOT_BOOTLOADER)
    set_property(GLOBAL PROPERTY LD_SCRIPT
                 "${NUTTX_BOARD_DIR}/scripts/flash-nxboot-loader.ld")
  else()
    set_property(GLOBAL PROPERTY LD_SCRIPT
                 "${NUTTX_BOARD_DIR}/scripts/flash-nxboot-app.ld")
  endif()
else()
  set_property(GLOBAL PROPERTY LD_SCRIPT "${NUTTX_BOARD_DIR}/scripts/flash.ld")
endif()
