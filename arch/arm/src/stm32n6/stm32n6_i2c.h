/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_i2c.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_I2C_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_I2C_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/i2c/i2c_master.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR struct i2c_master_s *stm32n6_i2cbus_initialize(int port);
int stm32n6_i2cbus_uninitialize(FAR struct i2c_master_s *dev);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_I2C_H */
