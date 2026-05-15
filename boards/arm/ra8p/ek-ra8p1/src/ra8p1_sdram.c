/****************************************************************************
 * boards/arm/ra8p/ek-ra8p1/src/ra8p1_sdram.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <syslog.h>

#include <arch/board/board.h>

#include "hardware/ra8p_sdram.h"
#include "hardware/ra8p_system.h"

#include "ek-ra8p1.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Timing values from the EK-RA8P1 quickstart board_sdram.h. */

#define RA8P1_SDRAM_TRAS                  6
#define RA8P1_SDRAM_TRCD                  3
#define RA8P1_SDRAM_TRP                   3
#define RA8P1_SDRAM_TWR                   2
#define RA8P1_SDRAM_CL                    3
#define RA8P1_SDRAM_TRFC                  8
#define RA8P1_SDRAM_REF_CMD_INTERVAL      937
#define RA8P1_SDRAM_INIT_REF_TIMES        2
#define RA8P1_SDRAM_ROW_ADDR_OFFSET       8
#define RA8P1_SDRAM_ENDIAN_MODE           0
#define RA8P1_SDRAM_CONTINUOUS_ACCESS     1
#define RA8P1_SDRAM_BUS_WIDTH             1

#define RA8P1_SDRAM_MR_WB_SINGLE_LOC      1
#define RA8P1_SDRAM_MR_OP_STANDARD        0
#define RA8P1_SDRAM_MR_BT_SEQUENTIAL      0
#define RA8P1_SDRAM_MR_BURST_LENGTH_1     0

#define RA8P1_SDRAM_TIMEOUT               1000000

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_sdram_initialized;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_RA8P_SDRAM
static uint8_t ra8p_getreg8(uintptr_t addr)
{
  return *(volatile uint8_t *)addr;
}

static uint16_t ra8p_getreg16(uintptr_t addr)
{
  return *(volatile uint16_t *)addr;
}

static void ra8p_putreg8(uint8_t value, uintptr_t addr)
{
  *(volatile uint8_t *)addr = value;
}

static void ra8p_putreg16(uint16_t value, uintptr_t addr)
{
  *(volatile uint16_t *)addr = value;
}

static void ra8p_putreg32(uint32_t value, uintptr_t addr)
{
  *(volatile uint32_t *)addr = value;
}

static void ra8p_modifyreg8(uintptr_t addr, uint8_t clearbits,
                            uint8_t setbits)
{
  uint8_t value = ra8p_getreg8(addr);

  value &= ~clearbits;
  value |= setbits;
  ra8p_putreg8(value, addr);
}

static int ra8p1_sdram_wait(uint8_t mask, const char *step)
{
  int count;

  for (count = 0; count < RA8P1_SDRAM_TIMEOUT; count++)
    {
      if ((ra8p_getreg8(RA8P_SDRAM_SDSR) & mask) == 0)
        {
          return 0;
        }
    }

  syslog(LOG_ERR, "ra8p1: SDRAM timeout at %s SDSR=%02" PRIx8 "\n",
         step, ra8p_getreg8(RA8P_SDRAM_SDSR));
  return -ETIMEDOUT;
}

static void ra8p1_sdram_clock_enable(void)
{
  uint16_t prcr;

  prcr = ra8p_getreg16(RA8P_SYSTEM_PRCR);
  ra8p_putreg16(SYSTEM_PRCR_KEY | (prcr & 0xff) | SYSTEM_PRCR_PRC1,
                RA8P_SYSTEM_PRCR);
  ra8p_modifyreg8(RA8P_SYSTEM_SDCKOCR, 0, SYSTEM_SDCKOCR_SDCKOEN);
  ra8p_putreg16(SYSTEM_PRCR_KEY | (prcr & 0xff), RA8P_SYSTEM_PRCR);
}

