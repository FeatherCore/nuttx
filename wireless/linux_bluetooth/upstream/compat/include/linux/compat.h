#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_COMPAT_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_COMPAT_H

#include <stdint.h>

static inline void *compat_ptr(uintptr_t ptr)
{
  return (void *)ptr;
}

#endif
