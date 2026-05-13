/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/src/stm32h7s78_xspi.c
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
#include <stddef.h>
#include <stdint.h>
#include <syslog.h>

#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/cache.h>

#include <arch/board/board.h>

#include "arm_internal.h"

#include "hardware/stm32h7rs_gpio.h"
#include "hardware/stm32h7rs_memorymap.h"
#include "hardware/stm32h7rs_rcc.h"
#include "hardware/stm32h7rs_xspi.h"

#include "stm32h7rs_xspi.h"
#include "stm32h7s78-dk.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32H7S78_XSPI_STARTUP_PRESCALER       3u
#define STM32H7S78_XSPI_NOR_OPTIONAL_PRESCALER  0u
#define STM32H7S78_XSPI_PSRAM_OPTIONAL_PRESCALER \
        0u
#define STM32H7S78_XSPI_NOR_RESET_DELAY_MS      100u
#define STM32H7S78_XSPI_STATUS_TIMEOUT          1000000u

#define MX66UW1G45G_RESET_ENABLE_CMD            0x66u
#define MX66UW1G45G_RESET_MEMORY_CMD            0x99u
#define MX66UW1G45G_OCTA_RESET_ENABLE_CMD       0x6699u
#define MX66UW1G45G_OCTA_RESET_MEMORY_CMD       0x9966u
#define MX66UW1G45G_READ_ID_CMD                 0x9fu
#define MX66UW1G45G_READ_STATUS_CMD             0x05u
#define MX66UW1G45G_WRITE_ENABLE_CMD            0x06u
#define MX66UW1G45G_CFG2_WRITE_CMD              0x72u
#define MX66UW1G45G_CFG2_READ_CMD               0x71u
#define MX66UW1G45G_OCTAL_CFG2_READAW_CMD       0x718eu
#define MX66UW1G45G_OCTAL_READ_STATUS_CMD       0x05fau
#define MX66UW1G45G_CFG2_DOPI_ADDR              0x00000300u
#define MX66UW1G45G_CFG2_DOPI_VALUE             0x00u
#define MX66UW1G45G_CFG2_DOPI_MASK              0x07u
#define MX66UW1G45G_CFG2_OPI_ADDR               0x00000000u
#define MX66UW1G45G_CFG2_OPI_VALUE              0x02u
#define MX66UW1G45G_CFG2_OPI_MASK               0x02u
#define MX66UW1G45G_OCTAL_READ_CMD              0xee11u
#define MX66UW1G45G_OCTAL_WRITE_CMD             0x12edu
#define MX66UW1G45G_SR_WIP                      0x01u
#define MX66UW1G45G_SR_WEL                      0x02u
#define MX66UW1G45G_DEVSIZE                     26u
#define MX66UW1G45G_CSHT                        8u
#define MX66UW1G45G_FIFO_THRESHOLD              2u
#define MX66UW1G45G_OCTAL_READ_DUMMY            20u
#define MX66UW1G45G_OCTAL_REG_DUMMY             4u

#define APS256_RESET_CMD                        0xffu
#define APS256_READ_LINEAR_BURST_CMD            0x20u
#define APS256_WRITE_LINEAR_BURST_CMD           0xa0u
#define APS256_REG_READ_CMD                     0x40u
#define APS256_REG_WRITE_CMD                    0xc0u
#define APS256_MR0_ADDR                         0x00000000u
#define APS256_MR4_ADDR                         0x00000004u
#define APS256_MR8_ADDR                         0x00000008u
#define APS256_MR0_VALUE                        0x11u
#define APS256_MR4_VALUE                        0x20u
#define APS256_MR8_VALUE                        0x40u
#define APS256_REG_DUMMY                        4u
#define APS256_HEXA_DUMMY                       6u
#define APS256_DEVSIZE                          24u
#define APS256_CSBOUND                          10u
#define APS256_FIFO_THRESHOLD                   2u
#define APS256_RESET_DELAY_MS                   1u

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32h7s78_psram_dcache_flush(FAR const volatile void *addr,
                                          size_t size)
{
#ifdef CONFIG_ARMV7M_DCACHE
  uintptr_t start = (uintptr_t)addr;
  uintptr_t end = start + size;

  up_clean_dcache(start, end);
  up_invalidate_dcache(start, end);
#else
  UNUSED(addr);
  UNUSED(size);
#endif
}

