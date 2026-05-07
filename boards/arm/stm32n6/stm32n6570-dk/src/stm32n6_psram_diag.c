/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32n6_psram_diag.c
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
#include <string.h>
#include <syslog.h>

#include <nuttx/fs/fs.h>

#include <arch/stm32n6/chip.h>

#include "stm32n6.h"
#include "hardware/stm32n6_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_PSRAM_DIAG_DEVPATH    "/dev/hspiram0"

#ifndef CONFIG_STM32N6_PSRAM_HEAP_OFFSET
#  define CONFIG_STM32N6_PSRAM_HEAP_OFFSET 0x00200000
#endif

#ifndef CONFIG_STM32N6_PSRAM_HEAP_SIZE
#  define CONFIG_STM32N6_PSRAM_HEAP_SIZE 0
#endif

#define STM32N6_PSRAM_HEAP_BASE \
        (STM32N6_XSPI1_MEM_BASE + CONFIG_STM32N6_PSRAM_HEAP_OFFSET)

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ssize_t stm32n6_psram_diag_read(FAR struct file *filep,
                                       FAR char *buffer, size_t buflen);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_psram_diag_fops =
{
  NULL,                    /* open */
  NULL,                    /* close */
  stm32n6_psram_diag_read, /* read */
  NULL,                    /* write */
  NULL,                    /* seek */
  NULL,                    /* ioctl */
  NULL,                    /* mmap */
  NULL,                    /* truncate */
  NULL,                    /* poll */
  NULL,                    /* readv */
  NULL,                    /* writev */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static ssize_t stm32n6_psram_diag_read(FAR struct file *filep,
                                       FAR char *buffer, size_t buflen)
{
  char text[256];
  ssize_t nread;
  int nbytes;
  int initret;
  int testret;

  initret = stm32n6_xspi1_psram_initialize();
  testret = initret < 0 ? initret :
            stm32n6_xspi1_psram_selftest(STM32N6_XSPI1_MEM_BASE);

  nbytes = snprintf(text, sizeof(text),
                    "base=0x%08" PRIx32 "\n"
                    "size=0x%08x\n"
                    "heap_base=0x%08x\n"
                    "heap_size=0x%08x\n"
                    "mapped=%s\n"
                    "selftest=%s ret=%d\n",
                    (uint32_t)STM32N6_XSPI1_MEM_BASE,
                    STM32N6_XSPI1_PSRAM_SIZE,
                    STM32N6_PSRAM_HEAP_BASE,
                    CONFIG_STM32N6_PSRAM_HEAP_SIZE,
                    initret < 0 ? "no" : "yes",
                    testret < 0 ? "fail" : "pass", testret);
  if (nbytes < 0)
    {
      return nbytes;
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

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6_psram_diag_register(void)
{
  int ret;

  ret = register_driver(STM32N6_PSRAM_DIAG_DEVPATH, &g_psram_diag_fops,
                        0444, NULL);
  if (ret < 0 && ret != -EEXIST)
    {
      syslog(LOG_ERR, "stm32n6: failed to register %s: %d\n",
             STM32N6_PSRAM_DIAG_DEVPATH, ret);
      return ret;
    }

  syslog(LOG_INFO, "stm32n6: registered %s%s\n",
         STM32N6_PSRAM_DIAG_DEVPATH,
         ret == -EEXIST ? " (already exists)" : "");
  return OK;
}
