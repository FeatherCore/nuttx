/****************************************************************************
 * boards/arm/stm32h7rs/stm32h7s78-dk/src/stm32_spi.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <nuttx/arch.h>
#include <nuttx/spi/spi.h>

#include "arm_internal.h"
#include "hardware/stm32_gpio.h"
#include "hardware/stm32_rcc.h"
#include "stm32.h"
#include "stm32_spi.h"

#ifdef CONFIG_WL_ESP_HOSTED_NG
extern void esp_hosted_ng_board_spi_ready(void);
bool esp_hosted_ng_board_handshake_ready(void);
bool esp_hosted_ng_board_data_ready(void);
#endif

#ifdef CONFIG_STM32H7RS_SPI4

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Arduino D10 / PF8 is the SPI chip-select GPIO on STM32H7S78-DK. */

#define GPIO_SPI4_ARDUINO_CS_PIN      8
#define GPIO_SPI4_ARDUINO_CS          (1u << GPIO_SPI4_ARDUINO_CS_PIN)
#define GPIO_SPI4_ARDUINO_CS_SET      GPIO_SPI4_ARDUINO_CS
#define GPIO_SPI4_ARDUINO_CS_CLR      (GPIO_SPI4_ARDUINO_CS << 16)

/* Extra ESP-Hosted NG control pins on Arduino digital GPIOs.
 *
 *   D2 / PF1: ESP GPIO3 handshake, input to H7
 *   D4 / PF2: ESP GPIO4 data-ready, input to H7
 *   D7 / PF3: ESP EN/reset, output from H7, active low
 */

#define GPIO_ESP_HANDSHAKE_PIN        1
#define GPIO_ESP_DATA_READY_PIN       2
#define GPIO_ESP_EN_PIN               3
#define GPIO_ESP_HANDSHAKE            (1u << GPIO_ESP_HANDSHAKE_PIN)
#define GPIO_ESP_DATA_READY           (1u << GPIO_ESP_DATA_READY_PIN)
#define GPIO_ESP_EN                   (1u << GPIO_ESP_EN_PIN)
#define GPIO_ESP_EN_SET               GPIO_ESP_EN
#define GPIO_ESP_EN_CLR               (GPIO_ESP_EN << 16)
#define ESP_RESET_ASSERT_MS           100
#define ESP_RESET_BOOT_MS             200

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void stm32_spi4_cs_initialize(void)
{
  uint32_t regval;

  modifyreg32(STM32_RCC_AHB4ENR, 0, RCC_AHB4ENR_GPIOFEN);
  (void)getreg32(STM32_RCC_AHB4ENR);

  putreg32(GPIO_SPI4_ARDUINO_CS_SET, STM32_GPIOF_BSRR);

  regval = getreg32(STM32_GPIOF_MODER);
  regval &= ~GPIO_MODE_MASK(GPIO_SPI4_ARDUINO_CS_PIN);
  regval |= GPIO_MODE_OUTPUT << GPIO_MODE_SHIFT(GPIO_SPI4_ARDUINO_CS_PIN);
  putreg32(regval, STM32_GPIOF_MODER);

  modifyreg32(STM32_GPIOF_OTYPER, GPIO_SPI4_ARDUINO_CS, 0);

  regval = getreg32(STM32_GPIOF_OSPEEDR);
  regval &= ~GPIO_SPEED_MASK(GPIO_SPI4_ARDUINO_CS_PIN);
  regval |= GPIO_SPEED_VERYHIGH <<
            GPIO_SPEED_SHIFT(GPIO_SPI4_ARDUINO_CS_PIN);
  putreg32(regval, STM32_GPIOF_OSPEEDR);

  regval = getreg32(STM32_GPIOF_PUPDR);
  regval &= ~GPIO_PUPD_MASK(GPIO_SPI4_ARDUINO_CS_PIN);
  regval |= GPIO_PULLUP << GPIO_PUPD_SHIFT(GPIO_SPI4_ARDUINO_CS_PIN);
  putreg32(regval, STM32_GPIOF_PUPDR);
}

