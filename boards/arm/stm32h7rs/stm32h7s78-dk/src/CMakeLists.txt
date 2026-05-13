# ##############################################################################
# boards/arm/stm32h7rs/stm32h7s78-dk/src/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# ##############################################################################

set(SRCS stm32h7rs_bringup.c stm32h7rs_autoleds.c stm32h7s78_heap.c)

if(CONFIG_STM32H7RS_XSPI)
  list(APPEND SRCS stm32h7rs_extmem.c stm32h7s78_xspi.c)
endif()

if(CONFIG_BOARDCTL_BOOT_IMAGE)
  list(APPEND SRCS stm32h7rs_boot_image.c)
endif()

target_sources(board PRIVATE ${SRCS})
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
