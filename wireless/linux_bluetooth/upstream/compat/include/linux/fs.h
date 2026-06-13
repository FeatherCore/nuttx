#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_FS_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_FS_H

#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __user
#define __user
#endif

/* NuttX already exposes POSIX loff_t and its own struct file/inode/
 * file_operations.  Keep the imported Linux Bluetooth sources on a private
 * compat type namespace so the upstream driver code can keep using Linux
 * field names such as private_data, f_flags and write_iter.
 */

#define inode linux_inode
#define file linux_file
#define file_operations linux_file_operations

struct inode
{
  void *i_private;
};
struct iov_iter
{
  const char *data;
  size_t count;
};

struct file
{
  void *private_data;
  int f_flags;
};

void linux_bt_compat_fput(struct file *file);

struct kiocb
{
  struct file *ki_filp;
};

struct file_operations
{
  void *owner;
  int (*open)(struct inode *inode, struct file *file);
  int (*release)(struct inode *inode, struct file *file);
  ssize_t (*read)(struct file *file, char __user *buf, size_t count,
                  loff_t *ppos);
  ssize_t (*write)(struct file *file, const char __user *buf, size_t count,
                   loff_t *ppos);
  ssize_t (*write_iter)(struct kiocb *iocb, struct iov_iter *from);
  unsigned int (*poll)(struct file *file, void *wait);
  long (*unlocked_ioctl)(struct file *file, unsigned int cmd,
                         unsigned long arg);
  loff_t (*llseek)(struct file *file, loff_t offset, int whence);
};

static inline int simple_open(struct inode *inode, struct file *file)
{
  (void)inode;
  (void)file;
  return 0;
}

static inline loff_t default_llseek(struct file *file, loff_t offset,
                                    int whence)
{
  (void)file;
  (void)whence;
  return offset;
}

static inline int nonseekable_open(struct inode *inode, struct file *file)
{
  (void)inode;
  (void)file;
  return 0;
}

static inline void fput(struct file *file)
{
  linux_bt_compat_fput(file);
}

static inline ssize_t simple_read_from_buffer(char __user *to, size_t count,
                                              loff_t *ppos,
                                              const void *from,
                                              size_t available)
{
  size_t remaining;
  size_t ncopy;

  if (*ppos >= (loff_t)available)
    {
      return 0;
    }

  remaining = available - (size_t)*ppos;
  ncopy = remaining < count ? remaining : count;
  memcpy(to, (const char *)from + *ppos, ncopy);
  *ppos += ncopy;
  return (ssize_t)ncopy;
}

static inline int copy_to_user(void __user *to, const void *from,
                               size_t count)
{
  memcpy(to, from, count);
  return 0;
}

static inline int copy_from_user(void *to, const void __user *from,
                                 size_t count)
{
  memcpy(to, from, count);
  return 0;
}

static inline size_t iov_iter_count(const struct iov_iter *from)
{
  return from != NULL ? from->count : 0;
}

static inline int copy_from_iter_full(void *to, size_t bytes,
                                      struct iov_iter *from)
{
  if (from == NULL || from->count < bytes)
    {
      return 0;
    }

  memcpy(to, from->data, bytes);
  from->data += bytes;
  from->count -= bytes;
  return 1;
}

static inline size_t copy_to_iter(const void *from, size_t bytes,
                                  struct iov_iter *to)
{
  size_t ncopy;

  if (from == NULL || to == NULL || to->data == NULL)
    {
      return 0;
    }

  ncopy = bytes < to->count ? bytes : to->count;
  memcpy((void *)to->data, from, ncopy);
  to->data += ncopy;
  to->count -= ncopy;
  return ncopy;
}

static inline int kstrtobool_from_user(const char __user *s, size_t count,
                                       bool *res)
{
  if (s == NULL || count == 0 || res == NULL)
    {
      return -EINVAL;
    }

  if (s[0] == '1' || s[0] == 'y' || s[0] == 'Y' ||
      s[0] == 't' || s[0] == 'T' ||
      ((s[0] == 'o' || s[0] == 'O') &&
       (s[1] == 'n' || s[1] == 'N')))
    {
      *res = true;
    }
  else
    {
      *res = false;
    }

  return 0;
}

static inline int kstrtoull_from_user(const char __user *s, size_t count,
                                      unsigned int base,
                                      unsigned long long *res)
{
  char buf[32];
  char *endptr;
  size_t ncopy;

  if (s == NULL || count == 0 || res == NULL)
    {
      return -EINVAL;
    }

  ncopy = count < sizeof(buf) - 1 ? count : sizeof(buf) - 1;
  memcpy(buf, s, ncopy);
  buf[ncopy] = '\0';

  *res = strtoull(buf, &endptr, base);
  if (endptr == buf)
    {
      return -EINVAL;
    }

  return 0;
}

#endif
