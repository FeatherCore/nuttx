/****************************************************************************
 * arch/arm/src/stm32h7rs/hardware/stm32_gpu2d.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_GPU2D_H
#define __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_GPU2D_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* STM32H7RS GPU2D/NemaP register offsets.  The STM32H7RS HAL exposes GPU2D as a
 * raw base address plus offset; the offsets match the NemaP register map used
 * by the STM32U5 integration and the H21.11 reference surface.
 */

#define STM32_GPU2D_TEX0_BASE             (STM32_GPU2D_BASE + 0x000)
#define STM32_GPU2D_TEX0_FSTRIDE          (STM32_GPU2D_BASE + 0x004)
#define STM32_GPU2D_TEX0_RESXY            (STM32_GPU2D_BASE + 0x008)
#define STM32_GPU2D_TEX1_BASE             (STM32_GPU2D_BASE + 0x010)
#define STM32_GPU2D_TEX1_FSTRIDE          (STM32_GPU2D_BASE + 0x014)
#define STM32_GPU2D_TEX1_RESXY            (STM32_GPU2D_BASE + 0x018)
#define STM32_GPU2D_TEX_COLOR             (STM32_GPU2D_BASE + 0x01c)
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
#define STM32_GPU2D_IMEM_DATAH            (STM32_GPU2D_BASE + 0x0c8)
#define STM32_GPU2D_IMEM_DATAL            (STM32_GPU2D_BASE + 0x0cc)
#define STM32_GPU2D_FLUSH_CTRL            (STM32_GPU2D_BASE + 0x0e4)
#define STM32_GPU2D_CMDSTATUS             (STM32_GPU2D_BASE + 0x0e8)
#define STM32_GPU2D_CMDRINGSTOP           (STM32_GPU2D_BASE + 0x0ec)
#define STM32_GPU2D_CMDADDR               (STM32_GPU2D_BASE + 0x0f0)
#define STM32_GPU2D_CMDSIZE               (STM32_GPU2D_BASE + 0x0f4)
#define STM32_GPU2D_INTERRUPT             (STM32_GPU2D_BASE + 0x0f8)
#define STM32_GPU2D_STATUS                (STM32_GPU2D_BASE + 0x0fc)

