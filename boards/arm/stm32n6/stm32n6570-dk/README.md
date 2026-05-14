# STM32N6570-DK

This board port starts the STM32N6570-DK bring-up with an NXboot boot model.

The STM32N6 BootROM still requires an ST trusted FSBL image header at
`0x70000000`.  The payload behind that header is a small NuttX/NXboot image
linked at `0x34180400`.  That NXboot image initializes board resources and
loads an NXboot application image from XSPI2 NOR.

Initial configurations:

- `stm32n6570-dk:nxboot`
- `stm32n6570-dk:nsh`

Planned image layout:

- `0x70000000`: ST BootROM header + NuttX/NXboot payload
- `0x70100000`: primary NXboot app image
- `0x70100400`: app vector table and text

The first code drop wires the N6 SoC family, board configs, USART1 early
console, SysTick, NXboot handoff, and external-memory registration.  The
`arch/arm/src/stm32n6` XSPI code is controller-level only; STM32N6570-DK NOR
and PSRAM device sequences live in the board source.

Current XIP policy:

- XSPI2 NOR is boot/app/OTA image storage, not a default filesystem volume.
- `/dev/mtd0` is the raw whole-device access point.
- `/dev/ota0`, `/dev/ota1`, and `/dev/ota2` are NXboot image slots.
- XSPI1 PSRAM initialization runs a startup self-test before it is used as
  heap.
- Startup never formats, erases, or writes external NOR automatically.
