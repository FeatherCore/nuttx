/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/scripts/nxboot.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#include <nuttx/config.h>

#if CONFIG_NXBOOT_HEADER_SIZE != 0x400
#  error "CONFIG_NXBOOT_HEADER_SIZE must be 0x400"
#endif

MEMORY
{
  ram (rwx) : ORIGIN = 0x34180400, LENGTH = 511K
}

OUTPUT_ARCH(arm)
EXTERN(_vectors)
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
    } > ram

    .init_section :
    {
        _sinit = ABSOLUTE(.);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array EXCLUDE_FILE(*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o) .ctors))
        _einit = ABSOLUTE(.);
    } > ram

    .ARM.extab :
    {
        *(.ARM.extab*)
    } > ram

    __exidx_start = ABSOLUTE(.);
    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } > ram
    __exidx_end = ABSOLUTE(.);

    .ramfunc :
    {
        _sramfuncs = ABSOLUTE(.);
        *(.ramfunc .ramfunc.*)
        . = ALIGN(4);
        _eramfuncs = ABSOLUTE(.);
    } > ram

    _framfuncs = LOADADDR(.ramfunc);

    .data :
    {
        _sdata = ABSOLUTE(.);
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        CONSTRUCTORS
        . = ALIGN(4);
        _edata = ABSOLUTE(.);
    } > ram

    _eronly = _sdata;

    .bss :
    {
        _sbss = ABSOLUTE(.);
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(8);
        _ebss = ABSOLUTE(.);
    } > ram

    .stab 0 : { *(.stab) }
    .stabstr 0 : { *(.stabstr) }
    .comment 0 : { *(.comment) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_info 0 : { *(.debug_info) }
    .debug_line 0 : { *(.debug_line) }
}
