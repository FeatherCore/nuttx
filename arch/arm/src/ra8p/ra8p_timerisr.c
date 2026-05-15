/****************************************************************************
 * arch/arm/src/ra8p/ra8p_timerisr.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <time.h>

#include <nuttx/irq.h>
#include <arch/board/board.h>

#include "arm_internal.h"
#include "clock/clock.h"
#include "nvic.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SYSTICK_RELOAD ((RA8P_CPUCLK_FREQUENCY / CLK_TCK) - 1)

#if SYSTICK_RELOAD > NVIC_SYSTICK_RELOAD_MASK
#  error SYSTICK_RELOAD exceeds the range of the RELOAD register
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int ra8p_timerisr(int irq, void *context, void *arg)
{
  nxsched_process_timer();
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void up_timer_initialize(void)
{
  putreg32(SYSTICK_RELOAD, NVIC_SYSTICK_RELOAD);
  putreg32(0, NVIC_SYSTICK_CURRENT);

  irq_attach(RA8P_IRQ_SYSTICK, ra8p_timerisr, NULL);

  putreg32(NVIC_SYSTICK_CTRL_CLKSOURCE | NVIC_SYSTICK_CTRL_TICKINT |
           NVIC_SYSTICK_CTRL_ENABLE, NVIC_SYSTICK_CTRL);
}
