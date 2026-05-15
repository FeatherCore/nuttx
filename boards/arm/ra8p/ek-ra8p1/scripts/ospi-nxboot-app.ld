/****************************************************************************
 * boards/arm/ra8p/ek-ra8p1/scripts/ospi-nxboot-app.ld
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#include <nuttx/config.h>

/* EK-RA8P1 NXboot application:
 *
 *   OSPI0 CS1 NOR slot 0 starts at 0x90000000.
 *   The NuttX application vector table is linked after the NXboot header:
 *   0x90000000 + 0x400.
 */

#if CONFIG_EK_RA8P1_OTA_PRIMARY_SLOT_OFFSET != 0x0
#  error "CONFIG_EK_RA8P1_OTA_PRIMARY_SLOT_OFFSET must be 0x0"
#endif

#if CONFIG_EK_RA8P1_OTA_SLOT_SIZE != 0x02000000
#  error "CONFIG_EK_RA8P1_OTA_SLOT_SIZE must be 0x02000000"
#endif

#if CONFIG_NXBOOT_HEADER_SIZE != 0x400
#  error "CONFIG_NXBOOT_HEADER_SIZE must be 0x400"
#endif

MEMORY
{
  flash (rx)   : ORIGIN = 0x90000400, LENGTH = 32M - 1K
  sram  (rwx)  : ORIGIN = 0x22000000, LENGTH = 1872K
  itcm  (rwx)  : ORIGIN = 0x00000000, LENGTH = 128K
  dtcm  (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K
  sdram (rwx)  : ORIGIN = 0x68000000, LENGTH = 64M
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

    _eronly = ABSOLUTE(.);

    .data :
    {
        _sdata = ABSOLUTE(.);
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        CONSTRUCTORS
        . = ALIGN(4);
        _edata = ABSOLUTE(.);
    } > sram AT > flash

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
