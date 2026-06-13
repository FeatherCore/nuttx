#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SRCU_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SRCU_H
struct srcu_struct { int unused; };
#define DEFINE_SRCU(name) struct srcu_struct name = { 0 }
static inline int srcu_read_lock(struct srcu_struct *srcu)
{
  (void)srcu;
  return 0;
}

static inline int init_srcu_struct(struct srcu_struct *srcu)
{
  (void)srcu;
  return 0;
}

static inline void srcu_read_unlock(struct srcu_struct *srcu, int idx)
{
  (void)srcu;
  (void)idx;
}

static inline void synchronize_srcu(struct srcu_struct *srcu)
{
  (void)srcu;
}

static inline void cleanup_srcu_struct(struct srcu_struct *srcu)
{
  (void)srcu;
}
#endif
