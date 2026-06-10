/****************************************************************************
 * drivers/sensors/lsm6dsox.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <syslog.h>

#include <nuttx/fs/fs.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/kmalloc.h>
#include <nuttx/sensors/lsm6dsox.h>
#include <nuttx/signal.h>

#if defined(CONFIG_I2C) && defined(CONFIG_SENSORS_LSM6DSOX)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LSM6DSOX_REG_WHO_AM_I      0x0f
#define LSM6DSOX_REG_CTRL1_XL      0x10
#define LSM6DSOX_REG_CTRL2_G       0x11
#define LSM6DSOX_REG_CTRL3_C       0x12
#define LSM6DSOX_REG_OUT_TEMP_L    0x20

#define LSM6DSOX_WHO_AM_I_VALUE    0x6c

#define LSM6DSOX_CTRL3_BDU         0x40
#define LSM6DSOX_CTRL3_IF_INC      0x04
#define LSM6DSOX_CTRL3_SW_RESET    0x01

#ifndef CONFIG_LSM6DSOX_ACCEL_ODR
#  define CONFIG_LSM6DSOX_ACCEL_ODR 4
#endif

#ifndef CONFIG_LSM6DSOX_ACCEL_FS
#  define CONFIG_LSM6DSOX_ACCEL_FS 0
#endif

#ifndef CONFIG_LSM6DSOX_GYRO_ODR
#  define CONFIG_LSM6DSOX_GYRO_ODR 4
#endif

#ifndef CONFIG_LSM6DSOX_GYRO_FS
#  define CONFIG_LSM6DSOX_GYRO_FS 0
#endif

#ifndef CONFIG_LSM6DSOX_RESET_WAIT_MS
#  define CONFIG_LSM6DSOX_RESET_WAIT_MS 50
#endif

#define LSM6DSOX_CTRL1_XL_VALUE \
  (((CONFIG_LSM6DSOX_ACCEL_ODR & 0x0f) << 4) | \
   ((CONFIG_LSM6DSOX_ACCEL_FS & 0x03) << 2))

#define LSM6DSOX_CTRL2_G_VALUE \
  (((CONFIG_LSM6DSOX_GYRO_ODR & 0x0f) << 4) | \
   ((CONFIG_LSM6DSOX_GYRO_FS & 0x03) << 2))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct lsm6dsox_dev_s
{
  FAR struct i2c_master_s *i2c;
  struct lsm6dsox_config_s config;
  uint8_t whoami;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ssize_t lsm6dsox_read(FAR struct file *filep, FAR char *buffer,
                             size_t buflen);
static int lsm6dsox_ioctl(FAR struct file *filep, int cmd,
                          unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_lsm6dsox_fops =
{
  NULL,             /* open */
  NULL,             /* close */
  lsm6dsox_read,    /* read */
  NULL,             /* write */
  NULL,             /* seek */
  lsm6dsox_ioctl,   /* ioctl */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int lsm6dsox_readregs(FAR struct lsm6dsox_dev_s *priv,
                             uint8_t regaddr, FAR uint8_t *buffer,
                             size_t buflen)
{
  struct i2c_config_s config;
  int ret;

  config.frequency = priv->config.frequency;
  config.address   = priv->config.address;
  config.addrlen   = 7;

  ret = i2c_write(priv->i2c, &config, &regaddr, sizeof(regaddr));
  if (ret < 0)
    {
      snerr("ERROR: i2c_write failed: %d\n", ret);
      return ret;
    }

  ret = i2c_read(priv->i2c, &config, buffer, buflen);
  if (ret < 0)
    {
      snerr("ERROR: i2c_read failed: %d\n", ret);
    }

  return ret;
}

static int lsm6dsox_read8(FAR struct lsm6dsox_dev_s *priv,
                          uint8_t regaddr, FAR uint8_t *value)
{
  return lsm6dsox_readregs(priv, regaddr, value, 1);
}

static int lsm6dsox_write8(FAR struct lsm6dsox_dev_s *priv,
                           uint8_t regaddr, uint8_t value)
{
  struct i2c_config_s config;
  uint8_t buffer[2];
  int ret;

  config.frequency = priv->config.frequency;
  config.address   = priv->config.address;
  config.addrlen   = 7;

  buffer[0] = regaddr;
  buffer[1] = value;

  ret = i2c_write(priv->i2c, &config, buffer, sizeof(buffer));
  if (ret < 0)
    {
      snerr("ERROR: i2c_write failed: %d\n", ret);
    }

  return ret;
}

static int16_t lsm6dsox_get_s16(FAR const uint8_t *buffer)
{
  return (int16_t)((uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8));
}

