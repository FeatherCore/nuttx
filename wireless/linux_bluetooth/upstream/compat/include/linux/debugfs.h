#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_DEBUGFS_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_DEBUGFS_H

#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/types.h>

#include <stdio.h>

struct dentry { int unused; };

#define DEFINE_DEBUGFS_ATTRIBUTE(name, get, set, fmt) \
  static ssize_t name##_read(struct file *file, char __user *user_buf, \
                             size_t count, loff_t *ppos) \
  { \
    char __buf[32]; \
    u64 __val; \
    int __ret = (get)(file->private_data, &__val); \
    if (__ret < 0) \
      { \
        return __ret; \
      } \
    __ret = snprintf(__buf, sizeof(__buf), fmt, \
                     (unsigned long long)__val); \
    if (__ret < 0) \
      { \
        return __ret; \
      } \
    return simple_read_from_buffer(user_buf, count, ppos, __buf, \
                                   (size_t)__ret); \
  } \
  static ssize_t name##_write(struct file *file, \
                              const char __user *user_buf, \
                              size_t count, loff_t *ppos) \
  { \
    unsigned long long __val; \
    int __ret; \
    (void)ppos; \
    __ret = kstrtoull_from_user(user_buf, count, 0, &__val); \
    if (__ret < 0) \
      { \
        return __ret; \
      } \
    __ret = (set)(file->private_data, (u64)__val); \
    return __ret < 0 ? __ret : (ssize_t)count; \
  } \
  static const struct file_operations name = \
  { \
    .open = simple_open, \
    .read = name##_read, \
    .write = name##_write, \
    .llseek = default_llseek, \
  }

static inline struct dentry *debugfs_create_dir(const char *name,
                                                struct dentry *parent)
{
  (void)name;
  (void)parent;
  return NULL;
}

static inline void debugfs_remove_recursive(struct dentry *dentry)
{
  (void)dentry;
}

static inline void debugfs_lookup_and_remove(const char *name,
                                             struct dentry *parent)
{
  (void)name;
  (void)parent;
}

static inline struct dentry *debugfs_create_file(const char *name,
                                                 unsigned int mode,
                                                 struct dentry *parent,
                                                 void *data,
                                                 const void *fops)
{
  (void)name;
  (void)mode;
  (void)parent;
  (void)data;
  (void)fops;
  return NULL;
}

static inline void debugfs_remove(struct dentry *dentry)
{
  (void)dentry;
}

#endif
