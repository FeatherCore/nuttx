#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_POLL_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_POLL_H
#include <linux/fs.h>
#include <poll.h>
typedef unsigned int __poll_t;
typedef void poll_table;
#ifndef EPOLLRDNORM
#  define EPOLLRDNORM POLLRDNORM
#endif
#ifndef EPOLLWRNORM
#  define EPOLLWRNORM POLLWRNORM
#endif
#ifndef EPOLLERR
#  define EPOLLERR POLLERR
#endif
#ifndef EPOLLHUP
#  define EPOLLHUP POLLHUP
#endif
#ifndef EPOLLPRI
#  define EPOLLPRI POLLPRI
#endif
#ifndef EPOLLRDHUP
#  define EPOLLRDHUP POLLHUP
#endif
#ifndef EPOLLWRBAND
#  define EPOLLWRBAND POLLWRBAND
#endif
#ifndef EPOLLIN
#  define EPOLLIN POLLIN
#endif
#ifndef EPOLLOUT
#  define EPOLLOUT POLLOUT
#endif
static inline void poll_wait(struct file *file, void *wait_address,
                             poll_table *wait)
{
  (void)file;
  (void)wait_address;
  (void)wait;
}
#endif
