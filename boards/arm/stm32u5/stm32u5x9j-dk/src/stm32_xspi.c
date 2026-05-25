/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32_xspi.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/param.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>

#include <debug.h>
#include <nuttx/arch.h>
#include <nuttx/fs/fs.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "stm32_gpio.h"
#include "stm32_pwr.h"
#include "stm32_xspi.h"
#include "stm32u5x9j-dk.h"

#if defined(CONFIG_STM32U5X9J_DK_OSPI_NOR) || \
    defined(CONFIG_STM32U5X9J_DK_HSPI_RAM)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32U5X9J_OSPI_INIT_PRESCALER      3u
#define STM32U5X9J_OSPI_RUN_PRESCALER       1u
#define STM32U5X9J_OSPI_CSHT                2u
#define STM32U5X9J_OSPI_FIFO_THRESHOLD      4u
#define STM32U5X9J_OSPI_DIAG_PATH           "/dev/ospi0"
#define STM32U5X9J_OSPI_DLYB_MAX_PHASE      12u
#define STM32U5X9J_OSPI_XSPIM_PORT          1u
#define STM32U5X9J_OSPI_XSPIM_REQ2ACK       1u

#define MX25UM51245G_FLASH_SIZE             STM32U5X9J_OSPI1_NOR_SIZE
#define MX25UM51245G_ERASE_SIZE             (64u * 1024u)
#define MX25UM51245G_SUBSECTOR_SIZE         (4u * 1024u)
#define MX25UM51245G_PAGE_SIZE              256u
#define MX25UM51245G_DEVSIZE                25u
#define MX25UM51245G_MANUFACTURER_ID        0xc2u
#define MX25UM51245G_READ_ID_CMD            0x9fu
#define MX25UM51245G_WRITE_ENABLE_CMD       0x06u
#define MX25UM51245G_READ_STATUS_CMD        0x05u
#define MX25UM51245G_WRITE_CFG_REG2_CMD     0x72u
#define MX25UM51245G_READ_CFG_REG2_CMD      0x71u
#define MX25UM51245G_RESET_ENABLE_CMD       0x66u
#define MX25UM51245G_RESET_MEMORY_CMD       0x99u
#define MX25UM51245G_OCTA_RESET_ENABLE_CMD  0x6699u
#define MX25UM51245G_OCTA_RESET_MEMORY_CMD  0x9966u
#define MX25UM51245G_OCTA_READ_DTR_CMD      0xee11u
#define MX25UM51245G_OCTA_PAGE_PROG_CMD     0x12edu
#define MX25UM51245G_OCTA_READ_STATUS_CMD   0x05fau
#define MX25UM51245G_OCTA_WRITE_CFG2_CMD    0x728du
#define MX25UM51245G_OCTA_READ_CFG2_CMD     0x718eu
#define MX25UM51245G_FAST_READ_4B_CMD       0x0cu
#define MX25UM51245G_FAST_READ_DUMMY        8u
#define MX25UM51245G_OCTA_DTR_READ_DUMMY    6u
#define MX25UM51245G_OCTA_DTR_REG_DUMMY     5u
#define MX25UM51245G_SUBSECTOR_ERASE_4B_CMD 0x21u
#define MX25UM51245G_PAGE_PROGRAM_4B_CMD    0x12u
#define MX25UM51245G_SUSPEND_CMD            0xb0u
#define MX25UM51245G_RESUME_CMD             0x30u
#define MX25UM51245G_OCTA_SUSPEND_CMD       0xb04fu
#define MX25UM51245G_OCTA_RESUME_CMD        0x30cfu
#define MX25UM51245G_STATUS_WIP             (1u << 0)
#define MX25UM51245G_STATUS_WEL             (1u << 1)
#define MX25UM51245G_CR2_REG1_ADDR          0x00000000u
#define MX25UM51245G_CR2_REG3_ADDR          0x00000300u
#define MX25UM51245G_CR2_DOPI               0x02u
#define MX25UM51245G_CR2_DC_MASK            0x07u
#define MX25UM51245G_CR2_DC_6_CYCLES        0x07u
#define MX25UM51245G_RESET_DELAY_MS         100u
#define MX25UM51245G_WRITE_REG_DELAY_MS     40u
#define MX25UM51245G_READY_TIMEOUT_MS       5000u
#define STM32U5X9J_OSPI_HSLV_PORTA_MASK     ((1u << 1) | (1u << 2))
#define STM32U5X9J_OSPI_HSLV_PORTC_MASK     0x000fu
#define STM32U5X9J_OSPI_HSLV_PORTF_MASK     0x07c0u

#define STM32U5X9J_HSPI_INIT_PRESCALER      3u
#define STM32U5X9J_HSPI_RUN_PRESCALER       0u
#define STM32U5X9J_HSPI_CSHT                1u
#define STM32U5X9J_HSPI_FIFO_THRESHOLD      2u
#define STM32U5X9J_HSPI_MAXTRAN             0u
#define STM32U5X9J_HSPI_CSBOUND             11u
#define STM32U5X9J_HSPI_DIAG_PATH           "/dev/hspiram0"
#define STM32U5X9J_HSPI_HSLV_PORTH_MASK     0xfe00u
#define STM32U5X9J_HSPI_HSLV_PORTI_MASK     0xff1fu
#define STM32U5X9J_HSPI_HSLV_PORTJ_MASK     0x0001u

#define APS512XX_RAM_SIZE                   STM32U5X9J_HSPI1_PSRAM_SIZE
#define APS512XX_FB_RESERVED                \
                                            STM32U5X9J_HSPI1_PSRAM_FB_RESERVED_SIZE
#define APS512XX_HEAP_BASE                  STM32U5X9J_HSPI1_PSRAM_HEAP_BASE
#define APS512XX_HEAP_SIZE                  STM32U5X9J_HSPI1_PSRAM_HEAP_SIZE
#define APS512XX_DEVSIZE                    25u
#define APS512XX_RESET_CMD                  0xffu
#define APS512XX_READ_LINEAR_BURST_CMD      0x20u
#define APS512XX_WRITE_LINEAR_BURST_CMD     0xa0u
#define APS512XX_READ_REG_CMD               0x40u
#define APS512XX_WRITE_REG_CMD              0xc0u
#define APS512XX_MR0_ADDRESS                0x00000000u
#define APS512XX_MR4_ADDRESS                0x00000004u
#define APS512XX_MR8_ADDRESS                0x00000008u
#define APS512XX_MR0_DS_HALF                0x01u
#define APS512XX_MR0_RLC_3                  0x00u
#define APS512XX_MR0_FIXED_LATENCY          0x20u
#define APS512XX_MR4_WLC_3                  0x00u
#define APS512XX_MR4_RF_4X                  0x00u
#define APS512XX_MR4_PASR_FULL              0x00u
#define APS512XX_MR8_BL_16_BYTES            0x00u
#define APS512XX_MR8_LINEAR_BURST           0x00u
#define APS512XX_MR8_IO_X16                 0x40u
#define APS512XX_MR0_VALUE                  (APS512XX_MR0_FIXED_LATENCY | \
                                             APS512XX_MR0_RLC_3 | \
                                             APS512XX_MR0_DS_HALF)
