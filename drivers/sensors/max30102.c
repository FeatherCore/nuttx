/****************************************************************************
 * drivers/sensors/max30102.c
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
#include <syslog.h>
#include <unistd.h>

#include <nuttx/fs/fs.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/kmalloc.h>
#include <nuttx/mutex.h>
#include <nuttx/sensors/max30102.h>

#if defined(CONFIG_I2C) && defined(CONFIG_SENSORS_MAX30102)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX30102_REG_INTR_STATUS1    0x00
#define MAX30102_REG_INTR_STATUS2    0x01
#define MAX30102_REG_FIFO_WR_PTR     0x04
#define MAX30102_REG_OVF_COUNTER     0x05
#define MAX30102_REG_FIFO_RD_PTR     0x06
#define MAX30102_REG_FIFO_DATA       0x07
#define MAX30102_REG_FIFO_CONFIG     0x08
#define MAX30102_REG_MODE_CONFIG     0x09
#define MAX30102_REG_SPO2_CONFIG     0x0a
#define MAX30102_REG_LED1_PA         0x0c
#define MAX30102_REG_LED2_PA         0x0d
#define MAX30102_REG_REV_ID          0xfe
#define MAX30102_REG_PART_ID         0xff

#define MAX30102_PART_ID             0x15

#define MAX30102_MODE_RESET          0x40
#define MAX30102_MODE_SHDN           0x80
#define MAX30102_MODE_SPO2           0x03

#define MAX30102_FIFO_DEPTH          32
#define MAX30102_FIFO_SAMPLE_BYTES   6
#define MAX30102_FIFO_ROLLOVER_EN    0x10
#define MAX30102_FIFO_A_FULL         0x0f
#define MAX30102_SAMPLE_MASK         0x3ffff

#ifndef CONFIG_MAX30102_SAMPLE_WAIT_MS
#  define CONFIG_MAX30102_SAMPLE_WAIT_MS 1000
#endif

#ifndef CONFIG_MAX30102_LED1_PA
#  define CONFIG_MAX30102_LED1_PA 0x24
#endif

#ifndef CONFIG_MAX30102_LED2_PA
#  define CONFIG_MAX30102_LED2_PA 0x24
#endif

#ifndef CONFIG_MAX30102_FIFO_AVERAGE
#  define CONFIG_MAX30102_FIFO_AVERAGE 2
#endif

#ifndef CONFIG_MAX30102_ADC_RANGE
#  define CONFIG_MAX30102_ADC_RANGE 2
#endif

#ifndef CONFIG_MAX30102_SAMPLE_RATE
#  define CONFIG_MAX30102_SAMPLE_RATE 1
#endif

#ifndef CONFIG_MAX30102_PULSE_WIDTH
#  define CONFIG_MAX30102_PULSE_WIDTH 3
#endif

#define MAX30102_FIFO_CONFIG_VALUE \
  (((CONFIG_MAX30102_FIFO_AVERAGE & 0x07) << 5) | \
   MAX30102_FIFO_ROLLOVER_EN | MAX30102_FIFO_A_FULL)

#define MAX30102_SPO2_CONFIG_VALUE \
  (((CONFIG_MAX30102_ADC_RANGE & 0x03) << 5) | \
   ((CONFIG_MAX30102_SAMPLE_RATE & 0x07) << 2) | \
   (CONFIG_MAX30102_PULSE_WIDTH & 0x03))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct max30102_dev_s
{
  FAR struct i2c_master_s *i2c;
  mutex_t lock;
  uint8_t address;
  uint32_t frequency;
  uint8_t part_id;
  uint8_t revision_id;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int max30102_open(FAR struct file *filep);
static int max30102_close(FAR struct file *filep);
static ssize_t max30102_read(FAR struct file *filep, FAR char *buffer,
                             size_t buflen);
static int max30102_ioctl(FAR struct file *filep, int cmd,
                          unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_max30102_fops =
{
  max30102_open,  /* open */
  max30102_close, /* close */
  max30102_read,  /* read */
  NULL,           /* write */
  NULL,           /* seek */
  max30102_ioctl, /* ioctl */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int max30102_write8(FAR struct max30102_dev_s *priv, uint8_t reg,
                           uint8_t value)
{
  uint8_t buffer[2];
  struct i2c_msg_s msg;
  int ret;

  buffer[0] = reg;
  buffer[1] = value;

  msg.frequency = priv->frequency;
  msg.addr      = priv->address;
  msg.flags     = 0;
  msg.buffer    = buffer;
  msg.length    = sizeof(buffer);

  ret = I2C_TRANSFER(priv->i2c, &msg, 1);
  return ret < 0 ? ret : OK;
}

