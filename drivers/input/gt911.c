/****************************************************************************
 * drivers/input/gt911.c
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
#include <nuttx/fs/fs.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/input/gt911.h>
#include <nuttx/input/touchscreen.h>
#include <nuttx/kmalloc.h>
#include <nuttx/wqueue.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEV_FORMAT             "/dev/input%d"
#define DEV_NAMELEN            16

#define GT911_PRODUCT_ID_REG   0x8140u
#define GT911_READ_XY_REG      0x814eu
#define GT911_READ_DATA_REG    0x8150u
#define GT911_BUFFER_STATUS    0x80u
#define GT911_TOUCH_MASK       0x0fu
#define GT911_COORD_MASK       0x0fu
#define GT911_MAX_POINTS       5u

#ifndef CONFIG_INPUT_GT911_NBUFFER
#  define CONFIG_INPUT_GT911_NBUFFER 1
#endif

#ifdef CONFIG_INPUT_GT911_DEBUG
#  define GT911_DEBUG_LOG_LIMIT      32u
#  define GT911_POLL_LOG_LIMIT       96u
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct gt911_dev_s
{
  struct touch_lowerhalf_s lower;
  FAR struct i2c_master_s *i2c;
  struct work_s work;
  uint8_t address;
  uint8_t refs;
  uint8_t id;
  uint16_t poll_ms;
  uint32_t frequency;
  int16_t x;
  int16_t y;
  bool open;
  bool down;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_INPUT_GT911_DEBUG
static unsigned int g_gt911_poll_count;
static unsigned int g_gt911_raw_count;
static unsigned int g_gt911_worker_count;
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int gt911_open(FAR struct touch_lowerhalf_s *lower);
static int gt911_close(FAR struct touch_lowerhalf_s *lower);
static int gt911_control(FAR struct touch_lowerhalf_s *lower,
                         int cmd, unsigned long arg);
static void gt911_worker(FAR void *arg);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int gt911_read_reg(FAR struct gt911_dev_s *priv, uint16_t reg,
                          FAR uint8_t *buffer, size_t len)
{
  uint8_t addr[2];
  struct i2c_msg_s msg[2];

  addr[0] = reg >> 8;
  addr[1] = reg & 0xff;

  msg[0].frequency = priv->frequency;
  msg[0].addr      = priv->address;
  msg[0].flags     = I2C_M_NOSTOP;
  msg[0].buffer    = addr;
  msg[0].length    = sizeof(addr);

  msg[1].frequency = priv->frequency;
  msg[1].addr      = priv->address;
  msg[1].flags     = I2C_M_READ;
  msg[1].buffer    = buffer;
  msg[1].length    = len;

  return I2C_TRANSFER(priv->i2c, msg, 2);
}

static int gt911_write_reg(FAR struct gt911_dev_s *priv, uint16_t reg,
                           uint8_t value)
{
  uint8_t buffer[3];
  struct i2c_msg_s msg;

  buffer[0] = reg >> 8;
  buffer[1] = reg & 0xff;
  buffer[2] = value;

  msg.frequency = priv->frequency;
  msg.addr      = priv->address;
  msg.flags     = 0;
  msg.buffer    = buffer;
  msg.length    = sizeof(buffer);

  return I2C_TRANSFER(priv->i2c, &msg, 1);
}

static void gt911_report(FAR struct gt911_dev_s *priv, uint8_t flags,
                         int16_t x, int16_t y, uint16_t pressure)
{
  struct touch_sample_s sample;

#ifdef CONFIG_INPUT_GT911_DEBUG
  static unsigned int report_count;

  if (report_count < GT911_DEBUG_LOG_LIMIT)
    {
      iinfo("event%u flags=%02x id=%u x=%d y=%d p=%u\n",
            report_count, (unsigned int)flags, (unsigned int)priv->id,
            x, y, (unsigned int)pressure);
      report_count++;
    }
#endif

  memset(&sample, 0, sizeof(sample));
  sample.npoints = 1;
  sample.point[0].id = priv->id;
  sample.point[0].flags = flags | TOUCH_ID_VALID | TOUCH_POS_VALID |
                          TOUCH_PRESSURE_VALID;
  sample.point[0].x = x;
  sample.point[0].y = y;
  sample.point[0].pressure = pressure;
  sample.point[0].timestamp = touch_get_time();

  touch_event(priv->lower.priv, &sample);
}

static void gt911_poll(FAR struct gt911_dev_s *priv)
{
  uint8_t status;
  uint8_t point[8];
  uint8_t flags;
  uint8_t count;
  uint16_t pressure;
  uint16_t x;
  uint16_t y;
  int ret;

  if (!priv->open)
    {
      return;
    }

  ret = gt911_read_reg(priv, GT911_READ_XY_REG, &status, 1);

#ifdef CONFIG_INPUT_GT911_DEBUG
  if (g_gt911_poll_count < GT911_POLL_LOG_LIMIT || ret < 0 ||
      (ret == OK && status != 0))
    {
      iinfo("poll%u ret=%d status=%02x down=%u\n",
            g_gt911_poll_count, ret,
            ret == OK ? (unsigned int)status : 0,
            (unsigned int)priv->down);
    }

  g_gt911_poll_count++;
#endif

  if (ret < 0)
    {
      return;
    }

  count = status & GT911_TOUCH_MASK;

#ifdef CONFIG_INPUT_GT911_DEBUG
  if ((status & GT911_BUFFER_STATUS) != 0)
    {
      static unsigned int status_count;

      if (status_count < GT911_DEBUG_LOG_LIMIT)
        {
          iinfo("status%u raw=%02x count=%u down=%u\n",
                status_count, (unsigned int)status, (unsigned int)count,
                (unsigned int)priv->down);
          status_count++;
        }
    }
#endif

  if ((status & GT911_BUFFER_STATUS) != 0 &&
      count > 0 && count <= GT911_MAX_POINTS)
    {
      ret = gt911_read_reg(priv, GT911_READ_DATA_REG, point, sizeof(point));
      if (ret == OK)
        {
          x = ((uint16_t)(point[1] & GT911_COORD_MASK) << 8) | point[0];
          y = ((uint16_t)(point[3] & GT911_COORD_MASK) << 8) | point[2];
          pressure =
            ((uint16_t)(point[5] & GT911_COORD_MASK) << 8) | point[4];

#ifdef CONFIG_INPUT_GT911_DEBUG
          if (g_gt911_raw_count < GT911_DEBUG_LOG_LIMIT)
            {
              iinfo("raw%u %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    g_gt911_raw_count, (unsigned int)point[0],
                    (unsigned int)point[1], (unsigned int)point[2],
                    (unsigned int)point[3], (unsigned int)point[4],
                    (unsigned int)point[5], (unsigned int)point[6],
                    (unsigned int)point[7]);
              g_gt911_raw_count++;
            }
#endif

          if (x >= priv->lower.xres)
            {
              x = priv->lower.xres - 1;
            }

          if (y >= priv->lower.yres)
            {
              y = priv->lower.yres - 1;
            }

          flags = priv->down ? TOUCH_MOVE : TOUCH_DOWN;
          if (!priv->down)
            {
              priv->id++;
            }

          priv->down = true;
          priv->x = x;
          priv->y = y;
          gt911_report(priv, flags, x, y, pressure);
        }

      (void)gt911_write_reg(priv, GT911_READ_XY_REG, 0);
    }
  else if ((status & GT911_BUFFER_STATUS) != 0)
    {
      /* Acknowledge stale ready bits with no valid touch point.  Otherwise
       * GT911 may stop publishing newer samples.
       */

      if (priv->down)
        {
          priv->down = false;
          gt911_report(priv, TOUCH_UP, priv->x, priv->y, 0);
        }

      (void)gt911_write_reg(priv, GT911_READ_XY_REG, 0);
    }
  else if (priv->down)
    {
      priv->down = false;
      gt911_report(priv, TOUCH_UP, priv->x, priv->y, 0);
      (void)gt911_write_reg(priv, GT911_READ_XY_REG, 0);
    }
}

