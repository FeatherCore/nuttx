/****************************************************************************
 * arch/arm/src/stm32h7rs/stm32h7rs_i2c.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_STM32H7RS_I2C_H
#define __ARCH_ARM_SRC_STM32H7RS_STM32H7RS_I2C_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

struct i2c_master_s;

FAR struct i2c_master_s *stm32h7rs_i2cbus_initialize(int port);
int stm32h7rs_i2cbus_uninitialize(FAR struct i2c_master_s *dev);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_ARM_SRC_STM32H7RS_STM32H7RS_I2C_H */
