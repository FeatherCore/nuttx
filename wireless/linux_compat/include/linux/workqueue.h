#ifndef __WIRELESS_LINUX_COMPAT_INCLUDE_LINUX_WORKQUEUE_H
#define __WIRELESS_LINUX_COMPAT_INCLUDE_LINUX_WORKQUEUE_H

#include <nuttx/config.h>

#include <stdbool.h>
#include <string.h>

#include <nuttx/wqueue.h>

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_TYPES_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_TYPES_DEFINED
struct work_struct
{
  void (*func)(struct work_struct *work);
  struct work_s nuttx_work;
};

struct delayed_work
{
  struct work_struct work;
  bool pending;
};

struct workqueue_struct
{
  int dummy;
};
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_GLOBALS_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_GLOBALS_DEFINED
static struct workqueue_struct linux_compat_system_wq;
#define system_dfl_wq (&linux_compat_system_wq)
#define system_power_efficient_wq (&linux_compat_system_wq)
#define system_freezable_wq (&linux_compat_system_wq)
#define WQ_MEM_RECLAIM 0
#define WQ_HIGHPRI 0
#define WQ_UNBOUND 0
#endif

#ifndef LINUX_COMPAT_WORKQ
#  ifdef LPWORK
#    define LINUX_COMPAT_WORKQ LPWORK
#  elif defined(HPWORK)
#    define LINUX_COMPAT_WORKQ HPWORK
#  else
#    define LINUX_COMPAT_WORKQ USRWORK
#  endif
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_CORE_HELPERS_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_CORE_HELPERS_DEFINED
static inline void linux_compat_work_trampoline(void *arg)
{
  struct work_struct *work = arg;

  if (work != NULL && work->func != NULL)
    {
      work->func(work);
    }
}

static inline struct workqueue_struct *
alloc_ordered_workqueue(const char *name, unsigned int flags, ...)
{
  (void)name;
  (void)flags;
  return system_dfl_wq;
}

static inline struct workqueue_struct *
alloc_workqueue(const char *name, unsigned int flags,
                int max_active, ...)
{
  (void)name;
  (void)flags;
  (void)max_active;
  return system_dfl_wq;
}

static inline struct workqueue_struct *
create_singlethread_workqueue(const char *name)
{
  return alloc_workqueue(name, 0, 1);
}

static inline void destroy_workqueue(struct workqueue_struct *wq)
{
  (void)wq;
}

static inline bool queue_work(struct workqueue_struct *wq,
                              struct work_struct *work)
{
  (void)wq;

  if (work == NULL || work->func == NULL)
    {
      return false;
    }

  return work_queue(LINUX_COMPAT_WORKQ, &work->nuttx_work,
                    linux_compat_work_trampoline, work, 0) == 0;
}

static inline bool queue_delayed_work(struct workqueue_struct *wq,
                                      struct delayed_work *dwork,
                                      unsigned long delay)
{
  (void)wq;

  if (dwork == NULL || dwork->work.func == NULL)
    {
      return false;
    }

  dwork->pending = work_queue(LINUX_COMPAT_WORKQ,
                              &dwork->work.nuttx_work,
                              linux_compat_work_trampoline,
                              &dwork->work,
                              (clock_t)delay) == 0;
  return dwork->pending;
}

static inline bool mod_delayed_work(struct workqueue_struct *wq,
                                    struct delayed_work *dwork,
                                    unsigned long delay)
{
  return queue_delayed_work(wq, dwork, delay);
}

static inline void flush_workqueue(struct workqueue_struct *wq)
{
  (void)wq;
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_INIT_HELPERS_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_INIT_HELPERS_DEFINED
#define DECLARE_WORK(name, fn) \
  struct work_struct name = { .func = (void (*)(struct work_struct *))(fn) }
#define DECLARE_DELAYED_WORK(name, fn) \
  struct delayed_work name = { .work = { .func = (void (*)(struct work_struct *))(fn) } }

#ifndef INIT_WORK
#  define INIT_WORK(work, fn) do { \
    memset((work), 0, sizeof(*(work))); \
    (work)->func = (void (*)(struct work_struct *))(fn); \
  } while (0)
#endif

#ifndef INIT_DELAYED_WORK
#  define INIT_DELAYED_WORK(dwork, fn) INIT_WORK(&(dwork)->work, fn)
#endif

#ifndef to_delayed_work
#  define to_delayed_work(work) container_of(work, struct delayed_work, work)
#endif
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_SCHEDULE_WORK_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_SCHEDULE_WORK_DEFINED
static inline bool schedule_work(struct work_struct *work)
{
  return queue_work(system_dfl_wq, work);
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_SCHEDULE_DELAYED_WORK_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_SCHEDULE_DELAYED_WORK_DEFINED
static inline bool schedule_delayed_work(struct delayed_work *dwork,
                                         unsigned long delay)
{
  return queue_delayed_work(system_dfl_wq, dwork, delay);
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_WORK_SYNC_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_WORK_SYNC_DEFINED
static inline bool cancel_work_sync(struct work_struct *work)
{
  if (work == NULL)
    {
      return false;
    }

  return work_cancel_sync(LINUX_COMPAT_WORKQ, &work->nuttx_work) == 0;
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_DELAYED_WORK_SYNC_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_DELAYED_WORK_SYNC_DEFINED
static inline bool cancel_delayed_work_sync(struct delayed_work *dwork)
{
  if (dwork == NULL)
    {
      return false;
    }

  return work_cancel_sync(LINUX_COMPAT_WORKQ,
                          &dwork->work.nuttx_work) == 0;
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_DELAYED_WORK_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_CANCEL_DELAYED_WORK_DEFINED
static inline bool cancel_delayed_work(struct delayed_work *dwork)
{
  if (dwork == NULL)
    {
      return false;
    }

  return work_cancel(LINUX_COMPAT_WORKQ, &dwork->work.nuttx_work) == 0;
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_DELAYED_WORK_PENDING_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_DELAYED_WORK_PENDING_DEFINED
static inline bool delayed_work_pending(struct delayed_work *dwork)
{
  return dwork != NULL && !work_available(&dwork->work.nuttx_work);
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_FLUSH_WORK_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_FLUSH_WORK_DEFINED
static inline void flush_work(struct work_struct *work)
{
  (void)work;
}
#endif

#ifndef __WIRELESS_LINUX_COMPAT_WORKQUEUE_FLUSH_DELAYED_WORK_DEFINED
#define __WIRELESS_LINUX_COMPAT_WORKQUEUE_FLUSH_DELAYED_WORK_DEFINED
static inline bool flush_delayed_work(struct delayed_work *dwork)
{
  if (delayed_work_pending(dwork))
    {
      linux_compat_work_trampoline(&dwork->work);
      return true;
    }

  return false;
}
#endif

#ifndef __WIRELESS_IEEE80211_JIFFIES_HELPERS_DEFINED
static inline unsigned long secs_to_jiffies(unsigned int s)
{
  return s * 100;
}
#endif

#endif
