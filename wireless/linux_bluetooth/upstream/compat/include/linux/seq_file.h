#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SEQ_FILE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_SEQ_FILE_H

#include <linux/fs.h>
#include <linux/list.h>
#include <stdarg.h>

struct seq_file
{
  struct file *file;
};

struct seq_operations
{
  void *(*start)(struct seq_file *seq, loff_t *pos);
  void *(*next)(struct seq_file *seq, void *v, loff_t *pos);
  void (*stop)(struct seq_file *seq, void *v);
  int (*show)(struct seq_file *seq, void *v);
};

#define SEQ_START_TOKEN ((void *)1)

static inline void *seq_hlist_start_head(struct hlist_head *head, loff_t pos)
{
  (void)pos;
  return head;
}

static inline void *seq_hlist_next(void *v, struct hlist_head *head,
                                   loff_t *pos)
{
  (void)v;
  (void)head;
  if (pos != NULL)
    {
      (*pos)++;
    }

  return NULL;
}

static inline void seq_puts(struct seq_file *seq, const char *s)
{
  (void)seq;
  (void)s;
}

static inline void seq_putc(struct seq_file *seq, char c)
{
  (void)seq;
  (void)c;
}

static inline void seq_printf(struct seq_file *seq, const char *fmt, ...)
{
  (void)seq;
  (void)fmt;
}

static inline int single_open(struct file *file,
                              int (*show)(struct seq_file *, void *),
                              void *data)
{
  (void)show;
  if (file != NULL)
    {
      file->private_data = data;
    }

  return 0;
}

static inline ssize_t seq_read(struct file *file, char __user *buf,
                               size_t size, loff_t *ppos)
{
  (void)file;
  (void)buf;
  (void)size;
  (void)ppos;
  return 0;
}

static inline loff_t seq_lseek(struct file *file, loff_t offset, int whence)
{
  (void)file;
  (void)whence;
  return offset;
}

static inline int single_release(struct inode *inode, struct file *file)
{
  (void)inode;
  (void)file;
  return 0;
}

#ifndef DEFINE_SHOW_ATTRIBUTE
#define DEFINE_SHOW_ATTRIBUTE(name) \
  static int name##_open(struct inode *inode, struct file *file) \
  { \
    (void)inode; \
    (void)file; \
    return 0; \
  } \
  static const struct file_operations name##_fops = \
  { \
    .open = name##_open, \
    .llseek = default_llseek, \
  }
#endif

#endif
