/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/suspend.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SUSPEND_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SUSPEND_H

#define PM_SUSPEND_PREPARE 1
#define PM_POST_SUSPEND 2
#define PM_HIBERNATION_PREPARE 3
#define PM_POST_HIBERNATION 4
#define NOTIFY_DONE 0

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NOTIFIER_BLOCK_DEFINED
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NOTIFIER_BLOCK_DEFINED
struct notifier_block
{
  int (*notifier_call)(struct notifier_block *nb, unsigned long action,
                       void *data);
};
#endif

static inline int register_pm_notifier(struct notifier_block *nb)
{
  (void)nb;
  return 0;
}

static inline int unregister_pm_notifier(struct notifier_block *nb)
{
  (void)nb;
  return 0;
}

#endif
