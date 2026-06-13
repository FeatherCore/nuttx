#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_WAIT_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_WAIT_H

#include <errno.h>
#include <sched.h>
#include <unistd.h>

typedef struct { int unused; } wait_queue_head_t;
typedef struct { int unused; } wait_queue_entry_t;
#define DECLARE_WAITQUEUE(name, task) wait_queue_entry_t name = { 0 }
#define DEFINE_WAIT_FUNC(name, function) wait_queue_entry_t name = { 0 }
#define init_waitqueue_head(wq) do { (void)(wq); } while (0)
#define add_wait_queue(wq, wait) do { (void)(wq); (void)(wait); } while (0)
#define add_wait_queue_exclusive(wq, wait) \
  do { (void)(wq); (void)(wait); } while (0)
#define remove_wait_queue(wq, wait) do { (void)(wq); (void)(wait); } while (0)
#define wake_up(wq) do { (void)(wq); sched_yield(); } while (0)
#define wake_up_interruptible(wq) do { (void)(wq); sched_yield(); } while (0)
#define wake_up_bit(word, bit) do { (void)(word); (void)(bit); } while (0)
#define wait_event_interruptible(wq, condition) ((condition) ? 0 : -EAGAIN)
#define wait_event_interruptible_timeout(wq, condition, timeout) \
  ((condition) ? (timeout) : 0)
#define wait_on_bit(word, bit, mode) ({ (void)(word); (void)(bit); 0; })

#define TASK_INTERRUPTIBLE 1
#define TASK_RUNNING 0

static inline void set_current_state(int state)
{
  (void)state;
}

static inline void __set_current_state(int state)
{
  (void)state;
}

static inline long schedule_timeout(long timeout)
{
  return 0;
}

static inline int signal_pending(void *task)
{
  (void)task;
  return 0;
}

static inline int woken_wake_function(wait_queue_entry_t *wait,
                                      unsigned int mode, int sync,
                                      void *key)
{
  (void)wait;
  (void)mode;
  (void)sync;
  (void)key;
  return 0;
}

static inline long wait_woken(wait_queue_entry_t *wait, unsigned int mode,
                              long timeout)
{
  (void)wait;
  (void)mode;
  (void)timeout;
  usleep(20000);
  return 0;
}

#endif
