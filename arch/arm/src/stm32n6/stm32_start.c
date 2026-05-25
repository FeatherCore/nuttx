/****************************************************************************
 * arch/arm/src/stm32n6/stm32_start.c
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

#ifdef CONFIG_ARCH_BOARD_STM32N6570_DK

#include <stdint.h>

#include <nuttx/cache.h>
#include <nuttx/init.h>
#include <nuttx/irq.h>

#include <arch/board/board.h>
#include <arch/barriers.h>

#include "arm_internal.h"
#include "chip.h"
#ifdef CONFIG_ARM_MPU
#  include "mpu.h"
#endif
#if defined(CONFIG_ARCH_HAVE_TRUSTZONE) && defined(CONFIG_ARCH_TRUSTZONE_DISABLED)
#  include "sau.h"
#endif
#include "hardware/stm32n6xxx_memorymap.h"
#include "stm32_lowputc.h"
#include "stm32_rcc.h"
#include "stm32_mpuinit.h"
#include "stm32_userspace.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_ARM_MPU
#  define STM32_NORMAL_CACHEABLE \
    (MPU_RBAR_AP_RWNO | MPU_RBAR_SH_NO)

#  define STM32_NORMAL_CACHEABLE_RO \
    (MPU_RBAR_AP_RONO | MPU_RBAR_SH_NO)

#  define STM32_NORMAL_CACHEABLE_XN \
    (STM32_NORMAL_CACHEABLE | MPU_RBAR_XN)

#  ifdef CONFIG_STM32N6_PSRAM_MPU_SHARE_NONE
#    define STM32_PSRAM_MPU_SHARE MPU_RBAR_SH_NO
#  else
#    define STM32_PSRAM_MPU_SHARE MPU_RBAR_SH_OUTER
#  endif

#  ifdef CONFIG_STM32N6_PSRAM_MPU_NONCACHEABLE
#    define STM32_PSRAM_MPU_ATTR MPU_RLAR_NONCACHEABLE
#  elif defined(CONFIG_STM32N6_PSRAM_MPU_WRITE_THROUGH)
#    ifdef CONFIG_STM32N6_PSRAM_MPU_NO_WRITE_ALLOCATE
#      define STM32_PSRAM_MPU_ATTR MPU_RLAR_WRITE_THROUGH_NWA
#    else
#      define STM32_PSRAM_MPU_ATTR MPU_RLAR_WRITE_THROUGH
#    endif
#  else
#    ifdef CONFIG_STM32N6_PSRAM_MPU_NO_WRITE_ALLOCATE
#      define STM32_PSRAM_MPU_ATTR MPU_RLAR_WRITE_BACK_NWA
#    else
#      define STM32_PSRAM_MPU_ATTR MPU_RLAR_WRITE_BACK
#    endif
#  endif

#  ifdef CONFIG_NXBOOT_BOOTLOADER
#    define STM32_RAM_MPU_FLAGS STM32_NORMAL_CACHEABLE
#  else
#    define STM32_RAM_MPU_FLAGS STM32_NORMAL_CACHEABLE_XN
#  endif

#  ifdef CONFIG_ARCH_RAMFUNCS
#    define STM32_RAMFUNC_MPU_BASE ((uintptr_t)_sramfuncs)
#    define STM32_RAMFUNC_MPU_SIZE \
       ((uintptr_t)_eramfuncs - (uintptr_t)_sramfuncs)
#    define STM32_RAMFUNC_MPU_END ((uintptr_t)_eramfuncs)
#    define STM32_RAM_AFTER_RAMFUNC_MPU_SIZE \
       (CONFIG_RAM_START + CONFIG_RAM_SIZE - STM32_RAMFUNC_MPU_END)
#  endif

#  ifdef CONFIG_BUILD_PROTECTED
#    define STM32_XSPI2_MPU_BASE \
       (BOARD_XSPI2_NOR_BASE + CONFIG_STM32N6_OTA_PRIMARY_SLOT_OFFSET + \
        CONFIG_NXBOOT_HEADER_SIZE)
#    define STM32_XSPI2_MPU_SIZE \
       (CONFIG_NUTTX_USERSPACE - STM32_XSPI2_MPU_BASE)
#    define STM32_PSRAM_MPU_FLAGS1 \
       (MPU_RBAR_AP_RWNO | STM32_PSRAM_MPU_SHARE | MPU_RBAR_XN)
#    define STM32_PSRAM_MPU_FLAGS2 \
       (STM32_PSRAM_MPU_ATTR | MPU_RLAR_PXN)
#    define STM32_USER_PSRAM_MPU_FLAGS1 \
       (MPU_RBAR_AP_RWRW | STM32_PSRAM_MPU_SHARE | MPU_RBAR_XN)
#  else
#    define STM32_XSPI2_MPU_BASE BOARD_XSPI2_NOR_BASE
#    define STM32_XSPI2_MPU_SIZE BOARD_XSPI2_NOR_SIZE
#    define STM32_PSRAM_MPU_FLAGS1 \
       (MPU_RBAR_AP_RWNO | STM32_PSRAM_MPU_SHARE | MPU_RBAR_XN)
#    define STM32_PSRAM_MPU_FLAGS2 \
       (STM32_PSRAM_MPU_ATTR | MPU_RLAR_PXN)
#  endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32_sau_reset(void)
{
#if defined(CONFIG_ARCH_HAVE_TRUSTZONE) && defined(CONFIG_ARCH_TRUSTZONE_DISABLED)
  uint32_t regions;
  uint32_t region;

  /* The NuttX STM32N6 port runs with TrustZone disabled, so do not inherit
   * a secure attribution layout from the first-stage image.
   */

  putreg32(0, SAU_CTRL);

  regions = (getreg32(SAU_TYPE) & SAU_TYPE_SREGION_MASK) >>
            SAU_TYPE_SREGION_SHIFT;

  for (region = 0; region < regions; region++)
    {
      putreg32(region, SAU_RNR);
      putreg32(0, SAU_RBAR);
      putreg32(0, SAU_RLAR);
    }

  putreg32(SAU_SFSR_MASK, SAU_SFSR);
  putreg32(0, SAU_CTRL);

  UP_DSB();
  UP_ISB();
