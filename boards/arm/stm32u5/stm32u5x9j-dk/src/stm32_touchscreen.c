/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32_touchscreen.c
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

#include <nuttx/clock.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/input/touchscreen.h>
#include <nuttx/wqueue.h>

#include "stm32_exti.h"
#include "stm32_gpio.h"
#include "stm32u5x9j-dk.h"

#ifdef CONFIG_STM32U5X9J_DK_TOUCH

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SITRONIX_ADDR        0x70
#define SITRONIX_BUFSIZE     28
#define SITRONIX_POLL_MSEC   30
#define SITRONIX_CONTACT     0x80
#define SITRONIX_XRES        480
#define SITRONIX_YRES        480

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32u5x9j_touch_s
{
  struct touch_lowerhalf_s lower;
  FAR struct i2c_master_s *i2c;
  struct work_s work;
  bool open;
  bool down;
  uint8_t id;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32u5x9j_touch_open(FAR struct touch_lowerhalf_s *lower);
static int stm32u5x9j_touch_close(FAR struct touch_lowerhalf_s *lower);
static int stm32u5x9j_touch_control(FAR struct touch_lowerhalf_s *lower,
                                    int cmd, unsigned long arg);
static int stm32u5x9j_touch_irq(int irq, FAR void *context, FAR void *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32u5x9j_touch_s g_touch =
{
  .lower =
    {
      .maxpoint = 1,
      .xres     = SITRONIX_XRES,
      .yres     = SITRONIX_YRES,
      .control  = stm32u5x9j_touch_control,
      .open     = stm32u5x9j_touch_open,
      .close    = stm32u5x9j_touch_close,
    }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32u5x9j_touch_read(FAR struct stm32u5x9j_touch_s *priv,
                                 FAR uint8_t *buf, size_t len)
{
  struct i2c_msg_s msg;

  msg.frequency = I2C_SPEED_FAST;
  msg.addr      = SITRONIX_ADDR;
  msg.flags     = I2C_M_READ;
  msg.buffer    = buf;
  msg.length    = len;

  return I2C_TRANSFER(priv->i2c, &msg, 1);
}

static void stm32u5x9j_touch_worker(FAR void *arg)
{
  FAR struct stm32u5x9j_touch_s *priv = arg;
  struct touch_sample_s sample;
  uint8_t buf[SITRONIX_BUFSIZE];
  uint8_t flags = 0;
  uint32_t x;
  uint32_t y;
  bool pressed;
  int ret;

  if (!priv->open)
    {
      return;
    }

  ret = stm32u5x9j_touch_read(priv, buf, sizeof(buf));
  if (ret == OK)
    {
      pressed = (buf[2] & SITRONIX_CONTACT) != 0;
      x = (((uint32_t)buf[2] & 0x70) << 4) | buf[3];
      y = (((uint32_t)buf[2] & 0x07) << 8) | buf[4];

      if (x >= SITRONIX_XRES)
        {
          x = SITRONIX_XRES - 1;
        }

      if (y >= SITRONIX_YRES)
        {
          y = SITRONIX_YRES - 1;
        }

      if (pressed)
        {
          flags = priv->down ? TOUCH_MOVE : TOUCH_DOWN;
          priv->down = true;
        }
      else if (priv->down)
        {
          flags = TOUCH_UP;
          priv->down = false;
        }

      if (flags != 0)
        {
          memset(&sample, 0, sizeof(sample));
          sample.npoints = 1;
          sample.point[0].id = 0;
          sample.point[0].flags = flags | TOUCH_ID_VALID | TOUCH_POS_VALID;
          sample.point[0].x = x;
          sample.point[0].y = y;
          sample.point[0].timestamp = touch_get_time();
          touch_event(priv->lower.priv, &sample);
        }
    }

  work_queue(LPWORK, &priv->work, stm32u5x9j_touch_worker,
             priv, MSEC2TICK(SITRONIX_POLL_MSEC));
}

static int stm32u5x9j_touch_irq(int irq, FAR void *context, FAR void *arg)
{
  FAR struct stm32u5x9j_touch_s *priv = arg;

  if (priv != NULL && priv->open && work_available(&priv->work))
    {
      work_queue(LPWORK, &priv->work, stm32u5x9j_touch_worker, priv, 0);
    }

  return OK;
}

static int stm32u5x9j_touch_open(FAR struct touch_lowerhalf_s *lower)
{
  FAR struct stm32u5x9j_touch_s *priv =
    (FAR struct stm32u5x9j_touch_s *)lower;

  priv->open = true;
  work_queue(LPWORK, &priv->work, stm32u5x9j_touch_worker, priv, 0);
  return OK;
}

static int stm32u5x9j_touch_close(FAR struct touch_lowerhalf_s *lower)
{
  FAR struct stm32u5x9j_touch_s *priv =
    (FAR struct stm32u5x9j_touch_s *)lower;

  priv->open = false;
  work_cancel(LPWORK, &priv->work);
  return OK;
}

static int stm32u5x9j_touch_control(FAR struct touch_lowerhalf_s *lower,
                                    int cmd, unsigned long arg)
{
  return -ENOTTY;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32u5x9j_touch_initialize(FAR struct i2c_master_s *i2c5)
{
  uint8_t buf[SITRONIX_BUFSIZE];
  int ret;

  if (i2c5 == NULL)
    {
      return -ENODEV;
    }

  g_touch.i2c = i2c5;
  stm32_configgpio(GPIO_TS_INT);
  stm32_gpiosetevent(GPIO_TS_INT, false, true, true,
                     stm32u5x9j_touch_irq, &g_touch);

  ret = stm32u5x9j_touch_read(&g_touch, buf, sizeof(buf));
  if (ret == OK)
    {
      g_touch.id = buf[0];
      syslog(LOG_INFO, "stm32u5x9j: Sitronix id=0x%02x\n", g_touch.id);
    }
  else
    {
      syslog(LOG_WARNING, "stm32u5x9j: Sitronix probe failed: %d\n", ret);
    }

  ret = touch_register(&g_touch.lower, "/dev/input0", 1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: touch_register(/dev/input0) failed: %d\n",
             ret);
    }

  return ret;
}

#endif /* CONFIG_STM32U5X9J_DK_TOUCH */
