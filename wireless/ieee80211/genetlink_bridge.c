/****************************************************************************
 * wireless/ieee80211/genetlink_bridge.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal Linux generic-netlink bridge for the imported IEEE 802.11 stack.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <linux/netlink.h>
#include <linux/nl80211.h>
#include <linux/notifier.h>

#include <nuttx/kmalloc.h>
#include <nuttx/net/netlink_kernel.h>
#include <nuttx/queue.h>

#include <net/genetlink.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

int netlink_add_terminator(NETLINK_HANDLE handle,
                           FAR const struct nlmsghdr *req, int group);

ssize_t nl80211_metadata_sendto(NETLINK_HANDLE handle,
                                FAR const struct nlmsghdr *nlmsg,
                                size_t len, int flags,
                                FAR const struct sockaddr_nl *to,
                                socklen_t tolen) __attribute__((weak));

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define GENL_BRIDGE_MAX_FAMILIES 8
#define GENL_BRIDGE_MAX_NOTIFIERS 8

static FAR struct genl_family *g_genl_families[GENL_BRIDGE_MAX_FAMILIES];
static FAR struct notifier_block
  *g_netlink_notifiers[GENL_BRIDGE_MAX_NOTIFIERS];

struct net init_net;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static FAR struct genl_family *genl_bridge_find_family(uint16_t id)
{
  int i;

  for (i = 0; i < GENL_BRIDGE_MAX_FAMILIES; i++)
    {
      if (g_genl_families[i] != NULL && g_genl_families[i]->id == id)
        {
          return g_genl_families[i];
        }
    }

  return NULL;
}

static FAR struct genl_family *genl_bridge_find_family_by_name(
    FAR const char *name)
{
  int i;

  for (i = 0; i < GENL_BRIDGE_MAX_FAMILIES; i++)
    {
      if (g_genl_families[i] != NULL &&
          strcmp(g_genl_families[i]->name, name) == 0)
        {
          return g_genl_families[i];
        }
    }

  return NULL;
}

static int genl_bridge_add_ack(NETLINK_HANDLE handle,
                               FAR const struct nlmsghdr *req, int error)
{
  FAR struct netlink_response_s *resp;
  FAR struct nlmsgerr *err;
  FAR const struct genlmsghdr *genlhdr;

  if ((req->nlmsg_flags & NLM_F_ACK) == 0)
    {
      return 0;
    }

  genlhdr = (FAR const struct genlmsghdr *)NLMSG_DATA(req);
  nuttx_hwsim_debugf("genl_bridge: ack type=%u cmd=%u seq=%u error=%d\n",
         req->nlmsg_type, genlhdr->cmd, req->nlmsg_seq, error);
  (void)genlhdr;

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
  return 0;
}

static bool genl_bridge_nlmsg_valid(FAR const struct nlmsghdr *nlh,
                                    size_t remaining)
{
  return remaining >= sizeof(*nlh) &&
         nlh->nlmsg_len >= sizeof(*nlh) &&
         nlh->nlmsg_len <= remaining;
}

static int genl_bridge_add_whole_skb_response(NETLINK_HANDLE handle,
                                              FAR const struct nlmsghdr *req,
                                              FAR const struct sk_buff *skb)
{
  FAR const struct nlmsghdr *nlh;
  FAR struct netlink_response_s *resp;

  if (skb == NULL || skb->len < sizeof(struct nlmsghdr))
    {
      return -EINVAL;
    }

  nlh = (FAR const struct nlmsghdr *)skb->data;
  nuttx_hwsim_debugf("genl_bridge: whole skb response skb_len=%u first_len=%u "
         "first_type=%u flags=0x%x req_flags=0x%x\n",
         skb->len, nlh->nlmsg_len, nlh->nlmsg_type, nlh->nlmsg_flags,
         req->nlmsg_flags);
  (void)nlh;

  resp = kmm_zalloc(SIZEOF_NETLINK_RESPONSE_S(skb->len -
                                              sizeof(struct nlmsghdr)));
  if (resp == NULL)
    {
      return -ENOMEM;
    }

  memcpy(&resp->msg, skb->data, skb->len);
  resp->msg.nlmsg_len = skb->len;
  resp->msg.nlmsg_seq = req->nlmsg_seq;
  resp->msg.nlmsg_pid = req->nlmsg_pid;
  netlink_add_response(handle, resp);
  return 0;
}

static int genl_bridge_add_skb_response(NETLINK_HANDLE handle,
                                        FAR const struct nlmsghdr *req,
                                        FAR const struct sk_buff *skb)
{
  FAR const struct nlmsghdr *nlh;
  FAR struct netlink_response_s *resp;
  size_t aligned;
  size_t remaining;
  size_t msglen;

  if (skb == NULL || skb->len == 0)
    {
      return 0;
    }

  remaining = skb->len;
  nlh = (FAR const struct nlmsghdr *)skb->data;
  nuttx_hwsim_debugf("genl_bridge: split skb len=%zu first_len=%u first_type=%u\n",
         remaining, remaining >= sizeof(*nlh) ? nlh->nlmsg_len : 0,
         remaining >= sizeof(*nlh) ? nlh->nlmsg_type : 0);

  while (remaining > 0)
    {
      if (remaining < sizeof(*nlh))
        {
          FAR const unsigned char *pad = (FAR const unsigned char *)nlh;
          size_t i;

          for (i = 0; i < remaining; i++)
            {
              if (pad[i] != 0)
                {
                  return genl_bridge_add_whole_skb_response(handle, req,
                                                            skb);
                }
            }

          return 0;
        }

      if (nlh->nlmsg_len < sizeof(*nlh) || nlh->nlmsg_len > remaining)
        {
          nuttx_hwsim_debugf("genl_bridge: bad nlmsg remaining=%zu len=%u type=%u\n",
                 remaining, nlh->nlmsg_len, nlh->nlmsg_type);
          return genl_bridge_add_whole_skb_response(handle, req, skb);
        }

      msglen = nlh->nlmsg_len;
      aligned = NLMSG_ALIGN(msglen);
      if (aligned < remaining)
        {
          FAR const struct nlmsghdr *aligned_next;
          FAR const struct nlmsghdr *raw_next;

          aligned_next = (FAR const struct nlmsghdr *)
                         ((FAR const char *)nlh + aligned);
          raw_next = (FAR const struct nlmsghdr *)
                     ((FAR const char *)nlh + msglen);
          if (!genl_bridge_nlmsg_valid(aligned_next, remaining - aligned) &&
              genl_bridge_nlmsg_valid(raw_next, remaining - msglen))
            {
              aligned = msglen;
            }
          else if (!genl_bridge_nlmsg_valid(aligned_next,
                                            remaining - aligned) &&
                   !genl_bridge_nlmsg_valid(raw_next,
                                            remaining - msglen))
            {
              FAR const unsigned char *bytes;
              size_t off;
              bool found = false;

              nuttx_hwsim_debugf("genl_bridge: no valid next raw_len=%u raw_type=%u "
                     "aligned_len=%u aligned_type=%u msglen=%zu "
                     "aligned=%zu remaining=%zu\n",
                     raw_next->nlmsg_len, raw_next->nlmsg_type,
                     aligned_next->nlmsg_len, aligned_next->nlmsg_type,
                     msglen, aligned, remaining);
              bytes = (FAR const unsigned char *)nlh;
              nuttx_hwsim_debugf("genl_bridge: bytes[248..291]=");
              for (off = 248; off < remaining && off < 292; off++)
                {
                  printf("%02x", bytes[off]);
                }
              printf("\n");
              for (off = msglen; off < remaining && off < msglen + 128;
                   off++)
                {
                  FAR const struct nlmsghdr *scan;

                  scan = (FAR const struct nlmsghdr *)
                         ((FAR const char *)nlh + off);
                  if (genl_bridge_nlmsg_valid(scan, remaining - off))
                    {
                      nuttx_hwsim_debugf("genl_bridge: candidate next off=%zu len=%u "
                             "type=%u seq=%u pid=%u\n", off,
                             scan->nlmsg_len, scan->nlmsg_type,
                             scan->nlmsg_seq, scan->nlmsg_pid);
                      if (!found && scan->nlmsg_type == nlh->nlmsg_type &&
                          (scan->nlmsg_flags & NLM_F_MULTI) != 0)
                        {
                          aligned = off;
                          found = true;
                        }
                    }
                }
            }
        }
      else if (aligned > remaining)
        {
          if (msglen != remaining)
            {
              nuttx_hwsim_debugf("genl_bridge: bad aligned remaining=%zu len=%zu aligned=%zu\n",
                     remaining, msglen, aligned);
              return genl_bridge_add_whole_skb_response(handle, req, skb);
            }

          aligned = msglen;
        }

      resp = kmm_zalloc(SIZEOF_NETLINK_RESPONSE_S(msglen - sizeof(*nlh)));
      if (resp == NULL)
        {
          return -ENOMEM;
        }

      memcpy(&resp->msg, nlh, msglen);
      resp->msg.nlmsg_seq = req->nlmsg_seq;
      resp->msg.nlmsg_pid = req->nlmsg_pid;
      netlink_add_response(handle, resp);

      remaining -= aligned;
      nlh = (FAR const struct nlmsghdr *)
            ((FAR const char *)nlh + aligned);
    }

  return 0;
}

static FAR const struct genl_ops *genl_bridge_find_ops(
    FAR const struct genl_family *family, uint8_t cmd)
{
  unsigned int i;

  for (i = 0; i < family->n_ops; i++)
    {
      if (family->ops[i].cmd == cmd)
        {
          return &family->ops[i];
        }
    }

  return NULL;
}

static FAR const struct genl_small_ops *genl_bridge_find_small_ops(
    FAR const struct genl_family *family, uint8_t cmd)
{
  unsigned int i;

  for (i = 0; i < family->n_small_ops; i++)
    {
      if (family->small_ops[i].cmd == cmd)
        {
          return &family->small_ops[i];
        }
    }

  return NULL;
}

static int genl_bridge_dispatch(NETLINK_HANDLE handle,
                                FAR struct genl_family *family,
                                FAR const struct nlmsghdr *nlmsg,
                                size_t len)
{
  FAR const struct genl_ops *ops;
  FAR const struct genl_small_ops *small_ops;
  int (*doit)(FAR struct sk_buff *skb, FAR struct genl_info *info);
  int (*dumpit)(FAR struct sk_buff *skb, FAR struct netlink_callback *cb);
  int (*done)(FAR struct netlink_callback *cb);
  int (*start)(FAR struct netlink_callback *cb);
  FAR struct genlmsghdr *genlhdr;
  FAR struct nlattr **attrs;
  struct genl_dumpit_info dump_info;
  struct netlink_callback cb;
  struct netlink_ext_ack extack;
  struct genl_split_ops split;
  struct genl_info info;
  FAR struct sk_buff *skb;
  size_t skb_size;
  unsigned int maxattr;
  bool is_dump;
  bool dump_more;
  int ret = -EOPNOTSUPP;

  if (len < NLMSG_HDRLEN + GENL_HDRLEN ||
      nlmsg->nlmsg_len < NLMSG_HDRLEN + GENL_HDRLEN ||
      nlmsg->nlmsg_len > len)
    {
      return -EINVAL;
    }

  genlhdr = (FAR struct genlmsghdr *)NLMSG_DATA(nlmsg);
  is_dump = (nlmsg->nlmsg_flags & NLM_F_DUMP) != 0;
  nuttx_hwsim_debugf("genl_bridge: enter family=%s cmd=%u flags=0x%x len=%zu dump=%u\n",
         family->name, genlhdr->cmd, nlmsg->nlmsg_flags, len,
         is_dump ? 1 : 0);

  ops = genl_bridge_find_ops(family, genlhdr->cmd);
  small_ops = genl_bridge_find_small_ops(family, genlhdr->cmd);
  if (ops == NULL && small_ops == NULL)
    {
      nuttx_hwsim_debugf("genl_bridge: no ops family=%s cmd=%u\n",
             family->name, genlhdr->cmd);
      return -EOPNOTSUPP;
    }

  maxattr = family->maxattr;
  if (ops != NULL && ops->maxattr > 0)
    {
      maxattr = ops->maxattr;
    }

  attrs = kmm_calloc(maxattr + 1, sizeof(attrs[0]));
  if (attrs == NULL)
    {
      return -ENOMEM;
    }

  memset(&extack, 0, sizeof(extack));
  ret = __nlmsg_parse(nlmsg, family->hdrsize + GENL_HDRLEN, attrs, maxattr,
                      ops != NULL && ops->policy != NULL ? ops->policy :
                      family->policy, NL_VALIDATE_LIBERAL, &extack);
  if (ret < 0)
    {
      nuttx_hwsim_debugf("genl_bridge: parse failed family=%s cmd=%u ret=%d\n",
             family->name, genlhdr->cmd, ret);
      goto out_free_attrs;
    }

  skb_size = is_dump ? 65536 : nlmsg->nlmsg_len;
  skb = alloc_skb(skb_size, GFP_KERNEL);
  if (skb == NULL)
    {
      ret = -ENOMEM;
      goto out_free_attrs;
    }

  memcpy(skb_put(skb, nlmsg->nlmsg_len), nlmsg, nlmsg->nlmsg_len);

  memset(&split, 0, sizeof(split));
  memset(&info, 0, sizeof(info));
  doit = NULL;
  dumpit = NULL;
  done = NULL;
  start = NULL;

  if (ops != NULL)
    {
      split.cmd = ops->cmd;
      split.internal_flags = ops->internal_flags;
      split.flags = ops->flags;
      split.validate = ops->validate;
      split.policy = ops->policy;
      split.maxattr = ops->maxattr;
      if (is_dump)
        {
          split.start = ops->start;
          split.dumpit = ops->dumpit;
          split.done = ops->done;
          start = ops->start;
          dumpit = ops->dumpit;
          done = ops->done;
        }
      else
        {
          split.doit = ops->doit;
          doit = ops->doit;
        }
    }
  else
    {
      split.cmd = small_ops->cmd;
      split.internal_flags = small_ops->internal_flags;
      split.flags = small_ops->flags;
      split.validate = small_ops->validate;
      split.policy = family->policy;
      split.maxattr = family->maxattr;
      if (is_dump)
        {
          split.dumpit = small_ops->dumpit;
          dumpit = small_ops->dumpit;
        }
      else
        {
          split.doit = small_ops->doit;
          doit = small_ops->doit;
        }
    }

  info.snd_seq = nlmsg->nlmsg_seq;
  info.snd_portid = nlmsg->nlmsg_pid;
  info.family = family;
  info.nlhdr = nlmsg;
  info.genlhdr = genlhdr;
  info.attrs = attrs;
  info.extack = &extack;
  genl_info_net_set(&info, &init_net);

  init_net.genl_sock = handle;

  if (!is_dump && family->pre_doit != NULL && doit != NULL)
    {
      ret = family->pre_doit(&split, skb, &info);
      if (ret < 0)
        {
          nuttx_hwsim_debugf("genl_bridge: pre_doit failed family=%s cmd=%u ret=%d\n",
                 family->name, genlhdr->cmd, ret);
          goto out_free_skb;
        }
    }

  nuttx_hwsim_debugf("genl_bridge: dispatch family=%s cmd=%u flags=0x%x dump=%u\n",
         family->name, genlhdr->cmd, nlmsg->nlmsg_flags, is_dump ? 1 : 0);

  if (is_dump && dumpit != NULL)
    {
      memset(&dump_info, 0, sizeof(dump_info));
      memset(&cb, 0, sizeof(cb));

      dump_info.op = split;
      dump_info.info = info;

      cb.skb = skb;
      cb.nlh = (FAR struct nlmsghdr *)nlmsg;
      cb.data = &dump_info;
      cb.portid = nlmsg->nlmsg_pid;
      cb.seq = nlmsg->nlmsg_seq;
      cb.extack = &extack;

      if (start != NULL)
        {
          ret = start(&cb);
          nuttx_hwsim_debugf("genl_bridge: dump start family=%s cmd=%u ret=%d\n",
                 family->name, genlhdr->cmd, ret);
          if (ret < 0)
            {
              goto out_free_skb;
            }
        }

      do
        {
          skb_trim(skb, 0);

          ret = dumpit(skb, &cb);
          nuttx_hwsim_debugf("genl_bridge: dumpit family=%s cmd=%u ret=%d skb_len=%u\n",
                 family->name, genlhdr->cmd, ret, skb->len);
          if (ret < 0)
            {
              break;
            }

          dump_more = skb->len > 0;

          if (dump_more)
            {
              ret = genl_bridge_add_skb_response(handle, nlmsg, skb);
              nuttx_hwsim_debugf("genl_bridge: add dump response family=%s cmd=%u ret=%d\n",
                     family->name, genlhdr->cmd, ret);
              if (ret < 0)
                {
                  break;
                }
            }
        }
      while (dump_more);

      if (ret == 0)
        {
          ret = netlink_add_terminator(handle, nlmsg, 0);
          nuttx_hwsim_debugf("genl_bridge: add dump done family=%s cmd=%u ret=%d\n",
                 family->name, genlhdr->cmd, ret);
        }

      if (done != NULL)
        {
          done(&cb);
        }
    }
  else if (doit != NULL)
    {
      ret = doit(skb, &info);
    }

  nuttx_hwsim_debugf("genl_bridge: complete family=%s cmd=%u ret=%d\n",
         family->name, genlhdr->cmd, ret);

  if (!is_dump && family->post_doit != NULL && doit != NULL)
    {
      skb->data = skb->head;
      skb->len = nlmsg->nlmsg_len;
      skb->tail = skb->data + skb->len;
      family->post_doit(&split, skb, &info);
    }

out_free_skb:
  consume_skb(skb);
out_free_attrs:
  kmm_free(attrs);
  return ret;
}

static ssize_t genl_bridge_sendto(NETLINK_HANDLE handle,
                                  FAR const struct nlmsghdr *nlmsg,
                                  size_t len, int flags,
                                  FAR const struct sockaddr_nl *to,
                                  socklen_t tolen)
{
  FAR struct genl_family *family;
  int ret;

  family = genl_bridge_find_family(nlmsg->nlmsg_type);
  if (family == NULL)
    {
      return -ENOENT;
    }

#ifdef CONFIG_WIRELESS_IEEE80211_NL80211_METADATA_ONLY
  if (strcmp(family->name, NL80211_GENL_NAME) == 0 &&
      nl80211_metadata_sendto != NULL)
    {
      return nl80211_metadata_sendto(handle, nlmsg, len, flags, to, tolen);
    }
#endif

  ret = genl_bridge_dispatch(handle, family, nlmsg, len);
  genl_bridge_add_ack(handle, nlmsg, ret);

  return len;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifndef __WIRELESS_IEEE80211_SKBUFF_COMPAT_INLINE
unsigned int skb_tailroom(const struct sk_buff *skb)
{
  return skb->end - skb->tail;
}
#endif

void *__skb_put(struct sk_buff *skb, unsigned int len)
{
  return skb_put(skb, len);
}

struct nlmsghdr *__nlmsg_put(struct sk_buff *skb, u32 portid, u32 seq,
                             int type, int len, int flags)
{
  FAR struct nlmsghdr *nlh;

  nlh = (FAR struct nlmsghdr *)skb_put(skb, NLMSG_ALIGN(NLMSG_LENGTH(len)));
  memset(nlh, 0, NLMSG_ALIGN(NLMSG_LENGTH(len)));

  nlh->nlmsg_type = type;
  nlh->nlmsg_len = NLMSG_LENGTH(len);
  nlh->nlmsg_flags = flags;
  nlh->nlmsg_pid = portid;
  nlh->nlmsg_seq = seq;

  return nlh;
}

#ifndef __WIRELESS_IEEE80211_SKBUFF_COMPAT_INLINE
struct sk_buff *alloc_skb(unsigned int size, gfp_t priority)
{
  return dev_alloc_skb(size);
}
#endif

struct sk_buff *netlink_alloc_large_skb(unsigned int size,
                                        unsigned int broadcast)
{
  (void)broadcast;
  return alloc_skb(size, GFP_KERNEL);
}

#ifndef __WIRELESS_IEEE80211_SKBUFF_COMPAT_INLINE
void skb_trim(struct sk_buff *skb, unsigned int len)
{
  if (skb == NULL || len >= skb->len)
    {
      return;
    }

  skb->len = len;
  skb->tail = skb->data + len;
}

void kfree_skb(struct sk_buff *skb)
{
  dev_kfree_skb(skb);
}

void consume_skb(struct sk_buff *skb)
{
  dev_kfree_skb(skb);
}
#endif

void *kmemdup_noprof(const void *src, size_t len, gfp_t gfp)
{
  return kmemdup(src, len, gfp);
}

struct nlattr *nla_find(const struct nlattr *head, int len, int attrtype)
{
  const struct nlattr *nla;
  int rem = len;

  for (nla = head; nla_ok(nla, rem); nla = nla_next(nla, &rem))
    {
      if (nla_type(nla) == attrtype)
        {
          return (FAR struct nlattr *)nla;
        }
    }

  return NULL;
}

int nla_memcpy(void *dest, const struct nlattr *src, int count)
{
  int len;

  if (src == NULL || dest == NULL)
    {
      return 0;
    }

  len = min(nla_len(src), count);
  memcpy(dest, nla_data(src), len);
  if (len < count)
    {
      memset((FAR char *)dest + len, 0, count - len);
    }

  return len;
}

struct nlattr *__nla_reserve(struct sk_buff *skb, int attrtype, int attrlen)
{
  FAR struct nlattr *nla;
  int total;

  total = nla_total_size(attrlen);
  if (skb_tailroom(skb) < (unsigned int)total)
    {
      return NULL;
    }

  nla = (FAR struct nlattr *)skb_put(skb, total);
  memset(nla, 0, total);
  nla->nla_type = attrtype;
  nla->nla_len = nla_attr_size(attrlen);
  return nla;
}

struct nlattr *nla_reserve(struct sk_buff *skb, int attrtype, int attrlen)
{
  return __nla_reserve(skb, attrtype, attrlen);
}

void *__nla_reserve_nohdr(struct sk_buff *skb, int attrlen)
{
  if (skb_tailroom(skb) < (unsigned int)NLA_ALIGN(attrlen))
    {
      return NULL;
    }

  return skb_put(skb, NLA_ALIGN(attrlen));
}

void *nla_reserve_nohdr(struct sk_buff *skb, int attrlen)
{
  return __nla_reserve_nohdr(skb, attrlen);
}

void __nla_put(struct sk_buff *skb, int attrtype, int attrlen,
               const void *data)
{
  FAR struct nlattr *nla;

  nla = __nla_reserve(skb, attrtype, attrlen);
  if (nla != NULL && attrlen > 0 && data != NULL)
    {
      memcpy(nla_data(nla), data, attrlen);
    }
}

int nla_put(struct sk_buff *skb, int attrtype, int attrlen, const void *data)
{
  FAR struct nlattr *nla;

  nla = __nla_reserve(skb, attrtype, attrlen);
  if (nla == NULL)
    {
      return -EMSGSIZE;
    }

  if (attrlen > 0 && data != NULL)
    {
      memcpy(nla_data(nla), data, attrlen);
    }

  return 0;
}

void __nla_put_nohdr(struct sk_buff *skb, int attrlen, const void *data)
{
  FAR void *dst = __nla_reserve_nohdr(skb, attrlen);

  if (dst != NULL && attrlen > 0 && data != NULL)
    {
      memcpy(dst, data, attrlen);
    }
}

int nla_put_nohdr(struct sk_buff *skb, int attrlen, const void *data)
{
  FAR void *dst = __nla_reserve_nohdr(skb, attrlen);

  if (dst == NULL)
    {
      return -EMSGSIZE;
    }

  if (attrlen > 0 && data != NULL)
    {
      memcpy(dst, data, attrlen);
    }

  return 0;
}

struct nlattr *__nla_reserve_64bit(struct sk_buff *skb, int attrtype,
                                   int attrlen, int padattr)
{
  (void)padattr;
  return __nla_reserve(skb, attrtype, attrlen);
}

struct nlattr *nla_reserve_64bit(struct sk_buff *skb, int attrtype,
                                 int attrlen, int padattr)
{
  return __nla_reserve_64bit(skb, attrtype, attrlen, padattr);
}

void __nla_put_64bit(struct sk_buff *skb, int attrtype, int attrlen,
                     const void *data, int padattr)
{
  (void)padattr;
  __nla_put(skb, attrtype, attrlen, data);
}

int nla_put_64bit(struct sk_buff *skb, int attrtype, int attrlen,
                  const void *data, int padattr)
{
  (void)padattr;
  return nla_put(skb, attrtype, attrlen, data);
}

int nla_append(struct sk_buff *skb, int attrlen, const void *data)
{
  return nla_put_nohdr(skb, attrlen, data);
}

int __nla_validate(const struct nlattr *head, int len, int maxtype,
                   const struct nla_policy *policy, unsigned int validate,
                   struct netlink_ext_ack *extack)
{
  (void)head;
  (void)len;
  (void)maxtype;
  (void)policy;
  (void)validate;
  (void)extack;
  return 0;
}

int __nla_parse(struct nlattr **tb, int maxtype, const struct nlattr *head,
                int len, const struct nla_policy *policy,
                unsigned int validate, struct netlink_ext_ack *extack)
{
  FAR const struct nlattr *nla;
  int rem = len;
  int type;

  (void)policy;
  (void)validate;
  (void)extack;

  memset(tb, 0, sizeof(tb[0]) * (maxtype + 1));

  for (nla = head; nla_ok(nla, rem); nla = nla_next(nla, &rem))
    {
      type = nla_type(nla);
      if (type <= maxtype)
        {
          tb[type] = (FAR struct nlattr *)nla;
        }
    }

  return 0;
}

int nla_policy_len(const struct nla_policy *policy, int n)
{
  (void)policy;
  (void)n;
  return 0;
}

void wireless_nlevent_flush(void)
{
}

void genl_lock(void)
{
}

void genl_unlock(void)
{
}

int genl_register_family(struct genl_family *family)
{
#ifdef CONFIG_NETLINK_GENERIC
  FAR struct genl_family *existing;
  FAR struct netlink_generic_family_s *generic;
  FAR struct netlink_generic_group_s *groups = NULL;
  unsigned int i;
  int ret;

  if (family == NULL || family->name[0] == '\0')
    {
      return -EINVAL;
    }

  existing = genl_bridge_find_family_by_name(family->name);
  if (existing != NULL)
    {
      family->id = existing->id;
      family->mcgrp_offset = existing->mcgrp_offset;
      return 0;
    }

  generic = kmm_zalloc(sizeof(*generic));
  if (generic == NULL)
    {
      return -ENOMEM;
    }

  if (family->n_mcgrps > 0)
    {
      groups = kmm_calloc(family->n_mcgrps, sizeof(*groups));
      if (groups == NULL)
        {
          kmm_free(generic);
          return -ENOMEM;
        }

      for (i = 0; i < family->n_mcgrps; i++)
        {
          groups[i].name = family->mcgrps[i].name;
        }
    }

  generic->id      = family->id;
  generic->name    = family->name;
  generic->version = family->version;
  generic->hdrsize = family->hdrsize;
  generic->maxattr = family->maxattr;
  generic->groups  = groups;
  generic->ngroups = family->n_mcgrps;
  generic->sendto  = genl_bridge_sendto;

  ret = netlink_generic_register(generic);
  nuttx_hwsim_debugf("genl_bridge: netlink_generic_register(%s) ret=%d id=%u\n",
         family->name, ret, generic->id);
  if (ret < 0)
    {
      kmm_free(groups);
      kmm_free(generic);
      return ret == -EEXIST ? 0 : ret;
    }

  family->id = generic->id;
  family->mcgrp_offset = family->n_mcgrps > 0 ? groups[0].id : 0;

  for (i = 0; i < GENL_BRIDGE_MAX_FAMILIES; i++)
    {
      if (g_genl_families[i] == NULL)
        {
          g_genl_families[i] = family;
          break;
        }
    }

  kmm_free(generic);
  return 0;
#else
  return -EOPNOTSUPP;
#endif
}

int genl_unregister_family(const struct genl_family *family)
{
#ifdef CONFIG_NETLINK_GENERIC
  int i;

  if (family == NULL)
    {
      return -EINVAL;
    }

  for (i = 0; i < GENL_BRIDGE_MAX_FAMILIES; i++)
    {
      if (g_genl_families[i] == family)
        {
          g_genl_families[i] = NULL;
          break;
        }
    }

  return netlink_generic_unregister(family->id, family->name);
#else
  return -EOPNOTSUPP;
#endif
}

void *genlmsg_put(struct sk_buff *skb, u32 portid, u32 seq,
                  const struct genl_family *family, int flags, u8 cmd)
{
  FAR struct nlmsghdr *nlh;
  FAR struct genlmsghdr *hdr;

  nlh = nlmsg_put(skb, portid, seq, family->id,
                  GENL_HDRLEN + family->hdrsize, flags);
  if (nlh == NULL)
    {
      return NULL;
    }

  hdr = (FAR struct genlmsghdr *)nlmsg_data(nlh);
  hdr->cmd = cmd;
  hdr->version = family->version;
  hdr->reserved = 0;

  return (FAR char *)hdr + GENL_HDRLEN;
}

void genl_notify(const struct genl_family *family, struct sk_buff *skb,
                 struct genl_info *info, u32 group, gfp_t flags)
{
  u32 portid = 0;

  if (info != NULL)
    {
      portid = info->snd_portid;
    }

  genlmsg_multicast(family, skb, portid, group, flags);
}

int genlmsg_multicast_allns(const struct genl_family *family,
                            struct sk_buff *skb, u32 portid,
                            unsigned int group)
{
  return genlmsg_multicast(family, skb, portid, group, GFP_KERNEL);
}

struct nlmsghdr *nlmsg_hdr(const struct sk_buff *skb)
{
  return (FAR struct nlmsghdr *)skb->data;
}

int netlink_register_notifier(struct notifier_block *nb)
{
  int i;

  if (nb == NULL)
    {
      return -EINVAL;
    }

  for (i = 0; i < GENL_BRIDGE_MAX_NOTIFIERS; i++)
    {
      if (g_netlink_notifiers[i] == nb)
        {
          return 0;
        }
    }

  for (i = 0; i < GENL_BRIDGE_MAX_NOTIFIERS; i++)
    {
      if (g_netlink_notifiers[i] == NULL)
        {
          g_netlink_notifiers[i] = nb;
          return 0;
        }
    }

  return -ENOSPC;
}

int netlink_unregister_notifier(struct notifier_block *nb)
{
  int i;

  for (i = 0; i < GENL_BRIDGE_MAX_NOTIFIERS; i++)
    {
      if (g_netlink_notifiers[i] == nb)
        {
          g_netlink_notifiers[i] = NULL;
          return 0;
        }
    }

  return 0;
}

void netlink_release_notify(int protocol, uint32_t portid)
{
  struct netlink_notify notify;
  int i;

  memset(&notify, 0, sizeof(notify));
  notify.protocol = protocol;
  notify.portid = portid;
  notify.net = &init_net;

  for (i = 0; i < GENL_BRIDGE_MAX_NOTIFIERS; i++)
    {
      if (g_netlink_notifiers[i] != NULL &&
          g_netlink_notifiers[i]->notifier_call != NULL)
        {
          g_netlink_notifiers[i]->notifier_call(g_netlink_notifiers[i],
                                                NETLINK_URELEASE,
                                                &notify);
        }
    }
}

int netlink_unicast(struct sock *sk, struct sk_buff *skb, u32 portid,
                    int nonblock)
{
  FAR struct netlink_response_s *resp;
  int ret;

  (void)nonblock;

  if (skb != NULL && skb->len >= sizeof(struct nlmsghdr))
    {
      resp = kmm_zalloc(sizeof(*resp) + skb->len);
      if (resp == NULL)
        {
          consume_skb(skb);
          return -ENOMEM;
        }

      memcpy(&resp->msg, skb->data, skb->len);

      if (portid != 0)
        {
          ret = netlink_add_response_pid(NETLINK_GENERIC, portid, resp);
          if (ret == OK)
            {
              nuttx_hwsim_debugf("genl_bridge: unicast delivered portid=%u len=%u\n",
                     portid, skb->len);
              consume_skb(skb);
              return 0;
            }

          nuttx_hwsim_debugf("genl_bridge: unicast portid=%u miss ret=%d len=%u\n",
                 portid, ret, skb->len);
        }

      if (sk != NULL)
        {
          netlink_add_response((NETLINK_HANDLE)sk, resp);
          consume_skb(skb);
          return 0;
        }

      kmm_free(resp);
      consume_skb(skb);
      return portid != 0 ? -ENOENT : 0;
    }

  consume_skb(skb);
  return 0;
}

int netlink_broadcast_filtered(struct sock *ssk, struct sk_buff *skb,
                               u32 portid, u32 group, gfp_t allocation,
                               void *filter, void *filter_data)
{
  FAR struct netlink_response_s *resp;

  (void)ssk;
  (void)portid;
  (void)allocation;
  (void)filter;
  (void)filter_data;

  if (skb != NULL && skb->len >= sizeof(struct nlmsghdr) && group > 0)
    {
      resp = kmm_zalloc(sizeof(*resp) + skb->len);
      if (resp == NULL)
        {
          consume_skb(skb);
          return -ENOMEM;
        }

      memcpy(&resp->msg, skb->data, skb->len);
      nuttx_hwsim_debugf("genl_bridge: multicast group=%u len=%u\n", group, skb->len);
      netlink_add_broadcast_protocol(NETLINK_GENERIC, group, resp);
      consume_skb(skb);
      return 0;
    }

  consume_skb(skb);
  return group > 0 ? -EINVAL : 0;
}
