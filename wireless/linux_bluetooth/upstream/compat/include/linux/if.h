/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/if.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_IF_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_IF_H

#ifndef IFNAMSIZ
#  define IFNAMSIZ 16
#endif

#ifndef IFF_UP
#  define IFF_UP 0x0001
#endif
#ifndef IFF_BROADCAST
#  define IFF_BROADCAST 0x0002
#endif
#ifndef IFF_DEBUG
#  define IFF_DEBUG 0x0004
#endif
#ifndef IFF_LOOPBACK
#  define IFF_LOOPBACK 0x0008
#endif
#ifndef IFF_POINTOPOINT
#  define IFF_POINTOPOINT 0x0010
#endif
#ifndef IFF_NOTRAILERS
#  define IFF_NOTRAILERS 0x0020
#endif
#ifndef IFF_RUNNING
#  define IFF_RUNNING 0x0040
#endif
#ifndef IFF_NOARP
#  define IFF_NOARP 0x0080
#endif
#ifndef IFF_PROMISC
#  define IFF_PROMISC 0x0100
#endif
#ifndef IFF_ALLMULTI
#  define IFF_ALLMULTI 0x0200
#endif
#ifndef IFF_MASTER
#  define IFF_MASTER 0x0400
#endif
#ifndef IFF_SLAVE
#  define IFF_SLAVE 0x0800
#endif
#ifndef IFF_MULTICAST
#  define IFF_MULTICAST 0x1000
#endif

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_IF_H */
