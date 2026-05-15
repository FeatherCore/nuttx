/****************************************************************************
 * boards/arm/ra8p/ek-ra8p1/src/ra8p1_pins.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include "hardware/ra8p_iopctl.h"

#include "ek-ra8p1.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ra8p1_pinmux_s
{
  uint8_t  port;
  uint8_t  pin;
  uint32_t config;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_RA8P_OSPI_B
static const struct ra8p1_pinmux_s g_ospi_pins[] =
{
  { 1,  0, PFS_PERIPHERAL_OSPI }, /* SIO0 */
  { 1,  1, PFS_PERIPHERAL_OSPI }, /* SIO3 */
  { 1,  2, PFS_PERIPHERAL_OSPI }, /* SIO4 */
  { 1,  3, PFS_PERIPHERAL_OSPI }, /* SIO2 */
  { 1,  4, PFS_PERIPHERAL_OSPI }, /* CS1 */
  { 1,  5, PFS_PERIPHERAL_OSPI }, /* ECSINT1 */
  { 1,  6, PFS_PERIPHERAL_OSPI }, /* RESET */
  { 8,  0, PFS_PERIPHERAL_OSPI }, /* SIO5 */
  { 8,  1, PFS_PERIPHERAL_OSPI }, /* DQS */
  { 8,  2, PFS_PERIPHERAL_OSPI }, /* SIO6 */
  { 8,  3, PFS_PERIPHERAL_OSPI }, /* SIO1 */
  { 8,  4, PFS_PERIPHERAL_OSPI }, /* SIO7 */
  { 8,  8, PFS_PERIPHERAL_OSPI }, /* SCLK */
};
#endif

#ifdef CONFIG_RA8P_SDRAM
static const struct ra8p1_pinmux_s g_sdram_pins[] =
{
  { 1, 12, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ3 */
  { 1, 13, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ4 */
  { 1, 14, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ5 */
  { 1, 15, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ6 */
  { 3,  0, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ2 */
  { 3,  1, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ1 */
  { 3,  2, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ0 */
  { 5,  3, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A4 */
  { 5,  4, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A5 */
  { 5,  5, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A6 */
  { 5,  6, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A7 */
  { 5,  7, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A8 */
  { 5,  8, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A9 */
  { 5,  9, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A10 */
  { 5, 10, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A11 */
  { 6,  7, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ31 */
  { 6,  8, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A12 */
  { 6,  9, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ7 */
  { 6, 10, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ12 */
  { 6, 11, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ13 */
  { 6, 12, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ14 */
  { 6, 13, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ15 */
  { 6, 14, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQM0 */
  { 6, 15, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQM2 */
  { 8, 13, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* CS */
  {10,  0, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A3 */
  {10,  1, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A2 */
  {10,  2, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A1 */
  {10,  3, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* A0 */
  {10,  4, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQM3 */
  {10,  5, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQM1 */
  {10,  6, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* CKE */
  {10,  8, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* WE */
  {10,  9, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* CAS */
  {10, 10, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* RAS */
  {10, 11, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ8 */
  {10, 12, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ9 */
  {10, 13, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ10 */
  {10, 14, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ11 */
  {10, 15, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* CLK */
  {12,  0, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ30 */
  {12,  1, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ29 */
  {12,  2, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ28 */
  {12,  3, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ27 */
  {12,  4, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ26 */
  {12,  5, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ25 */
  {12,  6, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ24 */
  {12,  7, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ23 */
  {12,  8, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ22 */
  {12,  9, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ21 */
  {12, 10, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ20 */
  {12, 11, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ19 */
  {12, 12, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ18 */
  {12, 13, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ17 */
  {12, 14, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* DQ16 */
  {12, 15, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* BA1 */
  {13,  0, PFS_PERIPHERAL_BUS | PFS_DSCR_HIGH }, /* BA0 */
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void ra8p_putreg8(uint8_t value, uintptr_t addr)
{
  *(volatile uint8_t *)addr = value;
}

static void ra8p_putreg32(uint32_t value, uintptr_t addr)
{
  *(volatile uint32_t *)addr = value;
}

static void ra8p1_pfs_unlock(void)
{
  ra8p_putreg8(0, RA8P_PMISC_PWPR);
  ra8p_putreg8(PMISC_PWPR_PFSWE, RA8P_PMISC_PWPR);
}

static void ra8p1_pfs_lock(void)
{
  ra8p_putreg8(0, RA8P_PMISC_PWPR);
  ra8p_putreg8(PMISC_PWPR_B0WI, RA8P_PMISC_PWPR);
}

static void ra8p1_pin_configure(uint8_t port, uint8_t pin, uint32_t config)
{
  uintptr_t pfs = RA8P_PFS_PMN(port, pin);

  ra8p_putreg32(0, pfs);
  ra8p_putreg32(config, pfs);
  ra8p_putreg32(config | PFS_PMR, pfs);
}

static void ra8p1_pin_configure_table(
    const struct ra8p1_pinmux_s *pins, unsigned int count)
{
  unsigned int i;

  for (i = 0; i < count; i++)
    {
      ra8p1_pin_configure(pins[i].port, pins[i].pin, pins[i].config);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void ra8p1_pin_initialize(void)
{
  ra8p1_pfs_unlock();

#ifdef CONFIG_RA8P_OSPI_B
  ra8p1_pin_configure_table(g_ospi_pins,
                            sizeof(g_ospi_pins) / sizeof(g_ospi_pins[0]));
#endif
#ifdef CONFIG_RA8P_SDRAM
  ra8p1_pin_configure_table(g_sdram_pins,
                            sizeof(g_sdram_pins) / sizeof(g_sdram_pins[0]));
#endif

  ra8p1_pfs_lock();
}
