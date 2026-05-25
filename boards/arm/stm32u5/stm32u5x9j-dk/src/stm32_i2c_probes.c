/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32_i2c_probes.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <syslog.h>

#include <nuttx/arch.h>
#include <nuttx/fs/fs.h>
#include <nuttx/i2c/i2c_master.h>

#include "stm32_gpio.h"
#include "stm32u5x9j-dk.h"

#ifdef HAVE_I2C_PROBES

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STTS22H_ADDR           0x7f
#define STTS22H_WHOAMI         0x01
#define STTS22H_ID             0xa0
#define STTS22H_CTRL           0x04
#define STTS22H_STATUS         0x05
#define STTS22H_TEMP_L_OUT     0x06
#define STTS22H_CTRL_ONE_SHOT  (1 << 0)
#define STTS22H_CTRL_IF_ADD_INC (1 << 3)
#define STTS22H_CTRL_BDU       (1 << 6)
#define STTS22H_STATUS_BUSY    (1 << 0)

#define VL53L5CX_ADDR          0x29
#define VL53L5CX_ID            0xf002
#define VL53L5CX_BANK_REG      0x7fff
#define VL53L5CX_DEVICE_REG    0x0000
#define VL53L5CX_REVISION_REG  0x0001
#define VL53L5CX_ZONES         64
#define VL53L5CX_RANGING_ERR   (-ENOTSUP)

#ifndef CONFIG_STM32U5X9J_DK_TOUCH
#define SITRONIX_ADDR          0x70
#define SITRONIX_ID            0x02
#define SITRONIX_BUFSIZE       28
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR struct i2c_master_s *g_i2c3;
static FAR struct i2c_master_s *g_i2c5;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ssize_t stm32_temp_read(FAR struct file *filep,
                                    FAR char *buffer, size_t buflen);
static ssize_t stm32_range_read(FAR struct file *filep,
                                     FAR char *buffer, size_t buflen);
#ifndef CONFIG_STM32U5X9J_DK_TOUCH
static ssize_t stm32_touch_read(FAR struct file *filep,
                                     FAR char *buffer, size_t buflen);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_temp_fops =
{
  NULL,                 /* open */
  NULL,                 /* close */
  stm32_temp_read, /* read */
};

static const struct file_operations g_range_fops =
{
  NULL,                  /* open */
  NULL,                  /* close */
  stm32_range_read, /* read */
};

