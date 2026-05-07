/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32n6_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <debug.h>

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_STM32N6_XSPI
int stm32n6_extmem_initialize(void);
#endif

#ifdef CONFIG_STM32N6_PSRAM_DIAG
int stm32n6_psram_diag_register(void);
#endif

#ifdef CONFIG_STM32N6_EXTNOR_DIAG
int stm32n6_nor_diag_register(void);
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BOARD_LATE_INITIALIZE
void board_late_initialize(void)
{
  int ret;

  syslog(LOG_INFO, "STM32N6570-DK NXboot-FSBL bring-up\n");

#ifdef CONFIG_STM32N6_XSPI
  ret = stm32n6_extmem_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32n6_extmem_initialize failed: %d\n", ret);
    }
  else
    {
#ifdef CONFIG_STM32N6_PSRAM_DIAG
      ret = stm32n6_psram_diag_register();
      if (ret < 0)
        {
          syslog(LOG_ERR, "stm32n6_psram_diag_register failed: %d\n",
                 ret);
        }
#endif
#ifdef CONFIG_STM32N6_EXTNOR_DIAG
      ret = stm32n6_nor_diag_register();
      if (ret < 0)
        {
          syslog(LOG_ERR, "stm32n6_nor_diag_register failed: %d\n", ret);
        }
#endif
    }
#else
  UNUSED(ret);
#endif
}
#endif
