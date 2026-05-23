/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32h7rs_ltdc.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>

#include <arch/board/board.h>
#include <nuttx/cache.h>
#include <nuttx/compiler.h>
#include <nuttx/video/fb.h>

#include "arm_internal.h"

#include "hardware/stm32h7rs_gpio.h"
#include "hardware/stm32h7rs_ltdc.h"
#include "hardware/stm32h7rs_rcc.h"

#include "stm32h7rs_ltdc.h"

#ifdef CONFIG_STM32H7RS_DISPLAY

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef BOARD_LTDC_WIDTH
#  error BOARD_LTDC_WIDTH must be defined in board.h
#endif

#ifndef BOARD_LTDC_HEIGHT
#  error BOARD_LTDC_HEIGHT must be defined in board.h
#endif

#ifndef BOARD_LTDC_BPP
#  error BOARD_LTDC_BPP must be defined in board.h
#endif

#if BOARD_LTDC_BPP != 16
#  error STM32H7RS LTDC currently supports only RGB565 framebuffer output
#endif

#ifndef BOARD_LTDC_STRIDE
#  define BOARD_LTDC_STRIDE ((BOARD_LTDC_WIDTH * BOARD_LTDC_BPP + 7) / 8)
#endif

#ifndef BOARD_LTDC_FB_BASE
#  error BOARD_LTDC_FB_BASE must be defined in board.h
#endif

#ifndef BOARD_LTDC_HSYNC
#  error BOARD_LTDC_HSYNC must be defined in board.h
#endif

#ifndef BOARD_LTDC_HBP
#  error BOARD_LTDC_HBP must be defined in board.h
#endif

#ifndef BOARD_LTDC_HFP
#  error BOARD_LTDC_HFP must be defined in board.h
#endif

#ifndef BOARD_LTDC_VSYNC
#  error BOARD_LTDC_VSYNC must be defined in board.h
#endif

#ifndef BOARD_LTDC_VBP
#  error BOARD_LTDC_VBP must be defined in board.h
#endif

#ifndef BOARD_LTDC_VFP
#  error BOARD_LTDC_VFP must be defined in board.h
#endif

#define LTDC_BYTES_PER_PIXEL    (BOARD_LTDC_BPP / 8)
#define LTDC_FRAME_SIZE         (BOARD_LTDC_STRIDE * BOARD_LTDC_HEIGHT)

#ifdef CONFIG_STM32H7RS_LTDC_FB_DOUBLE_BUFFER
#  define LTDC_FB_COUNT         2u
#else
#  define LTDC_FB_COUNT         1u
#endif

#define LTDC_FB_SIZE            (LTDC_FRAME_SIZE * LTDC_FB_COUNT)
#define LTDC_VIRTUAL_HEIGHT     (BOARD_LTDC_HEIGHT * LTDC_FB_COUNT)

#ifndef BOARD_LTDC_FB_SIZE
#  define BOARD_LTDC_FB_SIZE    LTDC_FB_SIZE
#endif

