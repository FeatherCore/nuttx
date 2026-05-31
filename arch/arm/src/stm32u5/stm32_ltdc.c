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
#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>

#include <nuttx/cache.h>
#include <nuttx/clock.h>
#include <nuttx/irq.h>
#include <nuttx/semaphore.h>
#include <nuttx/video/fb.h>

#include "arm_internal.h"
#include "stm32_ltdc.h"

#include "hardware/stm32_ltdc.h"
#include "hardware/stm32u5xx_rcc.h"

#ifdef CONFIG_STM32U5_LTDC

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_FB_SYNC) && defined(CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER)
#  define LTDC_VSYNC_TIMEOUT   MSEC2TICK(100)
#  define LTDC_RELOAD_IRQ_MASK LTDC_IER_RRIE
#  define LTDC_ERROR_IRQ_MASK  (LTDC_IER_FUIE | LTDC_IER_TERRIE)
#  define LTDC_IRQ_MASK        (LTDC_RELOAD_IRQ_MASK | LTDC_ERROR_IRQ_MASK)
#  define LTDC_IRQ_CLEAR_MASK  (LTDC_ICR_CRRIF | LTDC_ICR_CTERRIF | \
                                LTDC_ICR_CFUIF | LTDC_ICR_CLIF)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32_ltdc_s
{
  struct fb_vtable_s vtable;
  struct stm32_ltdc_config_s config;
#if defined(CONFIG_FB_SYNC) && defined(CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER)
  sem_t waitsem;
  bool pan_removed;
  bool pan_remove_late;
  bool reload_pending;
  unsigned int timeout_count;
  unsigned int sync_warn_count;
#endif
  bool initialized;
  int error;
  uint32_t yoffset;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32_ltdc_getvideoinfo(FAR struct fb_vtable_s *vtable,
                                   FAR struct fb_videoinfo_s *vinfo);
static int stm32_ltdc_getplaneinfo(FAR struct fb_vtable_s *vtable,
                                   int planeno,
                                   FAR struct fb_planeinfo_s *pinfo);
#ifdef CONFIG_FB_UPDATE
static int stm32_ltdc_updatearea(FAR struct fb_vtable_s *vtable,
                                 FAR const struct fb_area_s *area);
#endif
#ifdef CONFIG_FB_SYNC
static int stm32_ltdc_waitforvsync(FAR struct fb_vtable_s *vtable);
#endif
static int stm32_ltdc_open(FAR struct fb_vtable_s *vtable);
static int stm32_ltdc_close(FAR struct fb_vtable_s *vtable);
static int stm32_ltdc_getpower(FAR struct fb_vtable_s *vtable);
static int stm32_ltdc_setpower(FAR struct fb_vtable_s *vtable, int power);
#ifdef CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER
static int stm32_ltdc_pandisplay(FAR struct fb_vtable_s *vtable,
                                 FAR struct fb_planeinfo_s *pinfo);
#endif
static int stm32_ltdc_ioctl(FAR struct fb_vtable_s *vtable, int cmd,
                            unsigned long arg);
#if defined(CONFIG_FB_SYNC) && defined(CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER)
static void stm32_ltdc_irq_enable(uint32_t mask);
static void stm32_ltdc_irq_disable(uint32_t mask);
static void stm32_ltdc_irq_clear(uint32_t mask);
static void stm32_ltdc_drain_paninfo(void);
static int stm32_ltdc_interrupt(int irq, FAR void *context, FAR void *arg);
static int stm32_ltdc_irqconfig(void);
#endif
static uint32_t stm32_ltdc_layer_stride(void);
static uint32_t stm32_ltdc_format(uint8_t bpp);
static uint8_t stm32_ltdc_fb_format(uint8_t bpp);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint32_t g_ltdc_error_count;

static struct stm32_ltdc_s g_ltdc =
{
  .vtable =
    {
      .getvideoinfo = stm32_ltdc_getvideoinfo,
      .getplaneinfo = stm32_ltdc_getplaneinfo,
#ifdef CONFIG_FB_UPDATE
      .updatearea   = stm32_ltdc_updatearea,
#endif
#ifdef CONFIG_FB_SYNC
      .waitforvsync = stm32_ltdc_waitforvsync,
#endif
#ifdef CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER
      .pandisplay   = stm32_ltdc_pandisplay,
#endif
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

static void stm32_ltdc_check_errors(FAR const char *where)
{
  uint32_t clear = 0;
  uint32_t error;
  uint32_t isr;

  isr = getreg32(STM32_LTDC_ISR);
  error = isr & (LTDC_ISR_FUIF | LTDC_ISR_TERRIF);
  if (error == 0)
    {
      return;
    }

  g_ltdc_error_count++;
  if (g_ltdc_error_count <= 16 || (g_ltdc_error_count % 64) == 0)
    {
      syslog(LOG_WARNING,
             "stm32u5: LTDC %s error=%08" PRIx32
             " isr=%08" PRIx32 " srcr=%08" PRIx32
             " cfb=%08" PRIx32 " count=%" PRIu32 "\n",
             where, error, isr, getreg32(STM32_LTDC_SRCR),
             getreg32(STM32_LTDC_L1CFBAR), g_ltdc_error_count);
    }

  if ((error & LTDC_ISR_FUIF) != 0)
    {
      clear |= LTDC_ICR_CFUIF;
    }

  if ((error & LTDC_ISR_TERRIF) != 0)
    {
      clear |= LTDC_ICR_CTERRIF;
    }

  putreg32(clear, STM32_LTDC_ICR);
}

static uint32_t stm32_ltdc_frame_size(void)
{
  return g_ltdc.config.stride * g_ltdc.config.yres;
}

static uint32_t stm32_ltdc_fb_count(void)
{
#ifdef CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER
  return 2;
#else
  return 1;
#endif
}

static uintptr_t stm32_ltdc_fbmem(uint8_t display)
{
#ifdef CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER
  if (display == 1 && g_ltdc.config.fb1_base != 0)
    {
      return g_ltdc.config.fb1_base;
    }
#endif

  return g_ltdc.config.fb_base + display * stm32_ltdc_frame_size();
}

static uint32_t stm32_ltdc_display_yoffset(uint8_t display)
{
  return display * g_ltdc.config.yres;
}

static void stm32_ltdc_clean(uint8_t display, uintptr_t start,
                             uintptr_t end)
{
  UNUSED(display);

  /* Clean exactly the fbdev range scanned by LTDC.  Board code may expose
   * either a GFXMMU virtual window or a direct PSRAM framebuffer here.
   */

  up_clean_dcache(start, end);
}

static int stm32_ltdc_refresh(void)
{
  return OK;
}

#if defined(CONFIG_FB_SYNC) && defined(CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER)
static void stm32_ltdc_irq_enable(uint32_t mask)
{
  modifyreg32(STM32_LTDC_IER, 0, mask);
}

static void stm32_ltdc_irq_disable(uint32_t mask)
{
  modifyreg32(STM32_LTDC_IER, mask, 0);
}

static void stm32_ltdc_irq_clear(uint32_t mask)
{
  putreg32(mask, STM32_LTDC_ICR);
}

static int stm32_ltdc_remove_paninfo(void)
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
      /* FBIOPAN_DISPLAY adds the queue entry after pandisplay() returns.
       * The register-reload interrupt can arrive before that entry exists.
       */

      g_ltdc.pan_remove_late = true;
    }

  return ret;
}

static void stm32_ltdc_drain_paninfo(void)
{
  while (fb_remove_paninfo(&g_ltdc.vtable, FB_NO_OVERLAY) == OK)
    {
    }

  g_ltdc.pan_removed = true;
  g_ltdc.pan_remove_late = false;
}

static int stm32_ltdc_interrupt(int irq, FAR void *context, FAR void *arg)
{
  uint32_t pending;
  uint32_t isr;
  bool done = false;

  isr = getreg32(STM32_LTDC_ISR);
  pending = isr & getreg32(STM32_LTDC_IER);

  if ((pending & LTDC_ISR_RRIF) != 0)
    {
      stm32_ltdc_irq_disable(LTDC_RELOAD_IRQ_MASK);
      stm32_ltdc_irq_clear(LTDC_ICR_CRRIF);
      stm32_ltdc_remove_paninfo();
      done = true;
    }

  if ((pending & (LTDC_ISR_FUIF | LTDC_ISR_TERRIF)) != 0)
    {
      g_ltdc.error = -EIO;
      stm32_ltdc_check_errors("irq");
    }

  if (done)
    {
      if (g_ltdc.error != -EIO)
        {
          g_ltdc.error = OK;
        }

      if (g_ltdc.reload_pending)
        {
          nxsem_post(&g_ltdc.waitsem);
        }
    }

  return OK;
}

static int stm32_ltdc_irqconfig(void)
{
  int ret;

  stm32_ltdc_irq_disable(LTDC_IRQ_MASK);
  stm32_ltdc_irq_clear(LTDC_IRQ_CLEAR_MASK);

  nxsem_init(&g_ltdc.waitsem, 0, 0);
  nxsem_set_protocol(&g_ltdc.waitsem, SEM_PRIO_NONE);

  ret = irq_attach(STM32_IRQ_LTDC, stm32_ltdc_interrupt, NULL);
  if (ret < 0)
    {
      nxsem_destroy(&g_ltdc.waitsem);
      return ret;
    }

  ret = irq_attach(STM32_IRQ_LTDC_ER, stm32_ltdc_interrupt, NULL);
  if (ret < 0)
    {
      irq_detach(STM32_IRQ_LTDC);
      nxsem_destroy(&g_ltdc.waitsem);
      return ret;
    }

  up_enable_irq(STM32_IRQ_LTDC);
  up_enable_irq(STM32_IRQ_LTDC_ER);
  return OK;
}
#endif

static uint32_t stm32_ltdc_layer_stride(void)
{
  return g_ltdc.config.layer_stride != 0 ?
         g_ltdc.config.layer_stride : g_ltdc.config.stride;
}

static uint32_t stm32_ltdc_format(uint8_t bpp)
{
  return bpp == 16 ? LTDC_LXPFCR_RGB565 : LTDC_LXPFCR_ARGB8888;
}

static uint8_t stm32_ltdc_fb_format(uint8_t bpp)
{
  return bpp == 16 ? FB_FMT_RGB16_565 : FB_FMT_RGB32;
}

static uint32_t stm32_ltdc_virtual_height(void)
{
  return g_ltdc.config.yres * stm32_ltdc_fb_count();
}

static int stm32_ltdc_y_to_fb(fb_coord_t y, FAR uint8_t *display,
                              FAR fb_coord_t *line)
{
  if (y < g_ltdc.config.yres)
    {
      *display = 0;
      *line = y;
      return OK;
    }

#ifdef CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER
  if (y < stm32_ltdc_virtual_height())
    {
      *display = y / g_ltdc.config.yres;
      *line = y - stm32_ltdc_display_yoffset(*display);
      return OK;
    }
#endif

  return -EINVAL;
}

static int stm32_ltdc_getvideoinfo(FAR struct fb_vtable_s *vtable,
                                   FAR struct fb_videoinfo_s *vinfo)
{
  if (vinfo == NULL)
    {
      return -EINVAL;
    }

  memset(vinfo, 0, sizeof(*vinfo));
  vinfo->fmt     = stm32_ltdc_fb_format(g_ltdc.config.bpp);
  vinfo->xres    = g_ltdc.config.xres;
  vinfo->yres    = g_ltdc.config.yres;
  vinfo->nplanes = 1;

  return OK;
}

static int stm32_ltdc_getplaneinfo(FAR struct fb_vtable_s *vtable,
                                   int planeno,
                                   FAR struct fb_planeinfo_s *pinfo)
{
  uint8_t display;

  if (pinfo == NULL)
    {
      return -EINVAL;
    }

  if (planeno != 0)
    {
      return -EINVAL;
    }

  display = pinfo->display;
  if (display >= stm32_ltdc_fb_count())
    {
      return -EINVAL;
    }

  memset(pinfo, 0, sizeof(*pinfo));
  pinfo->fbmem        = (FAR void *)stm32_ltdc_fbmem(display);
  pinfo->fblen        = stm32_ltdc_frame_size();
  pinfo->stride       = g_ltdc.config.stride;
  pinfo->display      = display;
  pinfo->bpp          = g_ltdc.config.bpp;
  pinfo->xres_virtual = g_ltdc.config.xres;
  pinfo->yres_virtual = stm32_ltdc_virtual_height();
  pinfo->xoffset      = 0;
  pinfo->yoffset      = stm32_ltdc_display_yoffset(display);

  return OK;
}

#ifdef CONFIG_FB_UPDATE
static int stm32_ltdc_updatearea(FAR struct fb_vtable_s *vtable,
                                 FAR const struct fb_area_s *area)
{
  uintptr_t start;
  uintptr_t end;
  fb_coord_t chunk;
  fb_coord_t line;
  int ret;
  uint8_t display;

  if (area == NULL ||
      area->x >= g_ltdc.config.xres ||
      stm32_ltdc_y_to_fb(area->y, &display, &line) < 0)
    {
      return -EINVAL;
    }

  if (area->w == 0 || area->h == 0)
    {
      return OK;
    }

  chunk = area->h;
  if (line + chunk > g_ltdc.config.yres)
    {
      chunk = g_ltdc.config.yres - line;
    }

  start = stm32_ltdc_fbmem(display) + line * g_ltdc.config.stride +
          area->x * (g_ltdc.config.bpp / 8);
  end = stm32_ltdc_fbmem(display) +
        (line + chunk) * g_ltdc.config.stride;

  stm32_ltdc_clean(display, start, end);
  stm32_ltdc_check_errors("update");
  ret = stm32_ltdc_refresh();
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}
#endif

#ifdef CONFIG_FB_SYNC
static int stm32_ltdc_waitforvsync(FAR struct fb_vtable_s *vtable)
{
#if defined(CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER)
  int ret;
#endif

  if (vtable != &g_ltdc.vtable)
    {
      return -EINVAL;
    }

#if defined(CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER)
  if (!g_ltdc.reload_pending)
    {
      return OK;
    }

  ret = nxsem_tickwait_uninterruptible(&g_ltdc.waitsem,
                                       LTDC_VSYNC_TIMEOUT);
  if (ret < 0)
    {
      stm32_ltdc_irq_disable(LTDC_IRQ_MASK);
      g_ltdc.reload_pending = false;
      g_ltdc.timeout_count++;

      if (g_ltdc.sync_warn_count < 8)
        {
          syslog(LOG_WARNING,
                 "stm32u5: LTDC irq sync timeout%u srcr=%08" PRIx32
                 " isr=%08" PRIx32 " ier=%08" PRIx32
                 " cpsr=%08" PRIx32 " cdsr=%08" PRIx32
                 " cfbar=%08" PRIx32 "\n",
                 g_ltdc.timeout_count, getreg32(STM32_LTDC_SRCR),
                 getreg32(STM32_LTDC_ISR), getreg32(STM32_LTDC_IER),
                 getreg32(STM32_LTDC_CPSR), getreg32(STM32_LTDC_CDSR),
                 getreg32(STM32_LTDC_L1CFBAR));
          g_ltdc.sync_warn_count++;
        }

      stm32_ltdc_drain_paninfo();
      return ret;
    }

  if (g_ltdc.pan_remove_late)
    {
      (void)stm32_ltdc_remove_paninfo();
    }

  if (g_ltdc.error < 0)
    {
      stm32_ltdc_drain_paninfo();
    }

  g_ltdc.reload_pending = false;
  stm32_ltdc_check_errors("wait");
  return g_ltdc.error;
#else
  stm32_ltdc_check_errors("wait");

  return OK;
#endif
}
#endif

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

#ifdef CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER
static int stm32_ltdc_pandisplay(FAR struct fb_vtable_s *vtable,
                                 FAR struct fb_planeinfo_s *pinfo)
{
  uintptr_t fbaddr;
  uintptr_t frame_end;
  fb_coord_t line;
  int ret;
  uint8_t display;

  if (vtable != &g_ltdc.vtable ||
      pinfo == NULL ||
      pinfo->bpp != g_ltdc.config.bpp ||
      pinfo->stride != g_ltdc.config.stride ||
      pinfo->xoffset != 0 ||
      stm32_ltdc_y_to_fb(pinfo->yoffset, &display, &line) < 0 ||
      line != 0)
    {
      return -EINVAL;
    }

  fbaddr = stm32_ltdc_fbmem(display);
  frame_end = fbaddr + stm32_ltdc_frame_size();

#if defined(CONFIG_FB_SYNC)
  if (g_ltdc.reload_pending ||
      (getreg32(STM32_LTDC_SRCR) & LTDC_SRCR_VBR) != 0)
    {
      if (g_ltdc.sync_warn_count < 4)
        {
          syslog(LOG_WARNING,
                 "stm32u5: LTDC pan busy pending=%u srcr=%08" PRIx32
                 " isr=%08" PRIx32 " ier=%08" PRIx32
                 " cpsr=%08" PRIx32 " cdsr=%08" PRIx32 "\n",
                 g_ltdc.reload_pending ? 1 : 0,
                 getreg32(STM32_LTDC_SRCR), getreg32(STM32_LTDC_ISR),
                 getreg32(STM32_LTDC_IER), getreg32(STM32_LTDC_CPSR),
                 getreg32(STM32_LTDC_CDSR));
          g_ltdc.sync_warn_count++;
        }

      return -EBUSY;
    }

  stm32_ltdc_drain_paninfo();

  while (nxsem_trywait(&g_ltdc.waitsem) == OK)
    {
    }

  g_ltdc.error = -ETIMEDOUT;
  g_ltdc.pan_removed = false;
  g_ltdc.pan_remove_late = false;
  g_ltdc.reload_pending = true;

  stm32_ltdc_irq_clear(LTDC_IRQ_CLEAR_MASK);
  stm32_ltdc_irq_enable(LTDC_IRQ_MASK);
#endif

  stm32_ltdc_clean(display, fbaddr, frame_end);
  g_ltdc.yoffset = pinfo->yoffset;

  stm32_ltdc_check_errors("pan");
  putreg32(fbaddr, STM32_LTDC_L1CFBAR);
  putreg32(LTDC_SRCR_VBR, STM32_LTDC_SRCR);
  ret = stm32_ltdc_refresh();
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}
#endif

static int stm32_ltdc_ioctl(FAR struct fb_vtable_s *vtable, int cmd,
                            unsigned long arg)
{
  return -ENOTSUP;
}

static int stm32_ltdc_validate(const struct stm32_ltdc_config_s *config)
{
  uint32_t linebytes;
  uint32_t layer_stride;

  if (config == NULL || config->fb_base == 0 || config->layer_fb_base == 0 ||
      config->xres == 0 || config->yres == 0 || config->stride == 0 ||
      (config->bpp != 16 && config->bpp != 32) || config->fb_size == 0)
    {
      return -EINVAL;
    }

  linebytes = config->xres * (config->bpp / 8);
  layer_stride = config->layer_stride != 0 ?
                 config->layer_stride : config->stride;
  if (config->stride < linebytes || layer_stride < linebytes)
    {
      return -EINVAL;
    }

  if (config->fb_size < config->stride * config->yres)
    {
      return -EINVAL;
    }

#ifdef CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER
  if (config->fb1_base == 0 &&
      config->fb_size < config->stride * config->yres *
                        stm32_ltdc_fb_count())
    {
      return -EINVAL;
    }
#endif

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void stm32_ltdcuninitialize(void)
{
#if defined(CONFIG_FB_SYNC) && defined(CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER)
  if (g_ltdc.initialized)
    {
      stm32_ltdc_irq_disable(LTDC_IRQ_MASK);
      up_disable_irq(STM32_IRQ_LTDC);
      up_disable_irq(STM32_IRQ_LTDC_ER);
      irq_detach(STM32_IRQ_LTDC);
      irq_detach(STM32_IRQ_LTDC_ER);
      nxsem_destroy(&g_ltdc.waitsem);
    }
#endif

  modifyreg32(STM32_LTDC_GCR, LTDC_GCR_LTDCEN, 0);
  modifyreg32(STM32_RCC_APB2ENR, RCC_APB2ENR_LTDCEN, 0);
  g_ltdc.initialized = false;
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
  uint32_t pitchbytes;
  int ret;

  if (g_ltdc.initialized)
    {
      return OK;
    }

  ret = stm32_ltdc_validate(config);
  if (ret < 0)
    {
      return ret;
    }

  memcpy(&g_ltdc.config, config, sizeof(g_ltdc.config));
  g_ltdc.yoffset = 0;

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
  wvst      = avbp + config->window_y0 + 1;
  wvsp      = avbp + config->window_y0 + config->yres;
  linebytes = config->xres * (config->bpp / 8);
  pitchbytes = stm32_ltdc_layer_stride();

  putreg32(0, STM32_LTDC_L1CR);
  putreg32(LTDC_LXWHPCR_WHSP(whsp) | LTDC_LXWHPCR_WHST(whst),
           STM32_LTDC_L1WHPCR);
  putreg32(LTDC_LXWVPCR_WVSP(wvsp) | LTDC_LXWVPCR_WVST(wvst),
           STM32_LTDC_L1WVPCR);
  putreg32(0, STM32_LTDC_L1CKCR);
  putreg32(stm32_ltdc_format(config->bpp), STM32_LTDC_L1PFCR);
  putreg32(LTDC_LXCACR_CONSTA(0xff), STM32_LTDC_L1CACR);
  putreg32(0, STM32_LTDC_L1DCCR);

  /* The framebuffer is the final display surface, so ignore pixel alpha. */

  putreg32(LTDC_LXBFCR_BF1(LTDC_LXBFCR_CA) |
           LTDC_LXBFCR_BF2(LTDC_LXBFCR_1CA), STM32_LTDC_L1BFCR);
  putreg32(config->layer_fb_base, STM32_LTDC_L1CFBAR);
  putreg32(LTDC_LXCFBLR_CFBP(pitchbytes) |
           LTDC_LXCFBLR_CFBLL(linebytes + 3), STM32_LTDC_L1CFBLR);
  putreg32(config->yres, STM32_LTDC_L1CFBLNR);
  putreg32(LTDC_LXCR_LEN, STM32_LTDC_L1CR);

  putreg32(LTDC_SRCR_IMR, STM32_LTDC_SRCR);
  modifyreg32(STM32_LTDC_GCR, 0, LTDC_GCR_LTDCEN);

#if defined(CONFIG_FB_SYNC) && defined(CONFIG_STM32U5_LTDC_FB_DOUBLE_BUFFER)
  ret = stm32_ltdc_irqconfig();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5: LTDC IRQ config failed: %d\n", ret);
      modifyreg32(STM32_LTDC_GCR, LTDC_GCR_LTDCEN, 0);
      return ret;
    }

  syslog(LOG_INFO,
         "stm32u5: LTDC double-buffer global-vbr irq sync enabled\n");
#endif

  g_ltdc.initialized = true;
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
