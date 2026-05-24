/****************************************************************************
 * arch/arm/src/stm32u5/stm32_dsi.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_STM32_DSI_H
#define __ARCH_ARM_SRC_STM32U5_STM32_DSI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_DSI_DCS_SHORT_WRITE0  0x05
#define STM32_DSI_DCS_SHORT_WRITE1  0x15
#define STM32_DSI_DCS_LONG_WRITE    0x39

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct stm32_dsi_video_config_s
{
  uint16_t width;
  uint16_t hsync;
  uint16_t hbp;
  uint16_t hfp;
  uint16_t vsync;
  uint16_t vbp;
  uint16_t vfp;
  uint16_t vactive;
  uint8_t lanes;
  uint8_t bpp;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_STM32U5_DSIHOST
int stm32_dsiinitialize(const struct stm32_dsi_video_config_s *config);
int stm32_dsistart(void);
int stm32_dsienableltdc(void);
int stm32_dsirefresh(void);
int stm32_dsishortwrite(uint8_t datatype, uint8_t command, uint8_t param);
int stm32_dsilongwrite(uint8_t command, FAR const uint8_t *params,
                       uint16_t nparams);
#endif

#endif /* __ARCH_ARM_SRC_STM32U5_STM32_DSI_H */
