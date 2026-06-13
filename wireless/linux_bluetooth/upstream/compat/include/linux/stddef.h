/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __LINUX_BT_COMPAT_LINUX_STDDEF_H
#define __LINUX_BT_COMPAT_LINUX_STDDEF_H

#include <stddef.h>

#ifndef offsetofend
#  define offsetofend(TYPE, MEMBER) \
    (offsetof(TYPE, MEMBER) + sizeof(((TYPE *)0)->MEMBER))
#endif

#ifndef sizeof_field
#  define sizeof_field(TYPE, MEMBER) sizeof(((TYPE *)0)->MEMBER)
#endif

#define __struct_group(TAG, NAME, ATTRS, MEMBERS...) \
  union \
  { \
    struct \
    { \
      MEMBERS \
    } ATTRS; \
    struct TAG \
    { \
      MEMBERS \
    } ATTRS NAME; \
  }

#define struct_group(NAME, MEMBERS...) \
  __struct_group(/* no tag */, NAME, /* no attrs */, MEMBERS)

#define struct_group_attr(NAME, ATTRS, MEMBERS...) \
  __struct_group(/* no tag */, NAME, ATTRS, MEMBERS)

#define struct_group_tagged(TAG, NAME, MEMBERS...) \
  __struct_group(TAG, NAME, /* no attrs */, MEMBERS)

#endif /* __LINUX_BT_COMPAT_LINUX_STDDEF_H */
