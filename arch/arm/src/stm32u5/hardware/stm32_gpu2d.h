/****************************************************************************
 * arch/arm/src/stm32u5/hardware/stm32_gpu2d.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_GPU2D_H
#define __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_GPU2D_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* STM32U5G9 GPU2D/NemaP register offsets.  The STM32 HAL exposes only the
 * generic access model; these offsets are from the U5G9 TouchGFX/NemaGFX
 * integration and the NemaP command/register documentation.
 */

#define STM32_GPU2D_TEX0_BASE             (STM32_GPU2D_BASE + 0x000)
#define STM32_GPU2D_TEX0_FSTRIDE          (STM32_GPU2D_BASE + 0x004)
#define STM32_GPU2D_TEX0_RESXY            (STM32_GPU2D_BASE + 0x008)
#define STM32_GPU2D_TEX1_BASE             (STM32_GPU2D_BASE + 0x010)
#define STM32_GPU2D_TEX1_FSTRIDE          (STM32_GPU2D_BASE + 0x014)
#define STM32_GPU2D_TEX1_RESXY            (STM32_GPU2D_BASE + 0x018)
#define STM32_GPU2D_TEX2_BASE             (STM32_GPU2D_BASE + 0x020)
#define STM32_GPU2D_TEX2_FSTRIDE          (STM32_GPU2D_BASE + 0x024)
#define STM32_GPU2D_TEX2_RESXY            (STM32_GPU2D_BASE + 0x028)
#define STM32_GPU2D_TEX3_BASE             (STM32_GPU2D_BASE + 0x030)
#define STM32_GPU2D_TEX3_FSTRIDE          (STM32_GPU2D_BASE + 0x034)
#define STM32_GPU2D_TEX3_RESXY            (STM32_GPU2D_BASE + 0x038)

#define STM32_GPU2D_BREAKPOINT            (STM32_GPU2D_BASE + 0x080)
#define STM32_GPU2D_DIRTYMIN              (STM32_GPU2D_BASE + 0x098)
#define STM32_GPU2D_DIRTYMAX              (STM32_GPU2D_BASE + 0x09c)
#define STM32_GPU2D_IMEM_ADDR             (STM32_GPU2D_BASE + 0x0c4)
#define STM32_GPU2D_IMEM_DATAL            (STM32_GPU2D_BASE + 0x0c8)
#define STM32_GPU2D_IMEM_DATAH            (STM32_GPU2D_BASE + 0x0cc)
#define STM32_GPU2D_FLUSH_CTRL            (STM32_GPU2D_BASE + 0x0e4)
#define STM32_GPU2D_CMDSTATUS             (STM32_GPU2D_BASE + 0x0e8)
#define STM32_GPU2D_CMDRINGSTOP           (STM32_GPU2D_BASE + 0x0ec)
#define STM32_GPU2D_CMDADDR               (STM32_GPU2D_BASE + 0x0f0)
#define STM32_GPU2D_CMDSIZE               (STM32_GPU2D_BASE + 0x0f4)
#define STM32_GPU2D_INTERRUPT             (STM32_GPU2D_BASE + 0x0f8)
#define STM32_GPU2D_STATUS                (STM32_GPU2D_BASE + 0x0fc)