#if BOARD_LTDC_FB_SIZE < LTDC_FB_SIZE
#  error BOARD_LTDC_FB_SIZE is too small for the configured LTDC buffers
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32h7rs_ltdc_s
{
  struct fb_vtable_s vtable;
  bool initialized;
  int power;
  uint32_t yoffset;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32h7rs_getvideoinfo(FAR struct fb_vtable_s *vtable,
                                  FAR struct fb_videoinfo_s *vinfo);
static int stm32h7rs_getplaneinfo(FAR struct fb_vtable_s *vtable,
                                  int planeno,
                                  FAR struct fb_planeinfo_s *pinfo);
#ifdef CONFIG_FB_UPDATE
static int stm32h7rs_updatearea(FAR struct fb_vtable_s *vtable,
                                FAR const struct fb_area_s *area);
#endif
#ifdef CONFIG_FB_SYNC
static int stm32h7rs_waitforvsync(FAR struct fb_vtable_s *vtable);
#endif
static int stm32h7rs_getpower(FAR struct fb_vtable_s *vtable);
static int stm32h7rs_setpower(FAR struct fb_vtable_s *vtable, int power);
#ifdef CONFIG_STM32H7RS_LTDC_FB_DOUBLE_BUFFER
static int stm32h7rs_pandisplay(FAR struct fb_vtable_s *vtable,
                                FAR struct fb_planeinfo_s *pinfo);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32h7rs_ltdc_s g_ltdc =
{
  .vtable =
    {
      .getvideoinfo = stm32h7rs_getvideoinfo,
      .getplaneinfo = stm32h7rs_getplaneinfo,
#ifdef CONFIG_FB_UPDATE
      .updatearea   = stm32h7rs_updatearea,
#endif
#ifdef CONFIG_FB_SYNC
      .waitforvsync = stm32h7rs_waitforvsync,
#endif
#ifdef CONFIG_STM32H7RS_LTDC_FB_DOUBLE_BUFFER
      .pandisplay   = stm32h7rs_pandisplay,
#endif
      .getpower     = stm32h7rs_getpower,
      .setpower     = stm32h7rs_setpower,
    },
  .power = 1,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32h7rs_gpio_alt(uintptr_t portbase, uint32_t pins,
                               uint32_t af)
{
  uint32_t regval;
  int pin;

  regval = getreg32(portbase + STM32H7RS_GPIO_MODER_OFFSET);
  for (pin = 0; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_MODE_MASK(pin);
          regval |= GPIO_MODE_ALT << GPIO_MODE_SHIFT(pin);
        }
    }

  putreg32(regval, portbase + STM32H7RS_GPIO_MODER_OFFSET);
  modifyreg32(portbase + STM32H7RS_GPIO_OTYPER_OFFSET, pins, 0);

  regval = getreg32(portbase + STM32H7RS_GPIO_OSPEEDR_OFFSET);
  for (pin = 0; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_SPEED_MASK(pin);
          regval |= GPIO_SPEED_VERYHIGH << GPIO_SPEED_SHIFT(pin);
        }
    }

  putreg32(regval, portbase + STM32H7RS_GPIO_OSPEEDR_OFFSET);

  regval = getreg32(portbase + STM32H7RS_GPIO_PUPDR_OFFSET);
  for (pin = 0; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_PUPD_MASK(pin);
        }
    }

  putreg32(regval, portbase + STM32H7RS_GPIO_PUPDR_OFFSET);

  regval = getreg32(portbase + STM32H7RS_GPIO_AFRL_OFFSET);
  for (pin = 0; pin <= 7; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_AFR_MASK(pin);
          regval |= af << GPIO_AFR_SHIFT(pin);
        }
    }

  putreg32(regval, portbase + STM32H7RS_GPIO_AFRL_OFFSET);

  regval = getreg32(portbase + STM32H7RS_GPIO_AFRH_OFFSET);
  for (pin = 8; pin <= 15; pin++)
    {
      if ((pins & (1u << pin)) != 0)
        {
          regval &= ~GPIO_AFR_MASK(pin);
          regval |= af << GPIO_AFR_SHIFT(pin);
        }
    }

  putreg32(regval, portbase + STM32H7RS_GPIO_AFRH_OFFSET);
}

