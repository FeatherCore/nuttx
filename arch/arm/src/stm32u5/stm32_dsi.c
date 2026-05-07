/****************************************************************************
 * arch/arm/src/stm32u5/stm32_dsi.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdint.h>

#include "arm_internal.h"
#include "stm32_dsi.h"

#include "hardware/stm32_dsi.h"
#include "hardware/stm32u5xx_rcc.h"

#ifdef CONFIG_STM32U5_DSIHOST

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_dsi_wait(uint32_t reg, uint32_t mask, bool set)
{
  uint32_t timeout;

  for (timeout = 1000000; timeout > 0; timeout--)
    {
      if (((getreg32(reg) & mask) != 0) == set)
        {
          return OK;
        }
    }

  return -ETIMEDOUT;
}

static int stm32_dsi_wait_cmd_fifo(void)
{
  return stm32_dsi_wait(STM32_DSI_GPSR, DSI_GPSR_CMDFE, true);
}

static void stm32_dsi_video_config(const struct stm32_dsi_video_config_s *cfg)
{
  putreg32(0, STM32_DSI_MCR);
  putreg32(DSI_VMCR_BURST | DSI_VMCR_LPCE | DSI_VMCR_LPHFPE |
           DSI_VMCR_LPHBPE | DSI_VMCR_LPVAE | DSI_VMCR_LPVFPE |
           DSI_VMCR_LPVBPE | DSI_VMCR_LPVSAE | DSI_VMCR_FBTAAE,
           STM32_DSI_VMCR);
  putreg32(cfg->width, STM32_DSI_VPCR);
  putreg32(0, STM32_DSI_VCCR);
  putreg32(0x0fff, STM32_DSI_VNPCR);
  putreg32(0, STM32_DSI_LVCIDR);
  putreg32(DSI_LCOLCR_RGB888, STM32_DSI_LCOLCR);
  putreg32(0, STM32_DSI_LPCR);
  putreg32(cfg->hsync * 3, STM32_DSI_VHSACR);
  putreg32(cfg->hbp * 3, STM32_DSI_VHBPCR);
  putreg32((cfg->width + cfg->hsync + cfg->hbp + cfg->hfp) * 3,
           STM32_DSI_VLCR);
  putreg32(cfg->vsync, STM32_DSI_VVSACR);
  putreg32(cfg->vbp, STM32_DSI_VVBPCR);
  putreg32(cfg->vfp, STM32_DSI_VVFPCR);
  putreg32(cfg->vactive, STM32_DSI_VVACR);
  putreg32(64 << 16, STM32_DSI_LPMCR);
  modifyreg32(STM32_DSI_WCFGR, DSI_WCFGR_DSIM | DSI_WCFGR_COLMUX(7),
              DSI_WCFGR_COLMUX(DSI_LCOLCR_RGB888));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_dsiinitialize(const struct stm32_dsi_video_config_s *config)
{
  if (config == NULL || config->lanes == 0 || config->lanes > 2)
    {
      return -EINVAL;
    }

  modifyreg32(STM32_RCC_CCIPR2, RCC_CCIPR2_DSIHOSTSEL_MASK,
              RCC_CCIPR2_DSIHOSTSEL_PLL3);
  modifyreg32(STM32_RCC_APB2ENR, 0, RCC_APB2ENR_DSIHOSTEN);

  modifyreg32(STM32_RCC_APB2RSTR, 0, RCC_APB2RSTR_DSIHOSTRST);
  modifyreg32(STM32_RCC_APB2RSTR, RCC_APB2RSTR_DSIHOSTRST, 0);

  putreg32(DSI_CCR_TXECKDIV(4), STM32_DSI_CCR);
  putreg32(DSI_PCONFR_NL(config->lanes), STM32_DSI_PCONFR);
  putreg32(0, STM32_DSI_GVCIDR);
  putreg32(0, STM32_DSI_PCR);
  putreg32(0, STM32_DSI_CMCR);
  putreg32((11 << 0) | (40 << 16), STM32_DSI_CLTCR);
  putreg32((12 << 0) | (23 << 16), STM32_DSI_DLTCR);
  putreg32(DSI_WRPCR_NDIV(125) | DSI_WRPCR_IDF(4) |
           DSI_WRPCR_ODF(2) | DSI_WRPCR_PLLEN, STM32_DSI_WRPCR);

  stm32_dsi_video_config(config);

  return OK;
}

int stm32_dsistart(void)
{
  modifyreg32(STM32_DSI_CR, 0, DSI_CR_EN);
  modifyreg32(STM32_DSI_WCR, 0, DSI_WCR_DSIEN);

  return OK;
}

int stm32_dsienableltdc(void)
{
  modifyreg32(STM32_DSI_WCR, 0, DSI_WCR_LTDCEN);
  return OK;
}

int stm32_dsishortwrite(uint8_t datatype, uint8_t command, uint8_t param)
{
  int ret;

  ret = stm32_dsi_wait_cmd_fifo();
  if (ret < 0)
    {
      return ret;
    }

  putreg32(DSI_GHCR_DT(datatype) | DSI_GHCR_VCID(0) |
           DSI_GHCR_WC(command | (param << 8)), STM32_DSI_GHCR);

  return OK;
}

int stm32_dsilongwrite(uint8_t command, FAR const uint8_t *params,
                       uint16_t nparams)
{
  uint32_t word;
  uint16_t i;
  int ret;

  if (params == NULL && nparams > 0)
    {
      return -EINVAL;
    }

  ret = stm32_dsi_wait_cmd_fifo();
  if (ret < 0)
    {
      return ret;
    }

  word = command;
  for (i = 0; i < nparams; i++)
    {
      word |= ((uint32_t)params[i]) << (((i + 1) & 3) * 8);

      if (((i + 1) & 3) == 3)
        {
          putreg32(word, STM32_DSI_GPDR);
          word = 0;
        }
    }

  if (((nparams + 1) & 3) != 0)
    {
      putreg32(word, STM32_DSI_GPDR);
    }

  putreg32(DSI_GHCR_DT(STM32_DSI_DCS_LONG_WRITE) | DSI_GHCR_VCID(0) |
           DSI_GHCR_WC(nparams + 1), STM32_DSI_GHCR);

  return OK;
}

#endif /* CONFIG_STM32U5_DSIHOST */
