/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/rbtree.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_RBTREE_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_RBTREE_H

struct rb_node
{
  struct rb_node *rb_left;
  struct rb_node *rb_right;
  struct rb_node *rb_parent;
};

struct rb_root
{
  struct rb_node *rb_node;
};

#define RB_ROOT ((struct rb_root){ NULL })

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_RBTREE_H */