static int max30102_readregs(FAR struct max30102_dev_s *priv, uint8_t reg,
                             FAR uint8_t *buffer, size_t buflen)
{
  struct i2c_msg_s msg[2];
  int ret;

  msg[0].frequency = priv->frequency;
  msg[0].addr      = priv->address;
  msg[0].flags     = I2C_M_NOSTOP;
  msg[0].buffer    = &reg;
  msg[0].length    = 1;

  msg[1].frequency = priv->frequency;
  msg[1].addr      = priv->address;
  msg[1].flags     = I2C_M_READ;
  msg[1].buffer    = buffer;
  msg[1].length    = buflen;

  ret = I2C_TRANSFER(priv->i2c, msg, 2);
  return ret < 0 ? ret : OK;
}

static int max30102_read8(FAR struct max30102_dev_s *priv, uint8_t reg,
                          FAR uint8_t *value)
{
  return max30102_readregs(priv, reg, value, 1);
}

static uint32_t max30102_get_sample(FAR const uint8_t *buffer, int index)
{
  uint32_t value;

  value = ((uint32_t)buffer[index] << 16) |
          ((uint32_t)buffer[index + 1] << 8) |
          buffer[index + 2];
  return value & MAX30102_SAMPLE_MASK;
}

static int max30102_available(FAR struct max30102_dev_s *priv,
                              FAR uint8_t *available)
{
  uint8_t wrptr;
  uint8_t rdptr;
  int ret;

  ret = max30102_read8(priv, MAX30102_REG_FIFO_WR_PTR, &wrptr);
  if (ret < 0)
    {
      return ret;
    }

  ret = max30102_read8(priv, MAX30102_REG_FIFO_RD_PTR, &rdptr);
  if (ret < 0)
    {
      return ret;
    }

  wrptr &= 0x1f;
  rdptr &= 0x1f;

  if (wrptr >= rdptr)
    {
      *available = wrptr - rdptr;
    }
  else
    {
      *available = MAX30102_FIFO_DEPTH + wrptr - rdptr;
    }

  return OK;
}

static int max30102_wait_sample(FAR struct max30102_dev_s *priv)
{
  uint8_t available;
  int elapsed;
  int ret;

  for (elapsed = 0; elapsed < CONFIG_MAX30102_SAMPLE_WAIT_MS; elapsed += 5)
    {
      ret = max30102_available(priv, &available);
      if (ret < 0)
        {
          return ret;
        }

      if (available > 0)
        {
          return OK;
        }

      usleep(5000);
    }

  return -ETIMEDOUT;
}

static int max30102_read_sample(FAR struct max30102_dev_s *priv,
                                FAR struct max30102_sample_s *sample)
{
  uint8_t data[MAX30102_FIFO_SAMPLE_BYTES];
  int ret;

  ret = max30102_wait_sample(priv);
  if (ret < 0)
    {
      return ret;
    }

  ret = max30102_readregs(priv, MAX30102_REG_FIFO_DATA, data,
                          sizeof(data));
  if (ret < 0)
    {
      return ret;
    }

  sample->red = max30102_get_sample(data, 0);
  sample->ir = max30102_get_sample(data, 3);

  return OK;
}

static int max30102_reset(FAR struct max30102_dev_s *priv)
{
  uint8_t mode;
  int retry;
  int ret;

  ret = max30102_write8(priv, MAX30102_REG_MODE_CONFIG,
                        MAX30102_MODE_RESET);
  if (ret < 0)
    {
      return ret;
    }

  usleep(1000);

  for (retry = 0; retry < 20; retry++)
    {
      ret = max30102_read8(priv, MAX30102_REG_MODE_CONFIG, &mode);
      if (ret < 0)
        {
          return ret;
        }

      if ((mode & MAX30102_MODE_RESET) == 0)
        {
          return OK;
        }

      usleep(5000);
    }

  return -ETIMEDOUT;
}