static int lsm6dsox_soft_reset(FAR struct lsm6dsox_dev_s *priv)
{
  uint8_t value;
  int elapsed;
  int ret;

  ret = lsm6dsox_write8(priv, LSM6DSOX_REG_CTRL3_C,
                        LSM6DSOX_CTRL3_SW_RESET);
  if (ret < 0)
    {
      return ret;
    }

  for (elapsed = 0; elapsed < CONFIG_LSM6DSOX_RESET_WAIT_MS;
       elapsed += 2)
    {
      nxsig_usleep(2000);

      ret = lsm6dsox_read8(priv, LSM6DSOX_REG_CTRL3_C, &value);
      if (ret < 0)
        {
          return ret;
        }

      if ((value & LSM6DSOX_CTRL3_SW_RESET) == 0)
        {
          return OK;
        }
    }

  return -ETIMEDOUT;
}

static int lsm6dsox_configure(FAR struct lsm6dsox_dev_s *priv)
{
  int ret;

  ret = lsm6dsox_soft_reset(priv);
  if (ret < 0)
    {
      return ret;
    }

  ret = lsm6dsox_write8(priv, LSM6DSOX_REG_CTRL3_C,
                        LSM6DSOX_CTRL3_BDU | LSM6DSOX_CTRL3_IF_INC);
  if (ret < 0)
    {
      return ret;
    }

  ret = lsm6dsox_write8(priv, LSM6DSOX_REG_CTRL1_XL,
                        LSM6DSOX_CTRL1_XL_VALUE);
  if (ret < 0)
    {
      return ret;
    }

  return lsm6dsox_write8(priv, LSM6DSOX_REG_CTRL2_G,
                         LSM6DSOX_CTRL2_G_VALUE);
}

static int lsm6dsox_read_sample(FAR struct lsm6dsox_dev_s *priv,
                                FAR struct lsm6dsox_sample_s *sample)
{
  uint8_t buffer[14];
  int ret;

  ret = lsm6dsox_readregs(priv, LSM6DSOX_REG_OUT_TEMP_L, buffer,
                          sizeof(buffer));
  if (ret < 0)
    {
      return ret;
    }

  sample->temperature = lsm6dsox_get_s16(&buffer[0]);
  sample->gyro_x      = lsm6dsox_get_s16(&buffer[2]);
  sample->gyro_y      = lsm6dsox_get_s16(&buffer[4]);
  sample->gyro_z      = lsm6dsox_get_s16(&buffer[6]);
  sample->accel_x     = lsm6dsox_get_s16(&buffer[8]);
  sample->accel_y     = lsm6dsox_get_s16(&buffer[10]);
  sample->accel_z     = lsm6dsox_get_s16(&buffer[12]);

  return OK;
}

static ssize_t lsm6dsox_read(FAR struct file *filep, FAR char *buffer,
                             size_t buflen)
{
  FAR struct inode *inode = filep->f_inode;
  FAR struct lsm6dsox_dev_s *priv = inode->i_private;
  FAR struct lsm6dsox_sample_s *sample;
  size_t nsamples;
  size_t i;
  int ret;

  if (buflen < sizeof(struct lsm6dsox_sample_s))
    {
      return -EINVAL;
    }

  nsamples = buflen / sizeof(struct lsm6dsox_sample_s);
  sample = (FAR struct lsm6dsox_sample_s *)buffer;

  for (i = 0; i < nsamples; i++)
    {
      ret = lsm6dsox_read_sample(priv, &sample[i]);
      if (ret < 0)
        {
          return i > 0 ? i * sizeof(struct lsm6dsox_sample_s) : ret;
        }
    }

  return nsamples * sizeof(struct lsm6dsox_sample_s);
}

static int lsm6dsox_ioctl(FAR struct file *filep, int cmd,
                          unsigned long arg)
{
  FAR struct inode *inode = filep->f_inode;
  FAR struct lsm6dsox_dev_s *priv = inode->i_private;
  FAR struct lsm6dsox_id_s *id;

  switch (cmd)
    {
      case LSM6DSOXIOC_READ_ID:
        if (arg == 0)
          {
            return -EINVAL;
          }

        id = (FAR struct lsm6dsox_id_s *)((uintptr_t)arg);
        id->whoami = priv->whoami;
        return OK;

      default:
        return -ENOTTY;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int lsm6dsox_register(FAR struct i2c_master_s *i2c,
                      FAR const struct lsm6dsox_config_s *config)
{
  FAR struct lsm6dsox_dev_s *priv;
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
  priv->config = *config;

  ret = lsm6dsox_read8(priv, LSM6DSOX_REG_WHO_AM_I, &priv->whoami);
  if (ret < 0)
    {
      goto errout;
    }

  if (priv->whoami != LSM6DSOX_WHO_AM_I_VALUE)
    {
      syslog(LOG_ERR, "lsm6dsox: unexpected whoami 0x%02x\n",
             priv->whoami);
      ret = -ENODEV;
      goto errout;
    }

  syslog(LOG_INFO, "lsm6dsox: whoami 0x%02x\n", priv->whoami);

  ret = lsm6dsox_configure(priv);
  if (ret < 0)
    {
      goto errout;
    }

  ret = register_driver(config->devpath, &g_lsm6dsox_fops, 0666, priv);
  if (ret < 0)
    {
      goto errout;
    }

  return OK;

errout:
  kmm_free(priv);
  return ret;
}

#endif /* CONFIG_I2C && CONFIG_SENSORS_LSM6DSOX */
