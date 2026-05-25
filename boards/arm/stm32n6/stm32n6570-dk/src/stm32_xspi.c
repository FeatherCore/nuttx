/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32_xspi.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/param.h>
#include <syslog.h>

#include <arch/board/board.h>
#include <arch/stm32n6/chip.h>
#include <debug.h>
#include <nuttx/arch.h>
#include <nuttx/cache.h>
#include <nuttx/compiler.h>

#include "arm_internal.h"

#include "hardware/stm32n6xxx_memorymap.h"
#include "hardware/stm32n6xxx_rcc.h"
#include "hardware/stm32n6xxx_gpio.h"
#include "hardware/stm32_xspi.h"

#include "stm32_xspi.h"
#include "stm32n6570-dk.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BOARD_XSPI_200MHZ           200000000u
#define BOARD_XSPI_50MHZ            50000000u
#define BOARD_XSPI_PSRAM_MAPPED_HZ  BOARD_XSPI_200MHZ
#define BOARD_XSPI_NOR_RESET_DELAY_MS 100u
#define BOARD_XSPI_STATUS_TIMEOUT   1000000u

#define MX66UW1G45G_RESET_ENABLE_CMD     0x66u
#define MX66UW1G45G_RESET_MEMORY_CMD     0x99u
#define MX66UW1G45G_OCTA_RESET_ENABLE_CMD 0x6699u
#define MX66UW1G45G_OCTA_RESET_MEMORY_CMD 0x9966u
#define MX66UW1G45G_READ_ID_CMD          0x9fu
#define MX66UW1G45G_READ_STATUS_CMD      0x05u
#define MX66UW1G45G_WRITE_ENABLE_CMD     0x06u
#define MX66UW1G45G_CFG2_WRITE_CMD       0x72u
#define MX66UW1G45G_CFG2_READ_CMD        0x71u
#define MX66UW1G45G_OCTAL_CFG2_READAW_CMD 0x718eu
#define MX66UW1G45G_OCTAL_READ_STATUS_CMD 0x05fau
#define MX66UW1G45G_OCTAL_WRITE_ENABLE_CMD 0x06f9u
#define MX66UW1G45G_CFG2_DOPI_ADDR       0x00000300u
#define MX66UW1G45G_CFG2_DOPI_VALUE      0x00u
#define MX66UW1G45G_CFG2_DOPI_MASK       0x07u
#define MX66UW1G45G_CFG2_OPI_ADDR        0x00000000u
#define MX66UW1G45G_CFG2_OPI_VALUE       0x02u
#define MX66UW1G45G_CFG2_OPI_MASK        0x02u
#define MX66UW1G45G_OCTAL_READ_CMD       0xee11u
#define MX66UW1G45G_OCTAL_WRITE_CMD      0x12edu
#define MX66UW1G45G_OCTAL_SECTOR_ERASE_CMD 0x21deu
#define MX66UW1G45G_OCTAL_BLOCK_ERASE_CMD 0xdc23u

#define MX66UW1G45G_JEDEC0               0xc2u
#define MX66UW1G45G_JEDEC1               0x81u
#define MX66UW1G45G_JEDEC2               0x3bu
#define MX66UW1G45G_SR_WIP               0x01u
#define MX66UW1G45G_SR_WEL               0x02u
#define MX66UW1G45G_DEVSIZE              26u
#define MX66UW1G45G_SIZE                 BOARD_XSPI2_NOR_SIZE
#define MX66UW1G45G_PAGE_SIZE            256u
#define MX66UW1G45G_SECTOR_SIZE          0x1000u
#define MX66UW1G45G_BLOCK_SIZE           0x10000u
#define MX66UW1G45G_OCTAL_READ_DUMMY     20u
#define MX66UW1G45G_OCTAL_REG_DUMMY      4u

#define APS256_RESET_CMD                 0xffu
#define APS256_READ_CMD                  0x00u
#define APS256_WRITE_CMD                 0x80u
#define APS256_READ_LINEAR_BURST_CMD     0x20u
#define APS256_WRITE_LINEAR_BURST_CMD    0xa0u
#define APS256_REG_READ_CMD              0x40u
#define APS256_REG_WRITE_CMD             0xc0u

#define APS256_MR0_ADDR                  0x00000000u
#define APS256_MR4_ADDR                  0x00000004u
#define APS256_MR8_ADDR                  0x00000008u
#define APS256_MR0_VALUE                 0x30u
#define APS256_MR4_VALUE                 0x20u
#define APS256_MR8_VALUE                 0x4bu
#define APS256_MR8_MASK                  0xffu
#define APS256_REG_DUMMY                 4u
#define APS256_HEXA_DUMMY                6u
#define APS256_DEVSIZE                   24u
#define APS256_CSBOUND                   11u
#define APS256_FIFO_THRESHOLD            8u
#define APS256_RESET_DELAY_MS            1u

#ifdef CONFIG_ARCH_RAMFUNCS
#  define BOARD_RAMFUNC locate_code(".ramfunc") noinline_function
#else
#  define BOARD_RAMFUNC
#endif