#define APS512XX_MR4_VALUE                  (APS512XX_MR4_WLC_3 | \
                                             APS512XX_MR4_RF_4X | \
                                             APS512XX_MR4_PASR_FULL)
#define APS512XX_MR8_VALUE                  (APS512XX_MR8_BL_16_BYTES | \
                                             APS512XX_MR8_LINEAR_BURST | \
                                             APS512XX_MR8_IO_X16)
#define APS512XX_RESET_REG_LATENCY          5u
#define APS512XX_CONFIG_REG_LATENCY         3u
#define APS512XX_READ_LATENCY               6u
#define APS512XX_WRITE_LATENCY              3u

#if defined(CONFIG_STM32U5X9J_DK_OSPI_SCRATCH_TEST)
#  define HAVE_STM32U5X9J_OSPI_WRITE_CMDS   1
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#if defined(CONFIG_STM32U5X9J_DK_OSPI_DIAG) || \
    defined(CONFIG_STM32U5X9J_DK_HSPI_DIAG)
static ssize_t stm32_xspi_diag_copyout(FAR struct file *filep,
                                       FAR char *buffer, size_t buflen,
                                       FAR const char *text);
#endif

#ifdef CONFIG_STM32U5X9J_DK_OSPI_NOR
static int stm32_ospi_enter_memory_mapped(void);
#ifdef CONFIG_STM32U5X9J_DK_OSPI_DIAG
static ssize_t stm32_ospi_diag_read(FAR struct file *filep,
                                         FAR char *buffer, size_t buflen);
static ssize_t stm32_ospi_diag_write(FAR struct file *filep,
                                          FAR const char *buffer,
                                          size_t buflen);
static int stm32_ospi_diag_register(void);
#endif
#endif

#ifdef CONFIG_STM32U5X9J_DK_HSPI_DIAG
static ssize_t stm32_hspi_diag_read(FAR struct file *filep,
                                         FAR char *buffer, size_t buflen);
static ssize_t stm32_hspi_diag_write(FAR struct file *filep,
                                          FAR const char *buffer,
                                          size_t buflen);
static int stm32_hspi_diag_register(void);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_STM32U5X9J_DK_OSPI_NOR
static uint8_t g_ospi_jedec[3];
static bool g_ospi_nor_dtr;
static uint32_t g_ospi_dlyb_phase;
static uint32_t g_ospi_dlyb_units;

#ifdef CONFIG_STM32U5X9J_DK_OSPI_DIAG
static bool g_ospi_diag_registered;
static int g_ospi_last_scratch = -ENOTSUP;

static const struct file_operations g_ospi_diag_fops =
{
  NULL,                         /* open */
  NULL,                         /* close */
  stm32_ospi_diag_read,    /* read */
  stm32_ospi_diag_write,   /* write */
};
#endif
#endif

#ifdef CONFIG_STM32U5X9J_DK_HSPI_RAM
static bool g_hspi_ram_mapped;

#ifdef CONFIG_STM32U5X9J_DK_HSPI_DIAG
static bool g_hspi_diag_registered;
static int g_hspi_last_selftest = -ENODEV;

static const struct file_operations g_hspi_diag_fops =
{
  NULL,                         /* open */
  NULL,                         /* close */
  stm32_hspi_diag_read,    /* read */
  stm32_hspi_diag_write,   /* write */
};
#endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32_gpio_hslv_enable(uintptr_t base, uint32_t pinmask)
{
  modifyreg32(base + STM32_GPIO_HSLVR_OFFSET, 0, pinmask);
}

#if defined(CONFIG_STM32U5X9J_DK_OSPI_DIAG) || \
    defined(CONFIG_STM32U5X9J_DK_HSPI_DIAG)
static ssize_t stm32_xspi_diag_copyout(FAR struct file *filep,
                                       FAR char *buffer, size_t buflen,
                                       FAR const char *text)
{
  size_t len;
  size_t ncopy;

  if (buflen == 0)
    {
      return 0;
    }

  len = strlen(text);
  if (filep->f_pos >= len)
    {
      return 0;
    }

  ncopy = MIN(buflen, len - filep->f_pos);
  memcpy(buffer, &text[filep->f_pos], ncopy);
  filep->f_pos += ncopy;

  return ncopy;
}
#endif

#ifdef CONFIG_STM32U5X9J_DK_OSPI_NOR
static bool stm32_ospi_id_blank(FAR const uint8_t id[3])
{
  return (id[0] == 0x00 && id[1] == 0x00 && id[2] == 0x00) ||
         (id[0] == 0xff && id[1] == 0xff && id[2] == 0xff);
}

static void stm32_ospi_gpio_config(void)
{
  stm32_pwr_enableclk(true);
  modifyreg32(STM32_PWR_SVMCR, 0, PWR_SVMCR_IO2SV);

  stm32_gpio_hslv_enable(STM32_GPIOA_BASE,
                              STM32U5X9J_OSPI_HSLV_PORTA_MASK);
  stm32_gpio_hslv_enable(STM32_GPIOC_BASE,
                              STM32U5X9J_OSPI_HSLV_PORTC_MASK);
  stm32_gpio_hslv_enable(STM32_GPIOF_BASE,
                              STM32U5X9J_OSPI_HSLV_PORTF_MASK);

  stm32_configgpio(GPIO_OSPI1_CLK);
  stm32_configgpio(GPIO_OSPI1_DQS);
  stm32_configgpio(GPIO_OSPI1_NCS);
  stm32_configgpio(GPIO_OSPI1_IO0);
  stm32_configgpio(GPIO_OSPI1_IO1);
  stm32_configgpio(GPIO_OSPI1_IO2);
  stm32_configgpio(GPIO_OSPI1_IO3);
  stm32_configgpio(GPIO_OSPI1_IO4);
  stm32_configgpio(GPIO_OSPI1_IO5);
  stm32_configgpio(GPIO_OSPI1_IO6);
  stm32_configgpio(GPIO_OSPI1_IO7);
}

static int
stm32_ospi_configure(FAR const struct stm32_xspi_config_s *xspi,
                          FAR const struct stm32_xspim_config_s *xspim)
{
  int ret;

  ret = stm32_xspi_configure(xspi);
  if (ret < 0)
    {
      return ret;
    }

  return stm32_xspim_configure(xspim);
}