static uint32_t stm32h7s78_psram_refresh(uint32_t prescaler)
{
  uint32_t xspi_hz;
  uint32_t cycles;

  xspi_hz = STM32H7RS_PLL2S_FREQUENCY / (prescaler + 1u);
  cycles = 2u * (xspi_hz / 1000000u);

  return cycles > 4u ? cycles - 4u : 0u;
}

static void stm32h7s78_xspi2_gpio_config(void)
{
  uint32_t pins = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) |
                  (1u << 4) | (1u << 5) | (1u << 6) | (1u << 8) |
                  (1u << 9) | (1u << 10) | (1u << 11);

  modifyreg32(STM32H7RS_RCC_AHB4ENR, 0, RCC_AHB4ENR_GPIONEN);
  (void)getreg32(STM32H7RS_RCC_AHB4ENR);
  stm32h7rs_xspi_config_gpio(STM32H7RS_GPION_BASE, pins, GPIO_AF_XSPIM_P2);
}

static void stm32h7s78_xspi1_gpio_config(void)
{
  uint32_t gpioo_pins = (1u << 0) | (1u << 2) | (1u << 3) | (1u << 4);
  uint32_t gpiop_pins = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) |
                        (1u << 4) | (1u << 5) | (1u << 6) | (1u << 7) |
                        (1u << 8) | (1u << 9) | (1u << 10) |
                        (1u << 11) | (1u << 12) | (1u << 13) |
                        (1u << 14) | (1u << 15);

  modifyreg32(STM32H7RS_RCC_AHB4ENR, 0,
              RCC_AHB4ENR_GPIOOEN | RCC_AHB4ENR_GPIOPEN);
  (void)getreg32(STM32H7RS_RCC_AHB4ENR);
  stm32h7rs_xspi_config_gpio(STM32H7RS_GPIOO_BASE, gpioo_pins,
                             GPIO_AF_XSPIM_P1);
  stm32h7rs_xspi_config_gpio(STM32H7RS_GPIOP_BASE, gpiop_pins,
                             GPIO_AF_XSPIM_P1);
}

static bool stm32h7s78_nor_id_blank(FAR const uint8_t id[3])
{
  return (id[0] == 0x00 && id[1] == 0x00 && id[2] == 0x00) ||
         (id[0] == 0xff && id[1] == 0xff && id[2] == 0xff);
}

static int stm32h7s78_nor_readid(uint8_t id[3])
{
  return stm32h7rs_xspi_read_data(STM32H7RS_XSPI2_BASE,
                                  MX66UW1G45G_READ_ID_CMD, 0,
                                  XSPI_CCR_IMODE_1_LINE |
                                  XSPI_CCR_DMODE_1_LINE,
                                  0, 3, id);
}

static int stm32h7s78_nor_read_cfg2_spi(uint32_t address, uint8_t *value)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_1_LINE | XSPI_CCR_ADMODE_1_LINE |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_1_LINE;

  return stm32h7rs_xspi_read_data(STM32H7RS_XSPI2_BASE,
                                  MX66UW1G45G_CFG2_READ_CMD, address,
                                  ccr, 0, 1, value);
}

static int stm32h7s78_nor_read_status_spi(uint8_t *status)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_1_LINE | XSPI_CCR_DMODE_1_LINE;
  return stm32h7rs_xspi_read_data(STM32H7RS_XSPI2_BASE,
                                  MX66UW1G45G_READ_STATUS_CMD, 0,
                                  ccr, 0, 1, status);
}

static int stm32h7s78_nor_read_status_octal(uint8_t *status)
{
  uint8_t data[2];
  uint32_t ccr;
  int ret;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16 |
        XSPI_CCR_ADMODE_8_LINES | XSPI_CCR_ADDTR |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  ret = stm32h7rs_xspi_read_data(STM32H7RS_XSPI2_BASE,
                                 MX66UW1G45G_OCTAL_READ_STATUS_CMD,
                                 0, ccr,
                                 XSPI_TCR_DCYC(
                                   MX66UW1G45G_OCTAL_REG_DUMMY),
                                 sizeof(data), data);
  if (ret >= 0)
    {
      *status = data[0];
    }

  return ret;
}

static int stm32h7s78_nor_wait_status(
  int (*read_status)(uint8_t *status), uint8_t mask, uint8_t value)
{
  uint32_t timeout = STM32H7S78_XSPI_STATUS_TIMEOUT;
  uint8_t status;
  int ret;

  do
    {
      ret = read_status(&status);
      if (ret < 0)
        {
          return ret;
        }

      if ((status & mask) == value)
        {
          return OK;
        }
    }
  while (timeout-- > 0);

  return -ETIMEDOUT;
}

