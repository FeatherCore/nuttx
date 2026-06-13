#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_PROC_FS_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_PROC_FS_H

struct proc_dir_entry
{
  void *data;
};

static inline void *pde_data(const struct inode *inode)
{
  (void)inode;
  return NULL;
}

static inline struct inode *file_inode(const struct file *file)
{
  (void)file;
  return NULL;
}

static inline struct proc_dir_entry *
proc_create_seq_data(const char *name, unsigned int mode, void *parent,
                     const struct seq_operations *ops, void *data)
{
  static struct proc_dir_entry entry;

  (void)name;
  (void)mode;
  (void)parent;
  (void)ops;
  entry.data = data;
  return &entry;
}

static inline void remove_proc_entry(const char *name, void *parent)
{
  (void)name;
  (void)parent;
}

#endif