#ifdef CONFIG_STM32N6570_DK_PSRAM_SELFTEST_DEBUG
#  define psramtestinfo(fmt, ...) \
     syslog(LOG_INFO, "XSPI1 PSRAM self-test: " fmt, ##__VA_ARGS__)
#else
#  define psramtestinfo(fmt, ...)
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool g_psram_selftest_done;

static void stm32_nor_invalidate_cache(uint32_t offset, size_t nbytes)
{
  uintptr_t start = BOARD_XSPI2_NOR_BASE + offset;

  up_invalidate_dcache(start, start + nbytes);
}

static uint32_t stm32_psram_refresh(uint32_t source_hz,
                                         uint32_t prescaler)
{
  uint32_t xspi_hz = source_hz / (prescaler + 1u);
  uint32_t cycles = 2u * (xspi_hz / 1000000u);

  return cycles > 4u ? cycles - 4u : 0u;
}

static int stm32_nor_readid(uint8_t id[3])
{
  return stm32_xspi_read_data(STM32_XSPI2_BASE,
                                MX66UW1G45G_READ_ID_CMD, 0,
                                XSPI_CCR_IMODE_1_LINE |
                                XSPI_CCR_DMODE_1_LINE,
                                0, 3, id);
}

static bool stm32_nor_id_blank(FAR const uint8_t id[3])
{
  return (id[0] == 0x00 && id[1] == 0x00 && id[2] == 0x00) ||
         (id[0] == 0xff && id[1] == 0xff && id[2] == 0xff);
}

static void stm32_xspi2_gpio_config(void)
{
  uint32_t pins = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) |
                  (1u << 4) | (1u << 5) | (1u << 6) | (1u << 8) |
                  (1u << 9) | (1u << 10) | (1u << 11);

  putreg32(RCC_AHB4ENSR_GPIONENS, STM32_RCC_AHB4ENSR);
  (void)getreg32(STM32_RCC_AHB4ENR);
  stm32_xspi_config_gpio(STM32_GPION_BASE, pins, GPIO_AF_XSPIM);
}

static void stm32_xspi1_gpio_config(void)
{
  uint32_t gpioo_pins = (1u << 0) | (1u << 2) | (1u << 3) | (1u << 4);
  uint32_t gpiop_pins = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) |
                        (1u << 4) | (1u << 5) | (1u << 6) | (1u << 7) |
                        (1u << 8) | (1u << 9) | (1u << 10) |
                        (1u << 11) | (1u << 12) | (1u << 13) |
                        (1u << 14) | (1u << 15);

  putreg32(RCC_AHB4ENSR_GPIOOENS | RCC_AHB4ENSR_GPIOPENS,
           STM32_RCC_AHB4ENSR);
  (void)getreg32(STM32_RCC_AHB4ENR);
  stm32_xspi_config_gpio(STM32_GPIOO_BASE, gpioo_pins,
                           GPIO_AF_XSPIM);
  stm32_xspi_config_gpio(STM32_GPIOP_BASE, gpiop_pins,
                           GPIO_AF_XSPIM);
}

static void stm32_xspi_enable_clocks(void)
{
  putreg32(RCC_AHB5ENSR_XSPI1ENS | RCC_AHB5ENSR_XSPI2ENS |
           RCC_AHB5ENSR_XSPIMENS, STM32_RCC_AHB5ENSR);
  (void)getreg32(STM32_RCC_AHB5ENR);
}

static int stm32_nor_read_cfg2_spi(uint32_t address, uint8_t *value)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_1_LINE | XSPI_CCR_ADMODE_1_LINE |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_1_LINE;

  return stm32_xspi_read_data(STM32_XSPI2_BASE,
                                MX66UW1G45G_CFG2_READ_CMD, address,
                                ccr, 0, 1, value);
}

static int stm32_nor_read_status_spi(uint8_t *status)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_1_LINE | XSPI_CCR_DMODE_1_LINE;
  return stm32_xspi_read_data(STM32_XSPI2_BASE,
                                MX66UW1G45G_READ_STATUS_CMD, 0,
                                ccr, 0, 1, status);
}