static void stm32_esp_gpio_initialize(void)
{
  const uint32_t input_pins = GPIO_ESP_HANDSHAKE | GPIO_ESP_DATA_READY;
  const uint32_t pins = input_pins | GPIO_ESP_EN;
  uint32_t regval;
  int pin;

  modifyreg32(STM32_RCC_AHB4ENR, 0, RCC_AHB4ENR_GPIOFEN);
  (void)getreg32(STM32_RCC_AHB4ENR);

  putreg32(GPIO_ESP_EN_SET, STM32_GPIOF_BSRR);

  regval = getreg32(STM32_GPIOF_MODER);
  for (pin = GPIO_ESP_HANDSHAKE_PIN; pin <= GPIO_ESP_EN_PIN; pin++)
    {
      regval &= ~GPIO_MODE_MASK(pin);
    }

  regval |= GPIO_MODE_OUTPUT << GPIO_MODE_SHIFT(GPIO_ESP_EN_PIN);
  putreg32(regval, STM32_GPIOF_MODER);

  modifyreg32(STM32_GPIOF_OTYPER, pins, 0);

  regval = getreg32(STM32_GPIOF_OSPEEDR);
  for (pin = GPIO_ESP_HANDSHAKE_PIN; pin <= GPIO_ESP_EN_PIN; pin++)
    {
      regval &= ~GPIO_SPEED_MASK(pin);
      regval |= GPIO_SPEED_VERYHIGH << GPIO_SPEED_SHIFT(pin);
    }

  putreg32(regval, STM32_GPIOF_OSPEEDR);

  regval = getreg32(STM32_GPIOF_PUPDR);
  for (pin = GPIO_ESP_HANDSHAKE_PIN; pin <= GPIO_ESP_EN_PIN; pin++)
    {
      regval &= ~GPIO_PUPD_MASK(pin);
    }

  regval |= GPIO_PULLDOWN << GPIO_PUPD_SHIFT(GPIO_ESP_HANDSHAKE_PIN);
  regval |= GPIO_PULLDOWN << GPIO_PUPD_SHIFT(GPIO_ESP_DATA_READY_PIN);
  regval |= GPIO_PULLUP << GPIO_PUPD_SHIFT(GPIO_ESP_EN_PIN);
  putreg32(regval, STM32_GPIOF_PUPDR);
}

#ifdef CONFIG_WL_ESP_HOSTED_NG
static int stm32_esp_gpio_irq(int irq, FAR void *context, FAR void *arg)
{
  esp_hosted_ng_board_spi_ready();
  return OK;
}

static int stm32_esp_gpio_irq_initialize(void)
{
  int ret;

  ret = stm32_gpiosetevent(STM32_GPIOF_BASE, GPIO_ESP_HANDSHAKE_PIN,
                           true, false, false, stm32_esp_gpio_irq, NULL);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32_gpiosetevent(STM32_GPIOF_BASE, GPIO_ESP_DATA_READY_PIN,
                           true, false, false, stm32_esp_gpio_irq, NULL);
  if (ret < 0)
    {
      stm32_gpiosetevent(STM32_GPIOF_BASE, GPIO_ESP_HANDSHAKE_PIN,
                         false, false, false, NULL, NULL);
      return ret;
    }

  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_spi4_board_initialize(void)
{
  stm32_spi4_cs_initialize();
  return OK;
}

void stm32_spi4select(struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  if (devid == SPIDEV_WIRELESS(0))
    {
      putreg32(selected ? GPIO_SPI4_ARDUINO_CS_CLR :
                          GPIO_SPI4_ARDUINO_CS_SET,
               STM32_GPIOF_BSRR);
    }
}

uint8_t stm32_spi4status(struct spi_dev_s *dev, uint32_t devid)
{
  return SPI_STATUS_PRESENT;
}

#ifdef CONFIG_WL_ESP_HOSTED_NG
int esp_hosted_ng_board_initialize(void)
{
  int ret;

  stm32_esp_gpio_initialize();

  putreg32(GPIO_ESP_EN_CLR, STM32_GPIOF_BSRR);
  up_mdelay(ESP_RESET_ASSERT_MS);
  putreg32(GPIO_ESP_EN_SET, STM32_GPIOF_BSRR);
  up_mdelay(ESP_RESET_BOOT_MS);

  ret = stm32_esp_gpio_irq_initialize();
  if (ret < 0)
    {
      return ret;
    }

  if (esp_hosted_ng_board_handshake_ready() ||
      esp_hosted_ng_board_data_ready())
    {
      esp_hosted_ng_board_spi_ready();
    }

  return OK;
}

void esp_hosted_ng_board_reset(bool reset)
{
  putreg32(reset ? GPIO_ESP_EN_CLR : GPIO_ESP_EN_SET, STM32_GPIOF_BSRR);
}

bool esp_hosted_ng_board_handshake_ready(void)
{
  return (getreg32(STM32_GPIOF_IDR) & GPIO_ESP_HANDSHAKE) != 0;
}

bool esp_hosted_ng_board_data_ready(void)
{
  return (getreg32(STM32_GPIOF_IDR) & GPIO_ESP_DATA_READY) != 0;
}

FAR struct spi_dev_s *esp_hosted_ng_spibus_initialize(int bus)
{
  if (bus == 4)
    {
      stm32_spi4_board_initialize();
      return stm32_spibus_initialize(bus);
    }

  return NULL;
}
#endif

#endif /* CONFIG_STM32H7RS_SPI4 */