static int stm32h7rs_ltdc_gpio_config(void)
{
  modifyreg32(STM32H7RS_RCC_AHB4ENR, 0,
              RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
              RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOFEN |
              RCC_AHB4ENR_GPIOGEN);
  (void)getreg32(STM32H7RS_RCC_AHB4ENR);

  stm32h7rs_gpio_alt(STM32H7RS_GPIOB_BASE, (1u << 13) | (1u << 15),
                     GPIO_AF_LTDC10);
  stm32h7rs_gpio_alt(STM32H7RS_GPIOB_BASE,
                     (1u << 3) | (1u << 4) | (1u << 10) |
                     (1u << 11) | (1u << 12) | (1u << 14),
                     GPIO_AF_LTDC13);
  stm32h7rs_gpio_alt(STM32H7RS_GPIOA_BASE,
                     (1u << 0) | (1u << 1) | (1u << 8) |
                     (1u << 11) | (1u << 12) | (1u << 15),
                     GPIO_AF_LTDC13);
  stm32h7rs_gpio_alt(STM32H7RS_GPIOA_BASE, 1u << 6, GPIO_AF_LTDC12);
  stm32h7rs_gpio_alt(STM32H7RS_GPIOA_BASE, (1u << 9) | (1u << 10),
                     GPIO_AF_LTDC14);
  stm32h7rs_gpio_alt(STM32H7RS_GPIOG_BASE,
                     (1u << 0) | (1u << 1) | (1u << 2) |
                     (1u << 13) | (1u << 14),
                     GPIO_AF_LTDC13);
  stm32h7rs_gpio_alt(STM32H7RS_GPIOE_BASE, 1u << 11, GPIO_AF_LTDC11);
  stm32h7rs_gpio_alt(STM32H7RS_GPIOF_BASE, 1u << 0, GPIO_AF_LTDC11);
  stm32h7rs_gpio_alt(STM32H7RS_GPIOF_BASE, (1u << 10) | (1u << 11),
                     GPIO_AF_LTDC14);
  stm32h7rs_gpio_alt(STM32H7RS_GPIOF_BASE,
                     (1u << 7) | (1u << 9) | (1u << 15),
                     GPIO_AF_LTDC13);

  return OK;
}

static int stm32h7rs_ltdc_clock_enable(void)
{
  uint32_t timeout;

  if ((getreg32(STM32H7RS_RCC_CR) & RCC_CR_PLL3RDY) == 0)
    {
      modifyreg32(STM32H7RS_RCC_CR, 0, RCC_CR_HSION);
      timeout = 1000000;
      while ((getreg32(STM32H7RS_RCC_CR) & RCC_CR_HSIRDY) == 0)
        {
          if (timeout-- == 0)
            {
              return -ETIMEDOUT;
            }
        }

      modifyreg32(STM32H7RS_RCC_PLLCKSELR, RCC_PLLCKSELR_DIVM3_MASK,
                  RCC_PLLCKSELR_DIVM3(4));
      modifyreg32(STM32H7RS_RCC_PLLCFGR,
                  RCC_PLLCFGR_PLL3RGE_MASK | RCC_PLLCFGR_PLL3VCOSEL |
                  RCC_PLLCFGR_PLL3PEN | RCC_PLLCFGR_PLL3QEN |
                  RCC_PLLCFGR_PLL3REN | RCC_PLLCFGR_PLL3SEN,
                  RCC_PLLCFGR_PLL3RGE_8_16 | RCC_PLLCFGR_PLL3REN);
      putreg32(RCC_PLL3DIVR1_DIVN(25) | RCC_PLL3DIVR1_DIVP(2) |
               RCC_PLL3DIVR1_DIVQ(1) | RCC_PLL3DIVR1_DIVR(16),
               STM32H7RS_RCC_PLL3DIVR1);
      putreg32(0, STM32H7RS_RCC_PLL3FRACR);
      putreg32(RCC_PLL3DIVR2_DIVS(1) | RCC_PLL3DIVR2_DIVT(1),
               STM32H7RS_RCC_PLL3DIVR2);

      modifyreg32(STM32H7RS_RCC_CR, 0, RCC_CR_PLL3ON);
      timeout = 1000000;
      while ((getreg32(STM32H7RS_RCC_CR) & RCC_CR_PLL3RDY) == 0)
        {
          if (timeout-- == 0)
            {
              return -ETIMEDOUT;
            }
        }
    }

  modifyreg32(STM32H7RS_RCC_APB5ENR, 0, RCC_APB5ENR_LTDCEN);
  (void)getreg32(STM32H7RS_RCC_APB5ENR);

  modifyreg32(STM32H7RS_RCC_APB5RSTR, 0, RCC_APB5RSTR_LTDCRST);
  up_udelay(1);
  modifyreg32(STM32H7RS_RCC_APB5RSTR, RCC_APB5RSTR_LTDCRST, 0);
  return OK;
}

