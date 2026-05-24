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
#include <inttypes.h>
#include <stdint.h>
#include <syslog.h>

#include <nuttx/arch.h>

#include "arm_internal.h"
#include "stm32_dsi.h"

#include "hardware/stm32_dsi.h"
#include "hardware/stm32u5xx_rcc.h"

#ifdef CONFIG_STM32U5_DSIHOST

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_DSI_PLL3_M              4
#define STM32_DSI_PLL3_N              125
#define STM32_DSI_PLL3_P              8
#define STM32_DSI_PLL3_Q              8
#define STM32_DSI_PLL3_R              32
#define STM32_DSI_HSE_HZ              16000000ull
#define STM32_DSI_TIMEOUT_US          1000000

#define STM32_DSI_PHY_PLL_NDIV        125
#define STM32_DSI_PHY_PLL_IDF         4
#define STM32_DSI_PHY_PLL_ODF         2
#define STM32_DSI_PHY_BYTE_DIV        8

#define RCC_PLL3CFGR_PLL3SRC_SHIFT    0
#define RCC_PLL3CFGR_PLL3SRC_HSE      (3 << RCC_PLL3CFGR_PLL3SRC_SHIFT)
#define RCC_PLL3CFGR_PLL3RGE_SHIFT    2
#define RCC_PLL3CFGR_PLL3RGE_CUBE     (3 << RCC_PLL3CFGR_PLL3RGE_SHIFT)
#define RCC_PLL3CFGR_PLL3M_SHIFT      8
#define RCC_PLL3CFGR_PLL3M(n)         (((n) - 1) << RCC_PLL3CFGR_PLL3M_SHIFT)
#define RCC_PLL3CFGR_PLL3PEN          (1 << 16)
#define RCC_PLL3CFGR_PLL3REN          (1 << 18)

#define RCC_PLL3DIVR_PLL3N_SHIFT      0
#define RCC_PLL3DIVR_PLL3P_SHIFT      9
#define RCC_PLL3DIVR_PLL3Q_SHIFT      16
#define RCC_PLL3DIVR_PLL3R_SHIFT      24
#define RCC_PLL3DIVR_PLL3N(n)         (((n) - 1) << RCC_PLL3DIVR_PLL3N_SHIFT)
#define RCC_PLL3DIVR_PLL3P(n)         (((n) - 1) << RCC_PLL3DIVR_PLL3P_SHIFT)
#define RCC_PLL3DIVR_PLL3Q(n)         (((n) - 1) << RCC_PLL3DIVR_PLL3Q_SHIFT)
#define RCC_PLL3DIVR_PLL3R(n)         (((n) - 1) << RCC_PLL3DIVR_PLL3R_SHIFT)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_dsi_wait(uint32_t reg, uint32_t mask, bool set)
{
  uint32_t timeout;

  for (timeout = STM32_DSI_TIMEOUT_US; timeout > 0; timeout--)
    {
      if (((getreg32(reg) & mask) != 0) == set)
        {
          return OK;
        }

      up_udelay(1);
    }

  return -ETIMEDOUT;
}

static int stm32_dsi_waitbits(uint32_t reg, uint32_t mask, uint32_t value)
{
  uint32_t timeout;

  for (timeout = STM32_DSI_TIMEOUT_US; timeout > 0; timeout--)
    {
      if ((getreg32(reg) & mask) == value)
        {
          return OK;
        }

      up_udelay(1);
    }

  return -ETIMEDOUT;
}

static int stm32_dsi_wait_cmd_fifo(void)
{
  return stm32_dsi_wait(STM32_DSI_GPSR, DSI_GPSR_CMDFE, true);
}

static uint8_t stm32_dsi_colorcoding(uint8_t bpp)
{
  return bpp == 16 ? DSI_LCOLCR_RGB565 : DSI_LCOLCR_RGB888;
}

static uint8_t stm32_dsi_bytespp(uint8_t bpp)
{
  return bpp == 16 ? 2 : 3;
}

static uint64_t stm32_dsi_pixel_hz(void)
{
  return STM32_DSI_HSE_HZ / STM32_DSI_PLL3_M * STM32_DSI_PLL3_N /
         STM32_DSI_PLL3_R;
}

