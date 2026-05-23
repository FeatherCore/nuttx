/****************************************************************************
 * boards/arm/stm32n6/stm32n6570-dk/scripts/user-space.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/* NOTE: This depends on memory.ld having been included first. */

#include <nuttx/config.h>

OUTPUT_ARCH(arm)
SECTIONS
{
    .userspace :
    {
        *(.userspace)
    } > uflash

    .text :
    {
        _stext = ABSOLUTE(.);
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
    } > uflash

    .init_section :
    {
        _sinit = ABSOLUTE(.);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array EXCLUDE_FILE(*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o) .ctors))
        _einit = ABSOLUTE(.);
    } > uflash

    .ARM.extab :
    {
        *(.ARM.extab*)
    } > uflash

    __exidx_start = ABSOLUTE(.);
    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } > uflash
    __exidx_end = ABSOLUTE(.);

    _eronly = ABSOLUTE(.);

    .data :
    {
        _sdata = ABSOLUTE(.);
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        CONSTRUCTORS
        . = ALIGN(4);
        _edata = ABSOLUTE(.);
    } > usram AT > uflash

    .bss :
    {
        _sbss = ABSOLUTE(.);
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(8);

        /* Protected-user bootstrap heap.  STM32N6570-DK keeps the initial
         * protected user heap in internal SRAM so early allocator metadata
         * and locks are not placed in memory-mapped PSRAM during nx_start().
         * Board bring-up later appends PSRAM as an additional user heap
         * region.
         */

        _s_uheap_boot = ABSOLUTE(.);
        . += CONFIG_STM32N6_PROTECTED_UHEAP_SIZE;
        . = ALIGN(8);
        _e_uheap_boot = ABSOLUTE(.);

        _ebss = ABSOLUTE(.);
    } > usram

    .stab 0 : { *(.stab) }
    .stabstr 0 : { *(.stabstr) }
    .stab.excl 0 : { *(.stab.excl) }
    .stab.exclstr 0 : { *(.stab.exclstr) }
    .stab.index 0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment 0 : { *(.comment) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_info 0 : { *(.debug_info) }
    .debug_line 0 : { *(.debug_line) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    .debug_aranges 0 : { *(.debug_aranges) }
}