static void stm32h7rs_ltdc_config(void)
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

  ahbp   = BOARD_LTDC_HSYNC + BOARD_LTDC_HBP - 1;
  avbp   = BOARD_LTDC_VSYNC + BOARD_LTDC_VBP - 1;
  aaw    = BOARD_LTDC_HSYNC + BOARD_LTDC_HBP + BOARD_LTDC_WIDTH - 1;
  aah    = BOARD_LTDC_VSYNC + BOARD_LTDC_VBP + BOARD_LTDC_HEIGHT - 1;
  totalw = BOARD_LTDC_HSYNC + BOARD_LTDC_HBP + BOARD_LTDC_WIDTH +
           BOARD_LTDC_HFP - 1;
  totalh = BOARD_LTDC_VSYNC + BOARD_LTDC_VBP + BOARD_LTDC_HEIGHT +
           BOARD_LTDC_VFP - 1;
  whst   = ahbp + 1;
  whsp   = whst + BOARD_LTDC_WIDTH - 1;
  wvst   = avbp + 1;
  wvsp   = wvst + BOARD_LTDC_HEIGHT - 1;

  putreg32(0, STM32H7RS_LTDC_GCR);
  putreg32(0, STM32H7RS_LTDC_IER);
  putreg32(0xffffffff, STM32H7RS_LTDC_ICR);

  putreg32(LTDC_SSCR_HSW(BOARD_LTDC_HSYNC - 1) |
           LTDC_SSCR_VSH(BOARD_LTDC_VSYNC - 1), STM32H7RS_LTDC_SSCR);
  putreg32(LTDC_BPCR_AHBP(ahbp) | LTDC_BPCR_AVBP(avbp),
           STM32H7RS_LTDC_BPCR);
  putreg32(LTDC_AWCR_AAW(aaw) | LTDC_AWCR_AAH(aah),
           STM32H7RS_LTDC_AWCR);
  putreg32(LTDC_TWCR_TOTALW(totalw) | LTDC_TWCR_TOTALH(totalh),
           STM32H7RS_LTDC_TWCR);
  putreg32(0, STM32H7RS_LTDC_BCCR);
  putreg32(LTDC_GCR_LTDCEN, STM32H7RS_LTDC_GCR);

  putreg32(0, STM32H7RS_LTDC_L1CR);
  putreg32(LTDC_LXWHPCR_WHSP(whsp) | LTDC_LXWHPCR_WHST(whst),
           STM32H7RS_LTDC_L1WHPCR);
  putreg32(LTDC_LXWVPCR_WVSP(wvsp) | LTDC_LXWVPCR_WVST(wvst),
           STM32H7RS_LTDC_L1WVPCR);
  putreg32(0, STM32H7RS_LTDC_L1CKCR);
  putreg32(LTDC_LXPFCR_RGB565, STM32H7RS_LTDC_L1PFCR);
  putreg32(LTDC_LXCACR_CONSTA(0xff), STM32H7RS_LTDC_L1CACR);
  putreg32(0, STM32H7RS_LTDC_L1DCCR);
  putreg32(LTDC_LXBFCR_BF1(LTDC_LXBFCR_PAXCA) |
           LTDC_LXBFCR_BF2(LTDC_LXBFCR_PAXCA), STM32H7RS_LTDC_L1BFCR);
  putreg32(BOARD_LTDC_FB_BASE, STM32H7RS_LTDC_L1CFBAR);
  putreg32(LTDC_LXCFBLR_CFBP(BOARD_LTDC_STRIDE) |
           LTDC_LXCFBLR_CFBLL(BOARD_LTDC_STRIDE + 3),
           STM32H7RS_LTDC_L1CFBLR);
  putreg32(BOARD_LTDC_HEIGHT, STM32H7RS_LTDC_L1CFBLNR);
  putreg32(LTDC_LXCR_LEN, STM32H7RS_LTDC_L1CR);

  putreg32(LTDC_SRCR_IMR, STM32H7RS_LTDC_SRCR);
}

