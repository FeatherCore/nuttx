/****************************************************************************
 * arch/arm/src/armv8-m/arm_control.h
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

#ifndef __ARCH_ARM_SRC_ARMV8_M_ARM_CONTROL_H
#define __ARCH_ARM_SRC_ARMV8_M_ARM_CONTROL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifndef __ASSEMBLY__
#  include <stdbool.h>
#  include <stdint.h>

#  include <nuttx/irq.h>

#  include "exc_return.h"
#endif

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

#ifndef __ASSEMBLY__
static inline uint32_t arm_control_sync_stack(uint32_t control,
                                              uint32_t excreturn)
{
#  if CONFIG_ARCH_INTERRUPTSTACK > 7
  if ((excreturn & EXC_RETURN_PROCESS_STACK) != 0)
    {
      control |= CONTROL_SPSEL;
    }
  else
    {
      control &= ~CONTROL_SPSEL;
    }
#  endif

  return control;
}

static inline uint32_t arm_control_set_mode(uint32_t control,
                                            uint32_t excreturn,
                                            bool unprivileged)
{
  control = arm_control_sync_stack(control, excreturn);

#  ifdef CONFIG_ARMV8M_LAZYFPU
  /* Keep CONTROL.FPCA consistent with the frame the processor will unstack.
   * With lazy FPU enabled, integer-only threads should return with FPCA clear
   * and a basic frame; threads with an extended frame must return with FPCA
   * set so later exceptions preserve the active FP state correctly.
   */

  if ((excreturn & EXC_RETURN_STD_CONTEXT) != 0)
    {
      control &= ~(CONTROL_FPCA | CONTROL_SFPA);
    }
  else
    {
      control |= CONTROL_FPCA;
    }
#  endif

  if (unprivileged)
    {
      control |= CONTROL_NPRIV;
    }
  else
    {
      control &= ~CONTROL_NPRIV;
    }

  return control;
}
#endif

#endif /* __ARCH_ARM_SRC_ARMV8_M_ARM_CONTROL_H */
