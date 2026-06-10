/****************************************************************************
 * include/nuttx/wireless/virtual_hwsim.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_WIRELESS_VIRTUAL_HWSIM_H
#define __INCLUDE_NUTTX_WIRELESS_VIRTUAL_HWSIM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>

#include <nuttx/compiler.h>
#include <nuttx/net/wifi_sim.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_WL_NUTTX_HWSIM
int virtual_hwsim_init(FAR struct wifi_sim_lowerhalf_s *netdev);
void virtual_hwsim_remove(FAR struct wifi_sim_lowerhalf_s *netdev);
bool virtual_hwsim_connected(FAR struct wifi_sim_lowerhalf_s *netdev);
#endif

#endif /* __INCLUDE_NUTTX_WIRELESS_VIRTUAL_HWSIM_H */