static int stm32_ospi_dlyb_configure(void)
{
  uint32_t phase;
  uint32_t units;
  int ret;

  ret = stm32_xspi_dlyb_configure(STM32_XSPI_PORT_OCTOSPI1,
                                  &phase, &units);
  if (ret < 0)
    {
      return ret;
    }

  g_ospi_dlyb_phase = phase;
  g_ospi_dlyb_units = units;

  syslog(LOG_INFO, "stm32u5x9j: OSPI1 DLYB phase=%" PRIu32
         " units=%" PRIu32 "\n", phase, units);
  return OK;
}

static int stm32_ospi_readid(FAR uint8_t id[3])
{
  return stm32_xspi_read_data(STM32_XSPI_PORT_OCTOSPI1,
                              MX25UM51245G_READ_ID_CMD, 0,
                              XSPI_CCR_IMODE_1_LINE |
                              XSPI_CCR_DMODE_1_LINE,
                              0, 3, id);
}

static uint32_t stm32_ospi_dtr_ccr(void)
{
  return XSPI_CCR_IMODE_8_LINES | XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16 |
         XSPI_CCR_ADMODE_8_LINES | XSPI_CCR_ADDTR |
         XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
         XSPI_CCR_DDTR | XSPI_CCR_DQSE;
}

static int stm32_ospi_read_cfg2_spi(uint32_t address,
                                         FAR uint8_t *value)
{
  return stm32_xspi_read_data(STM32_XSPI_PORT_OCTOSPI1,
                              MX25UM51245G_READ_CFG_REG2_CMD, address,
                              XSPI_CCR_IMODE_1_LINE |
                              XSPI_CCR_ADMODE_1_LINE |
                              XSPI_CCR_ADSIZE_32 |
                              XSPI_CCR_DMODE_1_LINE,
                              0, 1, value);
}

static int stm32_ospi_write_cfg2_spi(uint32_t address, uint8_t value)
{
  return stm32_xspi_write_data(STM32_XSPI_PORT_OCTOSPI1,
                               MX25UM51245G_WRITE_CFG_REG2_CMD, address,
                               XSPI_CCR_IMODE_1_LINE |
                               XSPI_CCR_ADMODE_1_LINE |
                               XSPI_CCR_ADSIZE_32 |
                               XSPI_CCR_DMODE_1_LINE,
                               0, value, 1, true);
}

static int stm32_ospi_read_cfg2_dtr(uint32_t address,
                                         FAR uint8_t *value)
{
  uint8_t buf[2];
  int ret;

  ret = stm32_xspi_read_data(STM32_XSPI_PORT_OCTOSPI1,
                             MX25UM51245G_OCTA_READ_CFG2_CMD, address,
                             stm32_ospi_dtr_ccr(),
                             XSPI_TCR_DHQC |
                             XSPI_TCR_DCYC(
                               MX25UM51245G_OCTA_DTR_REG_DUMMY),
                             sizeof(buf), buf);
  if (ret < 0)
    {
      return ret;
    }

  *value = buf[0];
  return OK;
}

static int stm32_ospi_read_status_spi(FAR uint8_t *status)
{
  return stm32_xspi_read_data(STM32_XSPI_PORT_OCTOSPI1,
                              MX25UM51245G_READ_STATUS_CMD, 0,
                              XSPI_CCR_IMODE_1_LINE |
                              XSPI_CCR_DMODE_1_LINE,
                              0, 1, status);
}

static int stm32_ospi_read_status_dtr(FAR uint8_t *status)
{
  uint8_t buf[2];
  int ret;

  ret = stm32_xspi_read_data(STM32_XSPI_PORT_OCTOSPI1,
                             MX25UM51245G_OCTA_READ_STATUS_CMD, 0,
                             stm32_ospi_dtr_ccr(),
                             XSPI_TCR_DHQC |
                             XSPI_TCR_DCYC(
                               MX25UM51245G_OCTA_DTR_REG_DUMMY),
                             sizeof(buf), buf);
  if (ret < 0)
    {
      return ret;
    }

  *status = buf[0];
  return OK;
}

static int stm32_ospi_wait_status(
  int (*read_status)(FAR uint8_t *status), uint8_t mask, uint8_t value)
{
  uint32_t remaining = MX25UM51245G_READY_TIMEOUT_MS;
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

      up_mdelay(1);
    }
  while (remaining-- > 0);

  return -ETIMEDOUT;
}

static int stm32_ospi_wait_ready_spi(void)
{
  return stm32_ospi_wait_status(stm32_ospi_read_status_spi,
                                     MX25UM51245G_STATUS_WIP, 0);
}

static int stm32_ospi_wait_ready_dtr(void)
{
  return stm32_ospi_wait_status(stm32_ospi_read_status_dtr,
                                     MX25UM51245G_STATUS_WIP, 0);
}

static void stm32_ospi_reset_opi(bool dtr)
{
  uint32_t ccr;
  int ret;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ISIZE_16;
  if (dtr)
    {
      ccr |= XSPI_CCR_IDTR | XSPI_CCR_DDTR;
    }

  ret = stm32_xspi_command_tcr(STM32_XSPI_PORT_OCTOSPI1,
                               MX25UM51245G_OCTA_RESET_ENABLE_CMD, ccr,
                               dtr ? XSPI_TCR_DHQC : 0);
  if (ret < 0)
    {
      return;
    }

  ret = stm32_xspi_command_tcr(STM32_XSPI_PORT_OCTOSPI1,
                               MX25UM51245G_OCTA_RESET_MEMORY_CMD, ccr,
                               dtr ? XSPI_TCR_DHQC : 0);
  if (ret >= 0)
    {
      up_mdelay(MX25UM51245G_RESET_DELAY_MS);
    }
}

static int stm32_ospi_reset_spi(void)
{
  int ret;

  ret = stm32_xspi_command(STM32_XSPI_PORT_OCTOSPI1,
                           MX25UM51245G_RESET_ENABLE_CMD,
                           XSPI_CCR_IMODE_1_LINE);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_xspi_command(STM32_XSPI_PORT_OCTOSPI1,
                           MX25UM51245G_RESET_MEMORY_CMD,
                           XSPI_CCR_IMODE_1_LINE);
  if (ret < 0)
    {
      return ret;
    }

  up_mdelay(MX25UM51245G_RESET_DELAY_MS);
  return OK;
}

static int stm32_ospi_write_enable(void)
{
  int ret;

  ret = stm32_xspi_command(STM32_XSPI_PORT_OCTOSPI1,
                           MX25UM51245G_WRITE_ENABLE_CMD,
                           XSPI_CCR_IMODE_1_LINE);
  if (ret < 0)
    {
      return ret;
    }

  return stm32_ospi_wait_status(stm32_ospi_read_status_spi,
                                     MX25UM51245G_STATUS_WEL,
                                     MX25UM51245G_STATUS_WEL);
}

