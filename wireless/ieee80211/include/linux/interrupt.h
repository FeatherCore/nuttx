#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_INTERRUPT_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_INTERRUPT_H

#include <linux/cfg80211_compat.h>

struct tasklet_struct
{
  void (*func)(unsigned long data);
  unsigned long data;
};

#define from_tasklet(var, callback_tasklet, tasklet_field) \
  container_of(callback_tasklet, typeof(*var), tasklet_field)

static inline void tasklet_setup(struct tasklet_struct *tasklet,
                                 void (*callback)(struct tasklet_struct *))
{
  tasklet->func = (void (*)(unsigned long))callback;
  tasklet->data = (unsigned long)tasklet;
}

static inline void tasklet_schedule(struct tasklet_struct *tasklet)
{
  if (tasklet && tasklet->func)
    {
      tasklet->func(tasklet->data);
    }
}

static inline void tasklet_kill(struct tasklet_struct *tasklet)
{
  (void)tasklet;
}

#endif