#endif
}

static void stm32_cache_setup(void)
{
#ifdef CONFIG_ARM_MPU
  const struct mpu_region_s regions[] =
    {
#if defined(CONFIG_ARCH_RAMFUNCS) && !defined(CONFIG_NXBOOT_BOOTLOADER)
      {
        .base   = STM32_RAMFUNC_MPU_BASE,
        .size   = STM32_RAMFUNC_MPU_SIZE,
        .flags1 = STM32_NORMAL_CACHEABLE,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
      {
        .base   = STM32_RAMFUNC_MPU_END,
        .size   = STM32_RAM_AFTER_RAMFUNC_MPU_SIZE,
        .flags1 = STM32_RAM_MPU_FLAGS,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
#else
      {
        .base   = CONFIG_RAM_START,
        .size   = CONFIG_RAM_SIZE,
        .flags1 = STM32_RAM_MPU_FLAGS,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
#endif
      {
        .base   = STM32_XSPI2_MPU_BASE,
        .size   = STM32_XSPI2_MPU_SIZE,
        .flags1 = STM32_NORMAL_CACHEABLE_RO,
        .flags2 = MPU_RLAR_WRITE_BACK
      },
      {
        .base   = BOARD_XSPI1_PSRAM_BASE,
        .size   = BOARD_XSPI1_PSRAM_SIZE,
        .flags1 = STM32_PSRAM_MPU_FLAGS1,
        .flags2 = STM32_PSRAM_MPU_FLAGS2
      },
    };

#ifdef CONFIG_ARMV8M_DCACHE
  up_disable_dcache();
#endif
#ifdef CONFIG_ARMV8M_ICACHE
  up_disable_icache();
#endif

  mpu_reset();
  mpu_initialize(regions, sizeof(regions) / sizeof(regions[0]),
                 false, true);
#endif

#ifdef CONFIG_ARMV8M_ICACHE
  up_enable_icache();
#endif
#ifdef CONFIG_ARMV8M_DCACHE
  up_enable_dcache();
#endif
}

static void stm32_early_fault_setup(void)
{
#ifdef CONFIG_DEBUG_HARDFAULT_ALERT
  irq_attach(STM32_IRQ_HARDFAULT, arm_hardfault, NULL);
#endif
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

const uintptr_t g_idle_topstack =
  (uintptr_t)_ebss + CONFIG_IDLETHREAD_STACKSIZE;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

osentry_function
void __start(void)
{
  const uint32_t *src;
  uint32_t *dest;

  for (dest = (uint32_t *)_sbss; dest < (uint32_t *)_ebss; )
    {
      *dest++ = 0;
    }

  for (src = (const uint32_t *)_eronly,
       dest = (uint32_t *)_sdata; dest < (uint32_t *)_edata; )
    {
      *dest++ = *src++;
    }

#ifdef CONFIG_ARCH_RAMFUNCS
  for (src = (const uint32_t *)_framfuncs,
       dest = (uint32_t *)_sramfuncs; dest < (uint32_t *)_eramfuncs; )
    {
      *dest++ = *src++;
    }
#endif

  stm32_sau_reset();

  stm32_clockconfig();

  arm_fpuconfig();

  stm32_lowsetup();

  stm32_cache_setup();

  stm32_early_fault_setup();

#ifdef CONFIG_BUILD_PROTECTED
  stm32_userspace();

  stm32_mpuinitialize();
#endif

  nx_start();

  for (; ; );
}

#else /* CONFIG_ARCH_BOARD_STM32N6570_DK */

#include <stdint.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/init.h>
#include <arch/barriers.h>
#include <arch/board/board.h>

#include "arm_internal.h"
#include "nvic.h"

#include "stm32.h"
#include "stm32_gpio.h"
#include "stm32_pwr.h"
#include "stm32_start.h"
#include "hardware/stm32n6xxx_syscfg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Cortex-M55 CCR.LOB (Low-Overhead Branch) enable bit.  Not yet exposed by
 * the generic armv8-m nvic.h.
 */

#define NVIC_CFGCON_LOB             (1 << 19)

#define HEAP_BASE  ((uintptr_t)_ebss + CONFIG_IDLETHREAD_STACKSIZE)

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Top of the idle thread stack; the heap begins immediately above it. */

const uintptr_t g_idle_topstack = HEAP_BASE;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_FEATURES
#  define showprogress(c) arm_lowputc(c)
#else
#  define showprogress(c)
#endif

/* Cortex-M55 ARMv8.1-M Low-Overhead Branch: enables WLS/DLS/LE.
 * CCR.LOB resets to 0; enable before any loop the compiler may lower
 * with LE (and before MVE code, which is gated on the same bit).
 */

static inline void stm32_enable_lob(void)
{
  uint32_t regval;

  regval  = getreg32(NVIC_CFGCON);
  regval |= NVIC_CFGCON_LOB;
  putreg32(regval, NVIC_CFGCON);
  UP_ISB();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* __start_c() carries the real boot logic; __start() below is a naked
 * dispatcher that clears the boot-ROM stack limits before any prologue.
 * __start_c is reached via "b __start_c" from __start's inline asm and
 * therefore must have external linkage.
 */

void __start_c(void) noinstrument_function;

/****************************************************************************
 * Name: __start
 *
 * Description:
 *   Reset entry point.  The STM32N6 boot ROM (DEV mode) leaves MSPLIM and
 *   PSPLIM set such that the first stack push from C code can fault.
 *   This function is naked so the limits can be cleared before any
 *   compiler-generated prologue runs.  It then tail-calls __start_c.
 *
 ****************************************************************************/

void __attribute__((naked)) noinstrument_function __start(void)
{
  __asm__ volatile ("mov r0, #0\n\t"
                    "msr msplim, r0\n\t"
                    "msr psplim, r0\n\t"
                    "b __start_c\n\t");
}

/****************************************************************************
 * Name: __start_c
 *
 * Description:
 *   The C-level boot path, reached from the naked __start dispatcher.
 *
 ****************************************************************************/

void __start_c(void)
{
  const uint32_t *src;
  uint32_t *dest;

  /* The DEV-mode boot ROM leaves VTOR pointing at its own ROM region.  Set
   * VTOR to our SRAM vector table before enabling any exception path.
   */

  putreg32((uint32_t)_vectors, NVIC_VECTAB);

  /* When chain-loaded by an FSBL that called HAL_Init(), SysTick may be
   * left running.  Disable it and clear any pending interrupt so it does
   * not fire before NuttX has attached its handler.
   */

  putreg32(0, NVIC_SYSTICK_CTRL);
  putreg32(NVIC_INTCTRL_PENDSTCLR, NVIC_INTCTRL);

  /* Force plain SLEEP (not DEEPSLEEP) on WFI so the system clock keeps
   * running and SysTick continues to wake us.  Cleared once here so
   * up_idle() does not need to touch it on every idle entry.
   */

  modifyreg32(NVIC_SYSCON, NVIC_SYSCON_SLEEPDEEP, 0);

  /* Enable the FPU before stm32_clockconfig and the rest of init.  With
   * hard-float ABI the compiler may emit FPU instructions later, and any
   * exception entry will try to push FP context -- both require CP10/CP11.
   */

  arm_fpuconfig();

  /* Clear .bss inline rather than calling memset, so global state is sane
   * even if anything depends on it during the rest of __start.
   */

  for (dest = (uint32_t *)_sbss; dest < (uint32_t *)_ebss; )
    {
      *dest++ = 0;
    }

  /* Copy .data from its load address to SRAM.  For SRAM-only DEV-mode
   * builds the linker arranges _eronly == _sdata so the copy is skipped;
   * the explicit check avoids corrupting .data when alignment padding
   * would otherwise leave a gap between _eronly and _sdata.
   */

  if (&_eronly[0] != &_sdata[0])
    {
      for (src = (const uint32_t *)_eronly,
           dest = (uint32_t *)_sdata; dest < (uint32_t *)_edata;
          )
        {
          *dest++ = *src++;
        }
    }

  stm32_enable_lob();

  stm32_clockconfig();

  /* Per ES0620, BSECEN must remain set or WFI/sleep fails.  Set it via
   * the atomic SET register, together with SYSCFGEN which we need below.
   */

  putreg32(RCC_APB4HENR_SYSCFGEN | RCC_APB4HENR_BSECEN,
           STM32_RCC_APB4HENSR);

  /* Enable the LPEN bits that keep clocks running through WFI (CSLEEP).
   * Without these, WFI halts the clocks for the AXISRAM banks and any
   * enabled peripherals, and the system never wakes up.
   */

  putreg32(RCC_BUSLPENR_ACLKNLPEN | RCC_BUSLPENR_ACLKNCLPEN,
           STM32_RCC_BUSLPENSR);
  putreg32(RCC_MEMLPENR_ALLAXISRAM | RCC_MEMLPENR_CACHEAXIRAMLPEN,
           STM32_RCC_MEMLPENSR);
#ifdef CONFIG_STM32N6_USART1
  putreg32(RCC_APB2LPENR_USART1LPEN, STM32_RCC_APB2LPENSR);
#endif

  /* Mark the board's I/O voltage domains as supply-valid before any GPIO
   * pad is driven.  The mask of PWR_SVMCR3_* bits is board-specific and
   * provided by board.h via BOARD_PWR_VDDIO.
   */

  stm32_pwr_enablevddio(BOARD_PWR_VDDIO);

  /* Apply the ES0620 I/O-compensation mitigation (write 0x287) to the
   * domains we use.  Only VDDIO2 and VDDIO3 are touched: the other
   * VDDIOxCCCR registers cannot be accessed without their VDDIOxSV bit
   * set first (separate ES0620 erratum), and only VDDIO2/3 are declared
   * supply-valid in BOARD_PWR_VDDIO above.
   */

  putreg32(SYSCFG_CCCR_ES0620_MANUAL, STM32_SYSCFG_VDDIO2CCCR);
  putreg32(SYSCFG_CCCR_ES0620_MANUAL, STM32_SYSCFG_VDDIO3CCCR);
  putreg32(SYSCFG_CCCR_ES0620_MANUAL, STM32_SYSCFG_VDDCCCR);

  putreg32((uint32_t)_vectors, STM32_SYSCFG_INITSVTORCR);

  /* Read-back to ensure prior SYSCFG writes complete */

  (void)getreg32(STM32_SYSCFG_VDDCCCR);

#ifdef CONFIG_STM32N6_USART1
  /* Route USART1's kernel clock to HSI so the BRR computation is
   * independent of any later SYSCLK changes.
   */

  putreg32(RCC_CCIPR13_USART1SEL_HSI, STM32_RCC_CCIPR13);
#endif

  stm32_lowsetup();

#ifdef USE_EARLYSERIALINIT
  arm_earlyserialinit();
#endif

  stm32_board_initialize();

  /* Final DSB+ISB after SCB writes (VTOR, SYSTICK_CTRL, SYSCFG INITSVTORCR)
   * before handing off to NuttX.
   */

  UP_DSB();
  UP_ISB();

  showprogress('\r');
  showprogress('\n');

  nx_start();

  /* Shouldn't get here */

  for (; ; );
}

#endif /* CONFIG_ARCH_BOARD_STM32N6570_DK */