static uint64_t stm32_dsi_lane_byte_hz(void)
{
  return STM32_DSI_HSE_HZ / STM32_DSI_PHY_PLL_IDF *
         STM32_DSI_PHY_PLL_NDIV / STM32_DSI_PHY_BYTE_DIV;
}

static uint32_t stm32_dsi_hscale(void)
{
  uint64_t pixel_hz = stm32_dsi_pixel_hz();
  uint64_t lane_byte_hz = stm32_dsi_lane_byte_hz();

  return (uint32_t)((lane_byte_hz + pixel_hz / 2) / pixel_hz);
}

static int stm32_dsi_config_pll3(void)
{
  uint32_t regval;
  int ret;

  modifyreg32(STM32_RCC_CR, 0, RCC_CR_HSEON);

  ret = stm32_dsi_waitbits(STM32_RCC_CR, RCC_CR_HSERDY, RCC_CR_HSERDY);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5: DSI HSE ready timeout cr=%08" PRIx32
             "\n", getreg32(STM32_RCC_CR));
      return ret;
    }

  modifyreg32(STM32_RCC_CR, RCC_CR_PLL3ON, 0);

  ret = stm32_dsi_waitbits(STM32_RCC_CR, RCC_CR_PLL3RDY, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5: DSI PLL3 disable timeout cr=%08" PRIx32
             "\n", getreg32(STM32_RCC_CR));
      return ret;
    }

  regval = RCC_PLL3CFGR_PLL3SRC_HSE |
           RCC_PLL3CFGR_PLL3RGE_CUBE |
           RCC_PLL3CFGR_PLL3M(STM32_DSI_PLL3_M) |
           RCC_PLL3CFGR_PLL3PEN |
           RCC_PLL3CFGR_PLL3REN;
  putreg32(regval, STM32_RCC_PLL3CFGR);

  regval = RCC_PLL3DIVR_PLL3N(STM32_DSI_PLL3_N) |
           RCC_PLL3DIVR_PLL3P(STM32_DSI_PLL3_P) |
           RCC_PLL3DIVR_PLL3Q(STM32_DSI_PLL3_Q) |
           RCC_PLL3DIVR_PLL3R(STM32_DSI_PLL3_R);
  putreg32(regval, STM32_RCC_PLL3DIVR);
  putreg32(0, STM32_RCC_PLL3FRACR);

  modifyreg32(STM32_RCC_CR, 0, RCC_CR_PLL3ON);

  ret = stm32_dsi_waitbits(STM32_RCC_CR, RCC_CR_PLL3RDY,
                           RCC_CR_PLL3RDY);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5: DSI PLL3 ready timeout cr=%08" PRIx32
             " pll3cfgr=%08" PRIx32 " pll3divr=%08" PRIx32 "\n",
             getreg32(STM32_RCC_CR), getreg32(STM32_RCC_PLL3CFGR),
             getreg32(STM32_RCC_PLL3DIVR));
    }

  return ret;
}

