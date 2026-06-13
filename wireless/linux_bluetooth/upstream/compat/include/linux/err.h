/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/err.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_ERR_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_ERR_H

#include <errno.h>
#include <stdint.h>

#define MAX_ERRNO 4095

static inline void *ERR_PTR(long error)
{
  return (void *)(intptr_t)error;
}

static inline long PTR_ERR(const void *ptr)
{
  return (long)(intptr_t)ptr;
}

static inline int IS_ERR(const void *ptr)
{
  return (uintptr_t)ptr >= (uintptr_t)-MAX_ERRNO;
}

static inline int IS_ERR_OR_NULL(const void *ptr)
{
  return ptr == NULL || IS_ERR(ptr);
}

#define ERR_CAST(ptr) ((void *)(ptr))

#endif
