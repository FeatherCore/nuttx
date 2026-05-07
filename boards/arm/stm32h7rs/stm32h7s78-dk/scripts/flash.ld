/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/scripts/flash.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/* Initial internal-flash linker skeleton derived from the STM32CubeH7RS
 * Template_LL linker memory map:
 *
 *   FLASH: 0x08000000, 64 KiB
 *   RAM:   0x24000000, 456 KiB
 *
 * This script is a porting placeholder.  It will need the final NuttX section
 * layout once the STM32H7RS reset/start code is available.
 */

MEMORY
{
  flash (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  sram  (rwx) : ORIGIN = 0x24000000, LENGTH = 456K
}

ENTRY(__start)

SECTIONS
{
  .text :
  {
    _stext = ABSOLUTE(.);
    *(.vectors)
    *(.text .text.*)
    *(.rodata .rodata.*)
    _etext = ABSOLUTE(.);
  } > flash

  .data :
  {
    _sdata = ABSOLUTE(.);
    *(.data .data.*)
    _edata = ABSOLUTE(.);
  } > sram AT > flash

  .bss :
  {
    _sbss = ABSOLUTE(.);
    *(.bss .bss.*)
    *(COMMON)
    _ebss = ABSOLUTE(.);
  } > sram
}
