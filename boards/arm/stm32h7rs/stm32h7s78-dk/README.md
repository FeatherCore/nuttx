# STM32H7S78-DK

This is the initial NuttX board directory for the STMicroelectronics
STM32H7S78-DK discovery kit.

Reference material:

```text
/home/uan-wsl2/third/STM32CubeH7RS/Projects/STM32H7S78-DK
/home/uan-wsl2/third/STM32CubeH7RS/Drivers/BSP/STM32H7S78-DK
/home/uan-wsl2/third/STM32CubeH7RS/Drivers/CMSIS/Device/ST/STM32H7RSxx
```

The first usable milestone should be a small internal-flash NSH image with
early serial output.  XSPI NOR/PSRAM, LCD, touch, audio, Ethernet and USB-C
power delivery should be enabled only after the SoC clock, GPIO and interrupt
layers are stable.