static void gt911_worker(FAR void *arg)
{
  FAR struct gt911_dev_s *priv = arg;
  int ret;

#ifdef CONFIG_INPUT_GT911_DEBUG
  if (g_gt911_worker_count < GT911_POLL_LOG_LIMIT || priv->down)
    {
      iinfo("worker%u open=%u refs=%u down=%u\n",
            g_gt911_worker_count, (unsigned int)priv->open,
            (unsigned int)priv->refs, (unsigned int)priv->down);
    }

  g_gt911_worker_count++;
#endif

  if (!priv->open)
    {
      return;
    }

  gt911_poll(priv);

  if (!priv->open)
    {
      return;
    }

  ret = work_queue(HPWORK, &priv->work, gt911_worker, priv,
                   MSEC2TICK(priv->poll_ms));
  if (ret < 0)
    {
      ierr("GT911 requeue failed: %d\n", ret);
    }
}

static int gt911_open(FAR struct touch_lowerhalf_s *lower)
{
  FAR struct gt911_dev_s *priv = (FAR struct gt911_dev_s *)lower;
  int ret;

#ifdef CONFIG_INPUT_GT911_DEBUG
  iinfo("open lower=%p poll=%ums refs=%u\n",
        lower, (unsigned int)priv->poll_ms, (unsigned int)priv->refs);
#endif

  if (priv->refs > 0)
    {
      priv->refs++;
      return OK;
    }

  priv->open = true;
  priv->down = false;
  priv->refs = 1;

  ret = work_queue(HPWORK, &priv->work, gt911_worker, priv, 0);
  if (ret < 0)
    {
      priv->open = false;
      priv->refs = 0;
      ierr("GT911 queue failed: %d\n", ret);
      return ret;
    }

  return OK;
}

