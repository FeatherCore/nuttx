# ##############################################################################
# drivers/wireless/virtual/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# ##############################################################################

if(CONFIG_WL_VIRTUAL)
  set(SRCS)
  if(CONFIG_WL_NUTTX_HWSIM)
    list(APPEND SRCS virtual_hwsim.c)
    list(APPEND SRCS mac80211_hwsim_linux.c)
  endif()

  target_sources(drivers PRIVATE ${SRCS})
endif()
