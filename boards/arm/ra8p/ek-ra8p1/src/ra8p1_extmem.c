/****************************************************************************
 * boards/arm/ra8p/ek-ra8p1/src/ra8p1_extmem.c
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

#include <arch/board/board.h>

#include "ek-ra8p1.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define RA8P1_OSPI_NOR_BLOCK_SIZE 512u
#define RA8P1_OSPI_NOR_ERASE_SIZE 0x1000u

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ra8p1_xip_mtd_s
{
  struct mtd_dev_s mtd;
  uintptr_t        base;
  size_t           size;
};

struct ra8p1_ota_partition_s
{
  uint32_t    offset;
  uint32_t    size;
  const char *devpath;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ssize_t ra8p1_xip_bread(FAR struct mtd_dev_s *dev,
                               off_t startblock, size_t nblocks,
                               FAR uint8_t *buffer);
static int ra8p1_xip_erase(FAR struct mtd_dev_s *dev, off_t startblock,
                           size_t nblocks);
static ssize_t ra8p1_xip_bwrite(FAR struct mtd_dev_s *dev,
                                off_t startblock, size_t nblocks,
                                FAR const uint8_t *buffer);
static ssize_t ra8p1_xip_read(FAR struct mtd_dev_s *dev, off_t offset,
                              size_t nbytes, FAR uint8_t *buffer);
#ifdef CONFIG_MTD_BYTE_WRITE
static ssize_t ra8p1_xip_write(FAR struct mtd_dev_s *dev, off_t offset,
                               size_t nbytes, FAR const uint8_t *buffer);
#endif
static int ra8p1_xip_ioctl(FAR struct mtd_dev_s *dev, int cmd,
                           unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct ra8p1_xip_mtd_s g_ospi0_cs1_mtd =
{
  .mtd =
    {
      .erase  = ra8p1_xip_erase,
      .bread  = ra8p1_xip_bread,
      .bwrite = ra8p1_xip_bwrite,
      .read   = ra8p1_xip_read,
#ifdef CONFIG_MTD_BYTE_WRITE
      .write  = ra8p1_xip_write,
#endif
      .ioctl  = ra8p1_xip_ioctl,
      .name   = "ra8p1-ospi0-cs1-nor",
    },
  .base = RA8P_OSPI0_CS1_START,
  .size = RA8P_OSPI_NOR_SIZE,
};

#ifdef CONFIG_EK_RA8P1_EXTNOR_OTA_PARTITION
static const struct ra8p1_ota_partition_s g_ota_partition_table[] =
{
  {
    .offset  = CONFIG_EK_RA8P1_OTA_PRIMARY_SLOT_OFFSET,
    .size    = CONFIG_EK_RA8P1_OTA_SLOT_SIZE,
    .devpath = CONFIG_EK_RA8P1_OTA_PRIMARY_SLOT_DEVPATH
  },
  {
    .offset  = CONFIG_EK_RA8P1_OTA_SECONDARY_SLOT_OFFSET,
    .size    = CONFIG_EK_RA8P1_OTA_SLOT_SIZE,
    .devpath = CONFIG_EK_RA8P1_OTA_SECONDARY_SLOT_DEVPATH
  }
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static ssize_t ra8p1_xip_bread(FAR struct mtd_dev_s *dev,
                               off_t startblock, size_t nblocks,
                               FAR uint8_t *buffer)
{
  ssize_t ret;

  ret = ra8p1_xip_read(dev, startblock * RA8P1_OSPI_NOR_BLOCK_SIZE,
                       nblocks * RA8P1_OSPI_NOR_BLOCK_SIZE, buffer);
  return ret < 0 ? ret : ret / RA8P1_OSPI_NOR_BLOCK_SIZE;
}

static int ra8p1_xip_erase(FAR struct mtd_dev_s *dev, off_t startblock,
                           size_t nblocks)
{
  (void)dev;
  (void)startblock;
  (void)nblocks;

  return -EROFS;
}

static ssize_t ra8p1_xip_bwrite(FAR struct mtd_dev_s *dev,
                                off_t startblock, size_t nblocks,
                                FAR const uint8_t *buffer)
{
  (void)dev;
  (void)startblock;
  (void)nblocks;
  (void)buffer;

  return -EROFS;
}

static ssize_t ra8p1_xip_read(FAR struct mtd_dev_s *dev, off_t offset,
                              size_t nbytes, FAR uint8_t *buffer)
{
  FAR struct ra8p1_xip_mtd_s *priv =
    (FAR struct ra8p1_xip_mtd_s *)dev;

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
static ssize_t ra8p1_xip_write(FAR struct mtd_dev_s *dev, off_t offset,
                               size_t nbytes, FAR const uint8_t *buffer)
{
  (void)dev;
  (void)offset;
  (void)nbytes;
  (void)buffer;

  return -EROFS;
}
#endif

static int ra8p1_xip_ioctl(FAR struct mtd_dev_s *dev, int cmd,
                           unsigned long arg)
{
  FAR struct ra8p1_xip_mtd_s *priv =
    (FAR struct ra8p1_xip_mtd_s *)dev;

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
          geo->blocksize    = RA8P1_OSPI_NOR_BLOCK_SIZE;
          geo->erasesize    = RA8P1_OSPI_NOR_ERASE_SIZE;
          geo->neraseblocks = priv->size / RA8P1_OSPI_NOR_ERASE_SIZE;
          strlcpy(geo->model, "MX25LW51245G-xip", sizeof(geo->model));
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

#ifdef CONFIG_EK_RA8P1_EXTNOR_OTA_PARTITION
static int ra8p1_register_ota_partitions(FAR struct mtd_dev_s *mtd)
{
  int ret = OK;
  int i;

  for (i = 0; i < nitems(g_ota_partition_table); i++)
    {
      FAR const struct ra8p1_ota_partition_s *part =
        &g_ota_partition_table[i];
      FAR struct mtd_dev_s *partmtd;
      off_t firstblock;
      off_t nblocks;

      firstblock = part->offset / RA8P1_OSPI_NOR_BLOCK_SIZE;
      nblocks    = part->size / RA8P1_OSPI_NOR_BLOCK_SIZE;
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

      syslog(LOG_INFO,
             "ra8p1: registered %s offset=0x%08" PRIx32
             " size=0x%08" PRIx32 "%s\n",
             part->devpath, part->offset, part->size,
             ret == -EEXIST ? " (already exists)" : "");

      if (ret == -EEXIST)
        {
          ret = OK;
        }
    }

  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int ra8p1_extmem_initialize(void)
{
#if defined(CONFIG_RA8P_OSPI_B) || defined(CONFIG_RA8P_SDRAM)
  int ret;
#endif

  syslog(LOG_INFO, "ra8p1: extmem init begin\n");

#ifdef CONFIG_RA8P_OSPI_B
  ret = ra8p1_ospi0_cs1_nor_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ra8p1: OSPI0 CS1 NOR init failed: %d\n", ret);
      return ret;
    }

  ret = register_mtddriver("/dev/mtd0", &g_ospi0_cs1_mtd.mtd, 0777, NULL);
  if (ret < 0 && ret != -EEXIST)
    {
      return ret;
    }

  syslog(LOG_INFO,
         "ra8p1: registered /dev/mtd0 base=0x%08" PRIxPTR
         " size=0x%08zx%s\n",
         g_ospi0_cs1_mtd.base, g_ospi0_cs1_mtd.size,
         ret == -EEXIST ? " (already exists)" : "");

#  ifdef CONFIG_EK_RA8P1_EXTNOR_OTA_PARTITION
  ret = ra8p1_register_ota_partitions(&g_ospi0_cs1_mtd.mtd);
  if (ret < 0)
    {
      return ret;
    }
#  endif
#endif

#ifdef CONFIG_RA8P_SDRAM
  ret = ra8p1_sdram_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ra8p1: SDRAM init failed: %d\n", ret);
      return ret;
    }
#endif

  syslog(LOG_INFO, "ra8p1: extmem init done\n");
  return OK;
}