static int ra8p1_sdram_probe(void)
{
  static const uint32_t offsets[] =
    {
      0,
      0x1000,
      RA8P_SDRAM_SIZE / 2,
      RA8P_SDRAM_SIZE - sizeof(uint32_t)
    };
  static const uint32_t patterns[] =
    {
      0x00000000,
      0xffffffff,
      0x55aa55aa,
      0xaa55aa55
    };
  int i;
  int j;

  for (i = 0; i < (int)(sizeof(offsets) / sizeof(offsets[0])); i++)
    {
      volatile uint32_t *addr =
        (volatile uint32_t *)(uintptr_t)(RA8P_SDRAM_START + offsets[i]);
      uint32_t saved = *addr;

      for (j = 0; j < (int)(sizeof(patterns) / sizeof(patterns[0])); j++)
        {
          *addr = patterns[j];

          if (*addr != patterns[j])
            {
              syslog(LOG_ERR,
                     "ra8p1: SDRAM probe failed at 0x%08" PRIxPTR
                     " wrote=0x%08" PRIx32 " read=0x%08" PRIx32 "\n",
                     (uintptr_t)addr, patterns[j], *addr);
              *addr = saved;
              return -EIO;
            }
        }

      *addr = saved;
    }

  return 0;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int ra8p1_sdram_initialize(void)
{
#ifdef CONFIG_RA8P_SDRAM
  uint16_t sdir;
  uint16_t sdrfcr;
  uint16_t sdmod;
  uint32_t sdtr;
  int ret;

  if (g_sdram_initialized)
    {
      return 0;
    }

  syslog(LOG_INFO, "ra8p1: SDRAM init begin\n");

  ra8p1_sdram_clock_enable();

  sdir = (((RA8P1_SDRAM_TRP < 3 ? 3 : RA8P1_SDRAM_TRP - 3)
           << SDRAM_SDIR_PRC_SHIFT) |
          (RA8P1_SDRAM_INIT_REF_TIMES << SDRAM_SDIR_ARFC_SHIFT) |
          ((RA8P1_SDRAM_TRFC < 3 ? 0 : RA8P1_SDRAM_TRFC - 3)
           << SDRAM_SDIR_ARFI_SHIFT));

  ra8p_putreg16(sdir, RA8P_SDRAM_SDIR);

  ret = ra8p1_sdram_wait(UINT8_MAX, "SDIR");
  if (ret < 0)
    {
      return ret;
    }

  ra8p_putreg8(SDRAM_SDICR_INIRQ, RA8P_SDRAM_SDICR);

  ret = ra8p1_sdram_wait(SDRAM_SDSR_INIST, "init");
  if (ret < 0)
    {
      return ret;
    }

  ra8p_putreg8(RA8P1_SDRAM_BUS_WIDTH << SDRAM_SDCCR_BSIZE_SHIFT,
               RA8P_SDRAM_SDCCR);
  ra8p_putreg8(RA8P1_SDRAM_CONTINUOUS_ACCESS, RA8P_SDRAM_SDAMOD);
  ra8p_putreg8(RA8P1_SDRAM_ENDIAN_MODE, RA8P_SDRAM_SDCMOD);

  ret = ra8p1_sdram_wait(UINT8_MAX, "mode setup");
  if (ret < 0)
    {
      return ret;
    }

  sdmod = ((RA8P1_SDRAM_MR_WB_SINGLE_LOC << 9) |
           (RA8P1_SDRAM_MR_OP_STANDARD << 7) |
           (RA8P1_SDRAM_CL << 4) |
           (RA8P1_SDRAM_MR_BT_SEQUENTIAL << 3) |
           RA8P1_SDRAM_MR_BURST_LENGTH_1);
  ra8p_putreg16(sdmod, RA8P_SDRAM_SDMOD);

  ret = ra8p1_sdram_wait(SDRAM_SDSR_MRSST, "mode register");
  if (ret < 0)
    {
      return ret;
    }

  sdtr = (((RA8P1_SDRAM_TRAS - 1) << SDRAM_SDTR_RAS_SHIFT) |
          ((RA8P1_SDRAM_TRCD - 1) << SDRAM_SDTR_RCD_SHIFT) |
          ((RA8P1_SDRAM_TRP - 1) << SDRAM_SDTR_RP_SHIFT) |
          ((RA8P1_SDRAM_TWR - 1) << SDRAM_SDTR_WR_SHIFT) |
          (RA8P1_SDRAM_CL << SDRAM_SDTR_CL_SHIFT));
  ra8p_putreg32(sdtr, RA8P_SDRAM_SDTR);

  ra8p_putreg8(RA8P1_SDRAM_ROW_ADDR_OFFSET - 8, RA8P_SDRAM_SDADR);

  sdrfcr = (((RA8P1_SDRAM_TRFC - 1) << SDRAM_SDRFCR_REFW_SHIFT) |
            ((RA8P1_SDRAM_REF_CMD_INTERVAL - 1)
             << SDRAM_SDRFCR_RFC_SHIFT));
  ra8p_putreg16(sdrfcr, RA8P_SDRAM_SDRFCR);
  ra8p_putreg8(SDRAM_SDRFEN_RFEN, RA8P_SDRAM_SDRFEN);

  ra8p_putreg8(SDRAM_SDCCR_EXENB |
               (RA8P1_SDRAM_BUS_WIDTH << SDRAM_SDCCR_BSIZE_SHIFT),
               RA8P_SDRAM_SDCCR);

  __asm__ volatile ("dsb sy\nisb sy" : : : "memory");

  ret = ra8p1_sdram_probe();
  if (ret < 0)
    {
      return ret;
    }

  g_sdram_initialized = true;

  syslog(LOG_INFO,
         "ra8p1: SDRAM ready base=0x%08" PRIxPTR
         " size=0x%08" PRIx32 "\n",
         (uintptr_t)RA8P_SDRAM_START, (uint32_t)RA8P_SDRAM_SIZE);

  return 0;
#else
  return -ENODEV;
#endif
}
