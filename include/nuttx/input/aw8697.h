/****************************************************************************
 * include/nuttx/input/aw8697.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_INPUT_AW8697_H
#define __INCLUDE_NUTTX_INPUT_AW8697_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/fs/ioctl.h>
#include <nuttx/i2c/i2c_master.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Arg: pointer to uint8_t. Returns the current AW8697 chip ID register. */

#define AW8697IOC_READ_ID _FFIOC(0x40)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct aw8697_config_s
{
  uint8_t address;
  uint32_t frequency;
  uint16_t rated_frequency_hz;
  FAR const char *devpath;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int aw8697_register(FAR struct i2c_master_s *i2c,
                    FAR const struct aw8697_config_s *config);

#endif /* __INCLUDE_NUTTX_INPUT_AW8697_H */
