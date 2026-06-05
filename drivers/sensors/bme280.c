/****************************************************************************
 * drivers/sensors/bme280.c
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
#include <nuttx/sensors/bme280.h>

#if defined(CONFIG_I2C) && defined(CONFIG_SENSORS_BME280)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BME280_REG_ID             0xd0
#define BME280_REG_RESET          0xe0
#define BME280_REG_CTRL_HUM       0xf2
#define BME280_REG_STATUS         0xf3
#define BME280_REG_CTRL_MEAS      0xf4
#define BME280_REG_CONFIG         0xf5
#define BME280_REG_PRESS_MSB      0xf7

#define BME280_REG_CALIB_00       0x88
#define BME280_REG_CALIB_26       0xe1

#define BME280_CHIP_ID            0x60
#define BME280_SOFT_RESET         0xb6
#define BME280_STATUS_MEASURING   0x08
#define BME280_STATUS_IM_UPDATE   0x01

#define BME280_MODE_SLEEP         0x00
#define BME280_MODE_FORCED        0x01

#define BME280_OSRS_X1            0x01
#define BME280_CTRL_MEAS_VALUE    ((BME280_OSRS_X1 << 5) | \
                                   (BME280_OSRS_X1 << 2) | \
                                   BME280_MODE_FORCED)

#define BME280_CTRL_HUM_VALUE     BME280_OSRS_X1

#ifndef CONFIG_BME280_MEASUREMENT_WAIT_MS
#  define CONFIG_BME280_MEASUREMENT_WAIT_MS 10
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct bme280_calib_s
{
  uint16_t t1;
  int16_t t2;
  int16_t t3;
  uint16_t p1;
  int16_t p2;
  int16_t p3;
  int16_t p4;
  int16_t p5;
  int16_t p6;
  int16_t p7;
  int16_t p8;
  int16_t p9;
  uint8_t h1;
  int16_t h2;
  uint8_t h3;
  int16_t h4;
  int16_t h5;
  int8_t h6;
};

struct bme280_dev_s
{
  FAR struct i2c_master_s *i2c;
  mutex_t lock;
  struct bme280_calib_s calib;
  uint8_t address;
  uint32_t frequency;
  uint8_t chip_id;
  int32_t t_fine;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int bme280_open(FAR struct file *filep);
static int bme280_close(FAR struct file *filep);
static ssize_t bme280_read(FAR struct file *filep, FAR char *buffer,
                           size_t buflen);
static int bme280_ioctl(FAR struct file *filep, int cmd,
                        unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_bme280_fops =
{
  bme280_open,  /* open */
  bme280_close, /* close */
  bme280_read,  /* read */
  NULL,         /* write */
  NULL,         /* seek */
  bme280_ioctl, /* ioctl */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int16_t bme280_sign_extend12(uint16_t value)
{
  if ((value & 0x0800) != 0)
    {
      value |= 0xf000;
    }

  return (int16_t)value;
}

static uint16_t bme280_get_u16(FAR const uint8_t *buffer, int index)
{
  return (uint16_t)buffer[index] | ((uint16_t)buffer[index + 1] << 8);
}

static int16_t bme280_get_s16(FAR const uint8_t *buffer, int index)
{
  return (int16_t)bme280_get_u16(buffer, index);
}

