/****************************************************************************
 * wireless/linux_bluetooth/upstream/compat/include/linux/types.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_TYPES_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_TYPES_H

#include <nuttx/config.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef __u8
typedef uint8_t __u8;
#endif
#ifndef __u16
typedef uint16_t __u16;
#endif
#ifndef __u32
typedef uint32_t __u32;
#endif
#ifndef __u64
typedef uint64_t __u64;
#endif
#ifndef __s8
typedef int8_t __s8;
#endif
#ifndef __s16
typedef int16_t __s16;
#endif
#ifndef __s32
typedef int32_t __s32;
#endif
#ifndef __s64
typedef int64_t __s64;
#endif

typedef uint16_t __le16;
typedef uint32_t __le32;
typedef uint64_t __le64;
typedef uint16_t __be16;
typedef uint32_t __be32;
typedef uint64_t __be64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef int gfp_t;

#ifndef __aligned
#  define __aligned(x) __attribute__((aligned(x)))
#endif

#endif /* __WIRELESS_LINUX_BLUETOOTH_COMPAT_LINUX_TYPES_H */
