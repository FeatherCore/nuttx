/****************************************************************************
 * arch/arm/src/stm32u5/stm32_gfxmmu.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "arm_internal.h"
#include "stm32_gfxmmu.h"

#include "hardware/stm32_gfxmmu.h"
#include "hardware/stm32u5xx_rcc.h"

#ifdef CONFIG_STM32U5_GFXMMU

/****************************************************************************
 * Public Functions
 ****************************************************************************/

uintptr_t stm32_gfxmmu_buffer0(void)
{
  return STM32_GFXMMU_VIRTUAL_BUFFER0_BASE;
}

static uint32_t stm32_gfxmmu_isqrt(uint32_t value)
{
  uint32_t bit = 1u << 30;
  uint32_t res = 0;

  while (bit > value)
    {
      bit >>= 2;
    }

  while (bit != 0)
    {
      if (value >= res + bit)
        {
          value -= res + bit;
          res = (res >> 1) + bit;
        }
      else
        {
          res >>= 1;
        }

      bit >>= 2;
    }

  return res;
}

static void stm32_gfxmmu_line(const struct stm32_gfxmmu_config_s *config,
                              uint32_t line, FAR uint32_t *firstblock,
                              FAR uint32_t *blocks,
                              FAR uint32_t *offset)
{
  uint32_t bytespp = config->bpp / 8;

  *firstblock = 0;
  *blocks     = ((config->width * bytespp) + 15) / 16;
  *offset     = line * config->stride;

  if (config->circular)
    {
      int32_t radius = config->width / 2;
      int32_t cy     = config->height / 2;
      int32_t dy     = (int32_t)line - cy;
      uint32_t half;
      uint32_t x0;
      uint32_t x1;

      if (dy < -radius || dy >= radius)
        {
          *blocks = 0;
          return;
        }

      half = stm32_gfxmmu_isqrt((radius * radius) - (dy * dy));
      x0   = radius - half;
      x1   = radius + half;

      if (x1 > config->width)
        {
          x1 = config->width;
        }

      *firstblock = (x0 * bytespp) / 16;
      *blocks     = (((x1 * bytespp) + 15) / 16) - *firstblock;
      *offset     = line * config->stride + *firstblock * 16;
    }
}

int stm32_gfxmmuinitialize(const struct stm32_gfxmmu_config_s *config)
{
  uint32_t blocks;
  uint32_t firstblock;
  uint32_t offset;
  uint32_t line;

  if (config == NULL || config->width == 0 || config->height == 0 ||
      config->stride == 0 || config->bpp != 32)
    {
      return -EINVAL;
    }

  modifyreg32(STM32_RCC_AHB1ENR, 0, RCC_AHB1ENR_GFXMMUEN);

  stm32_gfxmmu_line(config, 0, &firstblock, &blocks, &offset);
  if (blocks > 192)
    {
      return -EINVAL;
    }

  putreg32(GFXMMU_CR_192BM, STM32_GFXMMU_CR);
  putreg32(0xffffffff, STM32_GFXMMU_DVR);
  putreg32(config->physical, STM32_GFXMMU_B0CR);

  for (line = 0; line < 1024; line++)
    {
      if (line < config->height)
        {
          stm32_gfxmmu_line(config, line, &firstblock, &blocks, &offset);
          if (blocks > 0)
            {
              putreg32(GFXMMU_LUTL_EN | GFXMMU_LUTL_FVB(firstblock) |
                       GFXMMU_LUTL_LVB(firstblock + blocks - 1),
                       STM32_GFXMMU_LUT(line * 2));
              putreg32(offset, STM32_GFXMMU_LUT(line * 2 + 1));
            }
          else
            {
              putreg32(0, STM32_GFXMMU_LUT(line * 2));
              putreg32(0, STM32_GFXMMU_LUT(line * 2 + 1));
            }
        }
      else
        {
          putreg32(0, STM32_GFXMMU_LUT(line * 2));
          putreg32(0, STM32_GFXMMU_LUT(line * 2 + 1));
        }
    }

  return OK;
}

#endif /* CONFIG_STM32U5_GFXMMU */
