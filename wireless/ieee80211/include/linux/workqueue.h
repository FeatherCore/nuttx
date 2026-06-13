#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_WORKQUEUE_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_WORKQUEUE_H
#include <linux/cfg80211_compat.h>
static inline bool schedule_work(struct work_struct *work)
{
  return queue_work(system_dfl_wq, work);
}
static inline bool schedule_delayed_work(struct delayed_work *dwork,
                                         unsigned long delay)
{
  return queue_delayed_work(system_dfl_wq, dwork, delay);
}
static inline bool cancel_work_sync(struct work_struct *work)
{
  if (!work)
    {
      return false;
    }

  return work_cancel_sync(CFG80211_COMPAT_WORKQ, &work->nuttx_work) == 0;
}
#ifndef __WIRELESS_IEEE80211_WORKQUEUE_CANCEL_COMPAT
#define __WIRELESS_IEEE80211_WORKQUEUE_CANCEL_COMPAT 1
static inline bool cancel_delayed_work_sync(struct delayed_work *dwork)
{
  (void)dwork;
  return false;
}
#endif
static inline bool cancel_delayed_work(struct delayed_work *dwork)
{
  if (!dwork)
    {
      return false;
    }

  return work_cancel(CFG80211_COMPAT_WORKQ, &dwork->work.nuttx_work) == 0;
}
static inline bool delayed_work_pending(struct delayed_work *dwork)
{
  return dwork && !work_available(&dwork->work.nuttx_work);
}
static inline bool flush_delayed_work(struct delayed_work *dwork)
{
  if (delayed_work_pending(dwork))
    {
      cfg80211_compat_work_trampoline(&dwork->work);
      return true;
    }

  return false;
}
#endif
