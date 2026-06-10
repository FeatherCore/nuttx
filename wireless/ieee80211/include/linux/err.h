#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_ERR_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_ERR_H
#include <linux/cfg80211_compat.h>
#define MAX_ERRNO 4095
#define IS_ERR_VALUE(x) ((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)
static inline void *ERR_PTR(long error)
{
  return (void *)error;
}
static inline long PTR_ERR(const void *ptr)
{
  return (long)ptr;
}
static inline bool IS_ERR(const void *ptr)
{
  return IS_ERR_VALUE((unsigned long)ptr);
}
static inline bool IS_ERR_OR_NULL(const void *ptr)
{
  return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}
static inline int PTR_ERR_OR_ZERO(const void *ptr)
{
  return IS_ERR(ptr) ? PTR_ERR(ptr) : 0;
}
#endif
