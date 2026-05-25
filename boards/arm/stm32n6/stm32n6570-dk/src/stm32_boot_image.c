/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/src/stm32_boot_image.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/boardctl.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>

#include <nuttx/cache.h>
#include <nuttx/fs/fs.h>
#include <nuttx/irq.h>

#include <arch/board/board.h>
#include <arch/barriers.h>
#include <arch/stm32n6/chip.h>

#include "arm_internal.h"
#ifdef CONFIG_ARM_MPU
#  include "mpu.h"
#endif
#include "hardware/stm32n6xxx_memorymap.h"
#include "nvic.h"
#include "stm32_lowputc.h"

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

  putreg32(NVIC_INTCTRL_PENDSTCLR | NVIC_INTCTRL_PENDSVCLR,
           NVIC_INTCTRL);

  for (i = 0; i < NR_IRQS; i += 32)
    {
      putreg32(0xffffffff, NVIC_IRQ_CLEAR(i));
    }

  for (i = 0; i < NR_IRQS; i += 32)
    {
      putreg32(0xffffffff, NVIC_IRQ_CLRPEND(i));
    }
}

static void systick_disable(void)
{
  putreg32(0, NVIC_SYSTICK_CTRL);
  putreg32(NVIC_SYSTICK_RELOAD_MASK, NVIC_SYSTICK_RELOAD);
  putreg32(0, NVIC_SYSTICK_CURRENT);
}

static uintptr_t stm32_slot_base(FAR const char *path)
{
  if (strcmp(path, CONFIG_STM32N6_OTA_PRIMARY_SLOT_DEVPATH) == 0)
    {
      return BOARD_XSPI2_NOR_BASE + CONFIG_STM32N6_OTA_PRIMARY_SLOT_OFFSET;
    }

  if (strcmp(path, CONFIG_STM32N6_OTA_SECONDARY_SLOT_DEVPATH) == 0)
    {
      return BOARD_XSPI2_NOR_BASE +
             CONFIG_STM32N6_OTA_SECONDARY_SLOT_OFFSET;
    }

  if (strcmp(path, CONFIG_STM32N6_OTA_TERTIARY_SLOT_DEVPATH) == 0)
    {
      return BOARD_XSPI2_NOR_BASE + CONFIG_STM32N6_OTA_TERTIARY_SLOT_OFFSET;
    }

  return 0;
}

static bool stm32_address_in_range(uintptr_t address, uintptr_t base,
                                     size_t size)
{
  return address >= base && address < base + size;
}

static bool stm32_valid_stack(uint32_t stack)
{
  return stm32_address_in_range(stack, BOARD_AXISRAM_BASE,
                                  BOARD_AXISRAM_SIZE);
}

static bool stm32_valid_reset(uint32_t reset, uintptr_t vtor)
{
  uintptr_t address = reset & ~1u;
  uintptr_t slotend = vtor + CONFIG_STM32N6_OTA_SLOT_SIZE;

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

  vtor = stm32_slot_base(path);
  if (vtor == 0)
    {
      syslog(LOG_ERR, "Unsupported boot slot path: %s\n", path);
      return -EINVAL;
    }

  vtor += hdr_size;

  syslog(LOG_INFO, "Boot vector msp=0x%08" PRIx32
         " reset=0x%08" PRIx32 " vtor=0x%08" PRIxPTR "\n",
         vt.spr, vt.reset, vtor);
  stm32_usart1_wait_txcomplete();

  if (!stm32_valid_stack(vt.spr) || !stm32_valid_reset(vt.reset, vtor))
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

  __asm__ __volatile__("\tmsr psplim, %0\n"
                       "\tmsr msplim, %0\n"
                       "\tmsr basepri, %0\n"
                       "\tmsr faultmask, %0\n"
                       "\tmsr psp, %1\n"
                       "\tmsr msp, %1\n"
                       "\tmsr control, %2\n"
                       "\tisb\n"
                       "\tbx %3\n"
                       :
                       : "r" (0), "r" (vt.spr), "r" (0),
                         "r" (vt.reset));

  return 0;
}
