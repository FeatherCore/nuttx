/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/rwsem.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_RWSEM_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_RWSEM_H

struct rw_semaphore
{
  int unused;
};

#define DECLARE_RWSEM(name) struct rw_semaphore name = { 0 }

static inline void down_write(struct rw_semaphore *sem)
{
  (void)sem;
}

static inline void up_write(struct rw_semaphore *sem)
{
  (void)sem;
}

static inline void down_read(struct rw_semaphore *sem)
{
  (void)sem;
}

static inline void up_read(struct rw_semaphore *sem)
{
  (void)sem;
}

#endif