static int stm32h7s78_nor_wait_wip_spi(void)
{
  return stm32h7s78_nor_wait_status(stm32h7s78_nor_read_status_spi,
                                    MX66UW1G45G_SR_WIP, 0);
}

static int stm32h7s78_nor_wait_wip_octal(void)
{
  return stm32h7s78_nor_wait_status(stm32h7s78_nor_read_status_octal,
                                    MX66UW1G45G_SR_WIP, 0);
}

static int stm32h7s78_nor_write_enable_spi(void)
{
  int ret;

  ret = stm32h7rs_xspi_command(STM32H7RS_XSPI2_BASE,
                               MX66UW1G45G_WRITE_ENABLE_CMD,
                               XSPI_CCR_IMODE_1_LINE);
  if (ret < 0)
    {
      return ret;
    }

  return stm32h7s78_nor_wait_status(stm32h7s78_nor_read_status_spi,
                                    MX66UW1G45G_SR_WEL,
                                    MX66UW1G45G_SR_WEL);
}

static int stm32h7s78_nor_write_cfg2_spi(uint32_t address, uint8_t value,
                                         bool wait_after)
{
  uint32_t ccr;
  int ret;

  ret = stm32h7s78_nor_wait_wip_spi();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7s78_nor_write_enable_spi();
  if (ret < 0)
    {
      return ret;
    }

  ccr = XSPI_CCR_IMODE_1_LINE | XSPI_CCR_ADMODE_1_LINE |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_1_LINE;

  ret = stm32h7rs_xspi_write_data(STM32H7RS_XSPI2_BASE,
                                  MX66UW1G45G_CFG2_WRITE_CMD, address,
                                  ccr, 0, value, 1, false);
  if (ret < 0 || !wait_after)
    {
      return ret;
    }

  return stm32h7s78_nor_wait_wip_spi();
}

static int stm32h7s78_nor_read_cfg2_octal(uint32_t address, uint8_t *value)
{
  uint8_t data[2];
  uint32_t ccr;
  int ret;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16 |
        XSPI_CCR_ADMODE_8_LINES | XSPI_CCR_ADDTR |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  ret = stm32h7rs_xspi_read_data(STM32H7RS_XSPI2_BASE,
                                 MX66UW1G45G_OCTAL_CFG2_READAW_CMD,
                                 address, ccr,
                                 XSPI_TCR_DCYC(
                                   MX66UW1G45G_OCTAL_REG_DUMMY),
                                 sizeof(data), data);
  if (ret >= 0)
    {
      *value = data[0];
    }

  return ret;
}

static int stm32h7s78_nor_config_cfg2_spi(uint32_t address, uint8_t value,
                                          uint8_t mask)
{
  uint8_t readback;
  uint8_t writeval;
  int ret;

  ret = stm32h7s78_nor_read_cfg2_spi(address, &readback);
  if (ret < 0)
    {
      return ret;
    }

  writeval = (readback & ~mask) | (value & mask);
  ret = stm32h7s78_nor_write_cfg2_spi(address, writeval, true);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7s78_nor_read_cfg2_spi(address, &readback);
  if (ret < 0)
    {
      return ret;
    }

  if ((readback & mask) != (value & mask))
    {
      syslog(LOG_ERR, "XSPI2 NOR CFG2[0x%08" PRIx32
             "] readback %02x expected %02x\n",
             address, readback, value);
      return -EIO;
    }

  return OK;
}

static int stm32h7s78_xspi2_enter_memory_mapped(void)
{
  uintptr_t base = STM32H7RS_XSPI2_BASE;
  uint32_t ccr;
  int ret;

  ret = stm32h7rs_xspi_set_prescaler(
    base, STM32H7S78_XSPI_NOR_OPTIONAL_PRESCALER);
  if (ret < 0)
    {
      return ret;
    }

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16 |
        XSPI_CCR_ADMODE_8_LINES | XSPI_CCR_ADDTR |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  return stm32h7rs_xspi_enter_memory_mapped(
    base, ccr, XSPI_TCR_DCYC(MX66UW1G45G_OCTAL_READ_DUMMY),
    MX66UW1G45G_OCTAL_READ_CMD, ccr, 0,
    MX66UW1G45G_OCTAL_WRITE_CMD);
}

