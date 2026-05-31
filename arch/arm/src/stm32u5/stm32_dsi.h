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

#include <stdbool.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_DSI_DCS_SHORT_WRITE0  0x05
#define STM32_DSI_DCS_SHORT_READ    0x06
#define STM32_DSI_DCS_SHORT_WRITE1  0x15
#define STM32_DSI_DCS_LONG_WRITE    0x39

#define STM32_DSI_DPHY_FRANGE_80MHZ_100MHZ  0
#define STM32_DSI_DPHY_FRANGE_100MHZ_120MHZ 1
#define STM32_DSI_DPHY_FRANGE_120MHZ_160MHZ 2
#define STM32_DSI_DPHY_FRANGE_160MHZ_200MHZ 3
#define STM32_DSI_DPHY_FRANGE_200MHZ_240MHZ 4
#define STM32_DSI_DPHY_FRANGE_240MHZ_320MHZ 5
#define STM32_DSI_DPHY_FRANGE_320MHZ_390MHZ 6
#define STM32_DSI_DPHY_FRANGE_390MHZ_450MHZ 7
#define STM32_DSI_DPHY_FRANGE_450MHZ_510MHZ 8

#define STM32_DSI_PHY_SWAP_CLK (1 << 0)
#define STM32_DSI_PHY_SWAP_D0  (1 << 1)
#define STM32_DSI_PHY_SWAP_D1  (1 << 2)

#define STM32_DSI_VIDEO_MODE_SYNC_PULSES 0
#define STM32_DSI_VIDEO_MODE_SYNC_EVENTS 1
#define STM32_DSI_VIDEO_MODE_BURST       2

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
  uint8_t hscale;
  uint8_t hscale_num;
  uint8_t hscale_den;
  uint16_t pixel_pll_n;
  uint8_t pixel_pll_r;
  uint16_t phy_ndiv;
  uint8_t phy_frange;
  uint8_t phy_swap;
  uint8_t video_mode;
  bool bta;
  bool lp_cmd;
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
int stm32_dsiread(uint8_t datatype, uint8_t command, FAR uint8_t *buffer,
                  uint16_t len);
int stm32_dsilongwrite(uint8_t command, FAR const uint8_t *params,
                       uint16_t nparams);
#endif

#endif /* __ARCH_ARM_SRC_STM32U5_STM32_DSI_H */
