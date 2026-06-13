/****************************************************************************
 * wireless/ieee80211/ieee80211_linux_init.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>

#include <nuttx/wireless/ieee80211_linux.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_WIRELESS_IEEE80211_CFG80211_LINUX
int cfg80211_linux_initialize(void);
int nl80211_init(void);
#endif

#ifdef CONFIG_WIRELESS_IEEE80211_MAC80211_LINUX
int mac80211_linux_initialize(void);
#endif

#ifdef CONFIG_WL_NUTTX_HWSIM
int mac80211_hwsim_linux_initialize(void);
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int ieee80211_linux_initialize(void)
{
#ifndef CONFIG_WIRELESS_IEEE80211_NL80211_METADATA_ONLY
  static bool initialized;
#endif
  int ret = 0;

#ifndef CONFIG_WIRELESS_IEEE80211_NL80211_METADATA_ONLY
  if (initialized)
    {
      return 0;
    }
#endif

#ifdef CONFIG_WIRELESS_IEEE80211_CFG80211_LINUX
#  ifdef CONFIG_WIRELESS_IEEE80211_NL80211_METADATA_ONLY
  ret = nl80211_init();
#  else
  ret = cfg80211_linux_initialize();
#  endif
  if (ret < 0)
    {
      return ret;
    }
#endif

#ifdef CONFIG_WIRELESS_IEEE80211_MAC80211_LINUX
  ret = mac80211_linux_initialize();
  if (ret < 0)
    {
      return ret;
    }
#endif

#ifdef CONFIG_WL_NUTTX_HWSIM
  ret = mac80211_hwsim_linux_initialize();
  if (ret < 0)
    {
      return ret;
    }
#endif

#ifndef CONFIG_WIRELESS_IEEE80211_NL80211_METADATA_ONLY
  initialized = true;
#endif
  return 0;
}
