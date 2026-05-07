/****************************************************************************
 * arch/arm/src/stm32u5/stm32_sdmmc.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_STM32_SDMMC_H
#define __ARCH_ARM_SRC_STM32U5_STM32_SDMMC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <stdbool.h>

#include "chip.h"
#include "hardware/stm32_sdmmc.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: stm32_sdmmc1_enable
 *
 * Description:
 *   Select and enable the SDMMC1 kernel and bus clocks.
 *
 ****************************************************************************/

#ifdef CONFIG_STM32U5_SDMMC1
int stm32_sdmmc1_enable(void);
#endif

/****************************************************************************
 * Name: sdio_initialize
 *
 * Description:
 *   Initialize SDIO for operation.
 *
 ****************************************************************************/

struct sdio_dev_s;
struct sdio_dev_s *sdio_initialize(int slotno);

/****************************************************************************
 * Name: sdio_mediachange
 *
 * Description:
 *   Report card insert/remove status to the SDIO lower half.
 *
 ****************************************************************************/

void sdio_mediachange(struct sdio_dev_s *dev, bool cardinslot);

/****************************************************************************
 * Name: sdio_wrprotect
 *
 * Description:
 *   Report write-protect status to the SDIO lower half.
 *
 ****************************************************************************/

void sdio_wrprotect(struct sdio_dev_s *dev, bool wrprotect);

#if defined(CONFIG_SDMMC1_SDIO_MODE) || defined(CONFIG_SDMMC2_SDIO_MODE)
void sdio_set_sdio_card_isr(struct sdio_dev_s *dev,
                            int (*func)(void *), void *arg);
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __ARCH_ARM_SRC_STM32U5_STM32_SDMMC_H */
