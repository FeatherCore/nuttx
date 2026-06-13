/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/kthread.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_KTHREAD_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_KTHREAD_H

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/sched/signal.h>

#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#define LINUX_BT_KTHREAD_STACKSIZE 65536
#define LINUX_BT_KTHREAD_MAX       8

typedef int (*linux_bt_kthread_fn_t)(void *data);

struct linux_bt_kthread_start
{
  linux_bt_kthread_fn_t threadfn;
  void *data;
};

static struct linux_bt_kthread_start *g_linux_bt_kthread_start[
  LINUX_BT_KTHREAD_MAX];

static inline int linux_bt_kthread_trampoline(int argc, char *argv[])
{
  struct linux_bt_kthread_start *start;
  int slot;

  if (argc < 2 || argv == NULL || argv[1] == NULL)
    {
      return -EINVAL;
    }

  slot = atoi(argv[1]);
  if (slot < 0 || slot >= LINUX_BT_KTHREAD_MAX)
    {
      return -EINVAL;
    }

  start = g_linux_bt_kthread_start[slot];
  g_linux_bt_kthread_start[slot] = NULL;

  if (start != NULL)
    {
      start->threadfn(start->data);
      kfree(start);
    }

  return 0;
}

static inline struct task_struct *
kthread_run(linux_bt_kthread_fn_t threadfn, void *data, const char *name, ...)
{
  struct linux_bt_kthread_start *start;
  struct task_struct *task;
  char slotarg[12];
  char *argv[3];
  int slot;
  int ret;

  (void)name;

  task = kzalloc(sizeof(*task), GFP_KERNEL);
  start = kzalloc(sizeof(*start), GFP_KERNEL);
  if (task == NULL || start == NULL)
    {
      kfree(task);
      kfree(start);
      return ERR_PTR(-ENOMEM);
    }

  task->pid.nr = 1;
  start->threadfn = threadfn;
  start->data = data;

  for (slot = 0; slot < LINUX_BT_KTHREAD_MAX; slot++)
    {
      if (g_linux_bt_kthread_start[slot] == NULL)
        {
          break;
        }
    }

  if (slot >= LINUX_BT_KTHREAD_MAX)
    {
      kfree(task);
      kfree(start);
      return ERR_PTR(-EAGAIN);
    }

  g_linux_bt_kthread_start[slot] = start;
  snprintf(slotarg, sizeof(slotarg), "%d", slot);
  argv[0] = "linux_bt_kthread";
  argv[1] = slotarg;
  argv[2] = NULL;

  ret = task_create("linux_bt_kthread", SCHED_PRIORITY_DEFAULT,
                    LINUX_BT_KTHREAD_STACKSIZE,
                    linux_bt_kthread_trampoline, argv);
  if (ret < 0)
    {
      int err = errno > 0 ? errno : -ret;

      g_linux_bt_kthread_start[slot] = NULL;
      kfree(task);
      kfree(start);
      return ERR_PTR(-err);
    }

  task->pid.nr = ret;
  return task;
}

static inline void module_put_and_kthread_exit(long code)
{
  (void)code;
}

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_KTHREAD_H */