static BOARD_RAMFUNC int stm32_nor_read_status_octal(uint8_t *status)
{
  uint8_t data[2];
  uint32_t ccr;
  int ret;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16 |
        XSPI_CCR_ADMODE_8_LINES | XSPI_CCR_ADDTR |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  ret = stm32_xspi_read_data(STM32_XSPI2_BASE,
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

static BOARD_RAMFUNC int stm32_nor_wait_status(
  int (*read_status)(uint8_t *status), uint8_t mask, uint8_t value)
{
  uint32_t timeout = BOARD_XSPI_STATUS_TIMEOUT;
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

static int stm32_nor_wait_wip_spi(void)
{
  return stm32_nor_wait_status(stm32_nor_read_status_spi,
                                 MX66UW1G45G_SR_WIP, 0);
}

static BOARD_RAMFUNC int stm32_nor_wait_wip_octal(void)
{
  return stm32_nor_wait_status(stm32_nor_read_status_octal,
                                 MX66UW1G45G_SR_WIP, 0);
}

static int stm32_nor_write_enable_spi(void)
{
  int ret;

  ret = stm32_xspi_command(STM32_XSPI2_BASE,
                             MX66UW1G45G_WRITE_ENABLE_CMD,
                             XSPI_CCR_IMODE_1_LINE);
  if (ret < 0)
    {
      return ret;
    }

  return stm32_nor_wait_status(stm32_nor_read_status_spi,
                                 MX66UW1G45G_SR_WEL,
                                 MX66UW1G45G_SR_WEL);
}

static BOARD_RAMFUNC int stm32_nor_write_enable_octal(void)
{
  int ret;

  ret = stm32_xspi_command(STM32_XSPI2_BASE,
                             MX66UW1G45G_OCTAL_WRITE_ENABLE_CMD,
                             XSPI_CCR_IMODE_8_LINES |
                             XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16);
  if (ret < 0)
    {
      return ret;
    }

  return stm32_nor_wait_status(stm32_nor_read_status_octal,
                                 MX66UW1G45G_SR_WEL,
                                 MX66UW1G45G_SR_WEL);
}

static int stm32_nor_write_cfg2_spi(uint32_t address, uint8_t value,
                                      bool wait_after)
{
  uint32_t ccr;
  int ret;

  ret = stm32_nor_wait_wip_spi();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_nor_write_enable_spi();
  if (ret < 0)
    {
      return ret;
    }

  ccr = XSPI_CCR_IMODE_1_LINE | XSPI_CCR_ADMODE_1_LINE |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_1_LINE;

  ret = stm32_xspi_write_data(STM32_XSPI2_BASE,
                                MX66UW1G45G_CFG2_WRITE_CMD, address,
                                ccr, 0, value, 1, false);
  if (ret < 0 || !wait_after)
    {
      return ret;
    }

  return stm32_nor_wait_wip_spi();
}

static int stm32_nor_read_cfg2_octal(uint32_t address, uint8_t *value)
{
  uint8_t data[2];
  uint32_t ccr;
  int ret;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16 |
        XSPI_CCR_ADMODE_8_LINES | XSPI_CCR_ADDTR |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  ret = stm32_xspi_read_data(STM32_XSPI2_BASE,
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

static int stm32_nor_config_cfg2_spi(uint32_t address, uint8_t value,
                                       uint8_t mask)
{
  uint8_t readback;
  uint8_t writeval;
  int ret;

  ret = stm32_nor_read_cfg2_spi(address, &readback);
  if (ret < 0)
    {
      return ret;
    }

  writeval = (readback & ~mask) | (value & mask);
  ret = stm32_nor_write_cfg2_spi(address, writeval, true);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_nor_read_cfg2_spi(address, &readback);
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

static BOARD_RAMFUNC int stm32_xspi2_enter_memory_mapped(void)
{
  uintptr_t base = STM32_XSPI2_BASE;
  uint32_t prescaler = 0;
  uint32_t source_hz;
  uint32_t ccr;
  int ret;

  source_hz = stm32_xspi_get_source_hz(base);
  if (source_hz < BOARD_XSPI_200MHZ)
    {
      syslog(LOG_WARNING,
             "stm32n6: XSPI2 HSLV disabled, NOR mapped at 50MHz\n");
    }

  ret = stm32_xspi_set_prescaler(base, prescaler);
  if (ret < 0)
    {
      return ret;
    }

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16 |
        XSPI_CCR_ADMODE_8_LINES | XSPI_CCR_ADDTR |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  return stm32_xspi_enter_memory_mapped(
           base, ccr, XSPI_TCR_DCYC(MX66UW1G45G_OCTAL_READ_DUMMY),
           MX66UW1G45G_OCTAL_READ_CMD, ccr, 0,
           MX66UW1G45G_OCTAL_WRITE_CMD);
}

static int stm32_xspi1_enter_memory_mapped(void)
{
  uintptr_t base = STM32_XSPI1_BASE;
  uint32_t prescaler;
  uint32_t source_hz;
  uint32_t target_hz;
  uint32_t ccr;
  int ret;

  source_hz = stm32_xspi_get_source_hz(base);
  target_hz = BOARD_XSPI_PSRAM_MAPPED_HZ;
  if (!stm32_xspi_hslv_enabled(base) && target_hz > BOARD_XSPI_50MHZ)
    {
      syslog(LOG_WARNING,
             "stm32n6: XSPI1 HSLV disabled, PSRAM mapped at 50MHz\n");
      target_hz = BOARD_XSPI_50MHZ;
    }

  prescaler = stm32_xspi_prescaler(source_hz, target_hz);

  ret = stm32_xspi_set_prescaler(base, prescaler);
  if (ret < 0)
    {
      return ret;
    }

  putreg32(stm32_psram_refresh(source_hz, prescaler),
           STM32_XSPI_DCR4(base));

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_16_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  return stm32_xspi_enter_memory_mapped(
           base, ccr, XSPI_TCR_DCYC(APS256_HEXA_DUMMY),
           APS256_READ_LINEAR_BURST_CMD,
           ccr, XSPI_TCR_DCYC(APS256_HEXA_DUMMY),
           APS256_WRITE_LINEAR_BURST_CMD);
}

static int stm32_psram_write_reg(uint32_t address, uint8_t value)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  return stm32_xspi_write_data(STM32_XSPI1_BASE,
                                 APS256_REG_WRITE_CMD, address, ccr,
                                 0, value, 2, true);
}

static int stm32_psram_read_reg(uint32_t address, uint8_t *value)
{
  uint8_t data[2];
  uint32_t ccr;
  int ret;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  ret = stm32_xspi_read_data(STM32_XSPI1_BASE,
                               APS256_REG_READ_CMD, address, ccr,
                               XSPI_TCR_DCYC(APS256_REG_DUMMY),
                               sizeof(data), data);
  if (ret >= 0)
    {
      *value = data[0];
    }

  return ret;
}

static int stm32_psram_config_reg(uint32_t address, uint8_t value,
                                    uint8_t mask)
{
  uint8_t initial;
  uint8_t readback;
  uint8_t writeval;
  int ret;

  ret = stm32_psram_read_reg(address, &initial);
  if (ret < 0)
    {
      return ret;
    }

  writeval = (initial & ~mask) | (value & mask);
  ret = stm32_psram_write_reg(address, writeval);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_psram_read_reg(address, &readback);
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

static void stm32_psram_dcache_flush(FAR const volatile void *addr,
                                          size_t size)
{
  uintptr_t start = (uintptr_t)addr;
  uintptr_t end = start + size;

  psramtestinfo("dcache flush start=%08" PRIxPTR " size=%zu\n",
                start, size);
  up_flush_dcache(start, end);
  psramtestinfo("dcache invalidate start=%08" PRIxPTR " size=%zu\n",
                start, size);
  up_invalidate_dcache(start, end);
  psramtestinfo("dcache done start=%08" PRIxPTR " size=%zu\n",
                start, size);
}

static int stm32_psram_test_pattern(uintptr_t base, uint32_t offset,
                                         FAR const uint32_t *pattern)
{
  volatile uint32_t *ptr = (volatile uint32_t *)(base + offset);
  uint32_t saved[8];
  int i;

  psramtestinfo("pattern offset=%08" PRIx32 " ptr=%p begin\n",
                offset, ptr);

  for (i = 0; i < nitems(saved); i++)
    {
      saved[i] = ptr[i];
    }

  psramtestinfo("pattern offset=%08" PRIx32 " saved\n", offset);

  for (i = 0; i < nitems(saved); i++)
    {
      ptr[i] = pattern[i];
    }

  psramtestinfo("pattern offset=%08" PRIx32 " written\n", offset);
  stm32_psram_dcache_flush(ptr, sizeof(saved));
  psramtestinfo("pattern offset=%08" PRIx32 " flushed\n", offset);

  for (i = 0; i < nitems(saved); i++)
    {
      if (ptr[i] != pattern[i])
        {
          ferr("ERROR: PSRAM self-test[0x%08" PRIx32 "+%d] got %08"
               PRIx32 " expected %08" PRIx32 "\n",
               offset, i * 4, ptr[i], pattern[i]);

          for (i = 0; i < nitems(saved); i++)
            {
              ptr[i] = saved[i];
            }

          stm32_psram_dcache_flush(ptr, sizeof(saved));
          return -EIO;
        }
    }

  psramtestinfo("pattern offset=%08" PRIx32 " verified\n", offset);

  for (i = 0; i < nitems(saved); i++)
    {
      ptr[i] = saved[i];
    }

  psramtestinfo("pattern offset=%08" PRIx32 " restored\n", offset);
  stm32_psram_dcache_flush(ptr, sizeof(saved));
  psramtestinfo("pattern offset=%08" PRIx32 " done\n", offset);
  return OK;
}

static int stm32_psram_selftest_range(uintptr_t base, size_t size,
                                           FAR const char *label)
{
  const uint32_t offsets[] =
    {
      0x00000000u,
      0x00000040u,
      0x00001000u,
      0x00001040u,
      0x00100000u,
      0x00ffffc0u,
      0x01ffffc0u
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

  uint32_t pattern[8] =
    {
      0x55aa55aau, 0xaa55aa55u, 0x01234567u, 0x89abcdefu,
      0xf0f00f0fu, 0x0f0ff0f0u, 0x13579bdfu, 0x2468ace0u
    };
  uint32_t alias_saved[sizeof(alias_offsets) / sizeof(alias_offsets[0])];
  size_t alias;
  size_t end_offset;
  size_t region;
  size_t restore;
  uint32_t last_offset = UINT32_MAX;
  int ret;

  psramtestinfo("range base=%08" PRIxPTR " size=%zu label=%s begin\n",
                base, size, label);

  if (base < BOARD_XSPI1_PSRAM_BASE ||
      base > BOARD_XSPI1_PSRAM_BASE + BOARD_XSPI1_PSRAM_SIZE ||
      size < sizeof(pattern) ||
      size > BOARD_XSPI1_PSRAM_BASE + BOARD_XSPI1_PSRAM_SIZE - base)
    {
      ferr("ERROR: PSRAM self-test range invalid base=0x%08" PRIx32
           " size=0x%08" PRIx32 "\n", (uint32_t)base, (uint32_t)size);
      return -EINVAL;
    }

  for (region = 0; region < nitems(offsets); region++)
    {
      if (offsets[region] + sizeof(pattern) <= size)
        {
          psramtestinfo("range pattern region=%zu offset=%08" PRIx32 "\n",
                        region, offsets[region]);
          ret = stm32_psram_test_pattern(base, offsets[region],
                                              pattern);
          if (ret < 0)
            {
              return ret;
            }

          last_offset = offsets[region];
        }
    }

  end_offset = size - sizeof(pattern);
  if (end_offset != last_offset)
    {
      psramtestinfo("range end-pattern offset=%08" PRIx32 "\n",
                    (uint32_t)end_offset);
      ret = stm32_psram_test_pattern(base, end_offset, pattern);
      if (ret < 0)
        {
          return ret;
        }
    }

  psramtestinfo("alias save/write begin\n");
  for (alias = 0; alias < nitems(alias_offsets); alias++)
    {
      volatile uint32_t *ptr =
        (volatile uint32_t *)(base + alias_offsets[alias]);

      if (alias_offsets[alias] + sizeof(uint32_t) > size)
        {
          continue;
        }

      alias_saved[alias] = *ptr;
      *ptr = base + alias_offsets[alias];
    }

  psramtestinfo("alias save/write done\n");
  stm32_psram_dcache_flush((FAR const void *)base, MIN(size, 0x300));
  psramtestinfo("alias flushed\n");

  psramtestinfo("alias verify begin\n");
  for (alias = 0; alias < nitems(alias_offsets); alias++)
    {
      volatile uint32_t *ptr =
        (volatile uint32_t *)(base + alias_offsets[alias]);
      uint32_t expected = base + alias_offsets[alias];

      if (alias_offsets[alias] + sizeof(uint32_t) > size)
        {
          continue;
        }

      if (*ptr != expected)
        {
          ferr("ERROR: PSRAM address self-test[0x%08" PRIx32
               "] got %08" PRIx32 " expected %08" PRIx32 "\n",
               alias_offsets[alias], *ptr, expected);
          for (restore = 0; restore < nitems(alias_offsets); restore++)
            {
              ptr = (volatile uint32_t *)(base + alias_offsets[restore]);
              if (alias_offsets[restore] + sizeof(uint32_t) > size)
                {
                  continue;
                }

              *ptr = alias_saved[restore];
            }

          stm32_psram_dcache_flush((FAR const void *)base,
                                        MIN(size, 0x300));
          return -EIO;
        }
    }

  psramtestinfo("alias verify done\n");

  for (alias = 0; alias < nitems(alias_offsets); alias++)
    {
      volatile uint32_t *ptr =
        (volatile uint32_t *)(base + alias_offsets[alias]);

      if (alias_offsets[alias] + sizeof(uint32_t) > size)
        {
          continue;
        }

      *ptr = alias_saved[alias];
    }

  psramtestinfo("alias restore done\n");
  stm32_psram_dcache_flush((FAR const void *)base, MIN(size, 0x300));
  psramtestinfo("range done\n");

  if (label[0] != '\0')
    {
      syslog(LOG_INFO, "XSPI1 PSRAM %s self-test passed\n", label);
    }
  else
    {
      syslog(LOG_INFO, "XSPI1 PSRAM self-test passed\n");
    }

  return OK;
}

static int stm32_psram_selftest(void)
{
  return stm32_psram_selftest_range(BOARD_XSPI1_PSRAM_BASE,
                                         BOARD_XSPI1_PSRAM_SIZE,
                                         "");
}

static int stm32_psram_verify_once(void)
{
  int ret;

  if (g_psram_selftest_done)
    {
      psramtestinfo("verify skipped\n");
      return OK;
    }

  psramtestinfo("verify begin\n");
  ret = stm32_psram_selftest();
  if (ret >= 0)
    {
      g_psram_selftest_done = true;
      psramtestinfo("verify passed\n");
    }
  else
    {
      psramtestinfo("verify failed ret=%d\n", ret);
    }

  return ret;
}

static BOARD_RAMFUNC uint32_t stm32_nor_octal_command_ccr(void)
{
  return XSPI_CCR_IMODE_8_LINES | XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16 |
         XSPI_CCR_ADMODE_8_LINES | XSPI_CCR_ADDTR |
         XSPI_CCR_ADSIZE_32;
}

static BOARD_RAMFUNC uint32_t stm32_nor_octal_data_ccr(void)
{
  return stm32_nor_octal_command_ccr() |
         XSPI_CCR_DMODE_8_LINES | XSPI_CCR_DDTR | XSPI_CCR_DQSE;
}

static void stm32_nor_log_error(FAR const char *op, int ret)
{
  uint8_t status = 0xff;

  (void)stm32_nor_read_status_octal(&status);
  syslog(LOG_ERR, "XSPI2 NOR %s failed: %d sr=%02x xspi_sr=%08" PRIx32
         " cr=%08" PRIx32 "\n",
         op, ret, status,
         getreg32(STM32_XSPI_SR(STM32_XSPI2_BASE)),
         getreg32(STM32_XSPI_CR(STM32_XSPI2_BASE)));
}

static BOARD_RAMFUNC int stm32_nor_restore_memory_mapped(int opret)
{
  int mapret;

  mapret = stm32_xspi2_enter_memory_mapped();
  if (mapret < 0)
    {
#ifdef CONFIG_NXBOOT_BOOTLOADER
      syslog(LOG_ERR, "XSPI2 NOR restore memory-map failed: %d\n", mapret);
#endif
      return opret < 0 ? opret : mapret;
    }

  return opret;
}

static BOARD_RAMFUNC int stm32_nor_finish_error(FAR const char *op,
                                                    int opret)
{
  int mapret;

  mapret = stm32_xspi2_enter_memory_mapped();
  if (mapret >= 0)
    {
      stm32_nor_log_error(op, opret);
    }

  return opret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_xspi2_nor_initialize(void)
{
  struct stm32_xspi_config_s nor_config;
  uint32_t startup_prescaler;
  uint32_t source_hz;
  uint8_t cfg2;
  uint32_t header;
  uint8_t id[3] = {0};
  int ret;

  stm32_xspi_enable_clocks();

  if (stm32_xspi_is_mapped(STM32_XSPI2_BASE))
    {
      syslog(LOG_INFO, "XSPI2 NOR already memory-mapped\n");
      return OK;
    }

  ret = stm32_xspi_common_setup();
  if (ret < 0)
    {
      ferr("ERROR: XSPI common setup failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR common setup failed: %d\n", ret);
      return ret;
    }

  source_hz = stm32_xspi_get_source_hz(STM32_XSPI2_BASE);
  startup_prescaler =
    stm32_xspi_prescaler(source_hz, BOARD_XSPI_50MHZ);

  stm32_xspi2_gpio_config();

  nor_config.base           = STM32_XSPI2_BASE;
  nor_config.reset          = RCC_AHB5RSTR_XSPI2RST;
  nor_config.memory_type    = XSPI_DCR1_MTYP_MACRONIX;
  nor_config.device_size    = MX66UW1G45G_DEVSIZE;
  nor_config.csht           = 2;
  nor_config.prescaler      = startup_prescaler;
  nor_config.fifo_threshold = 4;
  nor_config.csbound        = 0;
  nor_config.refresh        = 0;
  stm32_xspi_controller_config(&nor_config);

  ret = stm32_xspi_command(STM32_XSPI2_BASE,
                             MX66UW1G45G_OCTA_RESET_ENABLE_CMD,
                             XSPI_CCR_IMODE_8_LINES |
                             XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16);
  if (ret < 0)
    {
      syslog(LOG_WARNING, "XSPI2 NOR octal reset-enable failed: %d\n",
             ret);
    }

  ret = stm32_xspi_command(STM32_XSPI2_BASE,
                             MX66UW1G45G_OCTA_RESET_MEMORY_CMD,
                             XSPI_CCR_IMODE_8_LINES |
                             XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16);
  if (ret < 0)
    {
      syslog(LOG_WARNING, "XSPI2 NOR octal reset failed: %d\n", ret);
    }

  up_mdelay(BOARD_XSPI_NOR_RESET_DELAY_MS);

  ret = stm32_xspi_command(STM32_XSPI2_BASE,
                             MX66UW1G45G_RESET_ENABLE_CMD,
                             XSPI_CCR_IMODE_1_LINE);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR reset-enable failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR reset-enable failed: %d\n", ret);
      return ret;
    }

  ret = stm32_xspi_command(STM32_XSPI2_BASE,
                             MX66UW1G45G_RESET_MEMORY_CMD,
                             XSPI_CCR_IMODE_1_LINE);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR reset failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR reset failed: %d\n", ret);
      return ret;
    }

  up_mdelay(BOARD_XSPI_NOR_RESET_DELAY_MS);

  ret = stm32_nor_readid(id);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR read-id failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR read-id failed: %d sr=%08" PRIx32 "\n",
             ret, getreg32(STM32_XSPI_SR(STM32_XSPI2_BASE)));
      return ret;
    }

  syslog(LOG_INFO, "XSPI2 NOR JEDEC ID %02x %02x %02x\n",
         id[0], id[1], id[2]);
  if (stm32_nor_id_blank(id))
    {
      ferr("ERROR: XSPI2 NOR JEDEC ID is blank\n");
      syslog(LOG_ERR, "XSPI2 NOR JEDEC ID is blank\n");
      syslog(LOG_ERR, "XSPI2 regs CR=%08" PRIx32 " SR=%08" PRIx32
             " DCR1=%08" PRIx32 " DCR2=%08" PRIx32
             " CCR=%08" PRIx32 " TCR=%08" PRIx32
             " DLR=%08" PRIx32 "\n",
             getreg32(STM32_XSPI_CR(STM32_XSPI2_BASE)),
             getreg32(STM32_XSPI_SR(STM32_XSPI2_BASE)),
             getreg32(STM32_XSPI_DCR1(STM32_XSPI2_BASE)),
             getreg32(STM32_XSPI_DCR2(STM32_XSPI2_BASE)),
             getreg32(STM32_XSPI_CCR(STM32_XSPI2_BASE)),
             getreg32(STM32_XSPI_TCR(STM32_XSPI2_BASE)),
             getreg32(STM32_XSPI_DLR(STM32_XSPI2_BASE)));
      return -EIO;
    }

  ret = stm32_nor_config_cfg2_spi(MX66UW1G45G_CFG2_DOPI_ADDR,
                                    MX66UW1G45G_CFG2_DOPI_VALUE,
                                    MX66UW1G45G_CFG2_DOPI_MASK);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR DOPI config failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR DOPI config failed: %d\n", ret);
      return ret;
    }

  ret = stm32_nor_read_cfg2_spi(MX66UW1G45G_CFG2_OPI_ADDR, &cfg2);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR OPI config read failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR OPI config read failed: %d\n", ret);
      return ret;
    }

  cfg2 = (cfg2 & ~MX66UW1G45G_CFG2_OPI_MASK) |
         (MX66UW1G45G_CFG2_OPI_VALUE & MX66UW1G45G_CFG2_OPI_MASK);
  ret = stm32_nor_write_cfg2_spi(MX66UW1G45G_CFG2_OPI_ADDR, cfg2,
                                   false);
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR OPI config write failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR OPI config write failed: %d\n", ret);
      return ret;
    }

  ret = stm32_xspi_set_prescaler(STM32_XSPI2_BASE, 0);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_nor_wait_wip_octal();
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR optional WIP wait failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR optional WIP wait failed: %d\n", ret);
      return ret;
    }

  ret = stm32_nor_read_cfg2_octal(MX66UW1G45G_CFG2_OPI_ADDR, &cfg2);
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

  ret = stm32_xspi2_enter_memory_mapped();
  if (ret < 0)
    {
      ferr("ERROR: XSPI2 NOR memory-map failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI2 NOR memory-map failed: %d\n", ret);
      return ret;
    }

  header = *(volatile uint32_t *)(BOARD_XSPI2_NOR_BASE +
                                  CONFIG_STM32N6_OTA_PRIMARY_SLOT_OFFSET);
  syslog(LOG_INFO, "XSPI2 NOR mapped 0x%08x ota0[0]=0x%08" PRIx32 "\n",
         BOARD_XSPI2_NOR_BASE, header);
  return OK;
}

