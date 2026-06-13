/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/kcov.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_KCOV_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_KCOV_H

#include <linux/types.h>

static inline void kcov_remote_start_common(uint64_t handle)
{
  (void)handle;
}

static inline void kcov_remote_stop(void)
{
}

#endif
