#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_WORKQUEUE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_WORKQUEUE_H

#include <linux/kernel.h>
#include <linux/types.h>

struct work_struct
{
  void (*func)(struct work_struct *work);
};

struct delayed_work
{
  struct work_struct work;
};

struct workqueue_struct
{
  const char *name;
};

#define WQ_HIGHPRI 0x01

#define INIT_WORK(work, fn) \
  do { (work)->func = (fn); } while (0)

#define INIT_DELAYED_WORK(dwork, fn) \
  do { (dwork)->work.func = (fn); } while (0)

static inline int schedule_work(struct work_struct *work)
{
  if (work != NULL && work->func != NULL)
    {
      work->func(work);
    }

  return 1;
}

static inline int schedule_delayed_work(struct delayed_work *dwork,
                                        unsigned long delay)
{
  if (delay > 0)
    {
      return 1;
    }

  if (dwork != NULL && dwork->work.func != NULL)
    {
      dwork->work.func(&dwork->work);
    }

  return 1;
}

static inline int queue_delayed_work(struct workqueue_struct *wq,
                                     struct delayed_work *dwork,
                                     unsigned long delay)
{
  (void)wq;
  return schedule_delayed_work(dwork, delay);
}

static inline int queue_work(struct workqueue_struct *wq,
                             struct work_struct *work)
{
  (void)wq;
  return schedule_work(work);
}

static inline int work_pending(struct work_struct *work)
{
  (void)work;
  return 0;
}

static inline int delayed_work_pending(struct delayed_work *dwork)
{
  (void)dwork;
  return 0;
}

static inline struct workqueue_struct *
alloc_ordered_workqueue(const char *fmt, unsigned int flags, ...)
{
  static struct workqueue_struct wq = { "compat-wq" };

  (void)fmt;
  (void)flags;
  return &wq;
}

static inline void destroy_workqueue(struct workqueue_struct *wq)
{
  (void)wq;
}

static inline void flush_workqueue(struct workqueue_struct *wq)
{
  (void)wq;
}

static inline void drain_workqueue(struct workqueue_struct *wq)
{
  (void)wq;
}

static inline int disable_work_sync(struct work_struct *work)
{
  (void)work;
  return 0;
}

static inline int cancel_work_sync(struct work_struct *work)
{
  (void)work;
  return 0;
}

static inline int disable_delayed_work_sync(struct delayed_work *dwork)
{
  (void)dwork;
  return 0;
}

static inline int disable_delayed_work(struct delayed_work *dwork)
{
  (void)dwork;
  return 0;
}

static inline int cancel_delayed_work_sync(struct delayed_work *dwork)
{
  (void)dwork;
  return 0;
}

static inline int cancel_delayed_work(struct delayed_work *dwork)
{
  return cancel_delayed_work_sync(dwork);
}

static inline void flush_work(struct work_struct *work)
{
  (void)work;
}

static inline int flush_delayed_work(struct delayed_work *dwork)
{
  (void)dwork;
  return 0;
}

static inline struct work_struct *current_work(void)
{
  return NULL;
}

static inline unsigned long secs_to_jiffies(unsigned int secs)
{
  return (unsigned long)secs;
}

static inline unsigned long msecs_to_jiffies(unsigned int msecs)
{
  return (unsigned long)((msecs + 999) / 1000);
}

#endif