BOARD_RAMFUNC int stm32_xspi2_nor_erase(uint32_t offset,
                                            size_t nbytes)
{
  uint32_t start_offset = offset;
  size_t erase_nbytes = nbytes;
  uint32_t ccr;
  int ret;

  if (nbytes == 0)
    {
      return OK;
    }

  if ((offset % MX66UW1G45G_SECTOR_SIZE) != 0 ||
      (nbytes % MX66UW1G45G_SECTOR_SIZE) != 0 ||
      offset > MX66UW1G45G_SIZE ||
      nbytes > MX66UW1G45G_SIZE - offset)
    {
      return -EINVAL;
    }

  ret = stm32_xspi_common_setup();
  if (ret < 0)
    {
      return ret;
    }

  if (!stm32_xspi_is_mapped(STM32_XSPI2_BASE))
    {
      ret = stm32_xspi2_nor_initialize();
      if (ret < 0)
        {
          return ret;
        }
    }

  ret = stm32_xspi_abort_memory_mapped(STM32_XSPI2_BASE);
  if (ret < 0)
    {
      return stm32_nor_finish_error("abort", ret);
    }

  ccr = stm32_nor_octal_command_ccr();
  while (nbytes > 0)
    {
      uint32_t cmd;
      size_t erasesize;

      if ((offset % MX66UW1G45G_BLOCK_SIZE) == 0 &&
          nbytes >= MX66UW1G45G_BLOCK_SIZE)
        {
          cmd = MX66UW1G45G_OCTAL_BLOCK_ERASE_CMD;
          erasesize = MX66UW1G45G_BLOCK_SIZE;
        }
      else
        {
          cmd = MX66UW1G45G_OCTAL_SECTOR_ERASE_CMD;
          erasesize = MX66UW1G45G_SECTOR_SIZE;
        }

      ret = stm32_nor_write_enable_octal();
      if (ret < 0)
        {
          return stm32_nor_finish_error("write-enable erase", ret);
        }

      ret = stm32_xspi_command_addr(STM32_XSPI2_BASE, cmd, offset,
                                      ccr, 0);
      if (ret < 0)
        {
          return stm32_nor_finish_error("erase command", ret);
        }

      ret = stm32_nor_wait_wip_octal();
      if (ret < 0)
        {
          return stm32_nor_finish_error("erase wait", ret);
        }

      offset += erasesize;
      nbytes -= erasesize;
    }

  ret = stm32_nor_restore_memory_mapped(OK);
  if (ret < 0)
    {
      return ret;
    }

  stm32_nor_invalidate_cache(start_offset, erase_nbytes);
  return OK;
}

