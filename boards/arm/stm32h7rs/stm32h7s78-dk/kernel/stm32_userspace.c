/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/kernel/stm32_userspace.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdlib.h>

#include <nuttx/arch.h>
#include <nuttx/mm/mm.h>
#include <nuttx/userspace.h>
#include <nuttx/wqueue.h>

#if defined(CONFIG_BUILD_PROTECTED) && !defined(__KERNEL__)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_NUTTX_USERSPACE
#  error "CONFIG_NUTTX_USERSPACE not defined"
#endif

#if CONFIG_NUTTX_USERSPACE != 0x70080400
#  error "CONFIG_NUTTX_USERSPACE must be 0x70080400 to match memory.ld"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct userspace_data_s g_userspace_data =
{
  .us_heap = &g_mmheap,
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern uint8_t _stext[];
extern uint8_t _etext[];
extern const uint8_t _eronly[];
extern uint8_t _sdata[];
extern uint8_t _edata[];
extern uint8_t _sbss[];
extern uint8_t _ebss[];

const struct userspace_s userspace locate_data(".userspace") =
{
  .us_entrypoint    = CONFIG_INIT_ENTRYPOINT,
  .us_textstart     = (uintptr_t)_stext,
  .us_textend       = (uintptr_t)_etext,
  .us_datasource    = (uintptr_t)_eronly,
  .us_datastart     = (uintptr_t)_sdata,
  .us_dataend       = (uintptr_t)_edata,
  .us_bssstart      = (uintptr_t)_sbss,
  .us_bssend        = (uintptr_t)_ebss,
  .us_data          = &g_userspace_data,
  .task_startup     = nxtask_startup,
  .signal_handler   = up_signal_handler,

#ifdef CONFIG_LIBC_USRWORK
  .work_usrstart    = work_usrstart,
#endif
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#endif /* CONFIG_BUILD_PROTECTED && !__KERNEL__ */
