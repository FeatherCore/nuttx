#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_GPIO_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_GPIO_H

#include <linux/cfg80211_compat.h>

static inline bool gpio_is_valid(int gpio)
{
  return gpio >= 0;
}

static inline int gpio_request(unsigned int gpio, const char *label)
{
  (void)gpio;
  (void)label;
  return 0;
}

static inline void gpio_free(unsigned int gpio)
{
  (void)gpio;
}

static inline int gpio_direction_output(unsigned int gpio, int value)
{
  (void)gpio;
  (void)value;
  return 0;
}

static inline int gpio_direction_input(unsigned int gpio)
{
  (void)gpio;
  return 0;
}

static inline void gpio_set_value(unsigned int gpio, int value)
{
  (void)gpio;
  (void)value;
}

static inline int gpio_get_value(unsigned int gpio)
{
  (void)gpio;
  return 0;
}

#endif /* __WIRELESS_IEEE80211_INCLUDE_LINUX_GPIO_H */
