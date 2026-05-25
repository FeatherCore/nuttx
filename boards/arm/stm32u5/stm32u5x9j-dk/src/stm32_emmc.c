/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/src/stm32_emmc.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <syslog.h>

#include <nuttx/fs/fs.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/mmcsd.h>
#include <nuttx/sdio.h>

#include "stm32_gpio.h"
#include "stm32_sdmmc.h"
#include "stm32u5x9j-dk.h"

#include <arch/board/board.h>

#ifdef CONFIG_STM32U5X9J_DK_EMMC

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BOARD_EMMC_NODE       "/dev/mmcsd0"
#define BOARD_EMMC_INFO_NODE  "/dev/emmcinfo0"
#define BOARD_EMMC_BLOCK0_LEN 512

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct sdio_dev_s *g_emmc_sdio;

#ifdef CONFIG_STM32U5X9J_DK_EMMC_INFO
static bool g_emmc_info_registered;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_STM32U5X9J_DK_EMMC_INFO
static ssize_t stm32_emmcinfo_copyout(FAR struct file *filep,
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

static ssize_t stm32_emmcinfo_read(FAR struct file *filep,
                                        FAR char *buffer, size_t buflen)
{
  struct geometry geo;
  struct file block;
  uint8_t block0[BOARD_EMMC_BLOCK0_LEN];
  char text[512];
  uint32_t checksum = 0;
  ssize_t nread = -ENODEV;
  int geo_ret = -ENODEV;
  int open_ret;
  int close_ret = OK;
  int i;
  int len;

  memset(&geo, 0, sizeof(geo));
  memset(block0, 0, sizeof(block0));

  open_ret = file_open(&block, BOARD_EMMC_NODE, O_RDONLY);
  if (open_ret >= 0)
    {
      geo_ret = file_ioctl(&block, BIOC_GEOMETRY,
                           (unsigned long)(uintptr_t)&geo);
      nread = file_read(&block, block0, sizeof(block0));
      if (nread > 0)
        {
          for (i = 0; i < nread; i++)
            {
              checksum += block0[i];
            }
        }

      close_ret = file_close(&block);
    }

  len = snprintf(text, sizeof(text),
                 "emmc node=%s registered=%u\n"
                 "open=%d geometry=%d available=%u changed=%u writable=%u\n"
                 "sectors=%" PRIu64 " sector_size=%lu bytes=%" PRIu64
                 " model=%s\n"
                 "block0_read=%ld checksum=0x%08" PRIx32
                 " first16=%02x %02x %02x %02x %02x %02x %02x %02x "
                 "%02x %02x %02x %02x %02x %02x %02x %02x close=%d\n"
                 "mount=vfat:/mnt/emmc mount_only=1 idma=enabled\n",
                 BOARD_EMMC_NODE, g_emmc_sdio != NULL,
                 open_ret, geo_ret, geo.geo_available,
                 geo.geo_mediachanged, geo.geo_writeenabled,
                 (uint64_t)geo.geo_nsectors,
                 (unsigned long)geo.geo_sectorsize,
                 (uint64_t)geo.geo_nsectors * geo.geo_sectorsize,
                 geo.geo_model,
                 (long)nread, checksum,
                 block0[0], block0[1], block0[2], block0[3],
                 block0[4], block0[5], block0[6], block0[7],
                 block0[8], block0[9], block0[10], block0[11],
                 block0[12], block0[13], block0[14], block0[15],
                 close_ret);

  if (len < 0)
    {
      return len;
    }

  return stm32_emmcinfo_copyout(filep, buffer, buflen, text);
}

static const struct file_operations g_emmcinfo_fops =
{
  NULL,                         /* open */
  NULL,                         /* close */
  stm32_emmcinfo_read,     /* read */
};

static int stm32_emmcinfo_register(void)
{
  int ret;

  if (g_emmc_info_registered)
    {
      return OK;
    }

  ret = register_driver(BOARD_EMMC_INFO_NODE, &g_emmcinfo_fops,
                        0444, NULL);
  if (ret < 0)
    {
      return ret;
    }

  g_emmc_info_registered = true;
  syslog(LOG_INFO, "stm32u5x9j: eMMC read-only diagnostic at %s\n",
         BOARD_EMMC_INFO_NODE);

  return OK;
}
#endif

static void stm32_emmc_gpio_config(void)
{
  stm32_configgpio(GPIO_SDMMC1_CK);
  stm32_configgpio(GPIO_SDMMC1_CMD);
  stm32_configgpio(GPIO_SDMMC1_D0);
  stm32_configgpio(GPIO_SDMMC1_D1);
  stm32_configgpio(GPIO_SDMMC1_D2);
  stm32_configgpio(GPIO_SDMMC1_D3);
  stm32_configgpio(GPIO_SDMMC1_D4);
  stm32_configgpio(GPIO_SDMMC1_D5);
  stm32_configgpio(GPIO_SDMMC1_D6);
  stm32_configgpio(GPIO_SDMMC1_D7);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_emmc_initialize(void)
{
  int ret;

  stm32_emmc_gpio_config();

  ret = stm32_sdmmc1_enable();
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: SDMMC1 clock enable failed: %d\n", ret);
      return ret;
    }

  g_emmc_sdio = sdio_initialize(0);
  if (g_emmc_sdio == NULL)
    {
      syslog(LOG_ERR, "stm32u5x9j: SDMMC1 lower-half init failed\n");
      return -ENODEV;
    }

  ret = mmcsd_slotinitialize(0, g_emmc_sdio);
  if (ret < 0)
    {
      syslog(LOG_ERR, "stm32u5x9j: mmcsd_slotinitialize failed: %d\n",
             ret);
      return ret;
    }

  sdio_mediachange(g_emmc_sdio, true);
  syslog(LOG_INFO,
         "stm32u5x9j: eMMC registered at %s bus=8-bit idma=enabled\n",
         BOARD_EMMC_NODE);

#ifdef CONFIG_STM32U5X9J_DK_EMMC_INFO
  ret = stm32_emmcinfo_register();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: register %s failed: %d\n",
             BOARD_EMMC_INFO_NODE, ret);
      return ret;
    }
#endif

#ifdef CONFIG_STM32U5X9J_DK_EMMC_AUTOMOUNT
  mkdir("/mnt/emmc", 0777);
  ret = mount(BOARD_EMMC_NODE, "/mnt/emmc", "vfat", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_WARNING,
             "stm32u5x9j: %s vfat mount skipped: %d\n",
             BOARD_EMMC_NODE,
             -errno);
    }
  else
    {
      syslog(LOG_INFO, "stm32u5x9j: mounted %s at /mnt/emmc\n",
             BOARD_EMMC_NODE);
    }
#endif

  return OK;
}

#endif /* CONFIG_STM32U5X9J_DK_EMMC */