static int
stm32_ospi_enter_dopi(FAR const struct stm32_xspi_config_s *config,
                           FAR const struct stm32_xspim_config_s *xspim)
{
  uint8_t cr2;
  int ret;

  ret = stm32_ospi_write_enable();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR SPI write-enable"
             " before dummy failed: %d\n", ret);
      return ret;
    }

  ret = stm32_ospi_write_cfg2_spi(MX25UM51245G_CR2_REG3_ADDR,
                                       MX25UM51245G_CR2_DC_6_CYCLES);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR SPI write CR2 dummy"
             " failed: %d\n", ret);
      return ret;
    }

  ret = stm32_ospi_wait_ready_spi();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR SPI wait-ready"
             " after CR2 dummy failed: %d\n", ret);
      return ret;
    }

  ret = stm32_ospi_read_cfg2_spi(MX25UM51245G_CR2_REG3_ADDR, &cr2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR SPI read CR2 dummy"
             " failed: %d\n", ret);
      return ret;
    }

  if ((cr2 & MX25UM51245G_CR2_DC_MASK) != MX25UM51245G_CR2_DC_6_CYCLES)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR dummy verify failed:"
             " cr2=%02x expected=%02x\n", cr2,
             MX25UM51245G_CR2_DC_6_CYCLES);
      return -EIO;
    }

  ret = stm32_ospi_write_enable();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR SPI write-enable"
             " before DOPI failed: %d\n", ret);
      return ret;
    }

  ret = stm32_ospi_write_cfg2_spi(MX25UM51245G_CR2_REG1_ADDR,
                                       MX25UM51245G_CR2_DOPI);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR SPI write CR2 DOPI"
             " failed: %d\n", ret);
      return ret;
    }

  up_mdelay(MX25UM51245G_WRITE_REG_DELAY_MS);

  ret = stm32_ospi_configure(config, xspim);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_ospi_dlyb_configure();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_ospi_wait_ready_dtr();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR DTR wait-ready"
             " failed: %d\n", ret);
      return ret;
    }

  ret = stm32_ospi_read_cfg2_dtr(MX25UM51245G_CR2_REG1_ADDR, &cr2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR DTR read CR2"
             " failed: %d\n", ret);
      return ret;
    }

  if (cr2 != MX25UM51245G_CR2_DOPI)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR DOPI verify failed:"
             " cr2=%02x expected=%02x\n", cr2, MX25UM51245G_CR2_DOPI);
      return -EIO;
    }

  syslog(LOG_INFO, "stm32u5x9j: OSPI1 NOR DOPI readback %02x\n", cr2);

  g_ospi_nor_dtr = true;
  return OK;
}

static int
stm32_ospi_switch_dopi_run(FAR const struct stm32_xspi_config_s *config,
                                FAR const struct stm32_xspim_config_s *xspim)
{
  uint8_t cr2;
  int ret;

  ret = stm32_ospi_configure(config, xspim);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR DTR run configure"
             " failed: %d\n", ret);
      return ret;
    }

  ret = stm32_ospi_dlyb_configure();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR DTR run DLYB"
             " failed: %d\n", ret);
      return ret;
    }

  ret = stm32_ospi_wait_ready_dtr();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR DTR run wait-ready"
             " failed: %d\n", ret);
      return ret;
    }

  ret = stm32_ospi_read_cfg2_dtr(MX25UM51245G_CR2_REG1_ADDR, &cr2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR DTR run read CR2"
             " failed: %d\n", ret);
      return ret;
    }

  if (cr2 != MX25UM51245G_CR2_DOPI)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR DOPI run verify failed:"
             " cr2=%02x expected=%02x\n", cr2, MX25UM51245G_CR2_DOPI);
      return -EIO;
    }

  syslog(LOG_INFO, "stm32u5x9j: OSPI1 NOR DOPI run readback %02x\n", cr2);

  return OK;
}

#ifdef HAVE_STM32U5X9J_OSPI_WRITE_CMDS
static int stm32_ospi_erase_subsector(uint32_t address)
{
  int ret;

  ret = stm32_ospi_write_enable();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_xspi_command_addr(STM32_XSPI_PORT_OCTOSPI1,
                                MX25UM51245G_SUBSECTOR_ERASE_4B_CMD,
                                address,
                                XSPI_CCR_IMODE_1_LINE |
                                XSPI_CCR_ADMODE_1_LINE |
                                XSPI_CCR_ADSIZE_32,
                                0);
  if (ret < 0)
    {
      return ret;
    }

  return stm32_ospi_wait_ready_spi();
}

static int stm32_ospi_page_program(uint32_t address,
                                        FAR const uint8_t *buffer,
                                        size_t nbytes)
{
  int ret;

  if (nbytes == 0 || nbytes > MX25UM51245G_PAGE_SIZE)
    {
      return -EINVAL;
    }

  ret = stm32_ospi_write_enable();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_xspi_write_buffer(STM32_XSPI_PORT_OCTOSPI1,
                                MX25UM51245G_PAGE_PROGRAM_4B_CMD,
                                address,
                                XSPI_CCR_IMODE_1_LINE |
                                XSPI_CCR_ADMODE_1_LINE |
                                XSPI_CCR_ADSIZE_32 |
                                XSPI_CCR_DMODE_1_LINE,
                                0, nbytes, buffer);
  if (ret < 0)
    {
      return ret;
    }

  return stm32_ospi_wait_ready_spi();
}
#endif

static int stm32_ospi_enter_memory_mapped(void)
{
  uint32_t ccr;

  if (g_ospi_nor_dtr)
    {
      ccr = stm32_ospi_dtr_ccr();

      return stm32_xspi_enter_memory_mapped_rw(STM32_XSPI_PORT_OCTOSPI1,
                                               ccr,
                                               XSPI_TCR_DHQC |
                                               XSPI_TCR_DCYC(
                                                 MX25UM51245G_OCTA_DTR_READ_DUMMY),
                                               MX25UM51245G_OCTA_READ_DTR_CMD,
                                               ccr,
                                               XSPI_TCR_DHQC,
                                               MX25UM51245G_OCTA_PAGE_PROG_CMD);
    }

  ccr = XSPI_CCR_IMODE_1_LINE | XSPI_CCR_ADMODE_1_LINE |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_1_LINE;

  return stm32_xspi_enter_memory_mapped(STM32_XSPI_PORT_OCTOSPI1, ccr,
                                        XSPI_TCR_DCYC(
                                          MX25UM51245G_FAST_READ_DUMMY),
                                        MX25UM51245G_FAST_READ_4B_CMD);
}

#ifdef CONFIG_STM32U5X9J_DK_OSPI_DIAG
static int stm32_ospi_suspend(void)
{
  if (g_ospi_nor_dtr)
    {
      return stm32_xspi_command_tcr(STM32_XSPI_PORT_OCTOSPI1,
                                    MX25UM51245G_OCTA_SUSPEND_CMD,
                                    XSPI_CCR_IMODE_8_LINES |
                                    XSPI_CCR_IDTR |
                                    XSPI_CCR_ISIZE_16 |
                                    XSPI_CCR_DDTR,
                                    XSPI_TCR_DHQC);
    }

  return stm32_xspi_command(STM32_XSPI_PORT_OCTOSPI1,
                            MX25UM51245G_SUSPEND_CMD,
                            XSPI_CCR_IMODE_1_LINE);
}

