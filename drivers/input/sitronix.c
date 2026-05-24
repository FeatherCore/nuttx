/****************************************************************************
 * drivers/input/sitronix.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/clock.h>
#include <nuttx/debug.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/input/sitronix.h>
#include <nuttx/input/touchscreen.h>
#include <nuttx/kmalloc.h>
#include <nuttx/wqueue.h>

#ifdef CONFIG_INPUT_SITRONIX

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEV_FORMAT             "/dev/input%d"
#define DEV_NAMELEN            16

#define SITRONIX_BUFSIZE       28
#define SITRONIX_CONTACT       0x80
#define SITRONIX_DEFAULT_ADDR  0x70
#define SITRONIX_DEFAULT_FREQ  400000
#define SITRONIX_DEFAULT_POLL  30

#ifndef CONFIG_INPUT_SITRONIX_NBUFFER
#  define CONFIG_INPUT_SITRONIX_NBUFFER 1
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct sitronix_dev_s
{
  struct touch_lowerhalf_s lower;
  FAR struct i2c_master_s *i2c;
  FAR const struct sitronix_config_s *config;
  struct work_s work;
  uint32_t frequency;
  uint16_t poll_ms;
  uint8_t address;
  uint8_t refs;
  bool open;
  bool down;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int sitronix_open(FAR struct touch_lowerhalf_s *lower);
static int sitronix_close(FAR struct touch_lowerhalf_s *lower);
static int sitronix_control(FAR struct touch_lowerhalf_s *lower,
                            int cmd, unsigned long arg);
static void sitronix_worker(FAR void *arg);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int sitronix_read(FAR struct sitronix_dev_s *priv,
                         FAR uint8_t *buf, size_t len)
{
  struct i2c_msg_s msg;

  msg.frequency = priv->frequency;
  msg.addr      = priv->address;
  msg.flags     = I2C_M_READ;
  msg.buffer    = buf;
  msg.length    = len;

  return I2C_TRANSFER(priv->i2c, &msg, 1);
}

static void sitronix_report(FAR struct sitronix_dev_s *priv, uint8_t flags,
                            uint16_t x, uint16_t y)
{
  struct touch_sample_s sample;

  memset(&sample, 0, sizeof(sample));
  sample.npoints = 1;
  sample.point[0].id = 0;
  sample.point[0].flags = flags | TOUCH_ID_VALID | TOUCH_POS_VALID;
  sample.point[0].x = x;
  sample.point[0].y = y;
  sample.point[0].timestamp = touch_get_time();

  touch_event(priv->lower.priv, &sample);
}

static void sitronix_poll(FAR struct sitronix_dev_s *priv)
{
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

  if (priv->config->clear != NULL)
    {
      priv->config->clear(priv->config);
    }

  ret = sitronix_read(priv, buf, sizeof(buf));
  if (ret < 0)
    {
      return;
    }

  pressed = (buf[2] & SITRONIX_CONTACT) != 0;
  x = (((uint32_t)buf[2] & 0x70) << 4) | buf[3];
  y = (((uint32_t)buf[2] & 0x07) << 8) | buf[4];

  if (x >= priv->lower.xres)
    {
      x = priv->lower.xres - 1;
    }

  if (y >= priv->lower.yres)
    {
      y = priv->lower.yres - 1;
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
      sitronix_report(priv, flags, x, y);
    }
}

static void sitronix_worker(FAR void *arg)
{
  FAR struct sitronix_dev_s *priv = arg;
  int ret;

  if (!priv->open)
    {
      return;
    }

  sitronix_poll(priv);

  if (!priv->open)
    {
      return;
    }

  ret = work_queue(LPWORK, &priv->work, sitronix_worker, priv,
                   MSEC2TICK(priv->poll_ms));
  if (ret < 0)
    {
      ierr("Sitronix requeue failed: %d\n", ret);
    }
}

static int sitronix_irq(int irq, FAR void *context, FAR void *arg)
{
  FAR struct sitronix_dev_s *priv = arg;

  if (priv != NULL && priv->open && work_available(&priv->work))
    {
      work_queue(LPWORK, &priv->work, sitronix_worker, priv, 0);
    }

  return OK;
}

static int sitronix_open(FAR struct touch_lowerhalf_s *lower)
{
  FAR struct sitronix_dev_s *priv = (FAR struct sitronix_dev_s *)lower;
  int ret;

  if (priv->refs > 0)
    {
      priv->refs++;
      return OK;
    }

  priv->open = true;
  priv->down = false;
  priv->refs = 1;

  if (priv->config->enable != NULL)
    {
      priv->config->enable(priv->config, true);
    }

  ret = work_queue(LPWORK, &priv->work, sitronix_worker, priv, 0);
  if (ret < 0)
    {
      if (priv->config->enable != NULL)
        {
          priv->config->enable(priv->config, false);
        }

      priv->open = false;
      priv->refs = 0;
      ierr("Sitronix queue failed: %d\n", ret);
      return ret;
    }

  return OK;
}

static int sitronix_close(FAR struct touch_lowerhalf_s *lower)
{
  FAR struct sitronix_dev_s *priv = (FAR struct sitronix_dev_s *)lower;

  if (priv->refs > 0)
    {
      priv->refs--;
    }

  if (priv->refs == 0)
    {
      priv->open = false;
      work_cancel(LPWORK, &priv->work);

      if (priv->config->enable != NULL)
        {
          priv->config->enable(priv->config, false);
        }
    }

  return OK;
}

static int sitronix_control(FAR struct touch_lowerhalf_s *lower,
                            int cmd, unsigned long arg)
{
  FAR struct sitronix_dev_s *priv = (FAR struct sitronix_dev_s *)lower;
  int ret = OK;

  switch (cmd)
    {
      case TSIOC_SETFREQUENCY:
        {
          FAR uint32_t *frequency = (FAR uint32_t *)((uintptr_t)arg);

          DEBUGASSERT(frequency != NULL);
          priv->frequency = *frequency;
        }
        break;

      case TSIOC_GETFREQUENCY:
        {
          FAR uint32_t *frequency = (FAR uint32_t *)((uintptr_t)arg);

          DEBUGASSERT(frequency != NULL);
          *frequency = priv->frequency;
        }
        break;

      default:
        ret = -ENOTTY;
        break;
    }

  return ret;
}

static int sitronix_probe(FAR struct sitronix_dev_s *priv)
{
  uint8_t buf[SITRONIX_BUFSIZE];
  int ret;

  ret = sitronix_read(priv, buf, sizeof(buf));
  if (ret < 0)
    {
      ierr("Sitronix probe failed: %d\n", ret);
      return ret;
    }

  iinfo("Sitronix id=0x%02x\n", buf[0]);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int sitronix_register(FAR struct i2c_master_s *i2c,
                      FAR const struct sitronix_config_s *config,
                      int minor)
{
  FAR struct sitronix_dev_s *priv;
  char devname[DEV_NAMELEN];
  int ret;

  DEBUGASSERT(i2c != NULL && config != NULL && minor >= 0 && minor < 100);

  if (i2c == NULL || config == NULL || minor < 0 || minor >= 100)
    {
      return -EINVAL;
    }

  priv = kmm_zalloc(sizeof(struct sitronix_dev_s));
  if (priv == NULL)
    {
      return -ENOMEM;
    }

  priv->i2c = i2c;
  priv->config = config;
  priv->address = config->address == 0 ?
                  SITRONIX_DEFAULT_ADDR : config->address;
  priv->frequency = config->frequency == 0 ?
                    SITRONIX_DEFAULT_FREQ : config->frequency;
  priv->poll_ms = config->poll_ms == 0 ?
                  SITRONIX_DEFAULT_POLL : config->poll_ms;

  priv->lower.maxpoint = 1;
  priv->lower.flags = config->flags;
  priv->lower.xres = config->xres == 0 ? 480 : config->xres;
  priv->lower.yres = config->yres == 0 ? 480 : config->yres;
  priv->lower.control = sitronix_control;
  priv->lower.open = sitronix_open;
  priv->lower.close = sitronix_close;

  ret = sitronix_probe(priv);
  if (ret < 0)
    {
      goto errout_with_priv;
    }

  if (config->attach != NULL)
    {
      ret = config->attach(config, sitronix_irq, priv);
      if (ret < 0)
        {
          ierr("Sitronix IRQ attach failed: %d\n", ret);
          goto errout_with_priv;
        }
    }

  snprintf(devname, sizeof(devname), DEV_FORMAT, minor);

  ret = touch_register(&priv->lower, devname, CONFIG_INPUT_SITRONIX_NBUFFER);
  if (ret < 0)
    {
      ierr("touch_register(%s) failed: %d\n", devname, ret);
      goto errout_with_priv;
    }

  return OK;

errout_with_priv:
  kmm_free(priv);
  return ret;
}

#endif /* CONFIG_INPUT_SITRONIX */
