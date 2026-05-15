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

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>

#include <debug.h>

#ifdef CONFIG_STM32U5X9J_DK_OSPI_MTD
#  include <nuttx/fs/ioctl.h>
#  include <nuttx/mtd/mtd.h>
#endif

#include "stm32u5x9j-dk.h"

#if defined(CONFIG_STM32U5X9J_DK_OSPI_NOR) || \
    defined(CONFIG_STM32U5X9J_DK_HSPI_RAM)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MX25UM51245G_BLOCK_SIZE      512u
#define MX25UM51245G_ERASE_SIZE      (64u * 1024u)

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_STM32U5X9J_DK_OSPI_MTD
struct stm32u5x9j_xip_mtd_s
{
  struct mtd_dev_s mtd;
  uintptr_t        base;
  size_t           size;
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_STM32U5X9J_DK_OSPI_MTD
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
static int stm32u5x9j_xip_ioctl(FAR struct mtd_dev_s *dev,
                                int cmd, unsigned long arg);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_STM32U5X9J_DK_OSPI_MTD
static struct stm32u5x9j_xip_mtd_s g_ospi_mtd =
{
  .mtd =
    {
      .erase  = stm32u5x9j_xip_erase,
      .bread  = stm32u5x9j_xip_bread,
      .bwrite = stm32u5x9j_xip_bwrite,
      .read   = stm32u5x9j_xip_read,
      .ioctl  = stm32u5x9j_xip_ioctl,
      .name   = "stm32u5x9j-ospi1-nor",
    },
  .base = STM32U5X9J_OSPI1_NOR_MEM_BASE,
  .size = STM32U5X9J_OSPI1_NOR_SIZE,
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_STM32U5X9J_DK_OSPI_MTD
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
  return -EROFS;
}

static ssize_t stm32u5x9j_xip_bwrite(FAR struct mtd_dev_s *dev,
                                     off_t startblock, size_t nblocks,
                                     FAR const uint8_t *buffer)
{
  return -EROFS;
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
        return -EROFS;

      default:
        return -ENOTTY;
    }
}
#endif /* CONFIG_STM32U5X9J_DK_OSPI_MTD */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32u5x9j_extmem_initialize(void)
{
  int ret;

#ifdef CONFIG_STM32U5X9J_DK_OSPI_NOR
  ret = stm32u5x9j_ospi1_nor_initialize();
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_STM32U5X9J_DK_OSPI_MTD
  ret = register_mtddriver("/dev/mtd0", &g_ospi_mtd.mtd, 0777, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: register /dev/mtd0 failed: %d\n", ret);
      return ret;
    }
#endif

#ifdef CONFIG_STM32U5X9J_DK_OSPI_DIAG
  ret = stm32u5x9j_ospi_diag_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: register /dev/ospi0 failed: %d\n", ret);
      return ret;
    }
#endif
#endif

#ifdef CONFIG_STM32U5X9J_DK_HSPI_RAM
  ret = stm32u5x9j_hspi1_psram_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: HSPI1 PSRAM init failed: %d\n", ret);
      return ret;
    }
#endif

  return OK;
}

#endif /* OSPI NOR || HSPI RAM */