static int stm32_ospi_resume(void)
{
  if (g_ospi_nor_dtr)
    {
      return stm32_xspi_command_tcr(STM32_XSPI_PORT_OCTOSPI1,
                                    MX25UM51245G_OCTA_RESUME_CMD,
                                    XSPI_CCR_IMODE_8_LINES |
                                    XSPI_CCR_IDTR |
                                    XSPI_CCR_ISIZE_16 |
                                    XSPI_CCR_DDTR,
                                    XSPI_TCR_DHQC);
    }

  return stm32_xspi_command(STM32_XSPI_PORT_OCTOSPI1,
                            MX25UM51245G_RESUME_CMD,
                            XSPI_CCR_IMODE_1_LINE);
}

#ifdef CONFIG_STM32U5X9J_DK_OSPI_SCRATCH_TEST
static int stm32_ospi_scratch_test(void)
{
  uint8_t pattern[MX25UM51245G_PAGE_SIZE];
  uint32_t offset = CONFIG_STM32U5X9J_DK_OSPI_SCRATCH_OFFSET;
  FAR const uint8_t *xip;
  size_t i;
  int ret;

  if (g_ospi_nor_dtr)
    {
      return -ENOTSUP;
    }

  if ((offset & (MX25UM51245G_SUBSECTOR_SIZE - 1)) != 0 ||
      offset + MX25UM51245G_SUBSECTOR_SIZE > MX25UM51245G_FLASH_SIZE)
    {
      return -EINVAL;
    }

  for (i = 0; i < sizeof(pattern); i++)
    {
      pattern[i] = (uint8_t)(0xa5u ^ i);
    }

  ret = stm32_xspi_abort(STM32_XSPI_PORT_OCTOSPI1);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_ospi_erase_subsector(offset);
  if (ret < 0)
    {
      (void)stm32_ospi_enter_memory_mapped();
      return ret;
    }

  ret = stm32_ospi_page_program(offset, pattern, sizeof(pattern));
  if (ret < 0)
    {
      (void)stm32_ospi_enter_memory_mapped();
      return ret;
    }

  ret = stm32_ospi_enter_memory_mapped();
  if (ret < 0)
    {
      return ret;
    }

  xip = (FAR const uint8_t *)(STM32U5X9J_OSPI1_NOR_MEM_BASE + offset);
  for (i = 0; i < sizeof(pattern); i++)
    {
      if (xip[i] != pattern[i])
        {
          return -EIO;
        }
    }

  return OK;
}
#endif

static ssize_t stm32_ospi_diag_read(FAR struct file *filep,
                                         FAR char *buffer, size_t buflen)
{
  FAR const volatile uint32_t *xip =
    (FAR const volatile uint32_t *)STM32U5X9J_OSPI1_NOR_MEM_BASE;
  char text[512];
  int len;

  len = snprintf(text, sizeof(text),
                 "mx25um51245g size=%u erase64k=%u erase4k=%u page=%u\n"
                 "jedec=%02x %02x %02x manufacturer_ok=%u\n"
                 "xip_base=0x%08x xip_first=0x%08" PRIx32 " mode=%s\n"
#ifdef CONFIG_STM32U5X9J_DK_OSPI_MTD
                 "mtd=/dev/mtd0 mode=read-only filesystem=none\n"
#else
                 "mtd=disabled filesystem=none\n"
#endif
                 "suspend_resume=spi scratch=%s last_scratch=%d\n",
                 MX25UM51245G_FLASH_SIZE, MX25UM51245G_ERASE_SIZE,
                 MX25UM51245G_SUBSECTOR_SIZE, MX25UM51245G_PAGE_SIZE,
                 g_ospi_jedec[0], g_ospi_jedec[1], g_ospi_jedec[2],
                 g_ospi_jedec[0] == MX25UM51245G_MANUFACTURER_ID,
                 STM32U5X9J_OSPI1_NOR_MEM_BASE, *xip,
                 g_ospi_nor_dtr ? "opi-dtr" : "spi-str",
#ifdef CONFIG_STM32U5X9J_DK_OSPI_SCRATCH_TEST
                 "enabled",
#else
                 "disabled",
#endif
                 g_ospi_last_scratch);

  if (len < 0)
    {
      return len;
    }

  return stm32_xspi_diag_copyout(filep, buffer, buflen, text);
}

static ssize_t stm32_ospi_diag_write(FAR struct file *filep,
                                          FAR const char *buffer,
                                          size_t buflen)
{
  char cmd[16];
  size_t ncopy;
  int ret;

  ncopy = MIN(buflen, sizeof(cmd) - 1);
  memcpy(cmd, buffer, ncopy);
  cmd[ncopy] = '\0';

  if (strncmp(cmd, "suspend", 7) == 0)
    {
      ret = stm32_xspi_abort(STM32_XSPI_PORT_OCTOSPI1);
      if (ret < 0)
        {
          return ret;
        }

      ret = stm32_ospi_suspend();
      (void)stm32_ospi_enter_memory_mapped();
      return ret < 0 ? ret : (ssize_t)buflen;
    }

  if (strncmp(cmd, "resume", 6) == 0)
    {
      ret = stm32_xspi_abort(STM32_XSPI_PORT_OCTOSPI1);
      if (ret < 0)
        {
          return ret;
        }

      ret = stm32_ospi_resume();
      (void)stm32_ospi_enter_memory_mapped();
      return ret < 0 ? ret : (ssize_t)buflen;
    }

  if (strncmp(cmd, "scratch", 7) == 0)
    {
#ifdef CONFIG_STM32U5X9J_DK_OSPI_SCRATCH_TEST
      ret = stm32_ospi_scratch_test();
      g_ospi_last_scratch = ret;
      return ret < 0 ? ret : (ssize_t)buflen;
#else
      g_ospi_last_scratch = -ENOTSUP;
      return -ENOTSUP;
#endif
    }

  return -EINVAL;
}

static int stm32_ospi_diag_register(void)
{
  int ret;

  if (g_ospi_diag_registered)
    {
      return OK;
    }

  ret = register_driver(STM32U5X9J_OSPI_DIAG_PATH, &g_ospi_diag_fops,
                        0666, NULL);
  if (ret < 0)
    {
      return ret;
    }

  g_ospi_diag_registered = true;
  syslog(LOG_INFO, "stm32u5x9j: OSPI1 NOR diagnostic at %s\n",
         STM32U5X9J_OSPI_DIAG_PATH);

  return OK;
}
#endif /* CONFIG_STM32U5X9J_DK_OSPI_DIAG */
#endif /* CONFIG_STM32U5X9J_DK_OSPI_NOR */

