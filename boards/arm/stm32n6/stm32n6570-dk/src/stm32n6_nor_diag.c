/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32n6_nor_diag.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/param.h>
#include <sys/types.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <nuttx/fs/fs.h>
#include <nuttx/kmalloc.h>

#include <arch/stm32n6/chip.h>

#include "stm32n6.h"
#include "hardware/stm32n6_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_NOR_DIAG_DEVPATH       "/dev/nordiag0"
#define STM32N6_NOR_SECTOR_SIZE        0x1000u
#define STM32N6_NOR_PATTERN_SIZE       256u

#ifndef CONFIG_STM32N6_EXTNOR_SCRATCH_OFFSET
#  define CONFIG_STM32N6_EXTNOR_SCRATCH_OFFSET 0x07ff0000
#endif

#define STM32N6_NOR_TAIL_RESERVE_OFFSET \
        (CONFIG_STM32N6_OTA_TERTIARY_SLOT_OFFSET + \
         CONFIG_STM32N6_OTA_SLOT_SIZE)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_nor_slot_s
{
  FAR const char *name;
  FAR const char *devpath;
  uint32_t        offset;
  uint32_t        size;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ssize_t stm32n6_nor_diag_read(FAR struct file *filep,
                                     FAR char *buffer, size_t buflen);
static ssize_t stm32n6_nor_diag_write(FAR struct file *filep,
                                      FAR const char *buffer,
                                      size_t buflen);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_nor_diag_fops =
{
  NULL,                   /* open */
  NULL,                   /* close */
  stm32n6_nor_diag_read,  /* read */
  stm32n6_nor_diag_write, /* write */
  NULL,                   /* seek */
  NULL,                   /* ioctl */
  NULL,                   /* mmap */
  NULL,                   /* truncate */
  NULL,                   /* poll */
  NULL,                   /* readv */
  NULL,                   /* writev */
};

static const struct stm32n6_nor_slot_s g_nor_slots[] =
{
  {
    .name    = "ota0",
    .devpath = CONFIG_STM32N6_OTA_PRIMARY_SLOT_DEVPATH,
    .offset  = CONFIG_STM32N6_OTA_PRIMARY_SLOT_OFFSET,
    .size    = CONFIG_STM32N6_OTA_SLOT_SIZE,
  },
  {
    .name    = "ota1",
    .devpath = CONFIG_STM32N6_OTA_SECONDARY_SLOT_DEVPATH,
    .offset  = CONFIG_STM32N6_OTA_SECONDARY_SLOT_OFFSET,
    .size    = CONFIG_STM32N6_OTA_SLOT_SIZE,
  },
  {
    .name    = "ota2",
    .devpath = CONFIG_STM32N6_OTA_TERTIARY_SLOT_DEVPATH,
    .offset  = CONFIG_STM32N6_OTA_TERTIARY_SLOT_OFFSET,
    .size    = CONFIG_STM32N6_OTA_SLOT_SIZE,
  },
};

static char g_nor_diag_last[96] = "not-run";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32n6_nor_current_slot(void)
{
  uintptr_t pc = (uintptr_t)stm32n6_nor_diag_read & ~(uintptr_t)1;
  uint32_t offset;
  int i;

  if (pc < STM32N6_XSPI2_MEM_BASE ||
      pc >= STM32N6_XSPI2_MEM_BASE + STM32N6_XSPI2_NOR_SIZE)
    {
      return -1;
    }

  offset = pc - STM32N6_XSPI2_MEM_BASE;
  for (i = 0; i < nitems(g_nor_slots); i++)
    {
      if (offset >= g_nor_slots[i].offset &&
          offset < g_nor_slots[i].offset + g_nor_slots[i].size)
        {
          return i;
        }
    }

  return -1;
}

static FAR const char *stm32n6_nor_current_slot_name(void)
{
  int slot = stm32n6_nor_current_slot();

  return slot < 0 ? "unknown" : g_nor_slots[slot].name;
}

#ifdef CONFIG_STM32N6_EXTNOR_SCRATCH_TEST
static bool stm32n6_nor_range_overlap(uint32_t aoffset, uint32_t asize,
                                      uint32_t boffset, uint32_t bsize)
{
  return aoffset < boffset + bsize && boffset < aoffset + asize;
}

static int stm32n6_nor_validate_scratch(uint32_t offset)
{
  int slot = stm32n6_nor_current_slot();

  if ((offset % STM32N6_NOR_SECTOR_SIZE) != 0 ||
      offset < STM32N6_NOR_TAIL_RESERVE_OFFSET ||
      offset > STM32N6_XSPI2_NOR_SIZE - STM32N6_NOR_SECTOR_SIZE)
    {
      return -EINVAL;
    }

  if (slot >= 0 &&
      stm32n6_nor_range_overlap(offset, STM32N6_NOR_SECTOR_SIZE,
                                g_nor_slots[slot].offset,
                                g_nor_slots[slot].size))
    {
      return -EPERM;
    }

  return OK;
}

static int stm32n6_nor_restore_sector(uint32_t offset, FAR uint8_t *saved)
{
  int ret;

  ret = stm32n6_xspi2_nor_erase(offset, STM32N6_NOR_SECTOR_SIZE);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32n6_xspi2_nor_write(offset, saved,
                                STM32N6_NOR_SECTOR_SIZE);
  if (ret < 0)
    {
      return ret;
    }

  if (memcmp((FAR const void *)(STM32N6_XSPI2_MEM_BASE + offset),
             saved, STM32N6_NOR_SECTOR_SIZE) != 0)
    {
      return -EIO;
    }

  return OK;
}

static int stm32n6_nor_scratch_test(uint32_t offset)
{
  FAR const uint8_t *mapped =
    (FAR const uint8_t *)(STM32N6_XSPI2_MEM_BASE + offset);
  FAR uint8_t *saved;
  uint8_t pattern[STM32N6_NOR_PATTERN_SIZE];
  int restore = OK;
  int ret;
  int i;

  ret = stm32n6_nor_validate_scratch(offset);
  if (ret < 0)
    {
      snprintf(g_nor_diag_last, sizeof(g_nor_diag_last),
               "scratch rejected offset=0x%08" PRIx32 " ret=%d",
               offset, ret);
      return ret;
    }

  saved = kmm_malloc(STM32N6_NOR_SECTOR_SIZE);
  if (saved == NULL)
    {
      return -ENOMEM;
    }

  memcpy(saved, mapped, STM32N6_NOR_SECTOR_SIZE);

  for (i = 0; i < STM32N6_NOR_PATTERN_SIZE; i++)
    {
      pattern[i] = (uint8_t)(0xa5u ^ i);
    }

  ret = stm32n6_xspi2_nor_erase(offset, STM32N6_NOR_SECTOR_SIZE);
  if (ret < 0)
    {
      goto out;
    }

  ret = stm32n6_xspi2_nor_write(offset, pattern,
                                STM32N6_NOR_PATTERN_SIZE);
  if (ret < 0)
    {
      goto restore;
    }

  if (memcmp(mapped, pattern, STM32N6_NOR_PATTERN_SIZE) != 0)
    {
      ret = -EIO;
    }

restore:
  restore = stm32n6_nor_restore_sector(offset, saved);

out:
  if (ret < 0 || restore < 0)
    {
      snprintf(g_nor_diag_last, sizeof(g_nor_diag_last),
               "scratch fail offset=0x%08" PRIx32 " ret=%d restore=%d",
               offset, ret, restore);
    }
  else
    {
      snprintf(g_nor_diag_last, sizeof(g_nor_diag_last),
               "scratch pass offset=0x%08" PRIx32, offset);
    }

  kmm_free(saved);
  return ret < 0 ? ret : restore;
}
#endif

static ssize_t stm32n6_nor_diag_read(FAR struct file *filep,
                                     FAR char *buffer, size_t buflen)
{
  static char text[1024];
  struct stm32n6_xspi2_nor_info_s info;
  FAR const char *jedec_source;
  FAR const char *hslv2;
  FAR const char *hslv3;
  ssize_t nread;
  int nbytes;
  int ret;

  ret = stm32n6_xspi2_nor_getinfo(&info);
  if (ret < 0)
    {
      return ret;
    }

  jedec_source = info.jedec_valid ? "probed" : "expected";
  hslv2 = !info.common_ready ? "unknown" :
          (info.vddio2_hslv ? "enabled" : "disabled");
  hslv3 = !info.common_ready ? "unknown" :
          (info.vddio3_hslv ? "enabled" : "disabled");

  nbytes = snprintf(text, sizeof(text),
                    "role=xip-ota-image-space\n"
                    "xip_base=0x%08" PRIx32 "\n"
                    "size=0x%08" PRIx32 "\n"
                    "erase_size=0x%08" PRIx32 "\n"
                    "page_size=0x%08" PRIx32 "\n"
                    "jedec=%02x %02x %02x (%s)\n"
                    "mapped=%s\n"
                    "xspi2_source=%" PRIu32 "\n"
                    "xspi2_effective=%" PRIu32 "\n"
                    "vddio2_hslv=%s\n"
                    "vddio3_hslv=%s\n"
                    "current_slot=%s\n"
                    "slot0=%s offset=0x%08" PRIx32
                    " size=0x%08" PRIx32 "\n"
                    "slot1=%s offset=0x%08" PRIx32
                    " size=0x%08" PRIx32 "\n"
                    "slot2=%s offset=0x%08" PRIx32
                    " size=0x%08" PRIx32 "\n"
                    "tail_reserve=0x%08x..0x%08x\n"
                    "scratch_default=0x%08x\n"
#ifdef CONFIG_STM32N6_EXTNOR_SCRATCH_TEST
                    "scratch_test=enabled\n"
#else
                    "scratch_test=disabled\n"
#endif
                    "last=%s\n",
                    info.base, info.size, info.erase_size, info.page_size,
                    info.jedec[0], info.jedec[1], info.jedec[2],
                    jedec_source,
                    info.mapped ? "yes" : "no",
                    info.source_hz, info.effective_hz,
                    hslv2, hslv3, stm32n6_nor_current_slot_name(),
                    g_nor_slots[0].devpath, g_nor_slots[0].offset,
                    g_nor_slots[0].size,
                    g_nor_slots[1].devpath, g_nor_slots[1].offset,
                    g_nor_slots[1].size,
                    g_nor_slots[2].devpath, g_nor_slots[2].offset,
                    g_nor_slots[2].size,
                    STM32N6_NOR_TAIL_RESERVE_OFFSET,
                    STM32N6_XSPI2_NOR_SIZE - 1,
                    CONFIG_STM32N6_EXTNOR_SCRATCH_OFFSET,
                    g_nor_diag_last);
  if (nbytes < 0)
    {
      return nbytes;
    }

  if (nbytes >= sizeof(text))
    {
      nbytes = sizeof(text) - 1;
    }

  if (filep->f_pos >= nbytes)
    {
      return 0;
    }

  nread = MIN((ssize_t)buflen, (ssize_t)nbytes - filep->f_pos);
  memcpy(buffer, text + filep->f_pos, nread);
  filep->f_pos += nread;
  return nread;
}

static ssize_t stm32n6_nor_diag_write(FAR struct file *filep,
                                      FAR const char *buffer,
                                      size_t buflen)
{
  char command[64];
  FAR char *endptr;
  uint32_t offset = CONFIG_STM32N6_EXTNOR_SCRATCH_OFFSET;
  size_t ncopy;
  int ret;

  (void)filep;

  ncopy = MIN(buflen, sizeof(command) - 1);
  memcpy(command, buffer, ncopy);
  command[ncopy] = '\0';

  if (strncmp(command, "scratch", 7) != 0)
    {
      return -EINVAL;
    }

  endptr = command + 7;
  while (*endptr == ' ' || *endptr == '\t')
    {
      endptr++;
    }

  if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r')
    {
      FAR char *start = endptr;

      offset = strtoul(endptr, &endptr, 0);
      if (endptr == start)
        {
          return -EINVAL;
        }
    }

#ifdef CONFIG_STM32N6_EXTNOR_SCRATCH_TEST
  ret = stm32n6_nor_scratch_test(offset);
  return ret < 0 ? ret : (ssize_t)buflen;
#else
  snprintf(g_nor_diag_last, sizeof(g_nor_diag_last),
           "scratch disabled offset=0x%08" PRIx32, offset);
  (void)ret;
  return -EROFS;
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6_nor_diag_register(void)
{
  int ret;

  ret = register_driver(STM32N6_NOR_DIAG_DEVPATH, &g_nor_diag_fops,
                        0644, NULL);
  if (ret < 0 && ret != -EEXIST)
    {
      syslog(LOG_ERR, "stm32n6: failed to register %s: %d\n",
             STM32N6_NOR_DIAG_DEVPATH, ret);
      return ret;
    }

  syslog(LOG_INFO, "stm32n6: registered %s%s\n",
         STM32N6_NOR_DIAG_DEVPATH,
         ret == -EEXIST ? " (already exists)" : "");
  return OK;
}
