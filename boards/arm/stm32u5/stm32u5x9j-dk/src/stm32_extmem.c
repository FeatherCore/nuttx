/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32_extmem.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/mount.h>
#include <sys/param.h>
#include <sys/stat.h>
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
#include <nuttx/fs/ioctl.h>
#include <nuttx/mtd/mtd.h>

#include <arch/board/board.h>

#include "stm32_gpio.h"
#include "stm32_xspi.h"
#include "stm32u5x9j-dk.h"

#include "hardware/stm32_memorymap.h"

#if defined(CONFIG_STM32U5X9J_DK_OSPI_NOR) || \
    defined(CONFIG_STM32U5X9J_DK_HSPI_RAM)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MX25UM51245G_FLASH_SIZE           (64u * 1024u * 1024u)
#define MX25UM51245G_BLOCK_SIZE           512u
#define MX25UM51245G_ERASE_SIZE           (64u * 1024u)
#define MX25UM51245G_SUBSECTOR_SIZE       (4u * 1024u)
#define MX25UM51245G_PAGE_SIZE            256u
#define MX25UM51245G_DEVSIZE              25u
#define MX25UM51245G_MANUFACTURER_ID      0xc2u
#define MX25UM51245G_MEMORY_TYPE          0x81u
#define MX25UM51245G_CAPACITY_ID          0x3au
#define MX25UM51245G_READ_ID_CMD          0x9fu
#define MX25UM51245G_WRITE_ENABLE_CMD     0x06u
#define MX25UM51245G_READ_STATUS_CMD      0x05u
#define MX25UM51245G_RESET_ENABLE_CMD     0x66u
#define MX25UM51245G_RESET_MEMORY_CMD     0x99u
#define MX25UM51245G_OCTA_RESET_ENABLE_CMD 0x6699u
#define MX25UM51245G_OCTA_RESET_MEMORY_CMD 0x9966u
#define MX25UM51245G_FAST_READ_4B_CMD     0x0cu
#define MX25UM51245G_FAST_READ_DUMMY      8u
#define MX25UM51245G_SECTOR_ERASE_4B_CMD  0xdcu
#define MX25UM51245G_SUBSECTOR_ERASE_4B_CMD 0x21u
#define MX25UM51245G_PAGE_PROGRAM_4B_CMD  0x12u
#define MX25UM51245G_SUSPEND_CMD          0xb0u
#define MX25UM51245G_RESUME_CMD           0x30u
#define MX25UM51245G_STATUS_WIP           (1u << 0)
#define MX25UM51245G_READY_TIMEOUT_MS     5000u

#define APS512XX_RAM_SIZE                 (64u * 1024u * 1024u)
#define APS512XX_FB_RESERVED              (2u * 1024u * 1024u)
#define APS512XX_HEAP_BASE                (STM32_HSPI1_BANK + APS512XX_FB_RESERVED)
#define APS512XX_HEAP_SIZE                (APS512XX_RAM_SIZE - APS512XX_FB_RESERVED)
#define APS512XX_DEVSIZE                  25u
#define APS512XX_RESET_CMD                0xffu
#define APS512XX_READ_CMD                 0x00u
#define APS512XX_WRITE_LINEAR_BURST_CMD   0xa0u
#define APS512XX_READ_REG_CMD             0x40u
#define APS512XX_WRITE_REG_CMD            0xc0u
#define APS512XX_MR0_ADDRESS              0x00000000u
#define APS512XX_MR4_ADDRESS              0x00000004u
#define APS512XX_MR8_ADDRESS              0x00000008u
#define APS512XX_MR0_VALUE                0x09u
#define APS512XX_MR4_VALUE                0x40u
#define APS512XX_MR8_VALUE                0x41u
#define APS512XX_REG_LATENCY              4u
#define APS512XX_READ_LATENCY             5u
#define APS512XX_WRITE_LATENCY            5u

