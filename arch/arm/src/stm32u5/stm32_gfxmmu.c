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
  return stm32_gfxmmu_buffer(0);
}

uintptr_t stm32_gfxmmu_buffer(uint8_t buffer)
{
  return STM32_GFXMMU_VIRTUAL_BUFFERS_BASE + (uintptr_t)buffer * 0x400000;
}

void stm32_gfxmmu_setbuffer(uint8_t buffer, uintptr_t physical)
{
  putreg32(physical, STM32_GFXMMU_B0CR + (uintptr_t)buffer * 4);
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
                              FAR uint32_t *blocks)
{
  uint32_t bytespp = config->bpp / 8;

  *firstblock = 0;
  *blocks     = ((config->width * bytespp) + 15) / 16;

  if (config->circular)
    {
      int32_t radius = config->width / 2;
      int32_t cy     = config->height / 2 - 1;
      uint32_t row   = line;
      int32_t dy     = (int32_t)line - cy;
      uint32_t half;
      uint32_t x0;
      uint32_t x1;

      if (row >= config->height / 2)
        {
          row = config->height - 1 - row;
        }

      dy = (int32_t)row - cy;
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
    }
}

int stm32_gfxmmuinitialize(const struct stm32_gfxmmu_config_s *config)
{
  uint32_t blocks;
  uint32_t blocks_used;
  uint32_t buffer;
  uint32_t frames;
  uint32_t frame_size;
  uint32_t firstblock;
  int32_t line_offset;
  uint32_t line;

  if (config == NULL || config->width == 0 || config->height == 0 ||
      config->stride == 0 ||
      (config->bpp != 16 && config->bpp != 32))
    {
      return -EINVAL;
    }

  if (config->stride < config->width * (config->bpp / 8))
    {
      return -EINVAL;
    }

  frames = config->frames == 0 ? 1 : config->frames;
  frame_size = config->frame_size == 0 ?
               config->stride * config->height : config->frame_size;

  if (frames == 0 || frames > 4 || config->height > 1024)
    {
      return -EINVAL;
    }

  modifyreg32(STM32_RCC_AHB1ENR, 0, RCC_AHB1ENR_GFXMMUEN);

  stm32_gfxmmu_line(config, 0, &firstblock, &blocks);
  if (blocks > 192)
    {
      return -EINVAL;
    }

  putreg32(GFXMMU_CR_192BM, STM32_GFXMMU_CR);
  putreg32(0xffffffff, STM32_GFXMMU_DVR);

  for (buffer = 0; buffer < 4; buffer++)
    {
      stm32_gfxmmu_setbuffer(buffer,
                             buffer < frames ?
                             config->physical + buffer * frame_size : 0);
    }

  blocks_used = 0;
  for (line = 0; line < 1024; line++)
    {
      if (line < config->height)
        {
          stm32_gfxmmu_line(config, line, &firstblock, &blocks);
          if (blocks > 0)
            {
              if (config->circular)
                {
                  line_offset = (int32_t)(blocks_used * 16) -
                                (int32_t)(firstblock * 16);
                }
              else
                {
                  line_offset = line * config->stride;
                }

              putreg32(GFXMMU_LUTL_EN | GFXMMU_LUTL_FVB(firstblock) |
                       GFXMMU_LUTL_LVB(firstblock + blocks - 1),
                       STM32_GFXMMU_LUT(line * 2));
              putreg32(((uint32_t)line_offset) & GFXMMU_LUTH_OFFSET_MASK,
                       STM32_GFXMMU_LUT(line * 2 + 1));
              blocks_used += blocks;
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