BOARD_RAMFUNC ssize_t stm32_xspi2_nor_write(
  uint32_t offset, FAR const uint8_t *buffer, size_t nbytes)
{
  uint32_t start_offset = offset;
  size_t written = 0;
  uint32_t ccr;
  int ret;

  if (nbytes == 0)
    {
      return 0;
    }

  if (buffer == NULL || offset > MX66UW1G45G_SIZE ||
      nbytes > MX66UW1G45G_SIZE - offset)
    {
      return -EINVAL;
    }

  ret = stm32_xspi_common_setup();
  if (ret < 0)
    {
      return ret;
    }

  if (!stm32_xspi_is_mapped(STM32_XSPI2_BASE))
    {
      ret = stm32_xspi2_nor_initialize();
      if (ret < 0)
        {
          return ret;
        }
    }

  ret = stm32_xspi_abort_memory_mapped(STM32_XSPI2_BASE);
  if (ret < 0)
    {
      return stm32_nor_finish_error("abort", ret);
    }

  ccr = stm32_nor_octal_data_ccr();
  while (written < nbytes)
    {
      size_t pageleft = MX66UW1G45G_PAGE_SIZE -
                        ((offset + written) % MX66UW1G45G_PAGE_SIZE);
      size_t chunk = nbytes - written;

      if (chunk > pageleft)
        {
          chunk = pageleft;
        }

      ret = stm32_nor_write_enable_octal();
      if (ret < 0)
        {
          (void)stm32_nor_finish_error("write-enable program", ret);
          return written > 0 ? (ssize_t)written : ret;
        }

      ret = stm32_xspi_write_buffer(STM32_XSPI2_BASE,
                                      MX66UW1G45G_OCTAL_WRITE_CMD,
                                      offset + written, ccr, 0,
                                      buffer + written, chunk);
      if (ret < 0)
        {
          (void)stm32_nor_finish_error("page program", ret);
          return written > 0 ? (ssize_t)written : ret;
        }

      ret = stm32_nor_wait_wip_octal();
      if (ret < 0)
        {
          (void)stm32_nor_finish_error("program wait", ret);
          return written > 0 ? (ssize_t)written : ret;
        }

      written += chunk;
    }

  ret = stm32_nor_restore_memory_mapped(OK);
  if (ret < 0)
    {
      return ret;
    }

  stm32_nor_invalidate_cache(start_offset, written);
  return (ssize_t)written;
}

