/****************************************************************************
 * drivers/input/aw8697.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <nuttx/bits.h>
#include <nuttx/debug.h>
#include <nuttx/i2c/i2c_master.h>
#include <nuttx/input/aw8697.h>
#include <nuttx/input/ff.h>
#include <nuttx/kmalloc.h>
#include <nuttx/mutex.h>
#include <nuttx/nuttx.h>
#include <nuttx/wqueue.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define AW8697_REG_ID              0x00
#define AW8697_REG_SYSCTRL         0x04
#define AW8697_REG_GO              0x05
#define AW8697_REG_RTPDATA         0x06
#define AW8697_REG_WAVSEQ1         0x07
#define AW8697_REG_DATDBG          0x39

#define AW8697_CHIP_ID             0x97
#define AW8697_SOFT_RESET          0xaa

#define AW8697_SYSCTRL_STANDBY     0x01
#define AW8697_SYSCTRL_BOOST       0x02
#define AW8697_SYSCTRL_RTP_MODE    0x04
#define AW8697_SYSCTRL_DATMODE_2X  0x40

#define AW8697_GO_ENABLE           0x01
#define AW8697_DEFAULT_GAIN        0x80
#define AW8697_DEFAULT_DURATION_MS 80
#define AW8697_SAMPLE_POSITIVE     0x7f
#define AW8697_SAMPLE_NEGATIVE     0x81
#define AW8697_EFFECT_COUNT_MAX    4

#ifndef CONFIG_FF_AW8697_EFFECTS
#  define CONFIG_FF_AW8697_EFFECTS AW8697_EFFECT_COUNT_MAX
#endif

#ifndef CONFIG_FF_AW8697_RATED_FREQUENCY
#  define CONFIG_FF_AW8697_RATED_FREQUENCY 235
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct aw8697_dev_s
{
  struct ff_lowerhalf_s lower;
  FAR struct i2c_master_s *i2c;
  mutex_t lock;
  struct work_s play_work;
  struct work_s gain_work;
  uint8_t address;
  uint32_t frequency;
  uint16_t rated_frequency_hz;
  uint16_t gain;
  uint16_t duration_ms;
  uint8_t level;
  uint8_t chip_id;
  int current_effect;
  int play_value;
  bool effect_valid[CONFIG_FF_AW8697_EFFECTS];
  struct ff_effect effects[CONFIG_FF_AW8697_EFFECTS];
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int aw8697_write(FAR struct aw8697_dev_s *priv, uint8_t reg,
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

static int aw8697_read(FAR struct aw8697_dev_s *priv, uint8_t reg,
                       FAR uint8_t *value)
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
  msg[1].buffer    = value;
  msg[1].length    = 1;

  ret = I2C_TRANSFER(priv->i2c, msg, 2);
  return ret < 0 ? ret : OK;
}

static uint8_t aw8697_effect_level(FAR struct aw8697_dev_s *priv,
                                   FAR const struct ff_effect *effect)
{
  uint32_t magnitude = 0x7fff;
  uint32_t gain = priv->gain;

  switch (effect->type)
    {
      case FF_RUMBLE:
        magnitude = effect->u.rumble.strong_magnitude;
        if (effect->u.rumble.weak_magnitude > magnitude)
          {
            magnitude = effect->u.rumble.weak_magnitude;
          }
        break;

      case FF_PERIODIC:
        magnitude = effect->u.periodic.magnitude < 0 ?
                    -effect->u.periodic.magnitude :
                    effect->u.periodic.magnitude;
        break;

      case FF_CONSTANT:
        magnitude = effect->u.constant.level < 0 ?
                    -effect->u.constant.level :
                    effect->u.constant.level;
        break;

      default:
        break;
    }

  if (gain == 0)
    {
      gain = 0xffff;
    }

  if (magnitude > 0x7fff)
    {
      magnitude = 0x7fff;
    }

  magnitude = (magnitude * gain) / 0xffff;
  return (uint8_t)(0x20 + ((magnitude * 0xdf) / 0x7fff));
}

static uint16_t aw8697_effect_duration(FAR const struct ff_effect *effect)
{
  if (effect->replay.length != 0)
    {
      return effect->replay.length;
    }

  return AW8697_DEFAULT_DURATION_MS;
}

static int aw8697_stop(FAR struct aw8697_dev_s *priv)
{
  int ret;

  ret = aw8697_write(priv, AW8697_REG_GO, 0);
  if (ret < 0)
    {
      return ret;
    }

  return aw8697_write(priv, AW8697_REG_SYSCTRL,
                      AW8697_SYSCTRL_DATMODE_2X |
                      AW8697_SYSCTRL_BOOST |
                      AW8697_SYSCTRL_STANDBY);
}

static int aw8697_start_rtp(FAR struct aw8697_dev_s *priv, uint8_t level,
                            uint16_t duration_ms)
{
  uint32_t half_period_us;
  uint32_t elapsed_us = 0;
  uint32_t duration_us;
  int ret;

  if (priv->rated_frequency_hz == 0)
    {
      priv->rated_frequency_hz = CONFIG_FF_AW8697_RATED_FREQUENCY;
    }

  half_period_us = 1000000u / priv->rated_frequency_hz / 2u;
  duration_us = (uint32_t)duration_ms * 1000u;

  ret = aw8697_stop(priv);
  if (ret < 0)
    {
      return ret;
    }

  ret = aw8697_write(priv, AW8697_REG_DATDBG, level);
  if (ret < 0)
    {
      return ret;
    }

  ret = aw8697_write(priv, AW8697_REG_SYSCTRL,
                     AW8697_SYSCTRL_DATMODE_2X |
                     AW8697_SYSCTRL_RTP_MODE |
                     AW8697_SYSCTRL_BOOST);
  if (ret < 0)
    {
      return ret;
    }

  ret = aw8697_write(priv, AW8697_REG_GO, AW8697_GO_ENABLE);
  if (ret < 0)
    {
      return ret;
    }

  while (elapsed_us < duration_us)
    {
      ret = aw8697_write(priv, AW8697_REG_RTPDATA, AW8697_SAMPLE_POSITIVE);
      if (ret < 0)
        {
          break;
        }

      usleep(half_period_us);
      elapsed_us += half_period_us;

      ret = aw8697_write(priv, AW8697_REG_RTPDATA, AW8697_SAMPLE_NEGATIVE);
      if (ret < 0)
        {
          break;
        }

      usleep(half_period_us);
      elapsed_us += half_period_us;
    }

  (void)aw8697_stop(priv);
  return ret;
}

static void aw8697_play_work(FAR void *arg)
{
  FAR struct aw8697_dev_s *priv = arg;
  struct ff_effect effect;
  uint16_t duration_ms;
  uint8_t level;
  int effect_id;
  int play_value;

  nxmutex_lock(&priv->lock);
  effect_id = priv->current_effect;
  play_value = priv->play_value;

  if (effect_id >= 0 && effect_id < CONFIG_FF_AW8697_EFFECTS &&
      priv->effect_valid[effect_id])
    {
      memcpy(&effect, &priv->effects[effect_id], sizeof(effect));
    }
  else
    {
      memset(&effect, 0, sizeof(effect));
      effect.type = FF_RUMBLE;
      effect.replay.length = priv->duration_ms;
      effect.u.rumble.strong_magnitude = 0x7fff;
    }

  nxmutex_unlock(&priv->lock);

  if (play_value == 0)
    {
      (void)aw8697_stop(priv);
      return;
    }

  duration_ms = aw8697_effect_duration(&effect);
  level = aw8697_effect_level(priv, &effect);

  (void)aw8697_start_rtp(priv, level, duration_ms);
}

static void aw8697_gain_work(FAR void *arg)
{
  FAR struct aw8697_dev_s *priv = arg;
  uint8_t level;

  nxmutex_lock(&priv->lock);
  level = priv->level;
  nxmutex_unlock(&priv->lock);

  (void)aw8697_write(priv, AW8697_REG_DATDBG, level);
}

static int aw8697_upload(FAR struct ff_lowerhalf_s *lower,
                         FAR struct ff_effect *effect,
                         FAR struct ff_effect *old)
{
  FAR struct aw8697_dev_s *priv =
    container_of(lower, struct aw8697_dev_s, lower);

  if (effect->id < 0 || effect->id >= CONFIG_FF_AW8697_EFFECTS)
    {
      return -EINVAL;
    }

  if (effect->type != FF_RUMBLE && effect->type != FF_PERIODIC &&
      effect->type != FF_CONSTANT)
    {
      return -ENOSYS;
    }

  nxmutex_lock(&priv->lock);
  memcpy(&priv->effects[effect->id], effect, sizeof(*effect));
  priv->effect_valid[effect->id] = true;
  nxmutex_unlock(&priv->lock);

  return OK;
}

static int aw8697_erase(FAR struct ff_lowerhalf_s *lower, int effect_id)
{
  FAR struct aw8697_dev_s *priv =
    container_of(lower, struct aw8697_dev_s, lower);

  if (effect_id < 0 || effect_id >= CONFIG_FF_AW8697_EFFECTS)
    {
      return -EINVAL;
    }

  nxmutex_lock(&priv->lock);
  memset(&priv->effects[effect_id], 0, sizeof(priv->effects[effect_id]));
  priv->effect_valid[effect_id] = false;
  nxmutex_unlock(&priv->lock);

  return OK;
}

static int aw8697_playback(FAR struct ff_lowerhalf_s *lower, int effect_id,
                           int value)
{
  FAR struct aw8697_dev_s *priv =
    container_of(lower, struct aw8697_dev_s, lower);

  nxmutex_lock(&priv->lock);
  priv->current_effect = effect_id;
  priv->play_value = value;
  nxmutex_unlock(&priv->lock);

  work_cancel(LPWORK, &priv->play_work);
  return work_queue(LPWORK, &priv->play_work, aw8697_play_work, priv, 0);
}

static void aw8697_set_gain(FAR struct ff_lowerhalf_s *lower, uint16_t gain)
{
  FAR struct aw8697_dev_s *priv =
    container_of(lower, struct aw8697_dev_s, lower);

  nxmutex_lock(&priv->lock);
  priv->gain = gain;
  priv->level = (uint8_t)(0x20 + (((uint32_t)gain * 0xdf) / 0xffff));
  nxmutex_unlock(&priv->lock);

  work_cancel(LPWORK, &priv->gain_work);
  work_queue(LPWORK, &priv->gain_work, aw8697_gain_work, priv, 0);
}

static int aw8697_control(FAR struct ff_lowerhalf_s *lower, int cmd,
                          unsigned long arg)
{
  FAR struct aw8697_dev_s *priv =
    container_of(lower, struct aw8697_dev_s, lower);
  uint8_t id;
  int ret;

  switch (cmd)
    {
      case AW8697IOC_READ_ID:
        if (arg == 0)
          {
            return -EINVAL;
          }

        ret = aw8697_read(priv, AW8697_REG_ID, &id);
        if (ret < 0)
          {
            return ret;
          }

        priv->chip_id = id;
        *(FAR uint8_t *)(uintptr_t)arg = id;
        return OK;

      default:
        return -ENOTTY;
    }
}

static void aw8697_destroy(FAR struct ff_lowerhalf_s *lower)
{
  FAR struct aw8697_dev_s *priv =
    container_of(lower, struct aw8697_dev_s, lower);

  work_cancel(LPWORK, &priv->play_work);
  work_cancel(LPWORK, &priv->gain_work);
  nxmutex_destroy(&priv->lock);
  kmm_free(priv);
}

static int aw8697_probe(FAR struct aw8697_dev_s *priv)
{
  uint8_t id;
  int ret;

  ret = aw8697_read(priv, AW8697_REG_ID, &id);
  if (ret < 0)
    {
      ierr("aw8697: failed to read chip id: %d\n", ret);
      return ret;
    }

  if (id != AW8697_CHIP_ID)
    {
      ierr("aw8697: unexpected chip id 0x%02x\n", id);
      return -ENODEV;
    }

  priv->chip_id = id;
  syslog(LOG_INFO, "aw8697: chip id 0x%02x\n", id);

  ret = aw8697_write(priv, AW8697_REG_ID, AW8697_SOFT_RESET);
  if (ret < 0)
    {
      return ret;
    }

  usleep(1500);

  ret = aw8697_write(priv, AW8697_REG_SYSCTRL,
                     AW8697_SYSCTRL_DATMODE_2X |
                     AW8697_SYSCTRL_BOOST |
                     AW8697_SYSCTRL_STANDBY);
  if (ret < 0)
    {
      return ret;
    }

  ret = aw8697_write(priv, AW8697_REG_WAVSEQ1, 0);
  if (ret < 0)
    {
      return ret;
    }

  return aw8697_write(priv, AW8697_REG_DATDBG, AW8697_DEFAULT_GAIN);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int aw8697_register(FAR struct i2c_master_s *i2c,
                    FAR const struct aw8697_config_s *config)
{
  FAR struct aw8697_dev_s *priv;
  FAR struct ff_lowerhalf_s *lower;
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

  nxmutex_init(&priv->lock);
  priv->i2c = i2c;
  priv->address = config->address;
  priv->frequency = config->frequency;
  priv->rated_frequency_hz = config->rated_frequency_hz;
  priv->gain = 0xffff;
  priv->level = AW8697_DEFAULT_GAIN;
  priv->duration_ms = AW8697_DEFAULT_DURATION_MS;
  priv->current_effect = -1;

  ret = aw8697_probe(priv);
  if (ret < 0)
    {
      nxmutex_destroy(&priv->lock);
      kmm_free(priv);
      return ret;
    }

  lower = &priv->lower;
  lower->upload = aw8697_upload;
  lower->erase = aw8697_erase;
  lower->playback = aw8697_playback;
  lower->set_gain = aw8697_set_gain;
  lower->control = aw8697_control;
  lower->destroy = aw8697_destroy;

  set_bit(FF_RUMBLE, lower->ffbit);
  set_bit(FF_PERIODIC, lower->ffbit);
  set_bit(FF_CONSTANT, lower->ffbit);
  set_bit(FF_GAIN, lower->ffbit);

  ret = ff_register(lower, config->devpath, CONFIG_FF_AW8697_EFFECTS);
  if (ret < 0)
    {
      nxmutex_destroy(&priv->lock);
      kmm_free(priv);
    }

  return ret;
}