#ifdef CONFIG_STM32U5X9J_DK_HSPI_RAM
static uint32_t stm32_psram_refresh(uint32_t prescaler)
{
  uint32_t hspi_hz;
  uint32_t cycles;

  hspi_hz = STM32_SYSCLK_FREQUENCY / (prescaler + 1u);
  cycles = 2u * (hspi_hz / 1000000u);

  return cycles > 4u ? cycles - 4u : 0u;
}

static void stm32_hspi_gpio_config(void)
{
  stm32_pwr_enableclk(true);
  modifyreg32(STM32_PWR_SVMCR, 0, PWR_SVMCR_IO2SV);

  stm32_gpio_hslv_enable(STM32_GPIOH_BASE,
                              STM32U5X9J_HSPI_HSLV_PORTH_MASK);
  stm32_gpio_hslv_enable(STM32_GPIOI_BASE,
                              STM32U5X9J_HSPI_HSLV_PORTI_MASK);
  stm32_gpio_hslv_enable(STM32_GPIOJ_BASE,
                              STM32U5X9J_HSPI_HSLV_PORTJ_MASK);

  stm32_configgpio(GPIO_HSPI1_CLK);
  stm32_configgpio(GPIO_HSPI1_NCLK);
  stm32_configgpio(GPIO_HSPI1_DQS0);
  stm32_configgpio(GPIO_HSPI1_DQS1);
  stm32_configgpio(GPIO_HSPI1_NCS);
  stm32_configgpio(GPIO_HSPI1_D0);
  stm32_configgpio(GPIO_HSPI1_D1);
  stm32_configgpio(GPIO_HSPI1_D2);
  stm32_configgpio(GPIO_HSPI1_D3);
  stm32_configgpio(GPIO_HSPI1_D4);
  stm32_configgpio(GPIO_HSPI1_D5);
  stm32_configgpio(GPIO_HSPI1_D6);
  stm32_configgpio(GPIO_HSPI1_D7);
  stm32_configgpio(GPIO_HSPI1_D8);
  stm32_configgpio(GPIO_HSPI1_D9);
  stm32_configgpio(GPIO_HSPI1_D10);
  stm32_configgpio(GPIO_HSPI1_D11);
  stm32_configgpio(GPIO_HSPI1_D12);
  stm32_configgpio(GPIO_HSPI1_D13);
  stm32_configgpio(GPIO_HSPI1_D14);
  stm32_configgpio(GPIO_HSPI1_D15);
}

static int stm32_psram_write_reg(uint32_t address, uint8_t value)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR;

  return stm32_xspi_write_data(STM32_XSPI_PORT_HSPI1,
                               APS512XX_WRITE_REG_CMD, address,
                               ccr, 0, value, 2, true);
}

static int stm32_psram_read_reg(uint32_t address, FAR uint8_t *value,
                                     uint32_t latency)
{
  uint8_t buf[2];
  uint32_t ccr;
  int ret;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  ret = stm32_xspi_read_data(STM32_XSPI_PORT_HSPI1,
                             APS512XX_READ_REG_CMD, address,
                             ccr, XSPI_TCR_DCYC(latency - 1),
                             sizeof(buf), buf);
  if (ret < 0)
    {
      return ret;
    }

  *value = buf[0];
  return OK;
}

static int stm32_psram_config_reg(uint32_t address, uint8_t value,
                                       uint8_t mask, uint32_t old_latency,
                                       uint32_t new_latency)
{
  uint8_t initial;
  uint8_t readback;
  uint8_t writeval;
  int ret;

  ret = stm32_psram_read_reg(address, &initial, old_latency);
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

  ret = stm32_psram_read_reg(address, &readback, new_latency);
  if (ret < 0)
    {
      return ret;
    }

  if ((readback & mask) != (value & mask))
    {
      syslog(LOG_ERR, "stm32u5x9j: HSPI1 PSRAM MR%08" PRIx32
             " initial %02x write %02x readback %02x expected %02x\n",
             address, initial, writeval, readback, value);
      return -EIO;
    }

  syslog(LOG_INFO, "stm32u5x9j: HSPI1 PSRAM MR%08" PRIx32
         " initial %02x write %02x readback %02x\n",
         address, initial, writeval, readback);
  return OK;
}

static int stm32_psram_enter_memory_mapped(void)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_16_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  return stm32_xspi_enter_memory_mapped_rw(STM32_XSPI_PORT_HSPI1,
                                           ccr,
                                           XSPI_TCR_DCYC(
                                             APS512XX_READ_LATENCY - 1),
                                           APS512XX_READ_LINEAR_BURST_CMD,
                                           ccr,
                                           XSPI_TCR_DCYC(
                                             APS512XX_WRITE_LATENCY - 1),
                                           APS512XX_WRITE_LINEAR_BURST_CMD);
}

static int stm32_psram_selftest(void)
{
  volatile uint32_t *mem =
    (volatile uint32_t *)STM32U5X9J_HSPI1_PSRAM_MEM_BASE;
  uint32_t saved[8];
  uint32_t pattern[8] =
    {
      0x55aa55aau, 0xaa55aa55u, 0x01234567u, 0x89abcdefu,
      0xf0f00f0fu, 0x0f0ff0f0u, 0x13579bdfu, 0x2468ace0u
    };

  int i;

  for (i = 0; i < nitems(saved); i++)
    {
      saved[i] = mem[i];
    }

  for (i = 0; i < nitems(pattern); i++)
    {
      mem[i] = pattern[i];
    }

  for (i = 0; i < nitems(pattern); i++)
    {
      if (mem[i] != pattern[i])
        {
          while (i-- > 0)
            {
              mem[i] = saved[i];
            }

          return -EIO;
        }
    }

  for (i = 0; i < nitems(saved); i++)
    {
      mem[i] = saved[i];
    }

  syslog(LOG_INFO, "stm32u5x9j: HSPI1 PSRAM self-test passed\n");
  return OK;
}

#ifdef CONFIG_STM32U5X9J_DK_HSPI_DIAG
static ssize_t stm32_hspi_diag_read(FAR struct file *filep,
                                         FAR char *buffer, size_t buflen)
{
  char text[384];
  int len;

  len = snprintf(text, sizeof(text),
                 "aps512xx base=0x%08x size=%u mapped=%u\n"
                 "framebuffer base=0x%08x size=%u\n"
                 "heap base=0x%08x size=%u enabled=%u\n"
                 "selftest=save-write-read-restore-32B last=%d\n",
                 STM32U5X9J_HSPI1_PSRAM_MEM_BASE, APS512XX_RAM_SIZE,
                 stm32_hspi1_psram_is_mapped(),
                 STM32U5X9J_HSPI1_PSRAM_MEM_BASE, APS512XX_FB_RESERVED,
                 APS512XX_HEAP_BASE, APS512XX_HEAP_SIZE,
#ifdef CONFIG_STM32U5X9J_DK_HSPI_HEAP
                 1,
#else
                 0,
#endif
                 g_hspi_last_selftest);

  if (len < 0)
    {
      return len;
    }

  return stm32_xspi_diag_copyout(filep, buffer, buflen, text);
}