#ifndef CONFIG_STM32U5X9J_DK_TOUCH
static const struct file_operations g_touch_fops =
{
  NULL,                  /* open */
  NULL,                  /* close */
  stm32_touch_read, /* read */
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32_i2c_read(FAR struct i2c_master_s *i2c,
                               uint8_t addr, FAR const uint8_t *reg,
                               int reglen, FAR uint8_t *buf, int buflen)
{
  struct i2c_msg_s msg[2];

  if (i2c == NULL)
    {
      return -ENODEV;
    }

  if (reglen > 0)
    {
      msg[0].frequency = I2C_SPEED_FAST;
      msg[0].addr      = addr;
      msg[0].flags     = I2C_M_NOSTOP;
      msg[0].buffer    = (FAR uint8_t *)reg;
      msg[0].length    = reglen;
      msg[1].frequency = I2C_SPEED_FAST;
      msg[1].addr      = addr;
      msg[1].flags     = I2C_M_READ;
      msg[1].buffer    = buf;
      msg[1].length    = buflen;

      return I2C_TRANSFER(i2c, msg, 2);
    }

  msg[0].frequency = I2C_SPEED_FAST;
  msg[0].addr      = addr;
  msg[0].flags     = I2C_M_READ;
  msg[0].buffer    = buf;
  msg[0].length    = buflen;

  return I2C_TRANSFER(i2c, msg, 1);
}

static int stm32_i2c_write(FAR struct i2c_master_s *i2c,
                                uint8_t addr, FAR const uint8_t *reg,
                                int reglen, FAR const uint8_t *buf,
                                int buflen)
{
  struct i2c_msg_s msg;
  uint8_t tx[8];

  if (i2c == NULL)
    {
      return -ENODEV;
    }

  if (reglen + buflen > sizeof(tx))
    {
      return -EINVAL;
    }

  memcpy(tx, reg, reglen);
  memcpy(tx + reglen, buf, buflen);

  msg.frequency = I2C_SPEED_FAST;
  msg.addr      = addr;
  msg.flags     = 0;
  msg.buffer    = tx;
  msg.length    = reglen + buflen;

  return I2C_TRANSFER(i2c, &msg, 1);
}

static int stm32_read_u8(FAR struct i2c_master_s *i2c, uint8_t addr,
                              uint8_t reg, FAR uint8_t *val)
{
  return stm32_i2c_read(i2c, addr, &reg, 1, val, 1);
}

static int stm32_write_u8(FAR struct i2c_master_s *i2c, uint8_t addr,
                               uint8_t reg, uint8_t val)
{
  return stm32_i2c_write(i2c, addr, &reg, 1, &val, 1);
}

static int stm32_read_u16reg_u8(FAR struct i2c_master_s *i2c,
                                      uint8_t addr, uint16_t reg,
                                      FAR uint8_t *val)
{
  uint8_t regbuf[2];

  regbuf[0] = reg >> 8;
  regbuf[1] = reg & 0xff;

  return stm32_i2c_read(i2c, addr, regbuf, 2, val, 1);
}

static int stm32_write_u16reg_u8(FAR struct i2c_master_s *i2c,
                                      uint8_t addr, uint16_t reg,
                                      uint8_t val)
{
  uint8_t regbuf[2];

  regbuf[0] = reg >> 8;
  regbuf[1] = reg & 0xff;

  return stm32_i2c_write(i2c, addr, regbuf, 2, &val, 1);
}

static int stm32_stts22h_configure(void)
{
  uint8_t ctrl = STTS22H_CTRL_BDU | STTS22H_CTRL_IF_ADD_INC;

  return stm32_write_u8(g_i2c3, STTS22H_ADDR, STTS22H_CTRL, ctrl);
}

static int stm32_stts22h_oneshot(FAR int16_t *raw)
{
  uint8_t ctrl;
  uint8_t status;
  uint8_t reg = STTS22H_TEMP_L_OUT;
  uint8_t rawbuf[2];
  int ret;
  int retry;

  ret = stm32_read_u8(g_i2c3, STTS22H_ADDR, STTS22H_CTRL, &ctrl);
  if (ret < 0)
    {
      return ret;
    }

  ctrl |= STTS22H_CTRL_BDU | STTS22H_CTRL_IF_ADD_INC |
          STTS22H_CTRL_ONE_SHOT;

  ret = stm32_write_u8(g_i2c3, STTS22H_ADDR, STTS22H_CTRL, ctrl);
  if (ret < 0)
    {
      return ret;
    }

  for (retry = 0; retry < 50; retry++)
    {
      ret = stm32_read_u8(g_i2c3, STTS22H_ADDR, STTS22H_STATUS,
                               &status);
      if (ret < 0)
        {
          return ret;
        }

      if ((status & STTS22H_STATUS_BUSY) == 0)
        {
          ret = stm32_i2c_read(g_i2c3, STTS22H_ADDR, &reg, 1,
                                    rawbuf, 2);
          if (ret < 0)
            {
              return ret;
            }

          *raw = ((int16_t)rawbuf[1] << 8) | rawbuf[0];
          return OK;
        }

      up_mdelay(2);
    }

  return -ETIMEDOUT;
}

static int stm32_vl53l5cx_wakeup(void)
{
  stm32_gpiowrite(GPIO_VL53L5CX_LP, false);
  up_mdelay(2);
  stm32_gpiowrite(GPIO_VL53L5CX_LP, true);
  up_mdelay(10);

  return OK;
}

static int stm32_vl53l5cx_readid(FAR uint16_t *id,
                                      FAR uint8_t *device,
                                      FAR uint8_t *revision)
{
  int ret;

  ret = stm32_write_u16reg_u8(g_i2c3, VL53L5CX_ADDR,
                                   VL53L5CX_BANK_REG, 0x00);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_read_u16reg_u8(g_i2c3, VL53L5CX_ADDR,
                                  VL53L5CX_DEVICE_REG, device);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_read_u16reg_u8(g_i2c3, VL53L5CX_ADDR,
                                  VL53L5CX_REVISION_REG, revision);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_write_u16reg_u8(g_i2c3, VL53L5CX_ADDR,
                                   VL53L5CX_BANK_REG, 0x02);
  if (ret < 0)
    {
      return ret;
    }

  *id = ((uint16_t)*device << 8) | *revision;
  return OK;
}

static ssize_t stm32_copyout(FAR struct file *filep, FAR char *buffer,
                                  size_t buflen, FAR const char *line)
{
  size_t len;

  if (filep->f_pos > 0)
    {
      return 0;
    }

  if (buflen == 0)
    {
      return 0;
    }

  len = strnlen(line, buflen);
  memcpy(buffer, line, len);
  filep->f_pos += len;
  return len;
}

static ssize_t stm32_temp_read(FAR struct file *filep,
                                    FAR char *buffer, size_t buflen)
{
  uint8_t whoami;
  char line[96];
  int16_t raw;
  int ret;

  ret = stm32_read_u8(g_i2c3, STTS22H_ADDR, STTS22H_WHOAMI, &whoami);
  if (ret < 0)
    {
      return ret;
    }

  if (whoami != STTS22H_ID)
    {
      return -ENODEV;
    }

  ret = stm32_stts22h_oneshot(&raw);
  if (ret < 0)
    {
      return ret;
    }

  snprintf(line, sizeof(line),
           "stts22h id=0x%02x raw=%d milliC=%" PRId32
           " oneshot=ok bdu=1 auto_increment=1\n",
           whoami, raw, (int32_t)raw * 10);

  return stm32_copyout(filep, buffer, buflen, line);
}

static ssize_t stm32_range_read(FAR struct file *filep,
                                     FAR char *buffer, size_t buflen)
{
  uint16_t id;
  uint8_t device;
  uint8_t revision;
  char line[160];
  int ret;

  ret = stm32_vl53l5cx_readid(&id, &device, &revision);
  if (ret < 0)
    {
      return ret;
    }

  snprintf(line, sizeof(line),
           "vl53l5cx id=0x%04x device=0x%02x rev=0x%02x alive=%u "
           "lp_wakeup=ok zones=%u ranging=%d reason=firmware_config_blocked\n",
           id, device, revision, id == VL53L5CX_ID, VL53L5CX_ZONES,
           VL53L5CX_RANGING_ERR);

  return stm32_copyout(filep, buffer, buflen, line);
}

#ifndef CONFIG_STM32U5X9J_DK_TOUCH
static ssize_t stm32_touch_read(FAR struct file *filep,
                                     FAR char *buffer, size_t buflen)
{
  uint8_t buf[SITRONIX_BUFSIZE];
  uint32_t x;
  uint32_t y;
  char line[80];
  int ret;

  ret = stm32_i2c_read(g_i2c5, SITRONIX_ADDR, NULL, 0,
                            buf, sizeof(buf));
  if (ret < 0)
    {
      return ret;
    }

  x = (((uint32_t)buf[2] & 0x70) << 4) | buf[3];
  y = (((uint32_t)buf[2] & 0x07) << 8) | buf[4];

  snprintf(line, sizeof(line), "sitronix id=0x%02x touches=%u x=%lu y=%lu\n",
           buf[0], buf[1], (unsigned long)x, (unsigned long)y);

  return stm32_copyout(filep, buffer, buflen, line);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_i2c_probes_initialize(FAR struct i2c_master_s *i2c3,
                                     FAR struct i2c_master_s *i2c5)
{
#ifdef CONFIG_STM32U5X9J_DK_VL53L5CX
  uint16_t rangeid;
  uint8_t range_device;
  uint8_t range_revision;
#endif
#ifdef CONFIG_STM32U5X9J_DK_STTS22H
  uint8_t tempid;
#endif
#ifndef CONFIG_STM32U5X9J_DK_TOUCH
  uint8_t touchid[SITRONIX_BUFSIZE];
#endif
  int ret;

  g_i2c3 = i2c3;
  g_i2c5 = i2c5;

  stm32_configgpio(GPIO_TS_INT);
  stm32_configgpio(GPIO_VL53L5CX_LP);
  stm32_configgpio(GPIO_VL53L5CX_INT);

#ifdef CONFIG_STM32U5X9J_DK_STTS22H
  ret = register_driver("/dev/temp0", &g_temp_fops, 0444, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: register /dev/temp0 failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_STM32U5X9J_DK_VL53L5CX
  ret = register_driver("/dev/range0", &g_range_fops, 0444, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: register /dev/range0 failed: %d\n", ret);
    }
#endif

#ifndef CONFIG_STM32U5X9J_DK_TOUCH
  ret = register_driver("/dev/input0", &g_touch_fops, 0444, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: register /dev/input0 failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_STM32U5X9J_DK_STTS22H
  ret = stm32_read_u8(g_i2c3, STTS22H_ADDR, STTS22H_WHOAMI, &tempid);
  if (ret == OK)
    {
      syslog(LOG_INFO, "stm32u5x9j: STTS22H id=0x%02x\n", tempid);

      if (tempid != STTS22H_ID)
        {
          syslog(LOG_WARNING,
                 "stm32u5x9j: unexpected STTS22H id=0x%02x expected=0x%02x\n",
                 tempid, STTS22H_ID);
        }

      ret = stm32_stts22h_configure();
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: STTS22H configure failed: %d\n", ret);
        }
    }
#endif

#ifdef CONFIG_STM32U5X9J_DK_VL53L5CX
  ret = stm32_vl53l5cx_wakeup();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: VL53L5CX wakeup failed: %d\n", ret);
    }

  ret = stm32_vl53l5cx_readid(&rangeid, &range_device,
                                   &range_revision);
  if (ret == OK)
    {
      syslog(LOG_INFO,
             "stm32u5x9j: VL53L5CX id=0x%04x device=0x%02x rev=0x%02x "
             "ranging=blocked\n",
             rangeid, range_device, range_revision);

      if (rangeid != VL53L5CX_ID)
        {
          syslog(LOG_WARNING,
                 "stm32u5x9j: unexpected VL53L5CX id=0x%04x expected=0x%04x\n",
                 rangeid, VL53L5CX_ID);
        }
    }
#endif

#ifndef CONFIG_STM32U5X9J_DK_TOUCH
  ret = stm32_i2c_read(g_i2c5, SITRONIX_ADDR, NULL, 0,
                            touchid, sizeof(touchid));
  if (ret == OK)
    {
      syslog(LOG_INFO, "stm32u5x9j: Sitronix id=0x%02x\n", touchid[0]);
    }
#endif

  return OK;
}

#endif /* HAVE_I2C_PROBES */