static int stm32h7s78_psram_write_reg(uint32_t address, uint8_t value)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  return stm32h7rs_xspi_write_data(STM32H7RS_XSPI1_BASE,
                                   APS256_REG_WRITE_CMD, address, ccr,
                                   0, value, 2, true);
}

static int stm32h7s78_psram_read_reg(uint32_t address, uint8_t *value)
{
  uint8_t data[2];
  uint32_t ccr;
  int ret;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  ret = stm32h7rs_xspi_read_data(STM32H7RS_XSPI1_BASE,
                                 APS256_REG_READ_CMD, address, ccr,
                                 XSPI_TCR_DCYC(APS256_REG_DUMMY),
                                 sizeof(data), data);
  if (ret >= 0)
    {
      *value = data[0];
    }

  return ret;
}

static int stm32h7s78_psram_config_reg(uint32_t address, uint8_t value,
                                       uint8_t mask)
{
  uint8_t initial;
  uint8_t readback;
  uint8_t writeval;
  int ret;

  ret = stm32h7s78_psram_read_reg(address, &initial);
  if (ret < 0)
    {
      return ret;
    }

  writeval = (initial & ~mask) | (value & mask);
  ret = stm32h7s78_psram_write_reg(address, writeval);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7s78_psram_read_reg(address, &readback);
  if (ret < 0)
    {
      return ret;
    }

  if ((readback & mask) != (value & mask))
    {
      ferr("ERROR: PSRAM reg 0x%08" PRIx32 " readback %02x expected %02x\n",
           address, readback, value);
      syslog(LOG_ERR, "XSPI1 PSRAM MR%08" PRIx32
             " initial %02x write %02x readback %02x expected %02x\n",
             address, initial, writeval, readback, value);
      return -EIO;
    }

  syslog(LOG_INFO, "XSPI1 PSRAM MR%08" PRIx32
         " initial %02x write %02x readback %02x\n",
         address, initial, writeval, readback);
  return OK;
}

static int stm32h7s78_xspi1_enter_memory_mapped(void)
{
  uintptr_t base = STM32H7RS_XSPI1_BASE;
  uint32_t ccr;
  int ret;

  ret = stm32h7rs_xspi_set_prescaler(
    base, STM32H7S78_XSPI_PSRAM_OPTIONAL_PRESCALER);
  if (ret < 0)
    {
      return ret;
    }

  putreg32(XSPI_DCR4_REFRESH(stm32h7s78_psram_refresh(
             STM32H7S78_XSPI_PSRAM_OPTIONAL_PRESCALER)),
           STM32H7RS_XSPI_DCR4(base));

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_16_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  return stm32h7rs_xspi_enter_memory_mapped(
    base, ccr, XSPI_TCR_DCYC(APS256_HEXA_DUMMY),
    APS256_READ_LINEAR_BURST_CMD, ccr,
    XSPI_TCR_DCYC(APS256_HEXA_DUMMY),
    APS256_WRITE_LINEAR_BURST_CMD);
}

