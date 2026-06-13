/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/file.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_FILE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_FILE_H

#include <errno.h>

struct socket;

struct socket *linux_bt_compat_sockfd_lookup(int fd, int *err);
void linux_bt_compat_sockfd_put(struct socket *sock);

static inline struct socket *sockfd_lookup(int fd, int *err)
{
  return linux_bt_compat_sockfd_lookup(fd, err);
}

static inline void sockfd_put(struct socket *sock)
{
  linux_bt_compat_sockfd_put(sock);
}

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_FILE_H */