#define STM32U5X9J_OSPI_DIAG_PATH         "/dev/ospi0"
#define STM32U5X9J_HSPI_DIAG_PATH         "/dev/hspiram0"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32u5x9j_xip_mtd_s
{
  struct mtd_dev_s mtd;
  uintptr_t        base;
  size_t           size;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ssize_t stm32u5x9j_xip_bread(FAR struct mtd_dev_s *dev,
                                    off_t startblock, size_t nblocks,
                                    FAR uint8_t *buffer);
static int stm32u5x9j_xip_erase(FAR struct mtd_dev_s *dev,
                                 off_t startblock, size_t nblocks);
static ssize_t stm32u5x9j_xip_bwrite(FAR struct mtd_dev_s *dev,
                                     off_t startblock, size_t nblocks,
                                     FAR const uint8_t *buffer);
static ssize_t stm32u5x9j_xip_read(FAR struct mtd_dev_s *dev,
                                   off_t offset, size_t nbytes,
                                   FAR uint8_t *buffer);
static ssize_t stm32u5x9j_xip_write(FAR struct mtd_dev_s *dev,
                                    off_t offset, size_t nbytes,
                                    FAR const uint8_t *buffer);
static int stm32u5x9j_xip_ioctl(FAR struct mtd_dev_s *dev,
                                int cmd, unsigned long arg);
#ifdef CONFIG_STM32U5X9J_DK_OSPI_NOR
static int stm32u5x9j_ospi_enter_memory_mapped(void);
static int stm32u5x9j_ospi_erase_sector(uint32_t address);
#ifdef CONFIG_STM32U5X9J_DK_OSPI_SCRATCH_TEST
static int stm32u5x9j_ospi_erase_subsector(uint32_t address);
#endif
static int stm32u5x9j_ospi_page_program(uint32_t address,
                                        FAR const uint8_t *buffer,
                                        size_t nbytes);
#ifdef CONFIG_STM32U5X9J_DK_OSPI_DIAG
static ssize_t stm32u5x9j_ospi_diag_read(FAR struct file *filep,
                                         FAR char *buffer, size_t buflen);
static ssize_t stm32u5x9j_ospi_diag_write(FAR struct file *filep,
                                          FAR const char *buffer,
                                          size_t buflen);
static int stm32u5x9j_ospi_diag_register(void);
#endif
#endif
#ifdef CONFIG_STM32U5X9J_DK_HSPI_DIAG
static ssize_t stm32u5x9j_hspi_diag_read(FAR struct file *filep,
                                         FAR char *buffer, size_t buflen);
static ssize_t stm32u5x9j_hspi_diag_write(FAR struct file *filep,
                                          FAR const char *buffer,
                                          size_t buflen);
static int stm32u5x9j_hspi_diag_register(void);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_STM32U5X9J_DK_OSPI_NOR
static uint8_t g_ospi_jedec[3];

static struct stm32u5x9j_xip_mtd_s g_ospi_mtd =
{
  .mtd =
    {
      .erase  = stm32u5x9j_xip_erase,
      .bread  = stm32u5x9j_xip_bread,
      .bwrite = stm32u5x9j_xip_bwrite,
      .read   = stm32u5x9j_xip_read,
#ifdef CONFIG_MTD_BYTE_WRITE
      .write  = stm32u5x9j_xip_write,
#endif
      .ioctl  = stm32u5x9j_xip_ioctl,
      .name   = "stm32u5x9j-ospi1-nor",
    },
  .base = STM32_OCTOSPI1_BANK,
  .size = MX25UM51245G_FLASH_SIZE,
};

#ifdef CONFIG_STM32U5X9J_DK_OSPI_DIAG
static bool g_ospi_diag_registered;
static int g_ospi_last_scratch = -ENOTSUP;

static const struct file_operations g_ospi_diag_fops =
{
  NULL,                         /* open */
  NULL,                         /* close */
  stm32u5x9j_ospi_diag_read,    /* read */
  stm32u5x9j_ospi_diag_write,   /* write */
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
  stm32u5x9j_hspi_diag_read,    /* read */
  stm32u5x9j_hspi_diag_write,   /* write */
};
#endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_STM32U5X9J_DK_OSPI_DIAG) || \
    defined(CONFIG_STM32U5X9J_DK_HSPI_DIAG)
static ssize_t stm32u5x9j_diag_copyout(FAR struct file *filep,
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
static ssize_t stm32u5x9j_xip_bread(FAR struct mtd_dev_s *dev,
                                    off_t startblock, size_t nblocks,
                                    FAR uint8_t *buffer)
{
  return stm32u5x9j_xip_read(dev, startblock * MX25UM51245G_BLOCK_SIZE,
                             nblocks * MX25UM51245G_BLOCK_SIZE, buffer) /
         MX25UM51245G_BLOCK_SIZE;
}

static int stm32u5x9j_xip_erase(FAR struct mtd_dev_s *dev,
                                 off_t startblock, size_t nblocks)
{
  FAR struct stm32u5x9j_xip_mtd_s *priv =
    (FAR struct stm32u5x9j_xip_mtd_s *)dev;
  uint32_t address;
  size_t i;
  int ret;

  if (startblock < 0 ||
      (uint64_t)(startblock + nblocks) * MX25UM51245G_ERASE_SIZE >
      priv->size)
    {
      return -EINVAL;
    }

  if (nblocks == 0)
    {
      return 0;
    }

  ret = stm32_xspi_abort(STM32_XSPI_PORT_OCTOSPI1);
  if (ret < 0)
    {
      return ret;
    }

  for (i = 0; i < nblocks; i++)
    {
      address = (startblock + i) * MX25UM51245G_ERASE_SIZE;
      ret = stm32u5x9j_ospi_erase_sector(address);
      if (ret < 0)
        {
          (void)stm32u5x9j_ospi_enter_memory_mapped();
          return ret;
        }
    }

  ret = stm32u5x9j_ospi_enter_memory_mapped();
  if (ret < 0)
    {
      return ret;
    }

  return nblocks;
}

static ssize_t stm32u5x9j_xip_bwrite(FAR struct mtd_dev_s *dev,
                                     off_t startblock, size_t nblocks,
                                     FAR const uint8_t *buffer)
{
  ssize_t nwritten;

  nwritten = stm32u5x9j_xip_write(dev,
                                  startblock * MX25UM51245G_BLOCK_SIZE,
                                  nblocks * MX25UM51245G_BLOCK_SIZE,
                                  buffer);
  if (nwritten < 0)
    {
      return nwritten;
    }

  return nwritten / MX25UM51245G_BLOCK_SIZE;
}

static ssize_t stm32u5x9j_xip_read(FAR struct mtd_dev_s *dev,
                                   off_t offset, size_t nbytes,
                                   FAR uint8_t *buffer)
{
  FAR struct stm32u5x9j_xip_mtd_s *priv =
    (FAR struct stm32u5x9j_xip_mtd_s *)dev;

  if (offset < 0 || offset >= priv->size)
    {
      return 0;
    }

  if (offset + nbytes > priv->size)
    {
      nbytes = priv->size - offset;
    }

  memcpy(buffer, (FAR const void *)(priv->base + offset), nbytes);
  return nbytes;
}

static ssize_t stm32u5x9j_xip_write(FAR struct mtd_dev_s *dev,
                                    off_t offset, size_t nbytes,
                                    FAR const uint8_t *buffer)
{
  FAR struct stm32u5x9j_xip_mtd_s *priv =
    (FAR struct stm32u5x9j_xip_mtd_s *)dev;
  size_t chunk;
  size_t written = 0;
  int ret;

  if (buffer == NULL || offset < 0 || offset + nbytes > priv->size)
    {
      return -EINVAL;
    }

  if (nbytes == 0)
    {
      return 0;
    }

  ret = stm32_xspi_abort(STM32_XSPI_PORT_OCTOSPI1);
  if (ret < 0)
    {
      return ret;
    }

  while (written < nbytes)
    {
      chunk = MX25UM51245G_PAGE_SIZE -
              ((offset + written) & (MX25UM51245G_PAGE_SIZE - 1));
      chunk = MIN(chunk, nbytes - written);

      ret = stm32u5x9j_ospi_page_program(offset + written,
                                         &buffer[written], chunk);
      if (ret < 0)
        {
          (void)stm32u5x9j_ospi_enter_memory_mapped();
          return ret;
        }

      written += chunk;
    }

  ret = stm32u5x9j_ospi_enter_memory_mapped();
  if (ret < 0)
    {
      return ret;
    }

  return nbytes;
}

static int stm32u5x9j_xip_ioctl(FAR struct mtd_dev_s *dev,
                                int cmd, unsigned long arg)
{
  FAR struct stm32u5x9j_xip_mtd_s *priv =
    (FAR struct stm32u5x9j_xip_mtd_s *)dev;

  switch (cmd)
    {
      case MTDIOC_GEOMETRY:
        {
          FAR struct mtd_geometry_s *geo =
            (FAR struct mtd_geometry_s *)((uintptr_t)arg);

          if (geo == NULL)
            {
              return -EINVAL;
            }

          memset(geo, 0, sizeof(*geo));
          geo->blocksize    = MX25UM51245G_BLOCK_SIZE;
          geo->erasesize    = MX25UM51245G_ERASE_SIZE;
          geo->neraseblocks = priv->size / MX25UM51245G_ERASE_SIZE;
          strlcpy(geo->model, "MX25UM51245G-xip", sizeof(geo->model));
          return OK;
        }

      case BIOC_XIPBASE:
        {
          FAR void **base = (FAR void **)((uintptr_t)arg);

          if (base == NULL)
            {
              return -EINVAL;
            }

          *base = (FAR void *)priv->base;
          return OK;
        }

      case MTDIOC_ERASESTATE:
        {
          FAR uint8_t *erased = (FAR uint8_t *)((uintptr_t)arg);

          if (erased == NULL)
            {
              return -EINVAL;
            }

          *erased = 0xff;
          return OK;
        }

      case MTDIOC_BULKERASE:
        return stm32u5x9j_xip_erase(dev, 0,
                                    priv->size / MX25UM51245G_ERASE_SIZE);

      default:
        return -ENOTTY;
    }
}

static bool stm32u5x9j_ospi_id_blank(FAR const uint8_t id[3])
{
  return (id[0] == 0x00 && id[1] == 0x00 && id[2] == 0x00) ||
         (id[0] == 0xff && id[1] == 0xff && id[2] == 0xff);
}

static void stm32u5x9j_ospi_gpio_config(void)
{
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

static int stm32u5x9j_ospi_readid(FAR uint8_t id[3])
{
  return stm32_xspi_read_data(STM32_XSPI_PORT_OCTOSPI1,
                              MX25UM51245G_READ_ID_CMD, 0,
                              XSPI_CCR_IMODE_1_LINE |
                              XSPI_CCR_DMODE_1_LINE,
                              0, 3, id);
}

static int stm32u5x9j_ospi_read_status(FAR uint8_t *status)
{
  return stm32_xspi_read_data(STM32_XSPI_PORT_OCTOSPI1,
                              MX25UM51245G_READ_STATUS_CMD, 0,
                              XSPI_CCR_IMODE_1_LINE |
                              XSPI_CCR_DMODE_1_LINE,
                              0, 1, status);
}

static int stm32u5x9j_ospi_write_enable(void)
{
  return stm32_xspi_command(STM32_XSPI_PORT_OCTOSPI1,
                            MX25UM51245G_WRITE_ENABLE_CMD,
                            XSPI_CCR_IMODE_1_LINE);
}

static int stm32u5x9j_ospi_wait_ready(void)
{
  uint32_t remaining = MX25UM51245G_READY_TIMEOUT_MS;
  uint8_t status;
  int ret;

  do
    {
      ret = stm32u5x9j_ospi_read_status(&status);
      if (ret < 0)
        {
          return ret;
        }

      if ((status & MX25UM51245G_STATUS_WIP) == 0)
        {
          return OK;
        }

      up_mdelay(1);
    }
  while (remaining-- > 0);

  return -ETIMEDOUT;
}

static int stm32u5x9j_ospi_erase_sector(uint32_t address)
{
  int ret;

  ret = stm32u5x9j_ospi_write_enable();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_xspi_command_addr(STM32_XSPI_PORT_OCTOSPI1,
                                MX25UM51245G_SECTOR_ERASE_4B_CMD,
                                address,
                                XSPI_CCR_IMODE_1_LINE |
                                XSPI_CCR_ADMODE_1_LINE |
                                XSPI_CCR_ADSIZE_32,
                                0);
  if (ret < 0)
    {
      return ret;
    }

  return stm32u5x9j_ospi_wait_ready();
}

#ifdef CONFIG_STM32U5X9J_DK_OSPI_SCRATCH_TEST
static int stm32u5x9j_ospi_erase_subsector(uint32_t address)
{
  int ret;

  ret = stm32u5x9j_ospi_write_enable();
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

  return stm32u5x9j_ospi_wait_ready();
}
#endif

#ifdef CONFIG_STM32U5X9J_DK_OSPI_DIAG
static int stm32u5x9j_ospi_suspend(void)
{
  return stm32_xspi_command(STM32_XSPI_PORT_OCTOSPI1,
                            MX25UM51245G_SUSPEND_CMD,
                            XSPI_CCR_IMODE_1_LINE);
}

static int stm32u5x9j_ospi_resume(void)
{
  return stm32_xspi_command(STM32_XSPI_PORT_OCTOSPI1,
                            MX25UM51245G_RESUME_CMD,
                            XSPI_CCR_IMODE_1_LINE);
}
#endif

static int stm32u5x9j_ospi_page_program(uint32_t address,
                                        FAR const uint8_t *buffer,
                                        size_t nbytes)
{
  int ret;

  if (nbytes == 0 || nbytes > MX25UM51245G_PAGE_SIZE)
    {
      return -EINVAL;
    }

  ret = stm32u5x9j_ospi_write_enable();
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

  return stm32u5x9j_ospi_wait_ready();
}

static int stm32u5x9j_ospi_enter_memory_mapped(void)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_1_LINE | XSPI_CCR_ADMODE_1_LINE |
        XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_1_LINE;

  return stm32_xspi_enter_memory_mapped(STM32_XSPI_PORT_OCTOSPI1, ccr,
                                        XSPI_TCR_DCYC(
                                          MX25UM51245G_FAST_READ_DUMMY),
                                        MX25UM51245G_FAST_READ_4B_CMD);
}

#ifdef CONFIG_STM32U5X9J_DK_OSPI_DIAG
#ifdef CONFIG_STM32U5X9J_DK_OSPI_SCRATCH_TEST
static int stm32u5x9j_ospi_scratch_test(void)
{
  uint8_t pattern[MX25UM51245G_PAGE_SIZE];
  uint32_t offset = CONFIG_STM32U5X9J_DK_OSPI_SCRATCH_OFFSET;
  FAR const uint8_t *xip;
  size_t i;
  int ret;

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

  ret = stm32u5x9j_ospi_erase_subsector(offset);
  if (ret < 0)
    {
      (void)stm32u5x9j_ospi_enter_memory_mapped();
      return ret;
    }

  ret = stm32u5x9j_ospi_page_program(offset, pattern, sizeof(pattern));
  if (ret < 0)
    {
      (void)stm32u5x9j_ospi_enter_memory_mapped();
      return ret;
    }

  ret = stm32u5x9j_ospi_enter_memory_mapped();
  if (ret < 0)
    {
      return ret;
    }

  xip = (FAR const uint8_t *)(STM32_OCTOSPI1_BANK + offset);
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

static ssize_t stm32u5x9j_ospi_diag_read(FAR struct file *filep,
                                         FAR char *buffer, size_t buflen)
{
  FAR const volatile uint32_t *xip =
    (FAR const volatile uint32_t *)STM32_OCTOSPI1_BANK;
  char text[512];
  int len;

  len = snprintf(text, sizeof(text),
                 "mx25um51245g size=%u erase64k=%u erase4k=%u page=%u\n"
                 "jedec=%02x %02x %02x manufacturer_ok=%u\n"
                 "xip_base=0x%08x xip_first=0x%08" PRIx32 "\n"
                 "mtd=/dev/mtd0 littlefs=/mnt/flash mount_only=1\n"
                 "suspend_resume=available scratch=%s last_scratch=%d\n",
                 MX25UM51245G_FLASH_SIZE, MX25UM51245G_ERASE_SIZE,
                 MX25UM51245G_SUBSECTOR_SIZE, MX25UM51245G_PAGE_SIZE,
                 g_ospi_jedec[0], g_ospi_jedec[1], g_ospi_jedec[2],
                 g_ospi_jedec[0] == MX25UM51245G_MANUFACTURER_ID,
                 STM32_OCTOSPI1_BANK, *xip,
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

  return stm32u5x9j_diag_copyout(filep, buffer, buflen, text);
}

static ssize_t stm32u5x9j_ospi_diag_write(FAR struct file *filep,
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

      ret = stm32u5x9j_ospi_suspend();
      (void)stm32u5x9j_ospi_enter_memory_mapped();
      return ret < 0 ? ret : (ssize_t)buflen;
    }

  if (strncmp(cmd, "resume", 6) == 0)
    {
      ret = stm32_xspi_abort(STM32_XSPI_PORT_OCTOSPI1);
      if (ret < 0)
        {
          return ret;
        }

      ret = stm32u5x9j_ospi_resume();
      (void)stm32u5x9j_ospi_enter_memory_mapped();
      return ret < 0 ? ret : (ssize_t)buflen;
    }

  if (strncmp(cmd, "scratch", 7) == 0)
    {
#ifdef CONFIG_STM32U5X9J_DK_OSPI_SCRATCH_TEST
      ret = stm32u5x9j_ospi_scratch_test();
      g_ospi_last_scratch = ret;
      return ret < 0 ? ret : (ssize_t)buflen;
#else
      g_ospi_last_scratch = -ENOTSUP;
      return -ENOTSUP;
#endif
    }

  return -EINVAL;
}

static int stm32u5x9j_ospi_diag_register(void)
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
  syslog(LOG_INFO, "stm32u5x9j: OSPI diagnostic at %s\n",
         STM32U5X9J_OSPI_DIAG_PATH);

  return OK;
}
#endif /* CONFIG_STM32U5X9J_DK_OSPI_DIAG */
#endif /* CONFIG_STM32U5X9J_DK_OSPI_NOR */

#ifdef CONFIG_STM32U5X9J_DK_HSPI_RAM
static void stm32u5x9j_hspi_gpio_config(void)
{
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

static int stm32u5x9j_psram_write_reg(uint32_t address, uint8_t value)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR;

  return stm32_xspi_write_data(STM32_XSPI_PORT_HSPI1,
                               APS512XX_WRITE_REG_CMD, address,
                               ccr, 0, value, 2, true);
}

static int stm32u5x9j_psram_read_reg(uint32_t address, FAR uint8_t *value)
{
  uint8_t buf[2];
  uint32_t ccr;
  int ret;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_8_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  ret = stm32_xspi_read_data(STM32_XSPI_PORT_HSPI1,
                             APS512XX_READ_REG_CMD, address,
                             ccr, XSPI_TCR_DCYC(APS512XX_REG_LATENCY - 1),
                             sizeof(buf), buf);
  if (ret < 0)
    {
      return ret;
    }

  *value = buf[0];
  return OK;
}

static int stm32u5x9j_psram_config_reg(uint32_t address, uint8_t value,
                                       uint8_t mask)
{
  uint8_t initial;
  uint8_t readback;
  uint8_t writeval;
  int ret;

  ret = stm32u5x9j_psram_read_reg(address, &initial);
  if (ret < 0)
    {
      return ret;
    }

  writeval = (initial & ~mask) | (value & mask);
  ret = stm32u5x9j_psram_write_reg(address, writeval);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32u5x9j_psram_read_reg(address, &readback);
  if (ret < 0)
    {
      return ret;
    }

  if ((readback & mask) != (value & mask))
    {
      syslog(LOG_ERR, "stm32u5x9j: HSPI PSRAM MR%08" PRIx32
             " initial %02x write %02x readback %02x expected %02x\n",
             address, initial, writeval, readback, value);
      return -EIO;
    }

  syslog(LOG_INFO, "stm32u5x9j: HSPI PSRAM MR%08" PRIx32
         " initial %02x write %02x readback %02x\n",
         address, initial, writeval, readback);
  return OK;
}

static int stm32u5x9j_psram_enter_memory_mapped(void)
{
  uint32_t ccr;

  ccr = XSPI_CCR_IMODE_8_LINES | XSPI_CCR_ADMODE_8_LINES |
        XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE_32 | XSPI_CCR_DMODE_16_LINES |
        XSPI_CCR_DDTR | XSPI_CCR_DQSE;

  return stm32_xspi_enter_memory_mapped_rw(STM32_XSPI_PORT_HSPI1,
                                           ccr,
                                           XSPI_TCR_DCYC(
                                             APS512XX_READ_LATENCY - 1),
                                           APS512XX_READ_CMD,
                                           ccr,
                                           XSPI_TCR_DCYC(
                                             APS512XX_WRITE_LATENCY - 1),
                                           APS512XX_WRITE_LINEAR_BURST_CMD);
}

static int stm32u5x9j_psram_selftest(void)
{
  volatile uint32_t *mem = (volatile uint32_t *)STM32_HSPI1_BANK;
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

  syslog(LOG_INFO, "stm32u5x9j: HSPI PSRAM self-test passed\n");
  return OK;
}

#ifdef CONFIG_STM32U5X9J_DK_HSPI_DIAG
static ssize_t stm32u5x9j_hspi_diag_read(FAR struct file *filep,
                                         FAR char *buffer, size_t buflen)
{
  char text[384];
  int len;

  len = snprintf(text, sizeof(text),
                 "aps512xx base=0x%08x size=%u mapped=%u\n"
                 "framebuffer base=0x%08x size=%u\n"
                 "heap base=0x%08x size=%u enabled=%u\n"
                 "selftest=save-write-read-restore-32B last=%d\n",
                 STM32_HSPI1_BANK, APS512XX_RAM_SIZE, g_hspi_ram_mapped,
                 STM32_HSPI1_BANK, APS512XX_FB_RESERVED,
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

  return stm32u5x9j_diag_copyout(filep, buffer, buflen, text);
}

static ssize_t stm32u5x9j_hspi_diag_write(FAR struct file *filep,
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

  ret = stm32u5x9j_hspi_ram_initialize();
  if (ret == OK)
    {
      ret = stm32u5x9j_psram_selftest();
    }

  g_hspi_last_selftest = ret;
  return ret < 0 ? ret : (ssize_t)buflen;
}

static int stm32u5x9j_hspi_diag_register(void)
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
  syslog(LOG_INFO, "stm32u5x9j: HSPI PSRAM diagnostic at %s\n",
         STM32U5X9J_HSPI_DIAG_PATH);

  return OK;
}
#endif
#endif /* CONFIG_STM32U5X9J_DK_HSPI_RAM */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_STM32U5X9J_DK_OSPI_NOR
int stm32u5x9j_ospi_nor_initialize(void)
{
  const struct stm32_xspi_config_s config =
    {
      .port           = STM32_XSPI_PORT_OCTOSPI1,
      .memory_type    = XSPI_DCR1_MTYP_MACRONIX,
      .device_size    = MX25UM51245G_DEVSIZE,
      .csht           = 2,
      .prescaler      = 3,
      .fifo_threshold = 1,
      .csbound        = 0,
    };
  uint8_t id[3] = {0};
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

  stm32u5x9j_ospi_gpio_config();
  ret = stm32_xspi_configure(&config);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_xspi_command(STM32_XSPI_PORT_OCTOSPI1,
                           MX25UM51245G_OCTA_RESET_ENABLE_CMD,
                           XSPI_CCR_IMODE_8_LINES |
                           XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16);
  if (ret < 0)
    {
      syslog(LOG_WARNING,
             "stm32u5x9j: OSPI1 NOR octal reset-enable failed: %d\n", ret);
    }

  ret = stm32_xspi_command(STM32_XSPI_PORT_OCTOSPI1,
                           MX25UM51245G_OCTA_RESET_MEMORY_CMD,
                           XSPI_CCR_IMODE_8_LINES |
                           XSPI_CCR_IDTR | XSPI_CCR_ISIZE_16);
  if (ret < 0)
    {
      syslog(LOG_WARNING,
             "stm32u5x9j: OSPI1 NOR octal reset failed: %d\n", ret);
    }

  up_mdelay(100);

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

  up_mdelay(100);

  ret = stm32u5x9j_ospi_readid(id);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR read-id failed: %d\n", ret);
      return ret;
    }

  syslog(LOG_INFO, "stm32u5x9j: OSPI1 NOR JEDEC ID %02x %02x %02x\n",
         id[0], id[1], id[2]);

  memcpy(g_ospi_jedec, id, sizeof(g_ospi_jedec));

  if (stm32u5x9j_ospi_id_blank(id))
    {
      return -EIO;
    }

  ret = stm32u5x9j_ospi_enter_memory_mapped();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: OSPI1 NOR map failed: %d\n", ret);
      return ret;
    }

  first = *(volatile uint32_t *)STM32_OCTOSPI1_BANK;
  syslog(LOG_INFO, "stm32u5x9j: OSPI1 NOR mapped at 0x%08x first=0x%08"
         PRIx32 "\n", STM32_OCTOSPI1_BANK, first);
  return OK;
}

