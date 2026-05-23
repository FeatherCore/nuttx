/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/scripts/nxboot-app.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#include <nuttx/config.h>

#if CONFIG_STM32N6_OTA_PRIMARY_SLOT_OFFSET != 0x00100000
#  error "CONFIG_STM32N6_OTA_PRIMARY_SLOT_OFFSET must be 0x00100000"
#endif

#if CONFIG_STM32N6_OTA_SLOT_SIZE != 0x02000000
#  error "CONFIG_STM32N6_OTA_SLOT_SIZE must be 0x02000000"
#endif

#if CONFIG_NXBOOT_HEADER_SIZE != 0x400
#  error "CONFIG_NXBOOT_HEADER_SIZE must be 0x400"
#endif

MEMORY
{
  flash (rx)  : ORIGIN = 0x70100400, LENGTH =   32M - 1M - 1K
  sram  (rwx) : ORIGIN = 0x34000000, LENGTH =    2M
  psram (rwx) : ORIGIN = 0x90000000, LENGTH =   32M
}

OUTPUT_ARCH(arm)
EXTERN(_vectors)
EXTERN(stm32n6570_heap_link)
ENTRY(_stext)
SECTIONS
{
    .text :
    {
        _stext = ABSOLUTE(.);
        *(.vectors)
        *(.text .text.*)
        *(.fixup)
        *(.gnu.warning)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.t.*)
        *(.glue_7)
        *(.glue_7t)
        *(.got)
        *(.gcc_except_table)
        *(.gnu.linkonce.r.*)
        _etext = ABSOLUTE(.);
    } > flash

    .init_section :
    {
        _sinit = ABSOLUTE(.);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array EXCLUDE_FILE(*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o) .ctors))
        _einit = ABSOLUTE(.);
    } > flash

    .ARM.extab :
    {
        *(.ARM.extab*)
    } > flash

    __exidx_start = ABSOLUTE(.);
    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } > flash
    __exidx_end = ABSOLUTE(.);

    .ramfunc :
    {
        _sramfuncs = ABSOLUTE(.);
        *(.ramfunc .ramfunc.*)
        . = ALIGN(4);
        _eramfuncs = ABSOLUTE(.);
    } > sram AT > flash

    _framfuncs = LOADADDR(.ramfunc);

    .data :
    {
        _sdata = ABSOLUTE(.);
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        CONSTRUCTORS
        . = ALIGN(4);
        _edata = ABSOLUTE(.);
    } > sram AT > flash

    _eronly = LOADADDR(.data);

    .bss :
    {
        _sbss = ABSOLUTE(.);
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(8);
        _ebss = ABSOLUTE(.);
    } > sram

    .stab 0 : { *(.stab) }
    .stabstr 0 : { *(.stabstr) }
    .comment 0 : { *(.comment) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_info 0 : { *(.debug_info) }
    .debug_line 0 : { *(.debug_line) }
}
