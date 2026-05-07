/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/src/stm32h7rs_extmem.c
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

#include <nuttx/fs/fs.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/mtd/mtd.h>

#include "stm32h7rs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32H7RS_XSPI2_NOR_BASE        0x70000000u
#define STM32H7RS_NOR_BLOCK_SIZE        512u
#define STM32H7RS_NOR_ERASE_SIZE        0x10000u

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32h7rs_xip_mtd_s
{
  struct mtd_dev_s mtd;
  uintptr_t        base;
  size_t           size;
};

struct stm32h7rs_ota_partition_s
{
  uint32_t    offset;
  uint32_t    size;
  const char *devpath;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ssize_t stm32h7rs_xip_bread(FAR struct mtd_dev_s *dev,
                                   off_t startblock, size_t nblocks,
                                   FAR uint8_t *buffer);
static int stm32h7rs_xip_erase(FAR struct mtd_dev_s *dev, off_t startblock,
                               size_t nblocks);
static ssize_t stm32h7rs_xip_bwrite(FAR struct mtd_dev_s *dev,
                                    off_t startblock, size_t nblocks,
                                    FAR const uint8_t *buffer);
static ssize_t stm32h7rs_xip_read(FAR struct mtd_dev_s *dev, off_t offset,
                                  size_t nbytes, FAR uint8_t *buffer);
static int stm32h7rs_xip_ioctl(FAR struct mtd_dev_s *dev, int cmd,
                               unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32h7rs_xip_mtd_s g_nor_mtd =
{
  .mtd =
    {
      .erase  = stm32h7rs_xip_erase,
      .bread  = stm32h7rs_xip_bread,
      .bwrite = stm32h7rs_xip_bwrite,
      .read   = stm32h7rs_xip_read,
      .ioctl  = stm32h7rs_xip_ioctl,
      .name   = "stm32h7rs-xspi2-nor",
    },
  .base = STM32H7RS_XSPI2_NOR_BASE,
  .size = STM32H7RS_XSPI2_NOR_SIZE,
};

#ifdef CONFIG_STM32H7RS_EXTNOR_OTA_PARTITION
static const struct stm32h7rs_ota_partition_s g_ota_partition_table[] =
{
  {
    .offset  = CONFIG_STM32H7RS_OTA_PRIMARY_SLOT_OFFSET,
    .size    = CONFIG_STM32H7RS_OTA_SLOT_SIZE,
    .devpath = CONFIG_STM32H7RS_OTA_PRIMARY_SLOT_DEVPATH
  },
  {
    .offset  = CONFIG_STM32H7RS_OTA_SECONDARY_SLOT_OFFSET,
    .size    = CONFIG_STM32H7RS_OTA_SLOT_SIZE,
    .devpath = CONFIG_STM32H7RS_OTA_SECONDARY_SLOT_DEVPATH
  },
  {
    .offset  = CONFIG_STM32H7RS_OTA_TERTIARY_SLOT_OFFSET,
    .size    = CONFIG_STM32H7RS_OTA_SLOT_SIZE,
    .devpath = CONFIG_STM32H7RS_OTA_TERTIARY_SLOT_DEVPATH
  }
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static ssize_t stm32h7rs_xip_bread(FAR struct mtd_dev_s *dev,
                                   off_t startblock, size_t nblocks,
                                   FAR uint8_t *buffer)
{
  return stm32h7rs_xip_read(dev, startblock * STM32H7RS_NOR_BLOCK_SIZE,
                            nblocks * STM32H7RS_NOR_BLOCK_SIZE, buffer) /
         STM32H7RS_NOR_BLOCK_SIZE;
}

static int stm32h7rs_xip_erase(FAR struct mtd_dev_s *dev, off_t startblock,
                               size_t nblocks)
{
  return -EROFS;
}

static ssize_t stm32h7rs_xip_bwrite(FAR struct mtd_dev_s *dev,
                                    off_t startblock, size_t nblocks,
                                    FAR const uint8_t *buffer)
{
  return -EROFS;
}

static ssize_t stm32h7rs_xip_read(FAR struct mtd_dev_s *dev, off_t offset,
                                  size_t nbytes, FAR uint8_t *buffer)
{
  FAR struct stm32h7rs_xip_mtd_s *priv =
    (FAR struct stm32h7rs_xip_mtd_s *)dev;

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

static int stm32h7rs_xip_ioctl(FAR struct mtd_dev_s *dev, int cmd,
                               unsigned long arg)
{
  FAR struct stm32h7rs_xip_mtd_s *priv =
    (FAR struct stm32h7rs_xip_mtd_s *)dev;

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
          geo->blocksize    = STM32H7RS_NOR_BLOCK_SIZE;
          geo->erasesize    = STM32H7RS_NOR_ERASE_SIZE;
          geo->neraseblocks = priv->size / STM32H7RS_NOR_ERASE_SIZE;
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

      default:
        return -ENOTTY;
    }
}

#ifdef CONFIG_STM32H7RS_EXTNOR_OTA_PARTITION
static int stm32h7rs_register_ota_partitions(FAR struct mtd_dev_s *mtd)
{
  int ret = OK;
  int i;

  for (i = 0; i < nitems(g_ota_partition_table); i++)
    {
      FAR const struct stm32h7rs_ota_partition_s *part =
        &g_ota_partition_table[i];
      FAR struct mtd_dev_s *partmtd;
      off_t firstblock;
      off_t nblocks;

      firstblock = part->offset / STM32H7RS_NOR_BLOCK_SIZE;
      nblocks    = part->size / STM32H7RS_NOR_BLOCK_SIZE;
      partmtd    = mtd_partition(mtd, firstblock, nblocks);

      if (partmtd == NULL)
        {
          ferr("ERROR: failed to create %s partition\n", part->devpath);
          ret = -ENOMEM;
          continue;
        }

      ret = register_mtddriver(part->devpath, partmtd, 0777, NULL);
      if (ret < 0)
        {
          ferr("ERROR: failed to register %s: %d\n", part->devpath, ret);
          continue;
        }

      finfo("registered %s offset=0x%08" PRIx32 " size=0x%08" PRIx32 "\n",
            part->devpath, part->offset, part->size);
    }

  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32h7rs_extmem_initialize(void)
{
  int ret;

  ret = stm32h7rs_xspi2_nor_initialize();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32h7rs_xspi1_psram_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32h7rs: XSPI1 PSRAM init failed: %d\n", ret);
      return ret;
    }

#ifdef CONFIG_STM32H7RS_EXTNOR_OTA_PARTITION
  ret = stm32h7rs_register_ota_partitions(&g_nor_mtd.mtd);
  if (ret < 0)
    {
      return ret;
    }
#endif

  return OK;
}