static int stm32_dsi_enable_phy(const struct stm32_dsi_video_config_s *cfg)
{
  uint32_t stopmask = DSI_PSR_PSSC | DSI_PSR_PSS0;
  int ret;

  putreg32(DSI_BCFGR_PWRUP, STM32_DSI_BCFGR);
  up_mdelay(2);

  modifyreg32(STM32_DSI_WRPCR, DSI_WRPCR_NDIV(0x1ff) |
              DSI_WRPCR_IDF(0x1ff) | DSI_WRPCR_ODF(0x1ff) | DSI_WRPCR_BC,
              DSI_WRPCR_NDIV(STM32_DSI_PHY_PLL_NDIV) |
              DSI_WRPCR_IDF(STM32_DSI_PHY_PLL_IDF) |
              DSI_WRPCR_ODF(STM32_DSI_PHY_PLL_ODF) |
              DSI_WRPCR_BC | DSI_WRPCR_PLLEN);
  putreg32(0, STM32_DSI_WPTR);
  up_mdelay(1);

  ret = stm32_dsi_waitbits(STM32_DSI_WISR, DSI_WISR_PLLLS,
                           DSI_WISR_PLLLS);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5: DSI wrapper PLL lock timeout"
             " wisr=%08" PRIx32 " wrpcr=%08" PRIx32
             " bcfgr=%08" PRIx32 "\n",
             getreg32(STM32_DSI_WISR), getreg32(STM32_DSI_WRPCR),
             getreg32(STM32_DSI_BCFGR));
      return ret;
    }

  modifyreg32(STM32_DSI_CR, 0, DSI_CR_EN);
  putreg32(DSI_CCR_TXECKDIV(4) | DSI_CCR_TOCKDIV(1), STM32_DSI_CCR);

  putreg32(DSI_PCTLR_DEN, STM32_DSI_PCTLR);
  putreg32(DSI_DPCBCR_FRANGE(DSI_DPHY_FRANGE_450MHZ_510MHZ),
           STM32_DSI_DPCBCR);
  putreg32(DSI_DPCSRCR_SLEW(DSI_DPHY_SLEW_HS_TX_SPEED),
           STM32_DSI_DPCSRCR);
  putreg32(DSI_DPDL_BCR(DSI_DPHY_FRANGE_450MHZ_510MHZ),
           STM32_DSI_DPDL0BCR);
  putreg32(DSI_DPDL_BCR(DSI_DPHY_FRANGE_450MHZ_510MHZ),
           STM32_DSI_DPDL1BCR);
  putreg32(DSI_DPDL_SRCR(DSI_DPHY_SLEW_HS_TX_SPEED),
           STM32_DSI_DPDL0SRCR);
  putreg32(DSI_DPDL_SRCR(DSI_DPHY_SLEW_HS_TX_SPEED),
           STM32_DSI_DPDL1SRCR);
  putreg32(DSI_DPDL0HSOCR_HS(DSI_HS_PREPARE_OFFSET2),
           STM32_DSI_DPDL0HSOCR);
  putreg32(DSI_DPDL1HSOCR_HS(DSI_HS_PREPARE_OFFSET2),
           STM32_DSI_DPDL1HSOCR);
  putreg32(DSI_DPDL_LPXO(0), STM32_DSI_DPDL0LPXOCR);
  putreg32(DSI_DPDL_LPXO(0), STM32_DSI_DPDL1LPXOCR);

  modifyreg32(STM32_DSI_PCTLR, 0, DSI_PCTLR_CKE);
  putreg32(DSI_PCONFR_NL(cfg->lanes) | DSI_PCONFR_SW_TIME(7),
           STM32_DSI_PCONFR);

  if (cfg->lanes == 2)
    {
      stopmask |= DSI_PSR_PSS1;
    }

  ret = stm32_dsi_waitbits(STM32_DSI_PSR, stopmask, stopmask);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5: DSI PHY stop-state timeout"
             " psr=%08" PRIx32 " mask=%08" PRIx32
             " pctlr=%08" PRIx32 " pconfr=%08" PRIx32
             " wrpcr=%08" PRIx32 " wptr=%08" PRIx32
             " ccipr2=%08" PRIx32 "\n",
             getreg32(STM32_DSI_PSR), stopmask,
             getreg32(STM32_DSI_PCTLR), getreg32(STM32_DSI_PCONFR),
             getreg32(STM32_DSI_WRPCR), getreg32(STM32_DSI_WPTR),
             getreg32(STM32_RCC_CCIPR2));
      return ret;
    }

  modifyreg32(STM32_DSI_CR, DSI_CR_EN, 0);
  putreg32(DSI_CLCR_DPCC, STM32_DSI_CLCR);

  return OK;
}