int stm32u5x9j_flash_initialize(void)
{
  int ret;

  ret = stm32u5x9j_ospi_nor_initialize();
  if (ret < 0)
    {
      return ret;
    }

  ret = register_mtddriver("/dev/mtd0", &g_ospi_mtd.mtd, 0777, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: register /dev/mtd0 failed: %d\n", ret);
      return ret;
    }

#ifdef CONFIG_STM32U5X9J_DK_OSPI_DIAG
  ret = stm32u5x9j_ospi_diag_register();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: register /dev/ospi0 failed: %d\n", ret);
      return ret;
    }
#endif

#ifdef CONFIG_STM32U5X9J_DK_FLASH_AUTOMOUNT
  mkdir("/mnt/flash", 0777);
  ret = mount("/dev/mtd0", "/mnt/flash", "littlefs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_WARNING,
             "stm32u5x9j: littlefs mount /mnt/flash failed: %d\n",
             -errno);
    }
#endif

  return OK;
}
#endif /* CONFIG_STM32U5X9J_DK_OSPI_NOR */

#ifdef CONFIG_STM32U5X9J_DK_HSPI_RAM
int stm32u5x9j_hspi_ram_initialize(void)
{
  const struct stm32_xspi_config_s config =
    {
      .port           = STM32_XSPI_PORT_HSPI1,
      .memory_type    = XSPI_DCR1_MTYP_APMEM_16BIT,
      .device_size    = APS512XX_DEVSIZE,
      .csht           = 5,
      .prescaler      = 0,
      .fifo_threshold = 2,
      .csbound        = 0,
    };
  int ret;

  if (g_hspi_ram_mapped || stm32_xspi_is_mapped(STM32_XSPI_PORT_HSPI1))
    {
      g_hspi_ram_mapped = true;
      return OK;
    }

  ret = stm32_xspi_common_setup();
  if (ret < 0)
    {
      return ret;
    }

  stm32u5x9j_hspi_gpio_config();
  ret = stm32_xspi_configure(&config);
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
      syslog(LOG_ERR, "stm32u5x9j: HSPI PSRAM reset failed: %d\n", ret);
      return ret;
    }

  up_mdelay(1);

  ret = stm32u5x9j_psram_config_reg(APS512XX_MR0_ADDRESS,
                                    APS512XX_MR0_VALUE, 0x1fu);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32u5x9j_psram_config_reg(APS512XX_MR4_ADDRESS,
                                    APS512XX_MR4_VALUE, 0xffu);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32u5x9j_psram_config_reg(APS512XX_MR8_ADDRESS,
                                    APS512XX_MR8_VALUE, 0x43u);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32u5x9j_psram_enter_memory_mapped();
  if (ret < 0)
    {
      return ret;
    }

  g_hspi_ram_mapped = true;

  ret = stm32u5x9j_psram_selftest();
#ifdef CONFIG_STM32U5X9J_DK_HSPI_DIAG
  g_hspi_last_selftest = ret;
#endif
  if (ret < 0)
    {
      g_hspi_ram_mapped = false;
      return ret;
    }

  syslog(LOG_INFO, "stm32u5x9j: HSPI PSRAM mapped at 0x%08x\n",
         STM32_HSPI1_BANK);
  return OK;
}

bool stm32u5x9j_hspi_ram_is_mapped(void)
{
  return g_hspi_ram_mapped;
}

#ifdef CONFIG_STM32U5X9J_DK_HSPI_DIAG
int stm32u5x9j_hspi_diag_initialize(void)
{
  int ret;

  ret = stm32u5x9j_hspi_ram_initialize();
  if (ret < 0)
    {
      return ret;
    }

  return stm32u5x9j_hspi_diag_register();
}
#endif
#endif /* CONFIG_STM32U5X9J_DK_HSPI_RAM */

#endif /* OSPI NOR || HSPI RAM */
