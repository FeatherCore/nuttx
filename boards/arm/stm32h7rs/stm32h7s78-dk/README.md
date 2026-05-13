# STM32H7S78-DK

This board directory targets the STMicroelectronics STM32H7S78-DK discovery
kit.  The current bring-up follows the STM32CubeH7RS
`Templates/Template_XIP_Custom` project: a first-stage image runs from the
64 KiB internal Flash, configures XSPI2 NOR and XSPI1 PSRAM, then boots a
NuttX application linked for XIP from the XSPI2 NOR window.

Reference material:

```text
/home/uan-wsl2/third/STM32CubeH7RS/Projects/STM32H7S78-DK/Templates/Template_XIP_Custom
/home/uan-wsl2/third/STM32CubeH7RS/Drivers/BSP/STM32H7S78-DK
/home/uan-wsl2/third/STM32CubeH7RS/Drivers/CMSIS/Device/ST/STM32H7RSxx
```

## Configurations

`nsh` builds an NXboot application image linked at `0x70000400`, leaving the
first `0x400` bytes in the primary XSPI2 NOR slot for the NXboot header.  It
enables UART4 console, MPU/cache setup, XSPI bring-up, read-only OTA MTD
partitions, and adds XSPI1 PSRAM to the heap after a small guard offset.

`nxboot` builds the internal-Flash loader at `0x08000000`.  It initializes the
same clock tree, MPU/cache policy, UART4, XSPI2 NOR, and XSPI1 PSRAM path used
by the Cube boot template before handing off to the primary NXboot slot.

## Build

```sh
./tools/configure.sh -E stm32h7s78-dk:nxboot
make -j$(nproc)

./tools/configure.sh -E stm32h7s78-dk:nsh
make -j$(nproc)
```

To add an NXboot header to the application binary, use the shared NXboot image
tool from `../apps` with this board's header size and platform identifier:

```sh
python3 ../apps/boot/nxboot/tools/nximage.py \
  --version 0.1.0 \
  --header_size 0x400 \
  --identifier 0x48735378 \
  nuttx.bin \
  ../build/stm32h7s78-dk-nsh-nxboot.bin
```

The Python dependency is listed in
`../apps/boot/nxboot/tools/requirements.txt`.

## Memory Layout

```text
0x08000000  internal Flash   nxboot loader, 64 KiB
0x24000000  AXI SRAM         NuttX RAM, 456 KiB
0x30000000  AHB SRAM         valid boot stack range, 16 KiB
0x70000000  XSPI2 NOR        primary NXboot slot, 32 MiB
0x72000000  XSPI2 NOR        secondary NXboot slot, 32 MiB
0x74000000  XSPI2 NOR        tertiary NXboot slot, 32 MiB
0x90000000  XSPI1 PSRAM      application extra heap, 32 MiB
```

## Board Notes

UART4 on PD1/PD0 is the default console at 115200 8N1.  The Cube template
requires BOOT0 set to SW1 and these option bytes for high-speed XSPI I/O:
`XSPI1_HSLV=1`, `XSPI2_HSLV=1`, and `VDDIO_HSLV=0`.