#define STM32_GPU2D_DRAW_CMD              (STM32_GPU2D_BASE + 0x100)
#define STM32_GPU2D_DRAW_STARTXY          (STM32_GPU2D_BASE + 0x104)
#define STM32_GPU2D_DRAW_ENDXY            (STM32_GPU2D_BASE + 0x108)
#define STM32_GPU2D_CLIPMIN               (STM32_GPU2D_BASE + 0x110)
#define STM32_GPU2D_CLIPMAX               (STM32_GPU2D_BASE + 0x114)
#define STM32_GPU2D_MATMULT               (STM32_GPU2D_BASE + 0x118)
#define STM32_GPU2D_CODEPTR               (STM32_GPU2D_BASE + 0x11c)
#define STM32_GPU2D_DRAW_PT0_X            (STM32_GPU2D_BASE + 0x120)
#define STM32_GPU2D_DRAW_PT0_Y            (STM32_GPU2D_BASE + 0x124)
#define STM32_GPU2D_DRAW_COLOR            (STM32_GPU2D_BASE + 0x12c)
#define STM32_GPU2D_DRAW_PT1_X            (STM32_GPU2D_BASE + 0x130)
#define STM32_GPU2D_DRAW_PT1_Y            (STM32_GPU2D_BASE + 0x134)
#define STM32_GPU2D_DRAW_PT2_X            (STM32_GPU2D_BASE + 0x140)
#define STM32_GPU2D_DRAW_PT2_Y            (STM32_GPU2D_BASE + 0x144)
#define STM32_GPU2D_CLID                  (STM32_GPU2D_BASE + 0x148)
#define STM32_GPU2D_DRAW_PT3_X            (STM32_GPU2D_BASE + 0x150)
#define STM32_GPU2D_DRAW_PT3_Y            (STM32_GPU2D_BASE + 0x154)
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
#define STM32_GPU2D_RED_DX                (STM32_GPU2D_BASE + 0x1a0)
#define STM32_GPU2D_RED_DY                (STM32_GPU2D_BASE + 0x1a4)
#define STM32_GPU2D_GREEN_DX              (STM32_GPU2D_BASE + 0x1a8)
#define STM32_GPU2D_GREEN_DY              (STM32_GPU2D_BASE + 0x1ac)
#define STM32_GPU2D_BLUE_DX               (STM32_GPU2D_BASE + 0x1b0)
#define STM32_GPU2D_BLUE_DY               (STM32_GPU2D_BASE + 0x1b4)
#define STM32_GPU2D_ALPHA_DX              (STM32_GPU2D_BASE + 0x1b8)
#define STM32_GPU2D_ALPHA_DY              (STM32_GPU2D_BASE + 0x1bc)
#define STM32_GPU2D_RED_INIT              (STM32_GPU2D_BASE + 0x1c0)
#define STM32_GPU2D_GREEN_INIT            (STM32_GPU2D_BASE + 0x1c4)
#define STM32_GPU2D_BLUE_INIT             (STM32_GPU2D_BASE + 0x1c8)
#define STM32_GPU2D_ALPHA_INIT            (STM32_GPU2D_BASE + 0x1cc)
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
#define STM32_GPU2D_CONST2                (STM32_GPU2D_BASE + 0x208)
#define STM32_GPU2D_CONST3                (STM32_GPU2D_BASE + 0x20c)
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
#define GPU2D_CONFIG_ASYNC                (1 << 27)
#define GPU2D_CONFIG_DIRTY                (1 << 26)
#define GPU2D_CONFIG_MMU                  (1 << 21)
#define GPU2D_CONFIG_ZCOMPR               (1 << 20)
#define GPU2D_CONFIG_VRX                  (1 << 19)
#define GPU2D_CONFIG_ZBUF                 (1 << 18)
#define GPU2D_CONFIG_TSC                  (1 << 17)
#define GPU2D_CONFIG_CLOCK_GATE           (1 << 16)
#define GPU2D_CONFIG_VG                   (1 << 15)
#define GPU2D_CONFIG_DEBUG_LEVEL_SHIFT    12
#define GPU2D_CONFIG_DEBUG_LEVEL_MASK     (0x0f << GPU2D_CONFIG_DEBUG_LEVEL_SHIFT)
#define GPU2D_CONFIG_CORE_COUNT_SHIFT     8
#define GPU2D_CONFIG_CORE_COUNT_MASK      (0x0f << GPU2D_CONFIG_CORE_COUNT_SHIFT)
#define GPU2D_CONFIG_THREAD_COUNT_MASK    0x000000ff

#define GPU2D_CONFIGH_AA                  (1 << 0)
#define GPU2D_CONFIGH_DEC                 (1 << 1)
#define GPU2D_CONFIGH_10BIT               (1 << 2)
#define GPU2D_CONFIGH_GAMMA               (1 << 3)
#define GPU2D_CONFIGH_YUV_COEFF           (1 << 4)
#define GPU2D_CONFIGH_TEX_CHANNELS        (1 << 5)
#define GPU2D_CONFIGH_MBIST               (1 << 6)
#define GPU2D_CONFIGH_BLUE_WRAP           (1 << 7)
#define GPU2D_CONFIGH_RADIAL              (1 << 8)

#define GPU2D_CL_REG(reg)                 ((reg) & GPU2D_CL_REG_MASK)
#define GPU2D_CL_CMD(reg, flags)          (GPU2D_CL_REG(reg) | \
                                           ((flags) & ~GPU2D_CL_REG_MASK))
#define GPU2D_IS_ALIGNED_8(addr)          ((((uintptr_t)(addr)) & 7) == 0)

#endif /* __ARCH_ARM_SRC_STM32H7RS_HARDWARE_STM32_GPU2D_H */
