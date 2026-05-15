# ##############################################################################
# arch/arm/src/ra8p/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# ##############################################################################

set(SRCS
    ra8p_start.c
    ra8p_clockconfig.c
    ra8p_irq.c
    ra8p_allocateheap.c
    ra8p_lowputc.c
    ra8p_serial.c
    ra8p_timerisr.c)

if(CONFIG_BUILD_PROTECTED)
  list(APPEND SRCS ra8p_userspace.c ra8p_mpuinit.c)
endif()

target_sources(arch PRIVATE ${SRCS})
