# STM32H7RS Porting Notes

This directory is the NuttX SoC-family home for STM32H7RS devices.

STM32H7RS is kept separate from `arch/arm/src/stm32h7` because the ST Cube
package provides an independent `STM32H7RSxx` CMSIS device layer and
`STM32H7RSxx_HAL_Driver`.  The H7RS register map, boot model, flash size and
external-memory bring-up assumptions should be validated from the H7RS headers
instead of being inherited blindly from the older STM32H7 implementation.

Initial target board:

```text
boards/arm/stm32h7rs/stm32h7s78-dk
```

Reference package:

```text
/home/uan-wsl2/third/STM32CubeH7RS
```
