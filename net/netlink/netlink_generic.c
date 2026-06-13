/****************************************************************************
 * net/netlink/netlink_generic.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <nuttx/debug.h>
#include <nuttx/kmalloc.h>
#include <nuttx/net/net.h>
#include <nuttx/net/netlink.h>

#include "netlink/netlink.h"

#ifdef CONFIG_NETLINK_GENERIC

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define GENL_NAMSIZ                       16
#define GENL_ID_CTRL                      NLMSG_MIN_TYPE
#define GENL_START_ALLOC                  (NLMSG_MIN_TYPE + 3)
#define GENL_HDRLEN                       NLMSG_ALIGN(sizeof(struct genlmsghdr))

#define CTRL_VERSION                      1

#define CTRL_CMD_NEWFAMILY                1
#define CTRL_CMD_GETFAMILY                3

#define CTRL_ATTR_FAMILY_ID               1
#define CTRL_ATTR_FAMILY_NAME             2
#define CTRL_ATTR_VERSION                 3
#define CTRL_ATTR_HDRSIZE                 4
#define CTRL_ATTR_MAXATTR                 5
#define CTRL_ATTR_MCAST_GROUPS            7

#define CTRL_ATTR_MCAST_GRP_NAME          1
#define CTRL_ATTR_MCAST_GRP_ID            2

#define GENL_RESPONSE_PAYLOAD_MAX         512
#define GENL_ARRAY_SIZE(a)                (sizeof(a) / sizeof((a)[0]))
#define GENL_MAX_REGISTERED_FAMILIES      8

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct genlmsghdr
{
  uint8_t cmd;
  uint8_t version;
  uint16_t reserved;
};

struct generic_family_s
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

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct generic_family_s g_generic_families[] =
{
  {
    GENL_ID_CTRL,
    "nlctrl",
    CTRL_VERSION,
    0,
    CTRL_ATTR_MCAST_GROUPS,
    NULL,
    0,
    NULL
  }
};

static struct generic_family_s g_registered[GENL_MAX_REGISTERED_FAMILIES];
static uint16_t g_next_family_id = GENL_START_ALLOC;
static uint32_t g_next_group_id = 1;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool generic_align_cursor(FAR char **cursor, FAR char *end,
                                 size_t len)
{
  size_t aligned = NLA_ALIGN(len);

  if (*cursor + aligned > end)
    {
      return false;
    }

  if (aligned > len)
    {
      memset(*cursor + len, 0, aligned - len);
    }

  *cursor += aligned;
  return true;
}

static bool generic_put_attr(FAR char **cursor, FAR char *end,
                             uint16_t type, FAR const void *data,
                             size_t len)
{
  FAR struct nlattr *attr;
  size_t attrlen;

  attrlen = NLA_HDRLEN + len;
  if (*cursor + NLA_ALIGN(attrlen) > end)
    {
      return false;
    }

  attr = (FAR struct nlattr *)*cursor;
  attr->nla_len  = attrlen;
  attr->nla_type = type;

  if (len > 0)
    {
      memcpy(nla_data(attr), data, len);
    }

  return generic_align_cursor(cursor, end, attrlen);
}

static bool generic_put_u16(FAR char **cursor, FAR char *end,
                            uint16_t type, uint16_t value)
{
  return generic_put_attr(cursor, end, type, &value, sizeof(value));
}

static bool generic_put_u32(FAR char **cursor, FAR char *end,
                            uint16_t type, uint32_t value)
{
  return generic_put_attr(cursor, end, type, &value, sizeof(value));
}

static bool generic_put_string(FAR char **cursor, FAR char *end,
                               uint16_t type, FAR const char *value)
{
  return generic_put_attr(cursor, end, type, value, strlen(value) + 1);
}

static FAR struct nlattr *generic_start_nested(FAR char **cursor,
                                               FAR char *end,
                                               uint16_t type)
{
  FAR struct nlattr *attr;

  if (*cursor + NLA_HDRLEN > end)
    {
      return NULL;
    }

  attr = (FAR struct nlattr *)*cursor;
  attr->nla_len  = NLA_HDRLEN;
  attr->nla_type = type | NLA_F_NESTED;
  *cursor += NLA_HDRLEN;

  return attr;
}

static bool generic_end_nested(FAR char **cursor, FAR char *end,
                               FAR struct nlattr *attr)
{
  size_t attrlen;

  DEBUGASSERT((FAR char *)attr <= *cursor);

  attrlen = *cursor - (FAR char *)attr;
  attr->nla_len = attrlen;

  *cursor = (FAR char *)attr;
  return generic_align_cursor(cursor, end, attrlen);
}

static FAR const struct generic_family_s *
generic_find_family(FAR const char *name)
{
  size_t i;

  for (i = 0; i < GENL_ARRAY_SIZE(g_generic_families); i++)
    {
      if (strcmp(g_generic_families[i].name, name) == 0)
        {
          return &g_generic_families[i];
        }
    }

  for (i = 0; i < GENL_ARRAY_SIZE(g_registered); i++)
    {
      if (g_registered[i].name != NULL &&
          strcmp(g_registered[i].name, name) == 0)
        {
          return &g_registered[i];
        }
    }

  return NULL;
}

static FAR const struct generic_family_s *generic_find_family_by_id(
    uint16_t id)
{
  size_t i;

  for (i = 0; i < GENL_ARRAY_SIZE(g_generic_families); i++)
    {
      if (g_generic_families[i].id == id)
        {
          return &g_generic_families[i];
        }
    }

  for (i = 0; i < GENL_ARRAY_SIZE(g_registered); i++)
    {
      if (g_registered[i].name != NULL && g_registered[i].id == id)
        {
          return &g_registered[i];
        }
    }

  return NULL;
}

static FAR const char *generic_get_family_name(FAR const struct nlmsghdr *req)
{
  FAR struct nlattr *attr;
  int attrlen;
  int rem;

  attrlen = nlmsg_attrlen(req, GENL_HDRLEN);
  if (attrlen <= 0)
    {
      return NULL;
    }

  nla_for_each_attr(attr, nlmsg_attrdata(req, GENL_HDRLEN), attrlen, rem)
    {
      if (nla_type(attr) == CTRL_ATTR_FAMILY_NAME && nla_len(attr) > 0)
        {
          return (FAR const char *)nla_data(attr);
        }
    }

  return NULL;
}

static bool generic_put_groups(FAR const struct generic_family_s *family,
                               FAR char **cursor, FAR char *end)
{
  FAR struct nlattr *groups;
  size_t i;

  if (family->groups == NULL || family->ngroups == 0)
    {
      return true;
    }

  groups = generic_start_nested(cursor, end, CTRL_ATTR_MCAST_GROUPS);
  if (groups == NULL)
    {
      return false;
    }

  for (i = 0; i < family->ngroups; i++)
    {
      FAR struct nlattr *group;

      group = generic_start_nested(cursor, end, i + 1);
      if (group == NULL ||
          !generic_put_string(cursor, end, CTRL_ATTR_MCAST_GRP_NAME,
                              family->groups[i].name) ||
          !generic_put_u32(cursor, end, CTRL_ATTR_MCAST_GRP_ID,
                           family->groups[i].id) ||
          !generic_end_nested(cursor, end, group))
        {
          return false;
        }
    }

  return generic_end_nested(cursor, end, groups);
}

static FAR struct netlink_response_s *
generic_alloc_family_response(FAR const struct generic_family_s *family,
                              FAR const struct nlmsghdr *req,
                              bool multipart)
{
  FAR struct netlink_response_s *resp;
  FAR struct genlmsghdr *genlhdr;
  FAR char *cursor;
  FAR char *end;
  size_t payload;

  resp = kmm_zalloc(SIZEOF_NETLINK_RESPONSE_S(GENL_RESPONSE_PAYLOAD_MAX));
  if (resp == NULL)
    {
      return NULL;
    }

  genlhdr = (FAR struct genlmsghdr *)nlmsg_data(&resp->msg);
  genlhdr->cmd = CTRL_CMD_NEWFAMILY;
  genlhdr->version = CTRL_VERSION;

  cursor = (FAR char *)genlhdr + GENL_HDRLEN;
  end = cursor + GENL_RESPONSE_PAYLOAD_MAX - GENL_HDRLEN;

  if (!generic_put_u16(&cursor, end, CTRL_ATTR_FAMILY_ID, family->id) ||
      !generic_put_string(&cursor, end, CTRL_ATTR_FAMILY_NAME,
                          family->name) ||
      !generic_put_u32(&cursor, end, CTRL_ATTR_VERSION, family->version) ||
      !generic_put_u32(&cursor, end, CTRL_ATTR_HDRSIZE, family->hdrsize) ||
      !generic_put_u32(&cursor, end, CTRL_ATTR_MAXATTR, family->maxattr))
    {
      kmm_free(resp);
      return NULL;
    }

  if (!generic_put_groups(family, &cursor, end))
    {
      kmm_free(resp);
      return NULL;
    }

  payload = cursor - (FAR char *)genlhdr;

  resp->msg.nlmsg_len   = NLMSG_LENGTH(payload);
  resp->msg.nlmsg_type  = GENL_ID_CTRL;
  resp->msg.nlmsg_flags = multipart ? NLM_F_MULTI : 0;
  resp->msg.nlmsg_seq   = req->nlmsg_seq;
  resp->msg.nlmsg_pid   = req->nlmsg_pid;

  return resp;
}

static int generic_add_ack(NETLINK_HANDLE handle,
                           FAR const struct nlmsghdr *req, int error)
{
  FAR struct netlink_response_s *resp;
  FAR struct nlmsgerr *err;

  if ((req->nlmsg_flags & NLM_F_ACK) == 0)
    {
      return OK;
    }

  resp = kmm_zalloc(SIZEOF_NETLINK_RESPONSE_S(sizeof(struct nlmsgerr)));
  if (resp == NULL)
    {
      return -ENOMEM;
    }

  err = (FAR struct nlmsgerr *)NLMSG_DATA(&resp->msg);
  err->error = error;
  memcpy(&err->msg, req, sizeof(*req));

  resp->msg.nlmsg_len   = NLMSG_LENGTH(sizeof(struct nlmsgerr));
  resp->msg.nlmsg_type  = NLMSG_ERROR;
  resp->msg.nlmsg_flags = 0;
  resp->msg.nlmsg_seq   = req->nlmsg_seq;
  resp->msg.nlmsg_pid   = req->nlmsg_pid;

  netlink_add_response(handle, resp);
  return OK;
}

static int generic_getfamily(NETLINK_HANDLE handle,
                             FAR const struct nlmsghdr *req)
{
  FAR const struct generic_family_s *family;
  FAR const char *name;
  size_t i;
  bool multipart;

  name = generic_get_family_name(req);
  if (name != NULL)
    {
      FAR struct netlink_response_s *resp;

      family = generic_find_family(name);
      if (family == NULL)
        {
          return generic_add_ack(handle, req, -ENOENT);
        }

      resp = generic_alloc_family_response(family, req, false);
      if (resp == NULL)
        {
          return -ENOMEM;
        }

      netlink_add_response(handle, resp);
      return generic_add_ack(handle, req, 0);
    }

  multipart = (req->nlmsg_flags & NLM_F_DUMP) != 0;
  for (i = 0; i < GENL_ARRAY_SIZE(g_generic_families); i++)
    {
      FAR struct netlink_response_s *resp;

      resp = generic_alloc_family_response(&g_generic_families[i], req,
                                           multipart);
      if (resp == NULL)
        {
          return -ENOMEM;
        }

      netlink_add_response(handle, resp);
    }

  for (i = 0; i < GENL_ARRAY_SIZE(g_registered); i++)
    {
      FAR struct netlink_response_s *resp;

      if (g_registered[i].name == NULL)
        {
          continue;
        }

      resp = generic_alloc_family_response(&g_registered[i], req,
                                           multipart);
      if (resp == NULL)
        {
          return -ENOMEM;
        }

      netlink_add_response(handle, resp);
    }

  if (multipart)
    {
      return netlink_add_terminator(handle, req, 0);
    }

  return generic_add_ack(handle, req, 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int netlink_generic_register(FAR struct netlink_generic_family_s *family)
{
  size_t i;

  if (family == NULL || family->name == NULL || family->name[0] == '\0')
    {
      return -EINVAL;
    }

  if (generic_find_family(family->name) != NULL)
    {
      nerr("ERROR: duplicate generic netlink family %s\n", family->name);
      return -EEXIST;
    }

  for (i = 0; i < GENL_ARRAY_SIZE(g_registered); i++)
    {
      if (g_registered[i].name == NULL)
        {
          unsigned int j;

          if (family->id == 0)
            {
              family->id = g_next_family_id++;
            }

          for (j = 0; j < family->ngroups; j++)
            {
              if (family->groups[j].id == 0)
                {
                  family->groups[j].id = g_next_group_id++;
                }
              else if (family->groups[j].id >= g_next_group_id)
                {
                  g_next_group_id = family->groups[j].id + 1;
                }
            }

          g_registered[i].id      = family->id;
          g_registered[i].name    = family->name;
          g_registered[i].version = family->version;
          g_registered[i].hdrsize = family->hdrsize;
          g_registered[i].maxattr = family->maxattr;
          g_registered[i].groups  = family->groups;
          g_registered[i].ngroups = family->ngroups;
          g_registered[i].sendto  = family->sendto;
          ninfo("registered generic netlink family %s id=%u groups=%u\n",
                family->name, family->id, family->ngroups);
          return OK;
        }
    }

  return -ENOSPC;
}

int netlink_generic_unregister(uint16_t id, FAR const char *name)
{
  size_t i;

  for (i = 0; i < GENL_ARRAY_SIZE(g_registered); i++)
    {
      if (g_registered[i].name != NULL &&
          (g_registered[i].id == id ||
           (name != NULL && strcmp(g_registered[i].name, name) == 0)))
        {
          memset(&g_registered[i], 0, sizeof(g_registered[i]));
          return OK;
        }
    }

  return -ENOENT;
}

ssize_t netlink_generic_sendto(NETLINK_HANDLE handle,
                               FAR const struct nlmsghdr *nlmsg,
                               size_t len, int flags,
                               FAR const struct sockaddr_nl *to,
                               socklen_t tolen)
{
  FAR const struct genlmsghdr *genlhdr;
  int ret;

  UNUSED(flags);
  UNUSED(to);
  UNUSED(tolen);

  if (len < sizeof(struct nlmsghdr) ||
      nlmsg->nlmsg_len < NLMSG_LENGTH(GENL_HDRLEN) ||
      nlmsg->nlmsg_len > len)
    {
      return -EINVAL;
    }

  genlhdr = (FAR const struct genlmsghdr *)nlmsg_data(nlmsg);

  if (nlmsg->nlmsg_type == GENL_ID_CTRL &&
      genlhdr->cmd == CTRL_CMD_GETFAMILY)
    {
      ret = generic_getfamily(handle, nlmsg);
      return ret < 0 ? ret : (ssize_t)len;
    }

  if (nlmsg->nlmsg_type >= GENL_START_ALLOC)
    {
      FAR const struct generic_family_s *family;

      family = generic_find_family_by_id(nlmsg->nlmsg_type);
      if (family == NULL)
        {
          ret = generic_add_ack(handle, nlmsg, -ENOENT);
          return ret < 0 ? ret : (ssize_t)len;
        }

      if (family->sendto != NULL)
        {
          return family->sendto(handle, nlmsg, len, flags, to, tolen);
        }
    }

  ret = generic_add_ack(handle, nlmsg, -EOPNOTSUPP);
  return ret < 0 ? ret : (ssize_t)len;
}

#endif /* CONFIG_NETLINK_GENERIC */
