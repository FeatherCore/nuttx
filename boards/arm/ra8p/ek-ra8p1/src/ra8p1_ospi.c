/****************************************************************************
 * boards/arm/ra8p/ek-ra8p1/src/ra8p1_ospi.c
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

#include "hardware/ra8p_mstp.h"
#include "hardware/ra8p_ospi.h"

#include "ek-ra8p1.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RA8P1_OSPI_DIRECT_TIMEOUT        1000000
#define RA8P1_OSPI_RESET_DELAY           20000

#define RA8P1_OSPI_PROTOCOL_SPI          0x000
#define RA8P1_OSPI_PROTOCOL_8D_8D_8D     0x3ff

#define RA8P1_OSPI_FRAME_STANDARD        0
#define RA8P1_OSPI_FRAME_XSPI_PROFILE1   1
#define RA8P1_OSPI_ADDRESS_BYTES_4       3
#define RA8P1_OSPI_ADDRESS_REPLACE       0xf0
#define RA8P1_OSPI_COMMAND_BYTES_1       1
#define RA8P1_OSPI_COMMAND_BYTES_2       2

#define RA8P1_OSPI_CSMIN_CLOCKS_2        1
#define RA8P1_OSPI_DATA_LATCH_DELAY      16
#define RA8P1_OSPI_COMBINATION_64BYTE    0x1f

#define RA8P1_OSPI_CMD_WRITE_ENABLE_SPI  0x06
#define RA8P1_OSPI_CMD_READ_STATUS_SPI   0x05
#define RA8P1_OSPI_CMD_READ_ID_SPI       0x9f
#define RA8P1_OSPI_CMD_READ_CFG2_SPI     0x71
#define RA8P1_OSPI_CMD_WRITE_CFG2_SPI    0x72

#define RA8P1_OSPI_CMD_WRITE_ENABLE_OPI  0x06f9
#define RA8P1_OSPI_CMD_READ_STATUS_OPI   0x05fa

#define RA8P1_OSPI_CFG2_ADDR_MODE        0x000
#define RA8P1_OSPI_CFG2_ADDR_DQS         0x200
#define RA8P1_OSPI_CFG2_ADDR_DUMMY       0x300
#define RA8P1_OSPI_CFG2_SPI_MODE         0x00
#define RA8P1_OSPI_CFG2_DQS_STR          0x02
#define RA8P1_OSPI_CFG2_DUMMY_6          0x07
#define RA8P1_OSPI_CFG2_DTR_MODE         0x02

#define RA8P1_OSPI_STATUS_WEL            (1u << 1)
#define RA8P1_OSPI_DEVICE_ID             0x3a86c2

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ra8p1_ospi_command_set_s
{
  uint16_t read_command;
  uint16_t program_command;
  uint8_t  command_bytes;
  uint8_t  address_bytes;
  uint8_t  frame_format;
  uint8_t  read_dummy_cycles;
  uint8_t  program_dummy_cycles;
  uint16_t protocol;
};

struct ra8p1_ospi_transfer_s
{
  uint16_t command;
  uint32_t address;
  uint64_t data;
  uint8_t  command_length;
  uint8_t  address_length;
  uint8_t  data_length;
  uint8_t  dummy_cycles;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_ospi_initialized;

#ifdef CONFIG_RA8P_OSPI_B
static const struct ra8p1_ospi_command_set_s g_ospi_spi_command_set =
{
  .read_command         = 0x13,
  .program_command      = 0x12,
  .command_bytes        = RA8P1_OSPI_COMMAND_BYTES_1,
  .address_bytes        = RA8P1_OSPI_ADDRESS_BYTES_4,
  .frame_format         = RA8P1_OSPI_FRAME_STANDARD,
  .read_dummy_cycles    = 0,
  .program_dummy_cycles = 0,
  .protocol             = RA8P1_OSPI_PROTOCOL_SPI
};

static const struct ra8p1_ospi_command_set_s g_ospi_opi_command_set =
{
  .read_command         = 0xee11,
  .program_command      = 0x12ed,
  .command_bytes        = RA8P1_OSPI_COMMAND_BYTES_2,
  .address_bytes        = RA8P1_OSPI_ADDRESS_BYTES_4,
  .frame_format         = RA8P1_OSPI_FRAME_XSPI_PROFILE1,
  .read_dummy_cycles    = 6,
  .program_dummy_cycles = 0,
  .protocol             = RA8P1_OSPI_PROTOCOL_8D_8D_8D
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_RA8P_OSPI_B
static uint32_t ra8p_getreg32(uintptr_t addr)
{
  return *(volatile uint32_t *)addr;
}

static void ra8p_putreg32(uint32_t value, uintptr_t addr)
{
  *(volatile uint32_t *)addr = value;
}

static void ra8p_modifyreg32(uintptr_t addr, uint32_t clearbits,
                             uint32_t setbits)
{
  uint32_t value = ra8p_getreg32(addr);

  value &= ~clearbits;
  value |= setbits;
  ra8p_putreg32(value, addr);
}

static void ra8p1_ospi_delay(void)
{
  volatile unsigned int count;

  for (count = 0; count < RA8P1_OSPI_RESET_DELAY; count++)
    {
      __asm__ volatile ("nop");
    }
}

static int ra8p1_ospi_wait_clear(uintptr_t addr, uint32_t mask,
                                 const char *step)
{
  int count;

  for (count = 0; count < RA8P1_OSPI_DIRECT_TIMEOUT; count++)
    {
      if ((ra8p_getreg32(addr) & mask) == 0)
        {
          return 0;
        }
    }

  syslog(LOG_ERR,
         "ra8p1: OSPI timeout at %s reg=0x%08" PRIx32
         " mask=0x%08" PRIx32 "\n",
         step, ra8p_getreg32(addr), mask);
  return -ETIMEDOUT;
}

static bool ra8p1_ospi_running_from_nor(void)
{
  uintptr_t pc = (uintptr_t)__builtin_return_address(0);

  return pc >= RA8P_OSPI0_CS1_START &&
         pc < (RA8P_OSPI0_CS1_START + RA8P_OSPI_NOR_SIZE);
}

static void ra8p1_ospi_module_start(void)
{
  ra8p_modifyreg32(RA8P_MSTP_MSTPCRB, MSTP_MSTPCRB_OSPI0, 0);
  (void)ra8p_getreg32(RA8P_MSTP_MSTPCRB);
}

static void ra8p1_ospi_disable_cs1_map(void)
{
  ra8p_modifyreg32(RA8P_XSPI_BMCTL0, XSPI_BMCTL0_CS1ACC_MASK, 0);
}

static void ra8p1_ospi_enable_cs1_map(void)
{
  ra8p_modifyreg32(RA8P_XSPI_BMCTL0, 0, XSPI_BMCTL0_CS1ACC_MASK);
}

static uint16_t ra8p1_ospi_map_command(uint16_t command, uint8_t bytes)
{
  if (bytes == RA8P1_OSPI_COMMAND_BYTES_1)
    {
      return (command & 0xff) << 8;
    }

  return command;
}

static void ra8p1_ospi_set_command_set(
    const struct ra8p1_ospi_command_set_s *cmdset)
{
  uint32_t liocfg;
  uint32_t cmcfg0;
  uint32_t bmcfgch;
  uint16_t read_command;
  uint16_t program_command;

  ra8p1_ospi_disable_cs1_map();

  ra8p_modifyreg32(RA8P_XSPI_WRAPCFG, XSPI_WRAPCFG_DSSFTCS1_MASK,
                   RA8P1_OSPI_DATA_LATCH_DELAY <<
                   XSPI_WRAPCFG_DSSFTCS1_SHIFT);

  liocfg = cmdset->protocol |
           (RA8P1_OSPI_CSMIN_CLOCKS_2 << XSPI_LIOCFGCS_CSMIN_SHIFT);
  ra8p_putreg32(liocfg, RA8P_XSPI_LIOCFGCS(1));

  cmcfg0 = (cmdset->frame_format << XSPI_CMCFG0_FFMT_SHIFT) |
           (cmdset->address_bytes << XSPI_CMCFG0_ADDSIZE_SHIFT) |
           (RA8P1_OSPI_ADDRESS_REPLACE << XSPI_CMCFG0_ADDRPEN_SHIFT);

  ra8p_putreg32(cmcfg0, RA8P_XSPI_CMCFG0(1));

  read_command = ra8p1_ospi_map_command(cmdset->read_command,
                                        cmdset->command_bytes);
  program_command = ra8p1_ospi_map_command(cmdset->program_command,
                                           cmdset->command_bytes);

  ra8p_putreg32(read_command |
                (cmdset->read_dummy_cycles << XSPI_CMCFG1_RDLATE_SHIFT),
                RA8P_XSPI_CMCFG1(1));
  ra8p_putreg32(program_command |
                (cmdset->program_dummy_cycles << XSPI_CMCFG2_WRLATE_SHIFT),
                RA8P_XSPI_CMCFG2(1));

  bmcfgch = XSPI_BMCFGCH_PREEN |
            (RA8P1_OSPI_COMBINATION_64BYTE <<
             (XSPI_BMCFGCH_MWRSIZE_SHIFT - 1));

  ra8p_putreg32(bmcfgch, RA8P_XSPI_BMCFGCH(0));
  ra8p_putreg32(bmcfgch, RA8P_XSPI_BMCFGCH(1));

  ra8p1_ospi_enable_cs1_map();
}

static int ra8p1_ospi_direct_transfer(struct ra8p1_ospi_transfer_s *transfer,
                                      bool write)
{
  uint32_t cdt;
  int ret;

  ret = ra8p1_ospi_wait_clear(RA8P_XSPI_CDCTL0, XSPI_CDCTL0_TRREQ,
                              "direct idle");
  if (ret < 0)
    {
      return ret;
    }

  cdt = (transfer->command_length << XSPI_CDBUF_CDT_CMDSIZE_SHIFT) |
        (transfer->address_length << XSPI_CDBUF_CDT_ADDSIZE_SHIFT) |
        (transfer->data_length << XSPI_CDBUF_CDT_DATASIZE_SHIFT) |
        (transfer->dummy_cycles << XSPI_CDBUF_CDT_LATE_SHIFT);

  if (write)
    {
      cdt |= XSPI_CDBUF_CDT_TRTYPE;
    }

  if (transfer->command_length == RA8P1_OSPI_COMMAND_BYTES_1)
    {
      cdt |= (transfer->command & 0xff) << XSPI_CDBUF_CDT_CMD_1B_SHIFT;
    }
  else
    {
      cdt |= (transfer->command & 0xffff) << XSPI_CDBUF_CDT_CMD_SHIFT;
    }

  ra8p_putreg32(XSPI_CDCTL0_CSSEL, RA8P_XSPI_CDCTL0);
  ra8p_putreg32(cdt, RA8P_XSPI_CDBUF_CDT(0));
  ra8p_putreg32(transfer->address, RA8P_XSPI_CDBUF_CDA(0));

  if (write)
    {
      ra8p_putreg32((uint32_t)transfer->data, RA8P_XSPI_CDBUF_CDD0(0));
      if (transfer->data_length > sizeof(uint32_t))
        {
          ra8p_putreg32((uint32_t)(transfer->data >> 32),
                        RA8P_XSPI_CDBUF_CDD1(0));
        }
    }

  ra8p_putreg32(XSPI_CDCTL0_CSSEL | XSPI_CDCTL0_TRREQ, RA8P_XSPI_CDCTL0);

  ret = ra8p1_ospi_wait_clear(RA8P_XSPI_CDCTL0, XSPI_CDCTL0_TRREQ,
                              "direct complete");
  if (ret < 0)
    {
      return ret;
    }

  if (!write)
    {
      transfer->data = ra8p_getreg32(RA8P_XSPI_CDBUF_CDD0(0));
      if (transfer->data_length > sizeof(uint32_t))
        {
          transfer->data |=
            ((uint64_t)ra8p_getreg32(RA8P_XSPI_CDBUF_CDD1(0))) << 32;
        }
    }

  ra8p_putreg32(ra8p_getreg32(RA8P_XSPI_INTS), RA8P_XSPI_INTC);
  return 0;
}

static int ra8p1_ospi_read_status_spi(uint8_t *status)
{
  struct ra8p1_ospi_transfer_s transfer =
  {
    .command        = RA8P1_OSPI_CMD_READ_STATUS_SPI,
    .command_length = RA8P1_OSPI_COMMAND_BYTES_1,
    .data_length    = 1
  };
  int ret;

  ret = ra8p1_ospi_direct_transfer(&transfer, false);
  if (ret < 0)
    {
      return ret;
    }

  *status = (uint8_t)transfer.data;
  return 0;
}

static int ra8p1_ospi_read_id_spi(uint32_t *id)
{
  struct ra8p1_ospi_transfer_s transfer =
  {
    .command        = RA8P1_OSPI_CMD_READ_ID_SPI,
    .command_length = RA8P1_OSPI_COMMAND_BYTES_1,
    .data_length    = 6
  };
  int ret;

  ret = ra8p1_ospi_direct_transfer(&transfer, false);
  if (ret < 0)
    {
      return ret;
    }

  *id = transfer.data & 0x00ffffff;
  syslog(LOG_INFO,
         "ra8p1: OSPI0 CS1 JEDEC raw=0x%012" PRIx64
         " id=0x%06" PRIx32 "\n",
         transfer.data, *id);

  if (*id != RA8P1_OSPI_DEVICE_ID)
    {
      syslog(LOG_ERR,
             "ra8p1: unsupported OSPI NOR id=0x%06" PRIx32
             " expected=0x%06" PRIx32 "\n",
             *id, (uint32_t)RA8P1_OSPI_DEVICE_ID);
      return -ENODEV;
    }

  return 0;
}

static int ra8p1_ospi_write_enable_spi(void)
{
  struct ra8p1_ospi_transfer_s transfer =
  {
    .command        = RA8P1_OSPI_CMD_WRITE_ENABLE_SPI,
    .command_length = RA8P1_OSPI_COMMAND_BYTES_1
  };
  uint8_t status;
  int ret;
  int retry;

  for (retry = 0; retry < 5; retry++)
    {
      ret = ra8p1_ospi_direct_transfer(&transfer, true);
      if (ret < 0)
        {
          return ret;
        }

      ret = ra8p1_ospi_read_status_spi(&status);
      if (ret < 0)
        {
          return ret;
        }

      if ((status & RA8P1_OSPI_STATUS_WEL) != 0)
        {
          return 0;
        }
    }

  syslog(LOG_ERR, "ra8p1: OSPI WEL not set, status=0x%02" PRIx8 "\n",
         status);
  return -EIO;
}

static int ra8p1_ospi_write_cfg2_spi(uint32_t address, uint8_t data)
{
  struct ra8p1_ospi_transfer_s transfer =
  {
    .command        = RA8P1_OSPI_CMD_WRITE_CFG2_SPI,
    .address        = address,
    .data           = data,
    .command_length = RA8P1_OSPI_COMMAND_BYTES_1,
    .address_length = 4,
    .data_length    = 1
  };
  int ret;

  ret = ra8p1_ospi_write_enable_spi();
  if (ret < 0)
    {
      return ret;
    }

  return ra8p1_ospi_direct_transfer(&transfer, true);
}

static int ra8p1_ospi_read_cfg2_spi(uint32_t address, uint8_t *data)
{
  struct ra8p1_ospi_transfer_s transfer =
  {
    .command        = RA8P1_OSPI_CMD_READ_CFG2_SPI,
    .address        = address,
    .command_length = RA8P1_OSPI_COMMAND_BYTES_1,
    .address_length = 4,
    .data_length    = 1,
    .dummy_cycles   = 1
  };
  int ret;

  ret = ra8p1_ospi_direct_transfer(&transfer, false);
  if (ret < 0)
    {
      return ret;
    }

  *data = (uint8_t)transfer.data;
  return 0;
}

static int ra8p1_ospi_read_status_opi(uint8_t *status)
{
  struct ra8p1_ospi_transfer_s transfer =
  {
    .command        = RA8P1_OSPI_CMD_READ_STATUS_OPI,
    .address        = 0,
    .command_length = RA8P1_OSPI_COMMAND_BYTES_2,
    .address_length = 4,
    .data_length    = 1,
    .dummy_cycles   = 4
  };
  int ret;

  ret = ra8p1_ospi_direct_transfer(&transfer, false);
  if (ret < 0)
    {
      return ret;
    }

  *status = (uint8_t)transfer.data;
  return 0;
}

static void ra8p1_ospi_flash_reset(void)
{
  ra8p_modifyreg32(RA8P_XSPI_LIOCTL, XSPI_LIOCTL_RSTCS0, 0);
  ra8p1_ospi_delay();
  ra8p_modifyreg32(RA8P_XSPI_LIOCTL, 0, XSPI_LIOCTL_RSTCS0);
  ra8p1_ospi_delay();
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int ra8p1_ospi0_cs1_nor_initialize(void)
{
#ifdef CONFIG_RA8P_OSPI_B
  uint8_t readback;
  uint8_t status;
  uint32_t id;
  int ret;

  if (g_ospi_initialized)
    {
      return 0;
    }

  syslog(LOG_INFO, "ra8p1: OSPI0 CS1 NOR init begin\n");

  if (ra8p1_ospi_running_from_nor())
    {
      g_ospi_initialized = true;
      syslog(LOG_INFO,
             "ra8p1: OSPI0 CS1 NOR already mapped by bootloader\n");
      return 0;
    }

  ra8p1_ospi_module_start();
  ra8p1_ospi_set_command_set(&g_ospi_spi_command_set);
  ra8p1_ospi_flash_reset();

  ret = ra8p1_ospi_read_id_spi(&id);
  if (ret < 0)
    {
      return ret;
    }

  ret = ra8p1_ospi_write_cfg2_spi(RA8P1_OSPI_CFG2_ADDR_MODE,
                                  RA8P1_OSPI_CFG2_SPI_MODE);
  if (ret < 0)
    {
      return ret;
    }

  ret = ra8p1_ospi_write_cfg2_spi(RA8P1_OSPI_CFG2_ADDR_DUMMY,
                                  RA8P1_OSPI_CFG2_DUMMY_6);
  if (ret < 0)
    {
      return ret;
    }

  ret = ra8p1_ospi_read_cfg2_spi(RA8P1_OSPI_CFG2_ADDR_DUMMY, &readback);
  if (ret < 0)
    {
      return ret;
    }

  syslog(LOG_INFO,
         "ra8p1: OSPI0 CFG2 dummy readback=0x%02" PRIx8 "\n",
         readback);

  ret = ra8p1_ospi_write_cfg2_spi(RA8P1_OSPI_CFG2_ADDR_DQS,
                                  RA8P1_OSPI_CFG2_DQS_STR);
  if (ret < 0)
    {
      return ret;
    }

  ret = ra8p1_ospi_write_cfg2_spi(RA8P1_OSPI_CFG2_ADDR_DUMMY,
                                  RA8P1_OSPI_CFG2_DUMMY_6);
  if (ret < 0)
    {
      return ret;
    }

  ret = ra8p1_ospi_write_cfg2_spi(RA8P1_OSPI_CFG2_ADDR_MODE,
                                  RA8P1_OSPI_CFG2_DTR_MODE);
  if (ret < 0)
    {
      return ret;
    }

  ra8p1_ospi_set_command_set(&g_ospi_opi_command_set);

  ret = ra8p1_ospi_read_status_opi(&status);
  if (ret < 0)
    {
      return ret;
    }

  g_ospi_initialized = true;

  syslog(LOG_INFO,
         "ra8p1: OSPI0 CS1 NOR status=0x%02" PRIx8
         " mapped base=0x%08" PRIxPTR " size=0x%08" PRIx32 "\n",
         status, (uintptr_t)RA8P_OSPI0_CS1_START,
         (uint32_t)RA8P_OSPI_NOR_SIZE);

  return 0;
#else
  return -ENODEV;
#endif
}
