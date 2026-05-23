/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_ltdc.c
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
#include <nuttx/clock.h>
#include <nuttx/compiler.h>
#include <nuttx/irq.h>
#include <nuttx/semaphore.h>
#include <nuttx/video/fb.h>

#include "arm_internal.h"

#include "hardware/stm32n6_ltdc.h"
#include "hardware/stm32n6_memorymap.h"
#include "hardware/stm32n6_rcc.h"

#include "stm32n6_gpio.h"
#include "stm32n6_ltdc.h"

#ifdef CONFIG_STM32N6_DISPLAY

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
#  error STM32N6 LTDC currently supports only RGB565 framebuffer output
#endif

#ifndef BOARD_LTDC_STRIDE
#  define BOARD_LTDC_STRIDE ((BOARD_LTDC_WIDTH * BOARD_LTDC_BPP + 7) / 8)
#endif

#ifndef BOARD_LTDC_FB_SIZE
#  define BOARD_LTDC_FB_SIZE (BOARD_LTDC_STRIDE * BOARD_LTDC_HEIGHT)
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

#ifndef BOARD_LTDC_CLOCK_DIV
#  error BOARD_LTDC_CLOCK_DIV must be defined in board.h
#endif

#define LTDC_BYTES_PER_PIXEL    (BOARD_LTDC_BPP / 8)
#define LTDC_FRAME_SIZE         (BOARD_LTDC_STRIDE * BOARD_LTDC_HEIGHT)

#ifdef CONFIG_STM32N6_LTDC_FB_DOUBLE_BUFFER
#  define LTDC_FB_COUNT         2u
#else
#  define LTDC_FB_COUNT         1u
#endif

#define LTDC_FB_SIZE            (LTDC_FRAME_SIZE * LTDC_FB_COUNT)
#define LTDC_VIRTUAL_HEIGHT     (BOARD_LTDC_HEIGHT * LTDC_FB_COUNT)
#define LTDC_VSYNC_TIMEOUT      MSEC2TICK(100)
#define LTDC_IRQ_MASK           (LTDC_IER_RRIE | LTDC_IER_TERRIE | \
                                 LTDC_IER_FUIE)
#define LTDC_IRQ_CLEAR_MASK     (LTDC_ICR_CRRIF | LTDC_ICR_CTERRIF | \
                                 LTDC_ICR_CFUIF | LTDC_ICR_CFUWIF | \
                                 LTDC_ICR_CLIF | LTDC_ICR_CCRCIF)

#if BOARD_LTDC_FB_SIZE < LTDC_FB_SIZE
#  error BOARD_LTDC_FB_SIZE is too small for the configured LTDC buffers
#endif

#define RIFSC_RISC_SECCFGR(n)   (STM32N6_RIFSC_BASE + 0x0010 + \
                                 ((uintptr_t)(n) << 2))
#define RIFSC_RISC_PRIVCFGR(n)  (STM32N6_RIFSC_BASE + 0x0030 + \
                                 ((uintptr_t)(n) << 2))
#define RIFSC_RIMC_ATTR(n)      (STM32N6_RIFSC_BASE + 0x0c10 + \
                                 ((uintptr_t)(n) << 2))