static ssize_t stm32_hspi_diag_write(FAR struct file *filep,
                                          FAR const char *buffer,
                                          size_t buflen)
{
  char cmd[16];
  size_t ncopy;
  int ret;

  ncopy = MIN(buflen, sizeof(cmd) - 1);
  memcpy(cmd, buffer, ncopy);
  cmd[ncopy] = '\0';

  if (strncmp(cmd, "selftest", 8) != 0)
    {
      return -EINVAL;
    }

  ret = stm32_hspi1_psram_initialize();
  if (ret == OK)
    {
      ret = stm32_psram_selftest();
    }

  g_hspi_last_selftest = ret;
  return ret < 0 ? ret : (ssize_t)buflen;
}

static int stm32_hspi_diag_register(void)
{
  int ret;

  if (g_hspi_diag_registered)
    {
      return OK;
    }

  ret = register_driver(STM32U5X9J_HSPI_DIAG_PATH, &g_hspi_diag_fops,
                        0666, NULL);
  if (ret < 0)
    {
      return ret;
    }

  g_hspi_diag_registered = true;
  syslog(LOG_INFO, "stm32u5x9j: HSPI1 PSRAM diagnostic at %s\n",
         STM32U5X9J_HSPI_DIAG_PATH);

  return OK;
}
#endif /* CONFIG_STM32U5X9J_DK_HSPI_DIAG */
#endif /* CONFIG_STM32U5X9J_DK_HSPI_RAM */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_STM32U5X9J_DK_OSPI_NOR
int stm32_ospi1_nor_initialize(void)
{
  const struct stm32_xspi_config_s spi_init_config =
    {
      .port           = STM32_XSPI_PORT_OCTOSPI1,
      .memory_type    = XSPI_DCR1_MTYP_MICRON,
      .device_size    = MX25UM51245G_DEVSIZE,
      .csht           = STM32U5X9J_OSPI_CSHT,
      .prescaler      = STM32U5X9J_OSPI_INIT_PRESCALER,
      .fifo_threshold = STM32U5X9J_OSPI_FIFO_THRESHOLD,
      .maxtran        = 0,
      .csbound        = 0,
      .refresh        = 0,
    };

  const struct stm32_xspi_config_s spi_run_config =
    {
      .port           = STM32_XSPI_PORT_OCTOSPI1,
      .memory_type    = XSPI_DCR1_MTYP_MICRON,
      .device_size    = MX25UM51245G_DEVSIZE,
      .csht           = STM32U5X9J_OSPI_CSHT,
      .prescaler      = STM32U5X9J_OSPI_RUN_PRESCALER,
      .fifo_threshold = STM32U5X9J_OSPI_FIFO_THRESHOLD,
      .maxtran        = 0,
      .csbound        = 0,
      .refresh        = 0,
    };

  const struct stm32_xspi_config_s dtr_init_config =
    {
      .port           = STM32_XSPI_PORT_OCTOSPI1,
      .memory_type    = XSPI_DCR1_MTYP_MACRONIX,
      .device_size    = MX25UM51245G_DEVSIZE,
      .csht           = STM32U5X9J_OSPI_CSHT,
      .prescaler      = STM32U5X9J_OSPI_INIT_PRESCALER,
      .fifo_threshold = STM32U5X9J_OSPI_FIFO_THRESHOLD,
      .maxtran        = 0,
      .csbound        = 0,
      .refresh        = 0,
    };

  const struct stm32_xspi_config_s dtr_run_config =
    {
      .port           = STM32_XSPI_PORT_OCTOSPI1,
      .memory_type    = XSPI_DCR1_MTYP_MACRONIX,
      .device_size    = MX25UM51245G_DEVSIZE,
      .csht           = STM32U5X9J_OSPI_CSHT,
      .prescaler      = STM32U5X9J_OSPI_RUN_PRESCALER,
      .fifo_threshold = STM32U5X9J_OSPI_FIFO_THRESHOLD,
      .maxtran        = 0,
      .csbound        = 0,
      .refresh        = 0,
    };

  const struct stm32_xspim_config_s xspim_config =
    {
      .port         = STM32_XSPI_PORT_OCTOSPI1,
      .clk_port     = STM32U5X9J_OSPI_XSPIM_PORT,
      .dqs_port     = STM32U5X9J_OSPI_XSPIM_PORT,
      .ncs_port     = STM32U5X9J_OSPI_XSPIM_PORT,
      .iolow_port   = STM32U5X9J_OSPI_XSPIM_PORT,
      .iohigh_port  = STM32U5X9J_OSPI_XSPIM_PORT,
      .req2ack_time = STM32U5X9J_OSPI_XSPIM_REQ2ACK,
    };

  uint8_t id[3] =
    {
      0
    };

  uint32_t first;
  int ret;

  if (stm32_xspi_is_mapped(STM32_XSPI_PORT_OCTOSPI1))
    {
      syslog(LOG_INFO, "stm32u5x9j: OSPI1 NOR already memory-mapped\n");
      return OK;
    }

  ret = stm32_xspi_common_setup();
  if (ret < 0)
    {
      return ret;
    }

  g_ospi_nor_dtr = false;
  stm32_ospi_gpio_config();

  syslog(LOG_INFO, "stm32u5x9j: OSPI1 NOR init prescaler=%u"
         " run prescaler=%u\n", STM32U5X9J_OSPI_INIT_PRESCALER,
         STM32U5X9J_OSPI_RUN_PRESCALER);

  ret = stm32_ospi_configure(&spi_init_config, &xspim_config);
  if (ret < 0)
    {
      return ret;
    }

  stm32_ospi_reset_opi(false);

  ret = stm32_ospi_configure(&dtr_init_config, &xspim_config);
  if (ret == OK)
    {
      stm32_ospi_reset_opi(true);
    }

  ret = stm32_ospi_configure(&spi_init_config, &xspim_config);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_ospi_reset_spi();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR reset failed: %d\n", ret);
      return ret;
    }

  ret = stm32_ospi_readid(id);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR read-id failed: %d\n", ret);
      return ret;
    }

  syslog(LOG_INFO, "stm32u5x9j: OSPI1 NOR JEDEC ID %02x %02x %02x\n",
         id[0], id[1], id[2]);

  memcpy(g_ospi_jedec, id, sizeof(g_ospi_jedec));

  if (stm32_ospi_id_blank(id))
    {
      return -EIO;
    }

  ret = stm32_ospi_enter_dopi(&dtr_init_config, &xspim_config);

  if (ret >= 0)
    {
      ret = stm32_ospi_switch_dopi_run(&dtr_run_config,
                                            &xspim_config);
    }

  if (ret < 0)
    {
      syslog(LOG_WARNING, "stm32u5x9j: OSPI1 NOR DTR setup failed: %d,"
             " falling back to SPI STR\n", ret);

      g_ospi_nor_dtr = false;

      (void)stm32_ospi_configure(&dtr_init_config, &xspim_config);
      stm32_ospi_reset_opi(true);

      ret = stm32_ospi_configure(&spi_init_config, &xspim_config);
      if (ret < 0)
        {
          return ret;
        }

      ret = stm32_ospi_reset_spi();
      if (ret < 0)
        {
          return ret;
        }

      ret = stm32_ospi_readid(g_ospi_jedec);
      if (ret < 0)
        {
          return ret;
        }

      ret = stm32_ospi_configure(&spi_run_config, &xspim_config);
      if (ret < 0)
        {
          return ret;
        }
    }
  else
    {
      syslog(LOG_INFO, "stm32u5x9j: OSPI1 NOR DTR OPI enabled\n");
    }

  ret = stm32_ospi_enter_memory_mapped();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR map failed: %d\n", ret);
      return ret;
    }

  first = *(volatile uint32_t *)STM32U5X9J_OSPI1_NOR_MEM_BASE;
  syslog(LOG_INFO, "stm32u5x9j: OSPI1 NOR %s mapped at 0x%08x"
         " first=0x%08" PRIx32 "\n", g_ospi_nor_dtr ? "DTR" : "STR",
         STM32U5X9J_OSPI1_NOR_MEM_BASE, first);
  return OK;
}

