#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SCHED_SIGNAL_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SCHED_SIGNAL_H

#include <linux/sched.h>
#include <linux/wait.h>

#include <pthread.h>
#include <string.h>

struct pid
{
  int nr;
};

struct cred
{
  unsigned int uid;
  unsigned int gid;
};

struct task_struct
{
  struct pid pid;
  unsigned long flags;
  pthread_t thread;
};

static struct task_struct linux_bt_compat_current = { { 1 } };

#define current (&linux_bt_compat_current)

#ifndef PF_EXITING
#define PF_EXITING 0x00000004
#endif

#ifndef TASK_COMM_LEN
#define TASK_COMM_LEN 16
#endif

static inline void get_task_comm(char *buf, struct task_struct *task)
{
  (void)task;
  if (buf != NULL)
    {
      strlcpy(buf, "nsh", TASK_COMM_LEN);
    }
}

static inline void set_user_nice(struct task_struct *task, int nice)
{
  (void)task;
  (void)nice;
}

static inline struct pid *task_tgid(struct task_struct *task)
{
  return task != 0 ? &task->pid : 0;
}

static inline struct pid *get_pid(struct pid *pid)
{
  return pid;
}

static inline int pid_vnr(struct pid *pid)
{
  return pid != NULL ? pid->nr : 0;
}

static inline void put_pid(struct pid *pid)
{
  (void)pid;
}

static inline const struct cred *get_current_cred(void)
{
  static struct cred cred;

  return &cred;
}

static inline const struct cred *get_cred(const struct cred *cred)
{
  return cred;
}

static inline void put_cred(const struct cred *cred)
{
  (void)cred;
}

#endif