#define RIF_MASTER_LTDC1        10
#define RIF_MASTER_LTDC2        11
#define RIF_MCID_1              (1 << 4)
#define RIF_MCID_MASK           (7 << 4)
#define RIF_MSEC                (1 << 8)
#define RIF_MPRIV               (1 << 9)
#define RIF_RIMC_SEC_PRIV_MASK  (RIF_MCID_MASK | RIF_MSEC | RIF_MPRIV)
#define RIF_RISC_LTDC_REG       3
#define RIF_RISC_LTDC_BIT       (1 << 6)
#define RIF_RISC_LTDCL1_BIT     (1 << 7)
#define RIF_RISC_LTDCL2_BIT     (1 << 8)
#define RIF_RISC_LTDC_BITS      (RIF_RISC_LTDC_BIT | RIF_RISC_LTDCL1_BIT | \
                                 RIF_RISC_LTDCL2_BIT)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_ltdc_s
{
  struct fb_vtable_s vtable;
  sem_t waitsem;
  bool initialized;
#ifdef CONFIG_STM32N6_LTDC_FB_DOUBLE_BUFFER
  bool pan_removed;
  bool pan_remove_late;
  bool reload_pending;
  unsigned int timeout_count;
  unsigned int sync_warn_count;
#endif
  int power;
  int error;
  uint32_t yoffset;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32n6_getvideoinfo(FAR struct fb_vtable_s *vtable,
                                FAR struct fb_videoinfo_s *vinfo);
static int stm32n6_getplaneinfo(FAR struct fb_vtable_s *vtable,
                                int planeno,
                                FAR struct fb_planeinfo_s *pinfo);
#ifdef CONFIG_FB_UPDATE
static int stm32n6_updatearea(FAR struct fb_vtable_s *vtable,
                              FAR const struct fb_area_s *area);
#endif
#if defined(CONFIG_FB_SYNC) && defined(CONFIG_STM32N6_LTDC_FB_DOUBLE_BUFFER)
static int stm32n6_waitforvsync(FAR struct fb_vtable_s *vtable);
#endif
static int stm32n6_getpower(FAR struct fb_vtable_s *vtable);
static int stm32n6_setpower(FAR struct fb_vtable_s *vtable, int power);
#ifdef CONFIG_STM32N6_LTDC_FB_DOUBLE_BUFFER
static void stm32n6_ltdc_irq_enable(uint32_t mask);
static void stm32n6_ltdc_irq_disable(uint32_t mask);
static void stm32n6_ltdc_irq_clear(uint32_t mask);
static void stm32n6_ltdc_drain_paninfo(void);
static int stm32n6_ltdc_interrupt(int irq, FAR void *context,
                                  FAR void *arg);
static int stm32n6_ltdc_irqconfig(void);
static int stm32n6_pandisplay(FAR struct fb_vtable_s *vtable,
                              FAR struct fb_planeinfo_s *pinfo);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32n6_ltdc_s g_ltdc =
{
  .vtable =
    {
      .getvideoinfo = stm32n6_getvideoinfo,
      .getplaneinfo = stm32n6_getplaneinfo,
#ifdef CONFIG_FB_UPDATE
      .updatearea   = stm32n6_updatearea,
#endif
#if defined(CONFIG_FB_SYNC) && defined(CONFIG_STM32N6_LTDC_FB_DOUBLE_BUFFER)
      .waitforvsync = stm32n6_waitforvsync,
#endif
#ifdef CONFIG_STM32N6_LTDC_FB_DOUBLE_BUFFER
      .pandisplay   = stm32n6_pandisplay,
#endif
      .getpower     = stm32n6_getpower,
      .setpower     = stm32n6_setpower,
    },
  .power = 1,
};

static const uint32_t g_ltdcpins[] =
{
  GPIO_LTDC_R0, GPIO_LTDC_R1, GPIO_LTDC_R2, GPIO_LTDC_R3,
  GPIO_LTDC_R4, GPIO_LTDC_R5, GPIO_LTDC_R6, GPIO_LTDC_R7,
  GPIO_LTDC_G0, GPIO_LTDC_G1, GPIO_LTDC_G2, GPIO_LTDC_G3,
  GPIO_LTDC_G4, GPIO_LTDC_G5, GPIO_LTDC_G6, GPIO_LTDC_G7,
#ifdef GPIO_LTDC_B0
  GPIO_LTDC_B0,
#endif
  GPIO_LTDC_B1, GPIO_LTDC_B2, GPIO_LTDC_B3, GPIO_LTDC_B4,
  GPIO_LTDC_B5, GPIO_LTDC_B6, GPIO_LTDC_B7, GPIO_LTDC_HSYNC,
  GPIO_LTDC_VSYNC, GPIO_LTDC_CLK,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32n6_ltdc_gpio_config(void)
{
  unsigned int i;
  int ret;

  for (i = 0; i < sizeof(g_ltdcpins) / sizeof(g_ltdcpins[0]); i++)
    {
      ret = stm32n6_configgpio(g_ltdcpins[i]);
      if (ret < 0)
        {
          return ret;
        }
    }

  return OK;
}

static int stm32n6_ltdc_clock_enable(void)
{
  uint32_t timeout = 1000000;

  putreg32(RCC_ICCFGR(RCC_ICCFGR_SEL_PLL1, BOARD_LTDC_CLOCK_DIV),
           STM32N6_RCC_IC16CFGR);
  putreg32(RCC_DIVENSR_IC16ENS, STM32N6_RCC_DIVENSR);

  while ((getreg32(STM32N6_RCC_DIVENR) & RCC_DIVENR_IC16EN) == 0)
    {
      if (timeout-- == 0)
        {
          return -ETIMEDOUT;
        }
    }

  modifyreg32(STM32N6_RCC_CCIPR4, RCC_CCIPR4_LTDCSEL_MASK,
              RCC_CCIPR4_LTDCSEL_IC16);

  putreg32(RCC_APB5ENSR_LTDCENS, STM32N6_RCC_APB5ENSR);
  (void)getreg32(STM32N6_RCC_APB5ENR);

  putreg32(RCC_APB5RSTR_LTDCRST, STM32N6_RCC_APB5RSTR);
  up_udelay(1);
  putreg32(RCC_APB5RSTR_LTDCRST, STM32N6_RCC_APB5RSTCR);

  return OK;
}

static void stm32n6_ltdc_rif_master_config(unsigned int master)
{
  uint32_t regval;

  regval = getreg32(RIFSC_RIMC_ATTR(master));
  regval &= ~RIF_RIMC_SEC_PRIV_MASK;
  regval |= RIF_MCID_1 | RIF_MSEC | RIF_MPRIV;
  putreg32(regval, RIFSC_RIMC_ATTR(master));
}

static void stm32n6_ltdc_rif_config(void)
{
  putreg32(RCC_AHB3ENSR_RIFSCENS, STM32N6_RCC_AHB3ENSR);
  (void)getreg32(STM32N6_RCC_AHB3ENR);

  stm32n6_ltdc_rif_master_config(RIF_MASTER_LTDC1);
  stm32n6_ltdc_rif_master_config(RIF_MASTER_LTDC2);

  modifyreg32(RIFSC_RISC_SECCFGR(RIF_RISC_LTDC_REG), 0,
              RIF_RISC_LTDC_BITS);
  modifyreg32(RIFSC_RISC_PRIVCFGR(RIF_RISC_LTDC_REG), 0,
              RIF_RISC_LTDC_BITS);
}

static bool stm32n6_ltdc_layer_reload(uint32_t reload)
{
  uint32_t timeout = 100000;

  putreg32(reload | LTDC_LRCR_GRMSK, STM32N6_LTDC_LRCR);

  while ((getreg32(STM32N6_LTDC_LRCR) & reload) != 0)
    {
      if (timeout-- == 0)
        {
          return false;
        }
    }

  return true;
}

static bool stm32n6_ltdc_layer_enable(void)
{
  modifyreg32(STM32N6_LTDC_LCR, 0, LTDC_LCR_LEN);
  if (!stm32n6_ltdc_layer_reload(LTDC_LRCR_IMR))
    {
      return false;
    }

  return (getreg32(STM32N6_LTDC_LCR) & LTDC_LCR_LEN) != 0;
}

static void stm32n6_ltdc_config(void)
{
  uint32_t hsync = BOARD_LTDC_HSYNC - 1;
  uint32_t hbp = BOARD_LTDC_HSYNC + BOARD_LTDC_HBP - 1;
  uint32_t activew = BOARD_LTDC_HSYNC + BOARD_LTDC_WIDTH +
                     BOARD_LTDC_HBP - 1;
  uint32_t totalw = BOARD_LTDC_HSYNC + BOARD_LTDC_WIDTH +
                    BOARD_LTDC_HBP + BOARD_LTDC_HFP - 1;
  uint32_t vsync = BOARD_LTDC_VSYNC - 1;
  uint32_t vbp = BOARD_LTDC_VSYNC + BOARD_LTDC_VBP - 1;
  uint32_t activeh = BOARD_LTDC_VSYNC + BOARD_LTDC_HEIGHT +
                     BOARD_LTDC_VBP - 1;
  uint32_t totalh = BOARD_LTDC_VSYNC + BOARD_LTDC_HEIGHT +
                    BOARD_LTDC_VBP + BOARD_LTDC_VFP - 1;
  uint32_t xstart = hbp + 1;
  uint32_t xstop = xstart + BOARD_LTDC_WIDTH - 1;
  uint32_t ystart = vbp + 1;
  uint32_t ystop = ystart + BOARD_LTDC_HEIGHT - 1;

  putreg32(0, STM32N6_LTDC_GCR);
  putreg32(0xffffffff, STM32N6_LTDC_ICR);
  putreg32((hsync << 16) | vsync, STM32N6_LTDC_SSCR);
  putreg32((hbp << 16) | vbp, STM32N6_LTDC_BPCR);
  putreg32((activew << 16) | activeh, STM32N6_LTDC_AWCR);
  putreg32((totalw << 16) | totalh, STM32N6_LTDC_TWCR);
  putreg32(0, STM32N6_LTDC_BCCR);

  putreg32(LTDC_LRCR_GRMSK, STM32N6_LTDC_LRCR);
  putreg32(LTDC_GCR_LTDCEN, STM32N6_LTDC_GCR);

  putreg32(0, STM32N6_LTDC_LCR);
  putreg32((xstop << 16) | xstart, STM32N6_LTDC_LWHPCR);
  putreg32((ystop << 16) | ystart, STM32N6_LTDC_LWVPCR);
  putreg32(LTDC_PIXEL_FORMAT_RGB565, STM32N6_LTDC_LPFCR);
  putreg32(0, STM32N6_LTDC_LFPF0R);
  putreg32(0, STM32N6_LTDC_LFPF1R);
  putreg32(LTDC_FULL_ALPHA, STM32N6_LTDC_LCACR);
  putreg32(0, STM32N6_LTDC_LDCCR);
  putreg32(LTDC_BLENDING_FACTOR1_CA | LTDC_BLENDING_FACTOR2_CA,
           STM32N6_LTDC_LBFCR);
  putreg32(0, STM32N6_LTDC_LBLCR);
  putreg32(0, STM32N6_LTDC_LPCR);
  putreg32(BOARD_LTDC_FB_BASE, STM32N6_LTDC_LCFBAR);
  putreg32((BOARD_LTDC_STRIDE << 16) | (BOARD_LTDC_STRIDE + 7),
           STM32N6_LTDC_LCFBLR);
  putreg32(BOARD_LTDC_HEIGHT, STM32N6_LTDC_LCFBLNR);

  if (!stm32n6_ltdc_layer_enable())
    {
      syslog(LOG_WARNING,
             "stm32n6: LTDC layer enable did not latch cr=%08lx "
             "rcr=%08lx\n",
             (unsigned long)getreg32(STM32N6_LTDC_LCR),
             (unsigned long)getreg32(STM32N6_LTDC_LRCR));
    }

  putreg32(LTDC_SRCR_IMR, STM32N6_LTDC_SRCR);
}

static int stm32n6_getvideoinfo(FAR struct fb_vtable_s *vtable,
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

static int stm32n6_getplaneinfo(FAR struct fb_vtable_s *vtable,
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
static int stm32n6_updatearea(FAR struct fb_vtable_s *vtable,
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

  if (area->h > LTDC_VIRTUAL_HEIGHT - area->y)
    {
      yend = LTDC_VIRTUAL_HEIGHT;
    }
  else
    {
      yend = area->y + area->h;
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

#ifdef CONFIG_STM32N6_LTDC_FB_DOUBLE_BUFFER
static void stm32n6_ltdc_irq_enable(uint32_t mask)
{
  modifyreg32(STM32N6_LTDC_IER, 0, mask);
  modifyreg32(STM32N6_LTDC_IER2, 0, mask);
}

static void stm32n6_ltdc_irq_disable(uint32_t mask)
{
  modifyreg32(STM32N6_LTDC_IER, mask, 0);
  modifyreg32(STM32N6_LTDC_IER2, mask, 0);
}

static void stm32n6_ltdc_irq_clear(uint32_t mask)
{
  putreg32(mask, STM32N6_LTDC_ICR);
  putreg32(mask, STM32N6_LTDC_ICR2);
}

static int stm32n6_ltdc_remove_paninfo(void)
{
  int ret;

  if (g_ltdc.pan_removed)
    {
      return OK;
    }

  ret = fb_remove_paninfo(&g_ltdc.vtable, FB_NO_OVERLAY);
  if (ret == OK)
    {
      g_ltdc.pan_removed = true;
      g_ltdc.pan_remove_late = false;
    }
  else
    {
      /* FBIOPAN_DISPLAY adds the pan queue entry after pandisplay()
       * returns.  The reload interrupt can therefore arrive before the
       * queue entry exists; complete the queue removal in waitforvsync().
       */

      g_ltdc.pan_remove_late = true;
    }

  return ret;
}

static void stm32n6_ltdc_drain_paninfo(void)
{
  while (fb_remove_paninfo(&g_ltdc.vtable, FB_NO_OVERLAY) == OK)
    {
    }

  g_ltdc.pan_removed = true;
  g_ltdc.pan_remove_late = false;
}

static int stm32n6_ltdc_interrupt(int irq, FAR void *context,
                                  FAR void *arg)
{
  uint32_t pending;
  uint32_t isr;
  uint32_t isr2;
  int error = OK;
  bool done = false;

  isr = getreg32(STM32N6_LTDC_ISR);
  isr2 = getreg32(STM32N6_LTDC_ISR2);
  pending = (isr & getreg32(STM32N6_LTDC_IER)) |
            (isr2 & getreg32(STM32N6_LTDC_IER2));

  if ((pending & LTDC_ISR_RRIF) != 0)
    {
      stm32n6_ltdc_irq_disable(LTDC_IER_RRIE);
      stm32n6_ltdc_irq_clear(LTDC_ICR_CRRIF);
      stm32n6_ltdc_remove_paninfo();
      done = true;
    }

  if ((pending & LTDC_ISR_TERRIF) != 0)
    {
      stm32n6_ltdc_irq_clear(LTDC_ICR_CTERRIF);
      error = -EIO;
      done = true;
    }

  if ((pending & LTDC_ISR_FUIF) != 0)
    {
      stm32n6_ltdc_irq_clear(LTDC_ICR_CFUIF);
      error = -EIO;
      done = true;
    }

  if (done)
    {
      stm32n6_ltdc_irq_disable(LTDC_IRQ_MASK);
      g_ltdc.error = error;

      if (g_ltdc.reload_pending)
        {
          nxsem_post(&g_ltdc.waitsem);
        }
    }

  return OK;
}

static int stm32n6_ltdc_irqconfig(void)
{
  int ret;

  stm32n6_ltdc_irq_disable(LTDC_IRQ_MASK);
  stm32n6_ltdc_irq_clear(LTDC_IRQ_CLEAR_MASK);

  nxsem_init(&g_ltdc.waitsem, 0, 0);
  nxsem_set_protocol(&g_ltdc.waitsem, SEM_PRIO_NONE);

  ret = irq_attach(STM32_IRQ_LTDC_LO, stm32n6_ltdc_interrupt, NULL);
  if (ret < 0)
    {
      nxsem_destroy(&g_ltdc.waitsem);
      return ret;
    }

  ret = irq_attach(STM32_IRQ_LTDC_UP, stm32n6_ltdc_interrupt, NULL);
  if (ret < 0)
    {
      irq_detach(STM32_IRQ_LTDC_LO);
      nxsem_destroy(&g_ltdc.waitsem);
      return ret;
    }

  up_enable_irq(STM32_IRQ_LTDC_LO);
  up_enable_irq(STM32_IRQ_LTDC_UP);
  return OK;
}

static int stm32n6_pandisplay(FAR struct fb_vtable_s *vtable,
                              FAR struct fb_planeinfo_s *pinfo)
{
  uintptr_t fbaddr;
  uintptr_t fbend;
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
  fbend = BOARD_LTDC_FB_BASE + LTDC_FB_SIZE;
  frame_end = fbaddr + LTDC_FRAME_SIZE;

  if (fbaddr < BOARD_LTDC_FB_BASE || frame_end > fbend)
    {
      return -EINVAL;
    }

  if (g_ltdc.reload_pending ||
      (getreg32(STM32N6_LTDC_SRCR) & LTDC_SRCR_VBR) != 0)
    {
      if (g_ltdc.sync_warn_count < 4)
        {
          syslog(LOG_WARNING,
                 "stm32n6: LTDC pan busy pending=%u srcr=%08lx "
                 "isr=%08lx ier=%08lx isr2=%08lx ier2=%08lx\n",
                 g_ltdc.reload_pending ? 1 : 0,
                 (unsigned long)getreg32(STM32N6_LTDC_SRCR),
                 (unsigned long)getreg32(STM32N6_LTDC_ISR),
                 (unsigned long)getreg32(STM32N6_LTDC_IER),
                 (unsigned long)getreg32(STM32N6_LTDC_ISR2),
                 (unsigned long)getreg32(STM32N6_LTDC_IER2));
          g_ltdc.sync_warn_count++;
        }

      return -EBUSY;
    }

  stm32n6_ltdc_drain_paninfo();

  while (nxsem_trywait(&g_ltdc.waitsem) == OK)
    {
    }

  g_ltdc.error = -ETIMEDOUT;
  g_ltdc.pan_removed = false;
  g_ltdc.pan_remove_late = false;
  g_ltdc.reload_pending = true;

  stm32n6_ltdc_irq_clear(LTDC_IRQ_CLEAR_MASK);
  stm32n6_ltdc_irq_enable(LTDC_IRQ_MASK);

  g_ltdc.yoffset = pinfo->yoffset;

  putreg32((uint32_t)fbaddr, STM32N6_LTDC_LCFBAR);
  putreg32(LTDC_SRCR_VBR, STM32N6_LTDC_SRCR);

  return OK;
}
#endif

#if defined(CONFIG_FB_SYNC) && defined(CONFIG_STM32N6_LTDC_FB_DOUBLE_BUFFER)
static int stm32n6_waitforvsync(FAR struct fb_vtable_s *vtable)
{
  int ret;

  if (vtable != &g_ltdc.vtable)
    {
      return -EINVAL;
    }

  if (!g_ltdc.reload_pending)
    {
      return OK;
    }

  ret = nxsem_tickwait_uninterruptible(&g_ltdc.waitsem,
                                       LTDC_VSYNC_TIMEOUT);
  if (ret < 0)
    {
      stm32n6_ltdc_irq_disable(LTDC_IRQ_MASK);
      g_ltdc.reload_pending = false;
      g_ltdc.timeout_count++;

      if (g_ltdc.sync_warn_count < 8)
        {
          syslog(LOG_WARNING,
                 "stm32n6: LTDC irq sync timeout%u srcr=%08lx lrcr=%08lx "
                 "isr=%08lx ier=%08lx isr2=%08lx ier2=%08lx cpsr=%08lx "
                 "cdsr=%08lx cfbar=%08lx\n",
                 g_ltdc.timeout_count,
                 (unsigned long)getreg32(STM32N6_LTDC_SRCR),
                 (unsigned long)getreg32(STM32N6_LTDC_LRCR),
                 (unsigned long)getreg32(STM32N6_LTDC_ISR),
                 (unsigned long)getreg32(STM32N6_LTDC_IER),
                 (unsigned long)getreg32(STM32N6_LTDC_ISR2),
                 (unsigned long)getreg32(STM32N6_LTDC_IER2),
                 (unsigned long)getreg32(STM32N6_LTDC_CPSR),
                 (unsigned long)getreg32(STM32N6_LTDC_CDSR),
                 (unsigned long)getreg32(STM32N6_LTDC_LCFBAR));
          g_ltdc.sync_warn_count++;
        }

      stm32n6_ltdc_drain_paninfo();
      return ret;
    }

  if (g_ltdc.pan_remove_late)
    {
      (void)stm32n6_ltdc_remove_paninfo();
    }

  g_ltdc.reload_pending = false;

  return g_ltdc.error;
}
#endif

static int stm32n6_getpower(FAR struct fb_vtable_s *vtable)
{
  return g_ltdc.power;
}

static int stm32n6_setpower(FAR struct fb_vtable_s *vtable, int power)
{
  bool on = power > 0;

  g_ltdc.power = power;
  stm32n6_ltdcpower(on);

  if (on)
    {
      modifyreg32(STM32N6_LTDC_GCR, 0, LTDC_GCR_LTDCEN);
    }
  else
    {
      modifyreg32(STM32N6_LTDC_GCR, LTDC_GCR_LTDCEN, 0);
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void weak_function stm32n6_ltdcpower(bool on)
{
}

int stm32n6_ltdcinitialize(void)
{
  int ret;

  if (g_ltdc.initialized)
    {
      return OK;
    }

  ret = stm32n6_ltdc_clock_enable();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32n6: LTDC clock enable failed: %d\n", ret);
      return ret;
    }

  stm32n6_ltdc_rif_config();

  ret = stm32n6_ltdc_gpio_config();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32n6: LTDC GPIO config failed: %d\n", ret);
      return ret;
    }

  memset((FAR void *)BOARD_LTDC_FB_BASE, 0, LTDC_FB_SIZE);
  up_clean_dcache(BOARD_LTDC_FB_BASE,
                  BOARD_LTDC_FB_BASE + LTDC_FB_SIZE);

  stm32n6_ltdc_config();

#ifdef CONFIG_STM32N6_LTDC_FB_DOUBLE_BUFFER
  ret = stm32n6_ltdc_irqconfig();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32n6: LTDC IRQ config failed: %d\n", ret);
      return ret;
    }

  syslog(LOG_INFO,
         "stm32n6: LTDC double-buffer global-vbr irq sync enabled\n");
#endif

  g_ltdc.yoffset = 0;
  g_ltdc.power = 1;
  g_ltdc.initialized = true;
  return OK;
}

FAR struct fb_vtable_s *stm32n6_ltdcgetvplane(int vplane)
{
  if (vplane != 0)
    {
      return NULL;
    }

  return &g_ltdc.vtable;
}

void stm32n6_ltdcuninitialize(void)
{
  if (g_ltdc.initialized)
    {
#ifdef CONFIG_STM32N6_LTDC_FB_DOUBLE_BUFFER
      stm32n6_ltdc_irq_disable(LTDC_IRQ_MASK);
      up_disable_irq(STM32_IRQ_LTDC_LO);
      up_disable_irq(STM32_IRQ_LTDC_UP);
      irq_detach(STM32_IRQ_LTDC_LO);
      irq_detach(STM32_IRQ_LTDC_UP);
      nxsem_destroy(&g_ltdc.waitsem);
#endif

      stm32n6_setpower(&g_ltdc.vtable, 0);
      g_ltdc.initialized = false;
    }
}

#endif /* CONFIG_STM32N6_DISPLAY */
