# STM32N6570-DK

This board port starts the STM32N6570-DK bring-up with an NXboot boot model.

The STM32N6 BootROM still requires an ST trusted FSBL image header at
`0x70000000`.  The payload behind that header is a small NuttX/NXboot image
linked at `0x34180400`.  That NXboot image initializes board resources and
loads an NXboot application image from XSPI2 NOR.

Initial configurations:

- `stm32n6570-dk:nxboot`
- `stm32n6570-dk:nsh`
- `stm32n6570-dk:knsh`

Planned image layout:

- `0x70000000`: ST BootROM header + NuttX/NXboot payload
- `0x70100000`: primary NXboot app image
- `0x70100400`: app vector table and text
- `0x70180400`: protected user-space text/data for `knsh`

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

Bring-up status, 2026-05-15:

- NXboot initializes power, clocks, XSPI2 NOR, and XSPI1 PSRAM, then boots an
  NXboot app image from `/dev/ota0`.
- The clock tree follows the CubeN6 800 MHz FSBL profile: PF4 selects SMPS
  overdrive, VOS scale 0 is requested, PLL1 runs at 800 MHz, SYS/AXI remains
  400 MHz, and HCLK/PCLK remain 200 MHz.
- XSPI2 NOR is the MX66UW1G45G-compatible device on the DK board.  The current
  initialization reads JEDEC ID `c2 81 3b`, switches to OPI/DTR mode, maps NOR
  at `0x70000000`, and validates an NXboot app image at `0x70100000`.
- XSPI1 PSRAM is the APS256-compatible x16 device.  The current stable
  bring-up sequence follows the STM32CubeN6 DK BSP settings: MR0 `0x30`
  (read latency 7, fixed latency), MR4 `0x20` (write latency 7), MR8 `0x40`
  (x16 mode), linear-burst read command `0x20`, linear-burst write command
  `0xa0`, FIFO threshold 8 bytes, and refresh enabled.
- PSRAM register setup and self-test run at 50 MHz.  The final memory-mapped
  mode targets 200 MHz when the VDDIO2 HSLV fuse is set.  If HSLV is missing,
  NXboot keeps the conservative 50 MHz mapping instead of burning OTP or
  forcing an unsafe high-speed mode.
- In protected `knsh`, internal SRAM below the protected user SRAM window is
  used as the kernel heap.  User space starts with a small bootstrap heap in
  user SRAM, then adds the full 32 MiB PSRAM window at `0x90000000` as user
  heap.
- The protected KNSh path reached the `NuttShell (NSH)` prompt with the 50 MHz
  PSRAM mapping.  The 800 MHz CPU / 200 MHz PSRAM profile is the current target
  and should be stress-tested on hardware.

Expected high-speed KNSh boot checkpoints:

```text
stm32n6: clock CPU=800000000 SYS=400000000 HCLK=200000000 PCLK1=200000000 PCLK2=200000000
XSPI2 NOR JEDEC ID c2 81 3b
XSPI2 NOR mapped 0x70000000 ota0[0]=0x534f584e
XSPI1 PSRAM startup prescaler=3 effective=50000000Hz refresh=96
XSPI1 PSRAM MR00000000 initial 08 write 30 readback 30
XSPI1 PSRAM optional prescaler=0 effective=200000000Hz refresh=396
XSPI1 PSRAM self-test passed
Boot vector msp=0x34003400 reset=0x70101861 vtor=0x70100400
stm32n6: added PSRAM heap base=0x90000000 size=0x02000000
NuttShell (NSH)
nsh>
```
