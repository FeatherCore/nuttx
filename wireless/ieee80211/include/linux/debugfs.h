#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_DEBUGFS_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_DEBUGFS_H
#include <linux/cfg80211_compat.h>
#include <nuttx/fs/fs.h>

struct dentry
{
  int dummy;
};

#define DEFINE_DEBUGFS_ATTRIBUTE(name, get, set, fmt) \
  static const struct file_operations name = { 0 }

static inline struct dentry *debugfs_create_dir(const char *name,
                                                struct dentry *parent)
{
  (void)name;
  (void)parent;
  return NULL;
}

static inline struct dentry *debugfs_create_file(const char *name,
                                                 unsigned int mode,
                                                 struct dentry *parent,
                                                 void *data,
                                                 const struct file_operations *fops)
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
static inline void debugfs_remove_recursive(struct dentry *dentry)
{
  (void)dentry;
}
static inline void debugfs_change_name(struct dentry *dentry,
                                       const char *fmt, ...)
{
  (void)dentry;
  (void)fmt;
}
#endif