#ifdef CONFIG_STM32U5X9J_DK_OSPI_DIAG
int stm32_ospi_diag_initialize(void)
{
  int ret;

  ret = stm32_ospi1_nor_initialize();
  if (ret < 0)
    {
      return ret;
    }

  return stm32_ospi_diag_register();
}
#endif
#endif /* CONFIG_STM32U5X9J_DK_OSPI_NOR */

#ifdef CONFIG_STM32U5X9J_DK_HSPI_RAM
int stm32_hspi1_psram_initialize(void)
{
  const struct stm32_xspi_config_s init_config =
    {
      .port           = STM32_XSPI_PORT_HSPI1,
      .memory_type    = XSPI_DCR1_MTYP_APMEM_16BIT,
      .device_size    = APS512XX_DEVSIZE,
      .csht           = STM32U5X9J_HSPI_CSHT,
      .prescaler      = STM32U5X9J_HSPI_INIT_PRESCALER,
      .fifo_threshold = STM32U5X9J_HSPI_FIFO_THRESHOLD,
      .maxtran        = STM32U5X9J_HSPI_MAXTRAN,
      .csbound        = STM32U5X9J_HSPI_CSBOUND,
      .refresh        = stm32_psram_refresh(
                          STM32U5X9J_HSPI_INIT_PRESCALER),
    };

  const struct stm32_xspi_config_s run_config =
    {
      .port           = STM32_XSPI_PORT_HSPI1,
      .memory_type    = XSPI_DCR1_MTYP_APMEM_16BIT,
      .device_size    = APS512XX_DEVSIZE,
      .csht           = STM32U5X9J_HSPI_CSHT,
      .prescaler      = STM32U5X9J_HSPI_RUN_PRESCALER,
      .fifo_threshold = STM32U5X9J_HSPI_FIFO_THRESHOLD,
      .maxtran        = STM32U5X9J_HSPI_MAXTRAN,
      .csbound        = STM32U5X9J_HSPI_CSBOUND,
      .refresh        = stm32_psram_refresh(
                          STM32U5X9J_HSPI_RUN_PRESCALER),
    };

  int ret;

  if (stm32_xspi_is_mapped(STM32_XSPI_PORT_HSPI1))
    {
      g_hspi_ram_mapped = true;
      return OK;
    }

  g_hspi_ram_mapped = false;

  syslog(LOG_INFO, "stm32u5x9j: HSPI1 PSRAM init prescaler=%u"
         " run prescaler=%u\n", STM32U5X9J_HSPI_INIT_PRESCALER,
         STM32U5X9J_HSPI_RUN_PRESCALER);

  ret = stm32_xspi_common_setup();
  if (ret < 0)
    {
      return ret;
    }

  stm32_hspi_gpio_config();
  ret = stm32_xspi_configure(&init_config);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_xspi_command_addr(STM32_XSPI_PORT_HSPI1,
                                APS512XX_RESET_CMD, 0,
                                XSPI_CCR_IMODE_8_LINES |
                                XSPI_CCR_ADMODE_8_LINES |
                                XSPI_CCR_ADSIZE_24,
                                0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: HSPI1 PSRAM reset failed: %d\n", ret);
      return ret;
    }

  up_mdelay(1);

  ret = stm32_psram_config_reg(APS512XX_MR0_ADDRESS,
                                    APS512XX_MR0_VALUE, 0x3fu,
                                    APS512XX_RESET_REG_LATENCY,
                                    APS512XX_CONFIG_REG_LATENCY);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_psram_config_reg(APS512XX_MR4_ADDRESS,
                                    APS512XX_MR4_VALUE, 0xffu,
                                    APS512XX_CONFIG_REG_LATENCY,
                                    APS512XX_CONFIG_REG_LATENCY);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_psram_config_reg(APS512XX_MR8_ADDRESS,
                                    APS512XX_MR8_VALUE, 0x47u,
                                    APS512XX_CONFIG_REG_LATENCY,
                                    APS512XX_CONFIG_REG_LATENCY);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_xspi_configure(&run_config);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_psram_enter_memory_mapped();
  if (ret < 0)
    {
      return ret;
    }

  g_hspi_ram_mapped = true;

  ret = stm32_psram_selftest();
#ifdef CONFIG_STM32U5X9J_DK_HSPI_DIAG
  g_hspi_last_selftest = ret;
#endif
  if (ret < 0)
    {
      g_hspi_ram_mapped = false;
      return ret;
    }

  syslog(LOG_INFO, "stm32u5x9j: HSPI1 PSRAM mapped at 0x%08x\n",
         STM32U5X9J_HSPI1_PSRAM_MEM_BASE);
  return OK;
}

bool stm32_hspi1_psram_is_mapped(void)
{
  return g_hspi_ram_mapped &&
         stm32_xspi_is_mapped(STM32_XSPI_PORT_HSPI1);
}

#ifdef CONFIG_STM32U5X9J_DK_HSPI_DIAG
int stm32_hspi_diag_initialize(void)
{
  int ret;

  ret = stm32_hspi1_psram_initialize();
  if (ret < 0)
    {
      return ret;
    }

  return stm32_hspi_diag_register();
}
#endif
#endif /* CONFIG_STM32U5X9J_DK_HSPI_RAM */

#endif /* OSPI NOR || HSPI RAM */