static void stm32_dsi_video_config(const struct stm32_dsi_video_config_s *cfg)
{
  uint32_t colorcoding = stm32_dsi_colorcoding(cfg->bpp);
  uint32_t total_pixels;
  uint32_t bytespp = stm32_dsi_bytespp(cfg->bpp);
  uint32_t hscale = stm32_dsi_hscale();
  uint64_t pixel_hz;
  uint64_t lane_byte_hz;
  uint64_t refresh_x100;

  pixel_hz = stm32_dsi_pixel_hz();
  lane_byte_hz = stm32_dsi_lane_byte_hz();
  total_pixels = (cfg->width + cfg->hsync + cfg->hbp + cfg->hfp) *
                 (cfg->vactive + cfg->vsync + cfg->vbp + cfg->vfp);
  refresh_x100 = pixel_hz * 100 / total_pixels;

  syslog(LOG_INFO,
         "stm32u5: DSI video bpp=%u colorcoding=%s bytespp=%lu"
         " pclk=%" PRIu64 "Hz lane-byte=%" PRIu64 "Hz hscale=%lu"
         " refresh=%" PRIu64 ".%02" PRIu64 "Hz\n",
         (unsigned int)cfg->bpp, cfg->bpp == 16 ? "RGB565" : "RGB888",
         (unsigned long)bytespp, pixel_hz, lane_byte_hz,
         (unsigned long)hscale, refresh_x100 / 100, refresh_x100 % 100);

  putreg32(0, STM32_DSI_MCR);
  putreg32(DSI_VMCR_BURST | DSI_VMCR_LPCE | DSI_VMCR_LPHFPE |
           DSI_VMCR_LPHBPE | DSI_VMCR_LPVAE | DSI_VMCR_LPVFPE |
           DSI_VMCR_LPVBPE | DSI_VMCR_LPVSAE | DSI_VMCR_FBTAAE,
           STM32_DSI_VMCR);
  putreg32(cfg->width, STM32_DSI_VPCR);
  putreg32(0, STM32_DSI_VCCR);
  putreg32(0x0fff, STM32_DSI_VNPCR);
  putreg32(0, STM32_DSI_LVCIDR);
  putreg32(colorcoding, STM32_DSI_LCOLCR);
  putreg32(0, STM32_DSI_LPCR);
  putreg32(cfg->hsync * hscale, STM32_DSI_VHSACR);
  putreg32(cfg->hbp * hscale, STM32_DSI_VHBPCR);
  putreg32((cfg->width + cfg->hsync + cfg->hbp + cfg->hfp) * hscale,
           STM32_DSI_VLCR);
  putreg32(cfg->vsync, STM32_DSI_VVSACR);
  putreg32(cfg->vbp, STM32_DSI_VVBPCR);
  putreg32(cfg->vfp, STM32_DSI_VVFPCR);
  putreg32(cfg->vactive, STM32_DSI_VVACR);
  putreg32(64 << 16, STM32_DSI_LPMCR);
  putreg32(40 | (40 << 16), STM32_DSI_CLTCR);
  putreg32(23 | (12 << 16), STM32_DSI_DLTCR);
  putreg32(0, STM32_DSI_DLTRCR);
  putreg32(0, STM32_DSI_TCCR(0));
  putreg32(0, STM32_DSI_TCCR(1));
  putreg32(0, STM32_DSI_TCCR(2));
  putreg32(0, STM32_DSI_TCCR(3));
  putreg32(0, STM32_DSI_TCCR(4));
  putreg32(0, STM32_DSI_TCCR(5));
  putreg32(DSI_PCR_BTAE, STM32_DSI_PCR);
  modifyreg32(STM32_DSI_WCFGR, DSI_WCFGR_DSIM | DSI_WCFGR_COLMUX(7),
              DSI_WCFGR_COLMUX(colorcoding));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_dsiinitialize(const struct stm32_dsi_video_config_s *config)
{
  int ret;

  if (config == NULL || config->lanes == 0 || config->lanes > 2 ||
      (config->bpp != 16 && config->bpp != 32))
    {
      return -EINVAL;
    }

  ret = stm32_dsi_config_pll3();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5: DSI PLL3 clock setup failed: %d\n", ret);
      return ret;
    }

  modifyreg32(STM32_RCC_CCIPR2, RCC_CCIPR2_DSIHOSTSEL_MASK,
              RCC_CCIPR2_DSIHOSTSEL_PLL3);
  modifyreg32(STM32_RCC_APB2ENR, 0, RCC_APB2ENR_DSIHOSTEN);

  modifyreg32(STM32_RCC_APB2RSTR, 0, RCC_APB2RSTR_DSIHOSTRST);
  modifyreg32(STM32_RCC_APB2RSTR, RCC_APB2RSTR_DSIHOSTRST, 0);

  ret = stm32_dsi_enable_phy(config);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5: DSI PHY setup failed: %d\n", ret);
      return ret;
    }

  putreg32(0, STM32_DSI_GVCIDR);
  putreg32(0, STM32_DSI_PCR);
  putreg32(0, STM32_DSI_CMCR);

  stm32_dsi_video_config(config);

  modifyreg32(STM32_RCC_CCIPR2, RCC_CCIPR2_DSIHOSTSEL_MASK,
              RCC_CCIPR2_DSIHOSTSEL_DSIPHY);

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

int stm32_dsirefresh(void)
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
