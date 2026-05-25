/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32_extmem.c
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
#include <stdint.h>
#include <string.h>
#include <syslog.h>

#include <debug.h>

#include <arch/board/board.h>
#include <nuttx/fs/fs.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/mtd/mtd.h>

#include <arch/stm32n6/chip.h>

#include "hardware/stm32n6xxx_memorymap.h"
#include "stm32n6570-dk.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32_NOR_BLOCK_SIZE        512u
#define STM32_NOR_ERASE_SIZE        0x1000u

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32_xip_mtd_s
{
  struct mtd_dev_s mtd;
  uintptr_t        base;
  size_t           size;
};

struct stm32_ota_partition_s
{
  uint32_t    offset;
  uint32_t    size;
  const char *devpath;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ssize_t stm32_xip_bread(FAR struct mtd_dev_s *dev,
                               off_t startblock, size_t nblocks,
                               FAR uint8_t *buffer);
static int stm32_xip_erase(FAR struct mtd_dev_s *dev, off_t startblock,
                           size_t nblocks);
static ssize_t stm32_xip_bwrite(FAR struct mtd_dev_s *dev,
                                off_t startblock, size_t nblocks,
                                FAR const uint8_t *buffer);
static ssize_t stm32_xip_read(FAR struct mtd_dev_s *dev, off_t offset,
                              size_t nbytes, FAR uint8_t *buffer);
#ifdef CONFIG_MTD_BYTE_WRITE
static ssize_t stm32_xip_write(FAR struct mtd_dev_s *dev, off_t offset,
                               size_t nbytes,
                               FAR const uint8_t *buffer);
#endif
static int stm32_xip_ioctl(FAR struct mtd_dev_s *dev, int cmd,
                           unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32_xip_mtd_s g_nor_mtd =
{
  .mtd =
    {
      .erase  = stm32_xip_erase,
      .bread  = stm32_xip_bread,
      .bwrite = stm32_xip_bwrite,
      .read   = stm32_xip_read,
#ifdef CONFIG_MTD_BYTE_WRITE
      .write  = stm32_xip_write,
#endif
      .ioctl  = stm32_xip_ioctl,
      .name   = "stm32n6-xspi2-nor",
    },
  .base = BOARD_XSPI2_NOR_BASE,
  .size = BOARD_XSPI2_NOR_SIZE,
};

#ifdef CONFIG_STM32N6_EXTNOR_OTA_PARTITION
static const struct stm32_ota_partition_s g_ota_partition_table[] =
{
  {
    .offset  = CONFIG_STM32N6_OTA_PRIMARY_SLOT_OFFSET,
    .size    = CONFIG_STM32N6_OTA_SLOT_SIZE,
    .devpath = CONFIG_STM32N6_OTA_PRIMARY_SLOT_DEVPATH
  },
  {
    .offset  = CONFIG_STM32N6_OTA_SECONDARY_SLOT_OFFSET,
    .size    = CONFIG_STM32N6_OTA_SLOT_SIZE,
    .devpath = CONFIG_STM32N6_OTA_SECONDARY_SLOT_DEVPATH
  },
  {
    .offset  = CONFIG_STM32N6_OTA_TERTIARY_SLOT_OFFSET,
    .size    = CONFIG_STM32N6_OTA_SLOT_SIZE,
    .devpath = CONFIG_STM32N6_OTA_TERTIARY_SLOT_DEVPATH
  }
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static ssize_t stm32_xip_bread(FAR struct mtd_dev_s *dev,
                               off_t startblock, size_t nblocks,
                               FAR uint8_t *buffer)
{
  return stm32_xip_read(dev, startblock * STM32_NOR_BLOCK_SIZE,
                        nblocks * STM32_NOR_BLOCK_SIZE, buffer) /
         STM32_NOR_BLOCK_SIZE;
}

static int stm32_xip_erase(FAR struct mtd_dev_s *dev, off_t startblock,
                           size_t nblocks)
{
#ifdef CONFIG_STM32N6_EXTNOR_WRITE
  uint32_t offset = startblock * STM32_NOR_ERASE_SIZE;
  size_t nbytes = nblocks * STM32_NOR_ERASE_SIZE;
  int ret;

  ret = stm32_xspi2_nor_erase(offset, nbytes);
  return ret < 0 ? ret : (int)nblocks;
#else
  return -EROFS;
#endif
}

static ssize_t stm32_xip_bwrite(FAR struct mtd_dev_s *dev,
                                off_t startblock, size_t nblocks,
                                FAR const uint8_t *buffer)
{
#ifdef CONFIG_STM32N6_EXTNOR_WRITE
  uint32_t offset = startblock * STM32_NOR_BLOCK_SIZE;
  size_t nbytes = nblocks * STM32_NOR_BLOCK_SIZE;
  ssize_t ret;

  ret = stm32_xspi2_nor_write(offset, buffer, nbytes);
  return ret < 0 ? ret : ret / STM32_NOR_BLOCK_SIZE;
#else
  return -EROFS;
#endif
}

static ssize_t stm32_xip_read(FAR struct mtd_dev_s *dev, off_t offset,
                              size_t nbytes, FAR uint8_t *buffer)
{
  FAR struct stm32_xip_mtd_s *priv =
    (FAR struct stm32_xip_mtd_s *)dev;

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

#ifdef CONFIG_MTD_BYTE_WRITE
static ssize_t stm32_xip_write(FAR struct mtd_dev_s *dev, off_t offset,
                               size_t nbytes,
                               FAR const uint8_t *buffer)
{
#ifdef CONFIG_STM32N6_EXTNOR_WRITE
  return stm32_xspi2_nor_write(offset, buffer, nbytes);
#else
  return -EROFS;
#endif
}
#endif

static int stm32_xip_ioctl(FAR struct mtd_dev_s *dev, int cmd,
                           unsigned long arg)
{
  FAR struct stm32_xip_mtd_s *priv =
    (FAR struct stm32_xip_mtd_s *)dev;

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
          geo->blocksize    = STM32_NOR_BLOCK_SIZE;
          geo->erasesize    = STM32_NOR_ERASE_SIZE;
          geo->neraseblocks = priv->size / STM32_NOR_ERASE_SIZE;
          strlcpy(geo->model, "MX66UW1G45G-xip", sizeof(geo->model));
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

#ifdef CONFIG_STM32N6_EXTNOR_WRITE
      case MTDIOC_BULKERASE:
        return stm32_xspi2_nor_erase(0, priv->size);
#endif

      default:
        return -ENOTTY;
    }
}

#ifdef CONFIG_STM32N6_EXTNOR_OTA_PARTITION
static int stm32_register_ota_partitions(FAR struct mtd_dev_s *mtd)
{
  int ret = OK;
  int i;

  for (i = 0; i < nitems(g_ota_partition_table); i++)
    {
      FAR const struct stm32_ota_partition_s *part =
        &g_ota_partition_table[i];
      FAR struct mtd_dev_s *partmtd;
      off_t firstblock;
      off_t nblocks;

      firstblock = part->offset / STM32_NOR_BLOCK_SIZE;
      nblocks    = part->size / STM32_NOR_BLOCK_SIZE;
      partmtd    = mtd_partition(mtd, firstblock, nblocks);

      if (partmtd == NULL)
        {
          ferr("ERROR: failed to create %s partition\n", part->devpath);
          ret = -ENOMEM;
          continue;
        }

      ret = register_mtddriver(part->devpath, partmtd, 0777, NULL);
      if (ret < 0 && ret != -EEXIST)
        {
          ferr("ERROR: failed to register %s: %d\n", part->devpath, ret);
          continue;
        }

      finfo("registered %s offset=0x%08" PRIx32
            " size=0x%08" PRIx32 "%s\n",
            part->devpath, part->offset, part->size,
            ret == -EEXIST ? " (already exists)" : "");
    }

  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_extmem_initialize(void)
{
  int ret;

  syslog(LOG_INFO, "stm32n6: extmem init begin\n");

  ret = stm32_xspi2_nor_initialize();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_xspi1_psram_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32n6: XSPI1 PSRAM init failed: %d\n", ret);
      return ret;
    }

  ret = register_mtddriver("/dev/mtd0", &g_nor_mtd.mtd, 0777, NULL);
  if (ret < 0 && ret != -EEXIST)
    {
      return ret;
    }

  syslog(LOG_INFO, "stm32n6: registered /dev/mtd0%s\n",
         ret == -EEXIST ? " (already exists)" : "");

#ifdef CONFIG_STM32N6_EXTNOR_OTA_PARTITION
  ret = stm32_register_ota_partitions(&g_nor_mtd.mtd);
  if (ret < 0)
    {
      return ret;
    }
#endif

  syslog(LOG_INFO, "stm32n6: extmem init done\n");
  return OK;
}
