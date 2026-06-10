/****************************************************************************
 * include/nuttx/net/netlink_kernel.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_NET_NETLINK_KERNEL_H
#define __INCLUDE_NUTTX_NET_NETLINK_KERNEL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <sys/types.h>
#include <stdint.h>

#include <nuttx/queue.h>

/* Netlink kernel helpers are used both by native NuttX netlink code and by
 * the imported Linux ieee80211 stack.  The latter uses Linux UAPI netlink
 * definitions, so only include NuttX netpacket/netlink.h when no compatible
 * netlink UAPI header has already provided struct nlmsghdr/sockaddr_nl.
 */

#if !defined(__INCLUDE_NETPACKET_NETLINK_H) && \
    !defined(_UAPI__LINUX_NETLINK_H)
#  include <netpacket/netlink.h>
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This is the form of the obfuscated state structure passed to modules
 * outside of the networking layer.
 */

typedef FAR void *NETLINK_HANDLE;

/* Netlink uses a two step, request-get, referenced by a user managed
 * sequence number.  This means that the requested data must be buffered
 * until it is gotten by the client.  This structure holds that buffered
 * data.
 *
 * REVISIT:  There really should be a time stamp on this so that we can
 * someday weed out un-claimed responses.
 */

struct netlink_response_s
{
  sq_entry_t flink;
  uint32_t group;
  struct nlmsghdr msg;

  /* Message-specific data may follow */
};

#define SIZEOF_NETLINK_RESPONSE_S(n) (sizeof(struct netlink_response_s) + (n))

#ifdef CONFIG_NETLINK_GENERIC
struct netlink_generic_group_s
{
  uint32_t id;
  FAR const char *name;
};

struct netlink_generic_family_s
{
  uint16_t id;
  FAR const char *name;
  uint32_t version;
  uint32_t hdrsize;
  uint32_t maxattr;
  FAR struct netlink_generic_group_s *groups;
  unsigned int ngroups;
  ssize_t (*sendto)(NETLINK_HANDLE handle,
                    FAR const struct nlmsghdr *nlmsg,
                    size_t len, int flags,
                    FAR const struct sockaddr_nl *to,
                    socklen_t tolen);
};
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

void netlink_add_response(NETLINK_HANDLE handle,
                          FAR struct netlink_response_s *resp);
int netlink_add_response_pid(int protocol, uint32_t pid,
                             FAR struct netlink_response_s *resp);
void netlink_add_broadcast(int group, FAR struct netlink_response_s *data);
void netlink_add_broadcast_protocol(int protocol, int group,
                                    FAR struct netlink_response_s *data);

#ifdef CONFIG_NETLINK_GENERIC
int netlink_generic_register(FAR struct netlink_generic_family_s *family);
int netlink_generic_unregister(uint16_t id, FAR const char *name);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_NUTTX_NET_NETLINK_KERNEL_H */
