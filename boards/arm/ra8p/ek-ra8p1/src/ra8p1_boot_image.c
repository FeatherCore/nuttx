/****************************************************************************
 * boards/arm/ra8p/ek-ra8p1/src/ra8p1_boot_image.c
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
#include <stdbool.h>
#include <string.h>
#include <syslog.h>

#include <nuttx/cache.h>
#include <nuttx/fs/fs.h>

#include <arch/barriers.h>
#include <arch/board/board.h>

#include "arm_internal.h"
#ifdef CONFIG_ARM_MPU
#  include "mpu.h"
#endif
#include "nvic.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct arm_vector_table
{
  uint32_t spr;
  uint32_t reset;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void cleanup_arm_nvic(void)
{
  int i;

  UP_ISB();
  cpsid();

  putreg32(NVIC_INTCTRL_PENDSTCLR | NVIC_INTCTRL_PENDSVCLR, NVIC_INTCTRL);

  for (i = 0; i < NR_IRQS; i += 32)
    {
      putreg32(0xffffffff, NVIC_IRQ_CLEAR(i));
      putreg32(0xffffffff, NVIC_IRQ_CLRPEND(i));
    }
}

static void systick_disable(void)
{
  putreg32(0, NVIC_SYSTICK_CTRL);
  putreg32(NVIC_SYSTICK_RELOAD_MASK, NVIC_SYSTICK_RELOAD);
  putreg32(0, NVIC_SYSTICK_CURRENT);
}

static uintptr_t ra8p1_slot_base(FAR const char *path)
{
#ifdef CONFIG_EK_RA8P1_EXTNOR_OTA_PARTITION
  if (strcmp(path, CONFIG_EK_RA8P1_OTA_PRIMARY_SLOT_DEVPATH) == 0)
    {
      return RA8P_OSPI0_CS1_START +
             CONFIG_EK_RA8P1_OTA_PRIMARY_SLOT_OFFSET;
    }

  if (strcmp(path, CONFIG_EK_RA8P1_OTA_SECONDARY_SLOT_DEVPATH) == 0)
    {
      return RA8P_OSPI0_CS1_START +
             CONFIG_EK_RA8P1_OTA_SECONDARY_SLOT_OFFSET;
    }
#endif

  return 0;
}

static bool ra8p1_address_in_range(uintptr_t address, uintptr_t base,
                                   size_t size)
{
  return address >= base && address < base + size;
}

static bool ra8p1_valid_stack(uint32_t stack)
{
  return ra8p1_address_in_range(stack, RA8P_SRAM_START, RA8P_SRAM_SIZE);
}

static bool ra8p1_valid_reset(uint32_t reset, uintptr_t vtor)
{
  uintptr_t address = reset & ~1u;
  uintptr_t slotend = vtor + CONFIG_EK_RA8P1_OTA_SLOT_SIZE;

  return (reset & 1u) != 0 &&
         address >= vtor &&
         address < slotend;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int board_boot_image(FAR const char *path, uint32_t hdr_size)
{
  static struct arm_vector_table vt;
  uintptr_t vtor;
  struct file file;
  ssize_t bytes;
  int ret;

  ret = file_open(&file, path, O_RDONLY | O_CLOEXEC);
  if (ret < 0)
    {
      syslog(LOG_ERR, "Failed to open %s: %d\n", path, ret);
      return ret;
    }

  bytes = file_pread(&file, &vt, sizeof(vt), hdr_size);
  file_close(&file);

  if (bytes != sizeof(vt))
    {
      syslog(LOG_ERR, "Failed to read ARM vector table: %zd\n", bytes);
      return bytes < 0 ? bytes : -EIO;
    }

  vtor = ra8p1_slot_base(path);
  if (vtor == 0)
    {
      syslog(LOG_ERR, "Unsupported boot slot path: %s\n", path);
      return -EINVAL;
    }

  vtor += hdr_size;

  syslog(LOG_INFO, "Boot vector msp=0x%08" PRIx32
         " reset=0x%08" PRIx32 " vtor=0x%08" PRIxPTR "\n",
         vt.spr, vt.reset, vtor);

  if (!ra8p1_valid_stack(vt.spr) || !ra8p1_valid_reset(vt.reset, vtor))
    {
      syslog(LOG_ERR, "Invalid boot vector table\n");
      return -EINVAL;
    }

  systick_disable();
  cleanup_arm_nvic();

#ifdef CONFIG_ARMV8M_DCACHE
  up_disable_dcache();
#endif
#ifdef CONFIG_ARMV8M_ICACHE
  up_disable_icache();
#endif
#ifdef CONFIG_ARM_MPU
  mpu_control(false, false, false);
#endif

  putreg32(vtor, NVIC_VECTAB);
  UP_DSB();
  UP_ISB();

  __asm__ __volatile__("\tmsr msplim, %0\n"
                       "\tmsr basepri, %0\n"
                       "\tmsr faultmask, %0\n"
                       "\tmsr msp, %1\n"
                       "\tmsr control, %2\n"
                       "\tisb\n"
                       "\tbx %3\n"
                       :
                       : "r" (0), "r" (vt.spr), "r" (0),
                         "r" (vt.reset));

  return 0;
}
