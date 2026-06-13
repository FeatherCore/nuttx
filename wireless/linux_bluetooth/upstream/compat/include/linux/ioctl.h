/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/ioctl.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_IOCTL_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_IOCTL_H

#ifdef _IOC
#  undef _IOC
#endif
#ifdef _IO
#  undef _IO
#endif
#ifdef _IOR
#  undef _IOR
#endif
#ifdef _IOW
#  undef _IOW
#endif
#ifdef _IOWR
#  undef _IOWR
#endif
#ifdef _IOC_TYPECHECK
#  undef _IOC_TYPECHECK
#endif

#define _IOC_NRBITS 8
#define _IOC_TYPEBITS 8
#define _IOC_SIZEBITS 14
#define _IOC_DIRBITS 2

#define _IOC_NRMASK ((1 << _IOC_NRBITS) - 1)
#define _IOC_TYPEMASK ((1 << _IOC_TYPEBITS) - 1)
#define _IOC_SIZEMASK ((1 << _IOC_SIZEBITS) - 1)
#define _IOC_DIRMASK ((1 << _IOC_DIRBITS) - 1)

#define _IOC_NRSHIFT 0
#define _IOC_TYPESHIFT (_IOC_NRSHIFT + _IOC_NRBITS)
#define _IOC_SIZESHIFT (_IOC_TYPESHIFT + _IOC_TYPEBITS)
#define _IOC_DIRSHIFT (_IOC_SIZESHIFT + _IOC_SIZEBITS)

#define _IOC_NONE 0U
#define _IOC_WRITE 1U
#define _IOC_READ 2U

#define _IOC(dir, type, nr, size) \
  (((dir) << _IOC_DIRSHIFT) | ((type) << _IOC_TYPESHIFT) | \
   ((nr) << _IOC_NRSHIFT) | ((size) << _IOC_SIZESHIFT))

#define _IOC_TYPECHECK(t) (sizeof(t))

#define _IO(type, nr) _IOC(_IOC_NONE, (type), (nr), 0)
#define _IOR(type, nr, size) \
  _IOC(_IOC_READ, (type), (nr), _IOC_TYPECHECK(size))
#define _IOW(type, nr, size) \
  _IOC(_IOC_WRITE, (type), (nr), _IOC_TYPECHECK(size))
#define _IOWR(type, nr, size) \
  _IOC(_IOC_READ | _IOC_WRITE, (type), (nr), _IOC_TYPECHECK(size))

#endif
