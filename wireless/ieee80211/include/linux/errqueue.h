#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_ERRQUEUE_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_ERRQUEUE_H

#include <linux/types.h>

#ifndef __NUTTX_SOCK_EXTENDED_ERR_DEFINED
#define __NUTTX_SOCK_EXTENDED_ERR_DEFINED 1
struct sock_extended_err
{
  __u32 ee_errno;
  __u8 ee_origin;
  __u8 ee_type;
  __u8 ee_code;
  __u8 ee_pad;
  __u32 ee_info;
  __u32 ee_data;
};
#endif

#endif /* __WIRELESS_IEEE80211_INCLUDE_LINUX_ERRQUEUE_H */
