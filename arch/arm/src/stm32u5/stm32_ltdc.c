/****************************************************************************
 * arch/arm/src/stm32u5/stm32_ltdc.c
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
#include <string.h>

#include <nuttx/video/fb.h>

#include "arm_internal.h"
#include "stm32_ltdc.h"

#include "hardware/stm32_ltdc.h"
#include "hardware/stm32u5xx_rcc.h"

#ifdef CONFIG_STM32U5_LTDC

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32_ltdc_s
{
  struct fb_vtable_s vtable;
  struct stm32_ltdc_config_s config;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32_ltdc_getvideoinfo(FAR struct fb_vtable_s *vtable,
                                   FAR struct fb_videoinfo_s *vinfo);
static int stm32_ltdc_getplaneinfo(FAR struct fb_vtable_s *vtable,
                                   int planeno,
                                   FAR struct fb_planeinfo_s *pinfo);
static int stm32_ltdc_open(FAR struct fb_vtable_s *vtable);
static int stm32_ltdc_close(FAR struct fb_vtable_s *vtable);
static int stm32_ltdc_getpower(FAR struct fb_vtable_s *vtable);
static int stm32_ltdc_setpower(FAR struct fb_vtable_s *vtable, int power);
static int stm32_ltdc_ioctl(FAR struct fb_vtable_s *vtable, int cmd,
                            unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32_ltdc_s g_ltdc =
{
  .vtable =
    {
      .getvideoinfo = stm32_ltdc_getvideoinfo,
      .getplaneinfo = stm32_ltdc_getplaneinfo,
      .open         = stm32_ltdc_open,
      .close        = stm32_ltdc_close,
      .getpower     = stm32_ltdc_getpower,
      .setpower     = stm32_ltdc_setpower,
      .ioctl        = stm32_ltdc_ioctl,
    },
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_ltdc_getvideoinfo(FAR struct fb_vtable_s *vtable,
                                   FAR struct fb_videoinfo_s *vinfo)
{
  if (vinfo == NULL)
    {
      return -EINVAL;
    }

  memset(vinfo, 0, sizeof(*vinfo));
  vinfo->fmt     = FB_FMT_RGB32;
  vinfo->xres    = g_ltdc.config.xres;
  vinfo->yres    = g_ltdc.config.yres;
  vinfo->nplanes = 1;

  return OK;
}

static int stm32_ltdc_getplaneinfo(FAR struct fb_vtable_s *vtable,
                                   int planeno,
                                   FAR struct fb_planeinfo_s *pinfo)
{
  if (pinfo == NULL)
    {
      return -EINVAL;
    }

  if (planeno != 0)
    {
      return -EINVAL;
    }

  memset(pinfo, 0, sizeof(*pinfo));
  pinfo->fbmem        = (FAR void *)g_ltdc.config.fb_base;
  pinfo->fblen        = g_ltdc.config.stride * g_ltdc.config.yres;
  pinfo->stride       = g_ltdc.config.stride;
  pinfo->display      = 0;
  pinfo->bpp          = g_ltdc.config.bpp;
  pinfo->xres_virtual = g_ltdc.config.xres;
  pinfo->yres_virtual = g_ltdc.config.yres;

  return OK;
}

static int stm32_ltdc_open(FAR struct fb_vtable_s *vtable)
{
  return OK;
}

static int stm32_ltdc_close(FAR struct fb_vtable_s *vtable)
{
  return OK;
}

static int stm32_ltdc_getpower(FAR struct fb_vtable_s *vtable)
{
  return (getreg32(STM32_LTDC_GCR) & LTDC_GCR_LTDCEN) != 0;
}

static int stm32_ltdc_setpower(FAR struct fb_vtable_s *vtable, int power)
{
  if (power < 0)
    {
      return -EINVAL;
    }

  modifyreg32(STM32_LTDC_GCR, LTDC_GCR_LTDCEN,
              power > 0 ? LTDC_GCR_LTDCEN : 0);
  return OK;
}

static int stm32_ltdc_ioctl(FAR struct fb_vtable_s *vtable, int cmd,
                            unsigned long arg)
{
  return -ENOTSUP;
}

static int stm32_ltdc_validate(const struct stm32_ltdc_config_s *config)
{
  if (config == NULL || config->fb_base == 0 || config->layer_fb_base == 0 ||
      config->xres == 0 || config->yres == 0 || config->stride == 0 ||
      config->bpp != 32)
    {
      return -EINVAL;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void stm32_ltdcuninitialize(void)
{
  modifyreg32(STM32_LTDC_GCR, LTDC_GCR_LTDCEN, 0);
  modifyreg32(STM32_RCC_APB2ENR, RCC_APB2ENR_LTDCEN, 0);
}

int stm32_ltdcinitialize(const struct stm32_ltdc_config_s *config)
{
  uint32_t ahbp;
  uint32_t avbp;
  uint32_t aaw;
  uint32_t aah;
  uint32_t totalw;
  uint32_t totalh;
  uint32_t whst;
  uint32_t whsp;
  uint32_t wvst;
  uint32_t wvsp;
  uint32_t linebytes;
  int ret;

  ret = stm32_ltdc_validate(config);
  if (ret < 0)
    {
      return ret;
    }

  memcpy(&g_ltdc.config, config, sizeof(g_ltdc.config));

  modifyreg32(STM32_RCC_CCIPR2, RCC_CCIPR2_LTDCSEL_MASK,
              RCC_CCIPR2_LTDCSEL_PLL3);
  modifyreg32(STM32_RCC_APB2ENR, 0, RCC_APB2ENR_LTDCEN);

  modifyreg32(STM32_RCC_APB2RSTR, 0, RCC_APB2RSTR_LTDCRST);
  modifyreg32(STM32_RCC_APB2RSTR, RCC_APB2RSTR_LTDCRST, 0);

  ahbp   = config->hsync + config->hbp - 1;
  avbp   = config->vsync + config->vbp - 1;
  aaw    = config->hsync + config->hbp + config->xres - 1;
  aah    = config->vsync + config->vbp + config->yres - 1;
  totalw = config->hsync + config->hbp + config->xres + config->hfp - 1;
  totalh = config->vsync + config->vbp + config->yres + config->vfp - 1;

  putreg32(LTDC_SSCR_HSW(config->hsync - 1) |
           LTDC_SSCR_VSH(config->vsync - 1), STM32_LTDC_SSCR);
  putreg32(LTDC_BPCR_AHBP(ahbp) | LTDC_BPCR_AVBP(avbp),
           STM32_LTDC_BPCR);
  putreg32(LTDC_AWCR_AAW(aaw) | LTDC_AWCR_AAH(aah),
           STM32_LTDC_AWCR);
  putreg32(LTDC_TWCR_TOTALW(totalw) | LTDC_TWCR_TOTALH(totalh),
           STM32_LTDC_TWCR);

  putreg32(0, STM32_LTDC_BCCR);
  putreg32(0, STM32_LTDC_IER);
  putreg32(0xffffffff, STM32_LTDC_ICR);

  whst      = ahbp + 1;
  whsp      = whst + config->xres - 1;
  wvst      = avbp + 1;
  wvsp      = wvst + config->yres - 1;
  linebytes = config->stride;

  putreg32(0, STM32_LTDC_L1CR);
  putreg32(LTDC_LXWHPCR_WHSP(whsp) | LTDC_LXWHPCR_WHST(whst),
           STM32_LTDC_L1WHPCR);
  putreg32(LTDC_LXWVPCR_WVSP(wvsp) | LTDC_LXWVPCR_WVST(wvst),
           STM32_LTDC_L1WVPCR);
  putreg32(0, STM32_LTDC_L1CKCR);
  putreg32(LTDC_LXPFCR_ARGB8888, STM32_LTDC_L1PFCR);
  putreg32(LTDC_LXCACR_CONSTA(0xff), STM32_LTDC_L1CACR);
  putreg32(0, STM32_LTDC_L1DCCR);
  putreg32(LTDC_LXBFCR_BF1(LTDC_LXBFCR_CA) |
           LTDC_LXBFCR_BF2(LTDC_LXBFCR_1CA), STM32_LTDC_L1BFCR);
  putreg32(config->layer_fb_base, STM32_LTDC_L1CFBAR);
  putreg32(LTDC_LXCFBLR_CFBP(linebytes) |
           LTDC_LXCFBLR_CFBLL(linebytes + 3), STM32_LTDC_L1CFBLR);
  putreg32(config->yres, STM32_LTDC_L1CFBLNR);
  putreg32(LTDC_LXCR_LEN, STM32_LTDC_L1CR);

  putreg32(LTDC_SRCR_IMR, STM32_LTDC_SRCR);
  modifyreg32(STM32_LTDC_GCR, 0, LTDC_GCR_LTDCEN);

  return OK;
}

struct fb_vtable_s *stm32_ltdcgetvplane(int vplane)
{
  if (vplane != 0)
    {
      return NULL;
    }

  return &g_ltdc.vtable;
}

#endif /* CONFIG_STM32U5_LTDC */