static int gt911_close(FAR struct touch_lowerhalf_s *lower)
{
  FAR struct gt911_dev_s *priv = (FAR struct gt911_dev_s *)lower;

#ifdef CONFIG_INPUT_GT911_DEBUG
  iinfo("close lower=%p refs=%u\n", lower, (unsigned int)priv->refs);
#endif

  if (priv->refs > 0)
    {
      priv->refs--;
    }

  if (priv->refs == 0)
    {
      priv->open = false;
      work_cancel(HPWORK, &priv->work);
    }

  return OK;
}

static int gt911_control(FAR struct touch_lowerhalf_s *lower,
                         int cmd, unsigned long arg)
{
  FAR struct gt911_dev_s *priv = (FAR struct gt911_dev_s *)lower;
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

static int gt911_probe(FAR struct gt911_dev_s *priv)
{
  uint8_t product[5];
  int ret;

  ret = gt911_read_reg(priv, GT911_PRODUCT_ID_REG, product,
                       sizeof(product) - 1);
  if (ret < 0)
    {
      ierr("GT911 probe failed: %d\n", ret);
      return ret;
    }

  product[4] = '\0';
  iinfo("GT911 id=%s\n", product);

  ret = gt911_write_reg(priv, GT911_READ_XY_REG, 0);
  if (ret < 0)
    {
      ierr("GT911 clear status failed: %d\n", ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int gt911_register(FAR struct i2c_master_s *i2c,
                   FAR const struct gt911_config_s *config, int minor)
{
  FAR struct gt911_dev_s *priv;
  char devname[DEV_NAMELEN];
  int ret;

  DEBUGASSERT(i2c != NULL && config != NULL && minor >= 0 && minor < 100);

  priv = kmm_zalloc(sizeof(struct gt911_dev_s));
  if (priv == NULL)
    {
      return -ENOMEM;
    }

  priv->i2c = i2c;
  priv->address = config->address;
  priv->frequency = config->frequency;
  priv->poll_ms = config->poll_ms;
  priv->lower.maxpoint = 1;
  priv->lower.flags = config->flags;
  priv->lower.xres = config->xres;
  priv->lower.yres = config->yres;
  priv->lower.control = gt911_control;
  priv->lower.open = gt911_open;
  priv->lower.close = gt911_close;

  if (priv->poll_ms == 0)
    {
      priv->poll_ms = 30;
    }

  ret = gt911_probe(priv);
  if (ret < 0)
    {
      goto errout_with_priv;
    }

  snprintf(devname, sizeof(devname), DEV_FORMAT, minor);

  ret = touch_register(&priv->lower, devname, CONFIG_INPUT_GT911_NBUFFER);
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