static int stm32_xspi1_psram_map_internal(bool verify)
{
  struct stm32_xspi_config_s psram_config;
  uint32_t startup_prescaler;
  uint32_t source_hz;
  int ret;

  stm32_xspi_enable_clocks();

  if (stm32_xspi_is_mapped(STM32_XSPI1_BASE))
    {
      if (verify)
        {
          syslog(LOG_INFO, "XSPI1 PSRAM already memory-mapped refresh=%"
                 PRIu32 "\n",
                 getreg32(STM32_XSPI_DCR4(STM32_XSPI1_BASE)));
        }

      return verify ? stm32_psram_verify_once() : OK;
    }

#ifdef CONFIG_NXBOOT_BOOTLOADER
  ret = stm32_xspi_common_setup();
  if (ret < 0)
    {
      ferr("ERROR: XSPI common setup failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM common setup failed: %d\n", ret);
      return ret;
    }
#endif

  source_hz = stm32_xspi_get_source_hz(STM32_XSPI1_BASE);
  startup_prescaler =
    stm32_xspi_prescaler(source_hz, BOARD_XSPI_50MHZ);

  stm32_xspi1_gpio_config();

  psram_config.base           = STM32_XSPI1_BASE;
  psram_config.reset          = RCC_AHB5RSTR_XSPI1RST;
  psram_config.memory_type    = XSPI_DCR1_MTYP_APMEM_16BIT;
  psram_config.device_size    = APS256_DEVSIZE;
  psram_config.csht           = 5;
  psram_config.prescaler      = startup_prescaler;
  psram_config.fifo_threshold = APS256_FIFO_THRESHOLD;
  psram_config.csbound        = APS256_CSBOUND;
  psram_config.refresh        =
    stm32_psram_refresh(source_hz, startup_prescaler);
  stm32_xspi_controller_config(&psram_config);

  ret = stm32_xspi_command_addr(STM32_XSPI1_BASE,
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

  ret = stm32_psram_config_reg(APS256_MR0_ADDR, APS256_MR0_VALUE, 0x3f);
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM MR0 config failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM MR0 config failed: %d\n", ret);
      return ret;
    }

  ret = stm32_psram_config_reg(APS256_MR4_ADDR, APS256_MR4_VALUE, 0xe0);
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM MR4 config failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM MR4 config failed: %d\n", ret);
      return ret;
    }

  ret = stm32_psram_config_reg(APS256_MR8_ADDR, APS256_MR8_VALUE,
                                    APS256_MR8_MASK);
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM MR8 config failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM MR8 config failed: %d\n", ret);
      return ret;
    }

  ret = stm32_xspi1_enter_memory_mapped();
  if (ret < 0)
    {
      ferr("ERROR: XSPI1 PSRAM memory-map failed: %d\n", ret);
      syslog(LOG_ERR, "XSPI1 PSRAM memory-map failed: %d\n", ret);
      return ret;
    }

  if (verify)
    {
      ret = stm32_psram_verify_once();
      if (ret < 0)
        {
          return ret;
        }
    }

  syslog(LOG_INFO, "XSPI1 PSRAM mapped at 0x%08x refresh=%" PRIu32 "\n",
         BOARD_XSPI1_PSRAM_BASE,
         getreg32(STM32_XSPI_DCR4(STM32_XSPI1_BASE)));
  return OK;
}

int stm32_xspi1_psram_map(void)
{
  return stm32_xspi1_psram_map_internal(false);
}

int stm32_xspi1_psram_test(uintptr_t base, size_t size)
{
  int ret;

  ret = stm32_psram_selftest_range(base, size, "heap");
  if (ret >= 0)
    {
      g_psram_selftest_done = true;
    }

  return ret;
}

int stm32_xspi1_psram_initialize(void)
{
  return stm32_xspi1_psram_map_internal(true);
}