static int stm32h7s78_psram_selftest(void)
{
  volatile uint32_t *mem = (volatile uint32_t *)STM32H7RS_XSPI1_MEM_BASE;
  const uint32_t offsets[] =
    {
      0x00000000u,
      0x00000040u,
      0x00001000u,
      0x00001040u,
      0x00100000u,
      0x00ffffc0u,
      0x01ffffc0u,
      STM32H7RS_XSPI1_PSRAM_SIZE - 0x20u
    };

  const uint32_t alias_offsets[] =
    {
      0x00000000u,
      0x00000040u,
      0x000000c0u,
      0x00000100u,
      0x00000140u,
      0x000001c0u,
      0x00000200u,
      0x000002c0u
    };

  uint32_t saved[8];
  uint32_t alias_saved[sizeof(alias_offsets) / sizeof(alias_offsets[0])];
  uint32_t pattern[8] =
    {
      0x55aa55aau, 0xaa55aa55u, 0x01234567u, 0x89abcdefu,
      0xf0f00f0fu, 0x0f0ff0f0u, 0x13579bdfu, 0x2468ace0u
    };

  size_t alias;
  size_t region;
  size_t restore;
  int i;

  for (region = 0; region < sizeof(offsets) / sizeof(offsets[0]); region++)
    {
      volatile uint32_t *ptr =
        (volatile uint32_t *)((uintptr_t)mem + offsets[region]);

      for (i = 0; i < 8; i++)
        {
          saved[i] = ptr[i];
        }

      for (i = 0; i < 8; i++)
        {
          ptr[i] = pattern[i];
        }

      stm32h7s78_psram_dcache_flush(ptr, sizeof(pattern));

      for (i = 0; i < 8; i++)
        {
          if (ptr[i] != pattern[i])
            {
              ferr("ERROR: PSRAM self-test[0x%08" PRIx32 "+%d] got %08"
                   PRIx32 " expected %08" PRIx32 "\n",
                   offsets[region], i * 4, ptr[i], pattern[i]);
              while (i-- > 0)
                {
                  ptr[i] = saved[i];
                }

              return -EIO;
            }
        }

      for (i = 0; i < 8; i++)
        {
          ptr[i] = saved[i];
        }

      stm32h7s78_psram_dcache_flush(ptr, sizeof(saved));
    }

  for (alias = 0;
       alias < sizeof(alias_offsets) / sizeof(alias_offsets[0]);
       alias++)
    {
      volatile uint32_t *ptr =
        (volatile uint32_t *)((uintptr_t)mem + alias_offsets[alias]);

      alias_saved[alias] = *ptr;
      *ptr = STM32H7RS_XSPI1_MEM_BASE + alias_offsets[alias];
    }

  stm32h7s78_psram_dcache_flush(mem, 0x300);

  for (alias = 0;
       alias < sizeof(alias_offsets) / sizeof(alias_offsets[0]);
       alias++)
    {
      volatile uint32_t *ptr =
        (volatile uint32_t *)((uintptr_t)mem + alias_offsets[alias]);
      uint32_t expected = STM32H7RS_XSPI1_MEM_BASE + alias_offsets[alias];

      if (*ptr != expected)
        {
          ferr("ERROR: PSRAM address self-test[0x%08" PRIx32
               "] got %08" PRIx32 " expected %08" PRIx32 "\n",
               alias_offsets[alias], *ptr, expected);
          for (restore = 0;
               restore < sizeof(alias_offsets) / sizeof(alias_offsets[0]);
               restore++)
            {
              ptr = (volatile uint32_t *)((uintptr_t)mem +
                                          alias_offsets[restore]);
              *ptr = alias_saved[restore];
            }

          return -EIO;
        }
    }

  for (alias = 0;
       alias < sizeof(alias_offsets) / sizeof(alias_offsets[0]);
       alias++)
    {
      volatile uint32_t *ptr =
        (volatile uint32_t *)((uintptr_t)mem + alias_offsets[alias]);

      *ptr = alias_saved[alias];
    }

  stm32h7s78_psram_dcache_flush(mem, 0x300);

  syslog(LOG_INFO, "XSPI1 PSRAM self-test passed\n");
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32h7s78_xspi2_nor_initialize(void)
{
  static const struct stm32h7rs_xspi_config_s nor_config =
    {
      .base           = STM32H7RS_XSPI2_BASE,
      .reset          = RCC_AHB5RSTR_XSPI2RST,
      .memory_type    = XSPI_DCR1_MTYP_MACRONIX,
      .device_size    = MX66UW1G45G_DEVSIZE,
      .csht           = MX66UW1G45G_CSHT,
      .prescaler      = STM32H7S78_XSPI_STARTUP_PRESCALER,
      .fifo_threshold = MX66UW1G45G_FIFO_THRESHOLD,
      .csbound        = 0,
      .maxtran        = 0,
      .refresh        = 0
    };

  uint8_t cfg2;
  uint32_t header;
  uint8_t id[3];
  int ret;

  if (stm32h7rs_xspi_is_mapped(STM32H7RS_XSPI2_BASE))
    {
      syslog(LOG_INFO, "XSPI2 NOR already memory-mapped\n");
      return OK;
    }

  ret = stm32h7rs_xspi_common_setup();
  if (ret < 0)
    {
      ferr("ERROR: XSPI common setup failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR common setup failed: %d\n", ret);
      return ret;
    }

  stm32h7s78_xspi2_gpio_config();
  stm32h7rs_xspi_controller_config(&nor_config);

  /* Cube's MX66UW1G45G reset method is 6699/9966 followed by 66/99 when
   * optional octal configuration is present.  This lets the loader recover
   * a device left in Octal/DTR mode by a previous external loader or app.
   */

  ret = stm32h7rs_xspi_command(STM32H7RS_XSPI2_BASE,
                               MX66UW1G45G_OCTA_RESET_ENABLE_CMD,
                               XSPI_CCR_IMODE_8_LINES |
                               XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16);
  if (ret < 0)
    {
      syslog(LOG_WARNING, "XSPI2 NOR octal reset-enable failed: %d\n",
             ret);
    }

  ret = stm32h7rs_xspi_command(STM32H7RS_XSPI2_BASE,
                               MX66UW1G45G_OCTA_RESET_MEMORY_CMD,
                               XSPI_CCR_IMODE_8_LINES |
                               XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16);
  if (ret < 0)
    {
      syslog(LOG_WARNING, "XSPI2 NOR octal reset failed: %d\n", ret);
    }

  up_mdelay(STM32H7S78_XSPI_NOR_RESET_DELAY_MS);

  ret = stm32h7rs_xspi_command(STM32H7RS_XSPI2_BASE,
                               MX66UW1G45G_RESET_ENABLE_CMD,
                               XSPI_CCR_IMODE_1_LINE);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR reset-enable failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR reset-enable failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7rs_xspi_command(STM32H7RS_XSPI2_BASE,
                               MX66UW1G45G_RESET_MEMORY_CMD,
                               XSPI_CCR_IMODE_1_LINE);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR reset failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR reset failed: %d\n", ret);
      return ret;
    }

  up_mdelay(STM32H7S78_XSPI_NOR_RESET_DELAY_MS);

  ret = stm32h7s78_nor_readid(id);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR read-id failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR read-id failed: %d sr=%08" PRIx32 "\n",
             ret, getreg32(STM32H7RS_XSPI_SR(STM32H7RS_XSPI2_BASE)));
      return ret;
    }

  syslog(LOG_INFO, "XSPI2 NOR JEDEC ID %02x %02x %02x\n",
         id[0], id[1], id[2]);
  if (stm32h7s78_nor_id_blank(id))
    {
      ferr("ERROR: XSPI2 NOR JEDEC ID is blank\n");
      syslog(LOG_ERR, "XSPI2 NOR JEDEC ID is blank\n");
      syslog(LOG_ERR, "XSPI2 regs CR=%08" PRIx32 " SR=%08" PRIx32
             " DCR1=%08" PRIx32 " DCR2=%08" PRIx32
             " CCR=%08" PRIx32 " TCR=%08" PRIx32
             " DLR=%08" PRIx32 "\n",
             getreg32(STM32H7RS_XSPI_CR(STM32H7RS_XSPI2_BASE)),
             getreg32(STM32H7RS_XSPI_SR(STM32H7RS_XSPI2_BASE)),
             getreg32(STM32H7RS_XSPI_DCR1(STM32H7RS_XSPI2_BASE)),
             getreg32(STM32H7RS_XSPI_DCR2(STM32H7RS_XSPI2_BASE)),
             getreg32(STM32H7RS_XSPI_CCR(STM32H7RS_XSPI2_BASE)),
             getreg32(STM32H7RS_XSPI_TCR(STM32H7RS_XSPI2_BASE)),
             getreg32(STM32H7RS_XSPI_DLR(STM32H7RS_XSPI2_BASE)));
      return -EIO;
    }

  ret = stm32h7s78_nor_config_cfg2_spi(MX66UW1G45G_CFG2_DOPI_ADDR,
                                       MX66UW1G45G_CFG2_DOPI_VALUE,
                                       MX66UW1G45G_CFG2_DOPI_MASK);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR DOPI config failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR DOPI config failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7s78_nor_read_cfg2_spi(MX66UW1G45G_CFG2_OPI_ADDR, &cfg2);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR OPI config read failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR OPI config read failed: %d\n", ret);
      return ret;
    }

  cfg2 = (cfg2 & ~MX66UW1G45G_CFG2_OPI_MASK) |
         (MX66UW1G45G_CFG2_OPI_VALUE & MX66UW1G45G_CFG2_OPI_MASK);
  ret = stm32h7s78_nor_write_cfg2_spi(MX66UW1G45G_CFG2_OPI_ADDR, cfg2,
                                      false);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR OPI config write failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR OPI config write failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7rs_xspi_set_prescaler(
    STM32H7RS_XSPI2_BASE, STM32H7S78_XSPI_NOR_OPTIONAL_PRESCALER);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7s78_nor_wait_wip_octal();
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR optional WIP wait failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR optional WIP wait failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7s78_nor_read_cfg2_octal(MX66UW1G45G_CFG2_OPI_ADDR, &cfg2);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR optional read-after-write failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR optional read-after-write failed: %d\n",
             ret);
      return ret;
    }

  if ((cfg2 & MX66UW1G45G_CFG2_OPI_MASK) != MX66UW1G45G_CFG2_OPI_VALUE)
    {
      ferr("ERROR: XSPI2 NOR OPI readback %02x expected %02x\n",
           cfg2, MX66UW1G45G_CFG2_OPI_VALUE);
      syslog(LOG_ERR, "XSPI2 NOR OPI readback %02x expected %02x\n",
             cfg2, MX66UW1G45G_CFG2_OPI_VALUE);
      return -EIO;
    }

  syslog(LOG_INFO, "XSPI2 NOR OPI/DTR config readback %02x\n", cfg2);

  ret = stm32h7s78_xspi2_enter_memory_mapped();
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR memory-map failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR memory-map failed: %d\n", ret);
      return ret;
    }

  header = *(volatile uint32_t *)STM32H7RS_XSPI2_MEM_BASE;
  syslog(LOG_INFO, "XSPI2 NOR mapped 0x%08x header[0]=0x%08" PRIx32 "\n",
         STM32H7RS_XSPI2_MEM_BASE, header);
  return OK;
}