static int stm32h7rs_getvideoinfo(FAR struct fb_vtable_s *vtable,
                                  FAR struct fb_videoinfo_s *vinfo)
{
  if (vinfo == NULL)
    {
      return -EINVAL;
    }

  memset(vinfo, 0, sizeof(*vinfo));
  vinfo->fmt = FB_FMT_RGB16_565;
  vinfo->xres = BOARD_LTDC_WIDTH;
  vinfo->yres = BOARD_LTDC_HEIGHT;
  vinfo->nplanes = 1;
  return OK;
}

static int stm32h7rs_getplaneinfo(FAR struct fb_vtable_s *vtable,
                                  int planeno,
                                  FAR struct fb_planeinfo_s *pinfo)
{
  if (pinfo == NULL || planeno != 0)
    {
      return -EINVAL;
    }

  memset(pinfo, 0, sizeof(*pinfo));
  pinfo->fbmem = (FAR void *)BOARD_LTDC_FB_BASE;
  pinfo->fblen = LTDC_FB_SIZE;
  pinfo->stride = BOARD_LTDC_STRIDE;
  pinfo->display = 0;
  pinfo->bpp = BOARD_LTDC_BPP;
  pinfo->xres_virtual = BOARD_LTDC_WIDTH;
  pinfo->yres_virtual = LTDC_VIRTUAL_HEIGHT;
  pinfo->xoffset = 0;
  pinfo->yoffset = g_ltdc.yoffset;
  return OK;
}

#ifdef CONFIG_FB_UPDATE
static int stm32h7rs_updatearea(FAR struct fb_vtable_s *vtable,
                                FAR const struct fb_area_s *area)
{
  uintptr_t start;
  uintptr_t end;
  fb_coord_t yend;

  if (area == NULL ||
      area->x >= BOARD_LTDC_WIDTH ||
      area->y >= LTDC_VIRTUAL_HEIGHT)
    {
      return -EINVAL;
    }

  if (area->w == 0 || area->h == 0)
    {
      return OK;
    }

  yend = area->y + area->h;
  if (yend > LTDC_VIRTUAL_HEIGHT)
    {
      yend = LTDC_VIRTUAL_HEIGHT;
    }

  start = BOARD_LTDC_FB_BASE +
          area->y * BOARD_LTDC_STRIDE + area->x * LTDC_BYTES_PER_PIXEL;
  end = BOARD_LTDC_FB_BASE + yend * BOARD_LTDC_STRIDE;

  if (end > BOARD_LTDC_FB_BASE + LTDC_FB_SIZE)
    {
      end = BOARD_LTDC_FB_BASE + LTDC_FB_SIZE;
    }

  up_clean_dcache(start, end);
  return OK;
}
#endif