static int bme280_write8(FAR struct bme280_dev_s *priv, uint8_t reg,
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

static int bme280_readregs(FAR struct bme280_dev_s *priv, uint8_t reg,
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

static int bme280_read8(FAR struct bme280_dev_s *priv, uint8_t reg,
                        FAR uint8_t *value)
{
  return bme280_readregs(priv, reg, value, 1);
}

static int bme280_read_calibration(FAR struct bme280_dev_s *priv)
{
  uint8_t calib0[26];
  uint8_t calib1[7];
  int ret;

  ret = bme280_readregs(priv, BME280_REG_CALIB_00, calib0, sizeof(calib0));
  if (ret < 0)
    {
      return ret;
    }

  ret = bme280_readregs(priv, BME280_REG_CALIB_26, calib1, sizeof(calib1));
  if (ret < 0)
    {
      return ret;
    }

  priv->calib.t1 = bme280_get_u16(calib0, 0);
  priv->calib.t2 = bme280_get_s16(calib0, 2);
  priv->calib.t3 = bme280_get_s16(calib0, 4);
  priv->calib.p1 = bme280_get_u16(calib0, 6);
  priv->calib.p2 = bme280_get_s16(calib0, 8);
  priv->calib.p3 = bme280_get_s16(calib0, 10);
  priv->calib.p4 = bme280_get_s16(calib0, 12);
  priv->calib.p5 = bme280_get_s16(calib0, 14);
  priv->calib.p6 = bme280_get_s16(calib0, 16);
  priv->calib.p7 = bme280_get_s16(calib0, 18);
  priv->calib.p8 = bme280_get_s16(calib0, 20);
  priv->calib.p9 = bme280_get_s16(calib0, 22);
  priv->calib.h1 = calib0[25];
  priv->calib.h2 = bme280_get_s16(calib1, 0);
  priv->calib.h3 = calib1[2];
  priv->calib.h4 = bme280_sign_extend12(((uint16_t)calib1[3] << 4) |
                                        (calib1[4] & 0x0f));
  priv->calib.h5 = bme280_sign_extend12(((uint16_t)calib1[5] << 4) |
                                        (calib1[4] >> 4));
  priv->calib.h6 = (int8_t)calib1[6];

  return OK;
}

static int32_t bme280_compensate_temp(FAR struct bme280_dev_s *priv,
                                      int32_t adc_t)
{
  int32_t var1;
  int32_t var2;

  var1 = ((((adc_t >> 3) - ((int32_t)priv->calib.t1 << 1))) *
          (int32_t)priv->calib.t2) >> 11;
  var2 = (((((adc_t >> 4) - (int32_t)priv->calib.t1) *
            ((adc_t >> 4) - (int32_t)priv->calib.t1)) >> 12) *
          (int32_t)priv->calib.t3) >> 14;

  priv->t_fine = var1 + var2;
  return (priv->t_fine * 5 + 128) >> 8;
}

static uint32_t bme280_compensate_press(FAR struct bme280_dev_s *priv,
                                        int32_t adc_p)
{
  int64_t var1;
  int64_t var2;
  int64_t p;

  var1 = (int64_t)priv->t_fine - 128000;
  var2 = var1 * var1 * (int64_t)priv->calib.p6;
  var2 = var2 + ((var1 * (int64_t)priv->calib.p5) << 17);
  var2 = var2 + (((int64_t)priv->calib.p4) << 35);
  var1 = ((var1 * var1 * (int64_t)priv->calib.p3) >> 8) +
         ((var1 * (int64_t)priv->calib.p2) << 12);
  var1 = (((((int64_t)1) << 47) + var1) *
          (int64_t)priv->calib.p1) >> 33;
  if (var1 == 0)
    {
      return 0;
    }

  p = 1048576 - adc_p;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = ((int64_t)priv->calib.p9 * (p >> 13) * (p >> 13)) >> 25;
  var2 = ((int64_t)priv->calib.p8 * p) >> 19;
  p = ((p + var1 + var2) >> 8) + ((int64_t)priv->calib.p7 << 4);

  return (uint32_t)p;
}

static uint32_t bme280_compensate_humidity(FAR struct bme280_dev_s *priv,
                                           int32_t adc_h)
{
  int32_t value;

  value = priv->t_fine - 76800;
  value = (((((adc_h << 14) - ((int32_t)priv->calib.h4 << 20) -
              ((int32_t)priv->calib.h5 * value)) + 16384) >> 15) *
           (((((((value * (int32_t)priv->calib.h6) >> 10) *
                (((value * (int32_t)priv->calib.h3) >> 11) +
                 32768)) >> 10) + 2097152) *
              (int32_t)priv->calib.h2 + 8192) >> 14));
  value = value - (((((value >> 15) * (value >> 15)) >> 7) *
                    (int32_t)priv->calib.h1) >> 4);
  value = value < 0 ? 0 : value;
  value = value > 419430400 ? 419430400 : value;

  return (uint32_t)(value >> 12);
}

static int bme280_wait_ready(FAR struct bme280_dev_s *priv)
{
  uint8_t status;
  int retry;
  int ret;

  for (retry = 0; retry < 20; retry++)
    {
      ret = bme280_read8(priv, BME280_REG_STATUS, &status);
      if (ret < 0)
        {
          return ret;
        }

      if ((status & BME280_STATUS_MEASURING) == 0)
        {
          return OK;
        }

      usleep(2000);
    }

  return -ETIMEDOUT;
}

static int bme280_measure(FAR struct bme280_dev_s *priv,
                          FAR struct bme280_sample_s *sample)
{
  uint8_t data[8];
  int32_t adc_p;
  int32_t adc_t;
  int32_t adc_h;
  int ret;

  ret = bme280_write8(priv, BME280_REG_CTRL_HUM, BME280_CTRL_HUM_VALUE);
  if (ret < 0)
    {
      return ret;
    }

  ret = bme280_write8(priv, BME280_REG_CTRL_MEAS, BME280_CTRL_MEAS_VALUE);
  if (ret < 0)
    {
      return ret;
    }

  usleep(CONFIG_BME280_MEASUREMENT_WAIT_MS * 1000);

  ret = bme280_wait_ready(priv);
  if (ret < 0)
    {
      return ret;
    }

  ret = bme280_readregs(priv, BME280_REG_PRESS_MSB, data, sizeof(data));
  if (ret < 0)
    {
      return ret;
    }

  adc_p = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) |
          ((int32_t)data[2] >> 4);
  adc_t = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) |
          ((int32_t)data[5] >> 4);
  adc_h = ((int32_t)data[6] << 8) | data[7];

  sample->temperature = bme280_compensate_temp(priv, adc_t);
  sample->pressure = bme280_compensate_press(priv, adc_p);
  sample->humidity = bme280_compensate_humidity(priv, adc_h);

  return OK;
}