static int max30102_configure(FAR struct max30102_dev_s *priv)
{
  uint8_t value;
  int ret;

  ret = max30102_write8(priv, MAX30102_REG_FIFO_WR_PTR, 0);
  if (ret < 0)
    {
      return ret;
    }

  ret = max30102_write8(priv, MAX30102_REG_OVF_COUNTER, 0);
  if (ret < 0)
    {
      return ret;
    }

  ret = max30102_write8(priv, MAX30102_REG_FIFO_RD_PTR, 0);
  if (ret < 0)
    {
      return ret;
    }

  ret = max30102_write8(priv, MAX30102_REG_FIFO_CONFIG,
                        MAX30102_FIFO_CONFIG_VALUE);
  if (ret < 0)
    {
      return ret;
    }

  ret = max30102_write8(priv, MAX30102_REG_SPO2_CONFIG,
                        MAX30102_SPO2_CONFIG_VALUE);
  if (ret < 0)
    {
      return ret;
    }

  ret = max30102_write8(priv, MAX30102_REG_LED1_PA,
                        CONFIG_MAX30102_LED1_PA);
  if (ret < 0)
    {
      return ret;
    }

  ret = max30102_write8(priv, MAX30102_REG_LED2_PA,
                        CONFIG_MAX30102_LED2_PA);
  if (ret < 0)
    {
      return ret;
    }

  ret = max30102_read8(priv, MAX30102_REG_INTR_STATUS1, &value);
  if (ret < 0)
    {
      return ret;
    }

  ret = max30102_read8(priv, MAX30102_REG_INTR_STATUS2, &value);
  if (ret < 0)
    {
      return ret;
    }

  return max30102_write8(priv, MAX30102_REG_MODE_CONFIG,
                         MAX30102_MODE_SPO2);
}

static int max30102_probe(FAR struct max30102_dev_s *priv)
{
  int ret;

  ret = max30102_read8(priv, MAX30102_REG_PART_ID, &priv->part_id);
  if (ret < 0)
    {
      return ret;
    }

  ret = max30102_read8(priv, MAX30102_REG_REV_ID, &priv->revision_id);
  if (ret < 0)
    {
      return ret;
    }

  if (priv->part_id != MAX30102_PART_ID)
    {
      syslog(LOG_ERR, "max30102: unexpected part id 0x%02x\n",
             priv->part_id);
      return -ENODEV;
    }

  syslog(LOG_INFO, "max30102: part id 0x%02x revision 0x%02x\n",
         priv->part_id, priv->revision_id);

  ret = max30102_reset(priv);
  if (ret < 0)
    {
      return ret;
    }

  return max30102_configure(priv);
}

static int max30102_open(FAR struct file *filep)
{
  return OK;
}

static int max30102_close(FAR struct file *filep)
{
  return OK;
}

static ssize_t max30102_read(FAR struct file *filep, FAR char *buffer,
                             size_t buflen)
{
  FAR struct inode *inode = filep->f_inode;
  FAR struct max30102_dev_s *priv = inode->i_private;
  FAR struct max30102_sample_s *samples;
  size_t nsamples;
  size_t i;
  int ret;

  if (buflen < sizeof(struct max30102_sample_s))
    {
      return -EINVAL;
    }

  samples = (FAR struct max30102_sample_s *)buffer;
  nsamples = buflen / sizeof(struct max30102_sample_s);

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  for (i = 0; i < nsamples; i++)
    {
      ret = max30102_read_sample(priv, &samples[i]);
      if (ret < 0)
        {
          nxmutex_unlock(&priv->lock);
          return ret;
        }
    }

  nxmutex_unlock(&priv->lock);
  return nsamples * sizeof(struct max30102_sample_s);
}

static int max30102_ioctl(FAR struct file *filep, int cmd,
                          unsigned long arg)
{
  FAR struct inode *inode = filep->f_inode;
  FAR struct max30102_dev_s *priv = inode->i_private;
  FAR struct max30102_id_s *id = (FAR struct max30102_id_s *)arg;

  switch (cmd)
    {
      case MAX30102IOC_READ_ID:
        if (id == NULL)
          {
            return -EINVAL;
          }

        id->part_id = priv->part_id;
        id->revision_id = priv->revision_id;
        return OK;

      default:
        return -ENOTTY;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int max30102_register(FAR struct i2c_master_s *i2c,
                      FAR const struct max30102_config_s *config)
{
  FAR struct max30102_dev_s *priv;
  int ret;

  if (i2c == NULL || config == NULL || config->devpath == NULL)
    {
      return -EINVAL;
    }

  priv = kmm_zalloc(sizeof(*priv));
  if (priv == NULL)
    {
      return -ENOMEM;
    }

  priv->i2c = i2c;
  priv->address = config->address;
  priv->frequency = config->frequency;
  nxmutex_init(&priv->lock);

  ret = max30102_probe(priv);
  if (ret < 0)
    {
      goto err_free;
    }

  ret = register_driver(config->devpath, &g_max30102_fops, 0666, priv);
  if (ret < 0)
    {
      goto err_free;
    }

  return OK;

err_free:
  nxmutex_destroy(&priv->lock);
  kmm_free(priv);
  return ret;
}

#endif /* CONFIG_I2C && CONFIG_SENSORS_MAX30102 */