#ifdef CONFIG_STM32H7RS_LTDC_FB_DOUBLE_BUFFER
static int stm32h7rs_pandisplay(FAR struct fb_vtable_s *vtable,
                                FAR struct fb_planeinfo_s *pinfo)
{
  uintptr_t fbaddr;
  uintptr_t frame_end;

  if (vtable != &g_ltdc.vtable ||
      pinfo == NULL ||
      pinfo->bpp != BOARD_LTDC_BPP ||
      pinfo->stride != BOARD_LTDC_STRIDE ||
      pinfo->xoffset != 0 ||
      pinfo->yoffset > LTDC_VIRTUAL_HEIGHT - BOARD_LTDC_HEIGHT)
    {
      return -EINVAL;
    }

  fbaddr = BOARD_LTDC_FB_BASE + pinfo->yoffset * BOARD_LTDC_STRIDE;
  frame_end = fbaddr + LTDC_FRAME_SIZE;
  if (fbaddr < BOARD_LTDC_FB_BASE ||
      frame_end > BOARD_LTDC_FB_BASE + LTDC_FB_SIZE)
    {
      return -EINVAL;
    }

  g_ltdc.yoffset = pinfo->yoffset;

  putreg32((uint32_t)fbaddr, STM32H7RS_LTDC_L1CFBAR);
  putreg32(LTDC_SRCR_VBR, STM32H7RS_LTDC_SRCR);
  return OK;
}
#endif

#ifdef CONFIG_FB_SYNC
static int stm32h7rs_waitforvsync(FAR struct fb_vtable_s *vtable)
{
  uint32_t timeout = 1000000;
  bool timedout = false;

  if (vtable != &g_ltdc.vtable)
    {
      return -EINVAL;
    }

  while ((getreg32(STM32H7RS_LTDC_SRCR) & LTDC_SRCR_VBR) != 0)
    {
      if (timeout-- == 0)
        {
          timedout = true;
          break;
        }
    }

#ifdef CONFIG_STM32H7RS_LTDC_FB_DOUBLE_BUFFER
  while (fb_remove_paninfo(&g_ltdc.vtable, FB_NO_OVERLAY) == OK)
    {
    }
#endif

  if (timedout)
    {
      return -ETIMEDOUT;
    }

  return OK;
}
#endif

static int stm32h7rs_getpower(FAR struct fb_vtable_s *vtable)
{
  return g_ltdc.power;
}

static int stm32h7rs_setpower(FAR struct fb_vtable_s *vtable, int power)
{
  bool on = power > 0;

  if (power < 0)
    {
      return -EINVAL;
    }

  g_ltdc.power = power;
  stm32h7rs_ltdcpower(on);
  modifyreg32(STM32H7RS_LTDC_GCR, LTDC_GCR_LTDCEN,
              on ? LTDC_GCR_LTDCEN : 0);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void weak_function stm32h7rs_ltdcpower(bool on)
{
}

int stm32h7rs_ltdcinitialize(void)
{
  int ret;

  if (g_ltdc.initialized)
    {
      return OK;
    }

  ret = stm32h7rs_ltdc_clock_enable();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_ltdc_gpio_config();
  if (ret < 0)
    {
      return ret;
    }

  memset((FAR void *)BOARD_LTDC_FB_BASE, 0, LTDC_FB_SIZE);
  up_clean_dcache(BOARD_LTDC_FB_BASE,
                  BOARD_LTDC_FB_BASE + LTDC_FB_SIZE);

  stm32h7rs_ltdc_config();

  g_ltdc.yoffset = 0;
  g_ltdc.power = 1;
  g_ltdc.initialized = true;

  syslog(LOG_INFO, "stm32h7rs: LTDC framebuffer initialized\n");
  return OK;
}

FAR struct fb_vtable_s *stm32h7rs_ltdcgetvplane(int vplane)
{
  if (vplane != 0)
    {
      return NULL;
    }

  return &g_ltdc.vtable;
}

void stm32h7rs_ltdcuninitialize(void)
{
  if (g_ltdc.initialized)
    {
      stm32h7rs_setpower(&g_ltdc.vtable, 0);
      modifyreg32(STM32H7RS_RCC_APB5ENR, RCC_APB5ENR_LTDCEN, 0);
      g_ltdc.initialized = false;
    }
}

#endif /* CONFIG_STM32H7RS_DISPLAY */