static int bme280_probe(FAR struct bme280_dev_s *priv)
{
  uint8_t id;
  uint8_t status;
  int retry;
  int ret;

  ret = bme280_read8(priv, BME280_REG_ID, &id);
  if (ret < 0)
    {
      return ret;
    }

  if (id != BME280_CHIP_ID)
    {
      syslog(LOG_ERR, "bme280: unexpected chip id 0x%02x\n", id);
      return -ENODEV;
    }

  priv->chip_id = id;
  syslog(LOG_INFO, "bme280: chip id 0x%02x\n", id);

  ret = bme280_write8(priv, BME280_REG_RESET, BME280_SOFT_RESET);
  if (ret < 0)
    {
      return ret;
    }

  usleep(3000);

  for (retry = 0; retry < 20; retry++)
    {
      ret = bme280_read8(priv, BME280_REG_STATUS, &status);
      if (ret < 0)
        {
          return ret;
        }

      if ((status & BME280_STATUS_IM_UPDATE) == 0)
        {
          break;
        }

      usleep(2000);
    }

  if ((status & BME280_STATUS_IM_UPDATE) != 0)
    {
      return -ETIMEDOUT;
    }

  ret = bme280_read_calibration(priv);
  if (ret < 0)
    {
      return ret;
    }

  ret = bme280_write8(priv, BME280_REG_CONFIG, 0);
  if (ret < 0)
    {
      return ret;
    }

  return bme280_write8(priv, BME280_REG_CTRL_MEAS, BME280_MODE_SLEEP);
}

static int bme280_open(FAR struct file *filep)
{
  return OK;
}

static int bme280_close(FAR struct file *filep)
{
  return OK;
}

static ssize_t bme280_read(FAR struct file *filep, FAR char *buffer,
                           size_t buflen)
{
  FAR struct inode *inode = filep->f_inode;
  FAR struct bme280_dev_s *priv = inode->i_private;
  struct bme280_sample_s sample;
  int ret;

  if (buflen < sizeof(sample))
    {
      return -EINVAL;
    }

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  ret = bme280_measure(priv, &sample);
  nxmutex_unlock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  memcpy(buffer, &sample, sizeof(sample));
  return sizeof(sample);
}

static int bme280_ioctl(FAR struct file *filep, int cmd,
                        unsigned long arg)
{
  FAR struct inode *inode = filep->f_inode;
  FAR struct bme280_dev_s *priv = inode->i_private;
  FAR uint8_t *id = (FAR uint8_t *)((uintptr_t)arg);

  switch (cmd)
    {
      case BME280IOC_READ_ID:
        if (id == NULL)
          {
            return -EINVAL;
          }

        *id = priv->chip_id;
        return OK;

      default:
        return -ENOTTY;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int bme280_register(FAR struct i2c_master_s *i2c,
                    FAR const struct bme280_config_s *config)
{
  FAR struct bme280_dev_s *priv;
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

  ret = bme280_probe(priv);
  if (ret < 0)
    {
      goto err_free;
    }

  ret = register_driver(config->devpath, &g_bme280_fops, 0666, priv);
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

#endif /* CONFIG_I2C && CONFIG_SENSORS_BME280 */