#define STM32_GPU2D_DRAW_CMD              (STM32_GPU2D_BASE + 0x100)
#define STM32_GPU2D_CLIPMIN               (STM32_GPU2D_BASE + 0x110)
#define STM32_GPU2D_CLIPMAX               (STM32_GPU2D_BASE + 0x114)
#define STM32_GPU2D_MATMULT               (STM32_GPU2D_BASE + 0x118)
#define STM32_GPU2D_CODEPTR               (STM32_GPU2D_BASE + 0x11c)
#define STM32_GPU2D_CLID                  (STM32_GPU2D_BASE + 0x148)
#define STM32_GPU2D_CLIPMIN2              (STM32_GPU2D_BASE + 0x158)
#define STM32_GPU2D_CLIPMAX2              (STM32_GPU2D_BASE + 0x15c)
#define STM32_GPU2D_MM00                  (STM32_GPU2D_BASE + 0x160)
#define STM32_GPU2D_MM01                  (STM32_GPU2D_BASE + 0x164)
#define STM32_GPU2D_MM02                  (STM32_GPU2D_BASE + 0x168)
#define STM32_GPU2D_MM10                  (STM32_GPU2D_BASE + 0x16c)
#define STM32_GPU2D_MM11                  (STM32_GPU2D_BASE + 0x170)
#define STM32_GPU2D_MM12                  (STM32_GPU2D_BASE + 0x174)
#define STM32_GPU2D_MM20                  (STM32_GPU2D_BASE + 0x178)
#define STM32_GPU2D_MM21                  (STM32_GPU2D_BASE + 0x17c)
#define STM32_GPU2D_MM22                  (STM32_GPU2D_BASE + 0x180)
#define STM32_GPU2D_ROP_BLEND_MODE        (STM32_GPU2D_BASE + 0x1d0)
#define STM32_GPU2D_ROP_DST_CKEY          (STM32_GPU2D_BASE + 0x1d4)
#define STM32_GPU2D_ROP_CONST_COLOR       (STM32_GPU2D_BASE + 0x1d8)
#define STM32_GPU2D_IP_VERSION            (STM32_GPU2D_BASE + 0x1dc)
#define STM32_GPU2D_DEPTH_CTRL            (STM32_GPU2D_BASE + 0x1e0)
#define STM32_GPU2D_ID                    (STM32_GPU2D_BASE + 0x1ec)
#define STM32_GPU2D_CONFIG                (STM32_GPU2D_BASE + 0x1f0)
#define STM32_GPU2D_CONFIGH               (STM32_GPU2D_BASE + 0x1f4)
#define STM32_GPU2D_CORE_SELECT           (STM32_GPU2D_BASE + 0x1fc)
#define STM32_GPU2D_CONST0                (STM32_GPU2D_BASE + 0x200)
#define STM32_GPU2D_CONST1                (STM32_GPU2D_BASE + 0x204)
#define STM32_GPU2D_FRAG_CONADDR          (STM32_GPU2D_BASE + 0x210)
#define STM32_GPU2D_FRAG_CONDATA          (STM32_GPU2D_BASE + 0x214)
#define STM32_GPU2D_DBG_STATUS            (STM32_GPU2D_BASE + 0x2f0)
#define STM32_GPU2D_DBG_ADDR              (STM32_GPU2D_BASE + 0x2f4)
#define STM32_GPU2D_DBG_DATA              (STM32_GPU2D_BASE + 0x2f8)
#define STM32_GPU2D_DBG_CTRL              (STM32_GPU2D_BASE + 0x2fc)
#define STM32_GPU2D_TEX0_BASE_H           (STM32_GPU2D_BASE + 0x804)
#define STM32_GPU2D_TEX1_BASE_H           (STM32_GPU2D_BASE + 0x80c)
#define STM32_GPU2D_TEX2_BASE_H           (STM32_GPU2D_BASE + 0x814)
#define STM32_GPU2D_TEX3_BASE_H           (STM32_GPU2D_BASE + 0x81c)
#define STM32_GPU2D_IRQ_ID                (STM32_GPU2D_BASE + 0xff0)
#define STM32_GPU2D_GP_FLAGS              (STM32_GPU2D_BASE + 0xff4)
#define STM32_GPU2D_SYS_INTERRUPT         (STM32_GPU2D_BASE + 0xff8)
#define STM32_GPU2D_SYS_ERROR_MASK        (STM32_GPU2D_BASE + 0xffc)

#define GPU2D_ID_EXPECTED                 0x86362000

#define GPU2D_INTERRUPT_CLC               (1 << 0)
#define GPU2D_CL_NOP                      (1 << 16)
#define GPU2D_CL_PUSH                     (1 << 17)
#define GPU2D_CL_RETURN                   (1 << 18)
#define GPU2D_CL_ABORT                    (1 << 19)
#define GPU2D_CL_HOLD                     0xff000000
#define GPU2D_CL_REG_MASK                 0x00000fff
#define GPU2D_RINGSTOP_NOTRIGGER          (1 << 1)
#define GPU2D_RINGSTOP_ENABLE             (1 << 2)
#define GPU2D_CLID_MASK                   0x00ffffff

#define GPU2D_CONFIG_AXI_MASTER           (1 << 31)
#define GPU2D_CONFIG_TEX_FILTER           (1 << 30)
#define GPU2D_CONFIG_TSC6                 (1 << 29)
#define GPU2D_CONFIG_BLENDER              (1 << 28)
#define GPU2D_CONFIG_TSC                  (1 << 17)
#define GPU2D_CONFIG_CLOCK_GATE           (1 << 16)
#define GPU2D_CONFIG_DEBUG_LEVEL_SHIFT    12
#define GPU2D_CONFIG_DEBUG_LEVEL_MASK     (0x0f << GPU2D_CONFIG_DEBUG_LEVEL_SHIFT)
#define GPU2D_CONFIG_CORE_COUNT_SHIFT     8
#define GPU2D_CONFIG_CORE_COUNT_MASK      (0x0f << GPU2D_CONFIG_CORE_COUNT_SHIFT)
#define GPU2D_CONFIG_THREAD_COUNT_MASK    0x000000ff

#define GPU2D_CL_REG(reg)                 ((reg) & GPU2D_CL_REG_MASK)
#define GPU2D_CL_CMD(reg, flags)          (GPU2D_CL_REG(reg) | \
                                           ((flags) & ~GPU2D_CL_REG_MASK))
#define GPU2D_IS_ALIGNED_8(addr)          ((((uintptr_t)(addr)) & 7) == 0)

#endif /* __ARCH_ARM_SRC_STM32U5_HARDWARE_STM32_GPU2D_H */
