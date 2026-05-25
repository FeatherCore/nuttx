/****************************************************************************
 * boards/arm/stm32u5/stm32u5x9j-dk/scripts/memory.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#include <nuttx/config.h>

/* Protected internal-Flash layout without NXboot:
 *
 *   0x08000000..0x0807ffff  kernel blob, 512 KiB window
 *   0x08080000..            user blob, begins with struct userspace_s
 *
 * SRAM1/2/3/5 are contiguous on STM32U5x9J.  The lower window holds kernel
 * state and kernel heap; the top internal window holds protected user
 * .data/.bss and a small bootstrap user heap.  If the LCD framebuffer is
 * backed by internal SRAM, the framebuffer window sits between the kernel
 * heap and protected user SRAM.  The runtime user heap is extended with HSPI1
 * PSRAM from board-specific heap allocation code.
 */

#if CONFIG_NUTTX_USERSPACE != 0x08080000
#  error "CONFIG_NUTTX_USERSPACE must be 0x08080000 to match memory.ld"
#endif

#if CONFIG_RAM_START != 0x20000000
#  error "CONFIG_RAM_START must be 0x20000000"
#endif

#if defined(CONFIG_STM32U5X9J_DK_LCD_FB_SRAM)
/* Internal-SRAM framebuffer mode stores the compact GFXMMU circular backing
 * below protected user SRAM.  PSRAM framebuffer mode leaves this linker region
 * out entirely and reserves its linear display window from the PSRAM heap code.
 */
#  ifdef CONFIG_STM32U5X9J_DK_LCD_XRGB8888
#    define BOARD_LCD_FB_SRAM_SIZE 0x164180
#  else
#    define BOARD_LCD_FB_SRAM_SIZE 0x0b3b00
#  endif
#  define BOARD_LCD_FB_SRAM_BASE \
          (CONFIG_STM32U5X9J_PROTECTED_USRAM_BASE - \
           BOARD_LCD_FB_SRAM_SIZE)
#  if BOARD_LCD_FB_SRAM_BASE != CONFIG_RAM_START + CONFIG_RAM_SIZE
#    error "CONFIG_RAM_START + CONFIG_RAM_SIZE must end at the internal LCD framebuffer base"
#  endif
#else
#  if CONFIG_STM32U5X9J_PROTECTED_USRAM_BASE != CONFIG_RAM_START + CONFIG_RAM_SIZE
#    error "CONFIG_RAM_START + CONFIG_RAM_SIZE must end at CONFIG_STM32U5X9J_PROTECTED_USRAM_BASE"
#  endif
#endif

MEMORY
{
  kflash (rx)  : ORIGIN = 0x08000000, LENGTH =   512K
  uflash (rx)  : ORIGIN = 0x08080000, LENGTH =  3584K
  ksram  (rwx) : ORIGIN = CONFIG_RAM_START, LENGTH = CONFIG_RAM_SIZE
#if defined(CONFIG_STM32U5X9J_DK_LCD_FB_SRAM)
  fbmem  (rwx) : ORIGIN = BOARD_LCD_FB_SRAM_BASE, LENGTH = BOARD_LCD_FB_SRAM_SIZE
#endif
  usram  (rwx) : ORIGIN = CONFIG_STM32U5X9J_PROTECTED_USRAM_BASE, LENGTH = CONFIG_STM32U5X9J_PROTECTED_USRAM_SIZE
  psram  (rwx) : ORIGIN = 0xa0000000, LENGTH =    64M
}