int stm32h7s78_xspi1_psram_initialize(void)
{
  const struct stm32h7rs_xspi_config_s psram_config =
    {
      .base           = STM32H7RS_XSPI1_BASE,
      .reset          = RCC_AHB5RSTR_XSPI1RST,
      .memory_type    = XSPI_DCR1_MTYP_APMEM_16BIT,
      .device_size    = APS256_DEVSIZE,
      .csht           = 5,
      .prescaler      = STM32H7S78_XSPI_STARTUP_PRESCALER,
      .fifo_threshold = APS256_FIFO_THRESHOLD,
      .csbound        = APS256_CSBOUND,
      .maxtran        = 0,
      .refresh        = stm32h7s78_psram_refresh(
                           STM32H7S78_XSPI_STARTUP_PRESCALER)
    };

  int ret;

  if (stm32h7rs_xspi_is_mapped(STM32H7RS_XSPI1_BASE))
    {
      syslog(LOG_INFO, "XSPI1 PSRAM already memory-mapped refresh=%" PRIu32
             "\n", getreg32(STM32H7RS_XSPI_DCR4(STM32H7RS_XSPI1_BASE)));

      return stm32h7s78_psram_selftest();
    }

  ret = stm32h7rs_xspi_common_setup();
  if (ret < 0)
    {
      ferr("ERROR: XSPI common setup failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM common setup failed: %d\n", ret);
      return ret;
    }

  stm32h7s78_xspi1_gpio_config();
  stm32h7rs_xspi_controller_config(&psram_config);

  ret = stm32h7rs_xspi_command_addr(STM32H7RS_XSPI1_BASE,
                                    APS256_RESET_CMD, 0,
                                    XSPI_CCR_IMODE_8_LINES |
                                    XSPI_CCR_ADMODE_8_LINES |
                                    XSPI_CCR_ADSIZE_32,
                                    0);
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM reset failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM reset failed: %d\n", ret);
      return ret;
    }

  up_mdelay(APS256_RESET_DELAY_MS);

  ret = stm32h7s78_psram_config_reg(APS256_MR0_ADDR, APS256_MR0_VALUE,
                                    0x3f);
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM MR0 config failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM MR0 config failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7s78_psram_config_reg(APS256_MR4_ADDR, APS256_MR4_VALUE,
                                    0xe0);
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM MR4 config failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM MR4 config failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7s78_psram_config_reg(APS256_MR8_ADDR, APS256_MR8_VALUE,
                                    0x40);
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM MR8 config failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM MR8 config failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7s78_xspi1_enter_memory_mapped();
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM memory-map failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM memory-map failed: %d\n", ret);
      return ret;
    }

  ret = stm32h7s78_psram_selftest();
  if (ret < 0)
    {
      return ret;
    }

  syslog(LOG_INFO, "XSPI1 PSRAM mapped at 0x%08x refresh=%" PRIu32 "\n",
         STM32H7RS_XSPI1_MEM_BASE,
         getreg32(STM32H7RS_XSPI_DCR4(STM32H7RS_XSPI1_BASE)));
  return OK;
}
