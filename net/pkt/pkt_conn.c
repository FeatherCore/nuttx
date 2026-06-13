/****************************************************************************
 * net/pkt/pkt_conn.c
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
#if defined(CONFIG_NET) && defined(CONFIG_NET_PKT)

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <nuttx/debug.h>

#include <arch/irq.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>

#include <nuttx/kmalloc.h>
#include <nuttx/mutex.h>
#include <nuttx/mm/iob.h>
#include <nuttx/net/netconfig.h>
#include <nuttx/net/net.h>
#include <nuttx/net/netdev.h>
#include <nuttx/net/ethernet.h>

#include "devif/devif.h"
#include "netdev/netdev.h"
#include "pkt/pkt.h"
#include "utils/utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define eth_addr_cmp(addr1, addr2) \
  ((addr1[0] == addr2[0]) && (addr1[1] == addr2[1]) && \
   (addr1[2] == addr2[2]) && (addr1[3] == addr2[3]) && \
   (addr1[4] == addr2[4]) && (addr1[5] == addr2[5]))

#define ETHERTYPE_EAPOL 0x888e

#define BPF_CLASS(code) ((code) & 0x07)
#define BPF_SIZE(code)  ((code) & 0x18)
#define BPF_MODE(code)  ((code) & 0xe0)
#define BPF_OP(code)    ((code) & 0xf0)
#define BPF_SRC(code)   ((code) & 0x08)
#define BPF_RVAL(code)  ((code) & 0x18)

#define BPF_LD    0x00
#define BPF_LDX   0x01
#define BPF_ST    0x02
#define BPF_STX   0x03
#define BPF_ALU   0x04
#define BPF_JMP   0x05
#define BPF_RET   0x06
#define BPF_MISC  0x07
#define BPF_W     0x00
#define BPF_H     0x08
#define BPF_B     0x10
#define BPF_IMM   0x00
#define BPF_ABS   0x20
#define BPF_IND   0x40
#define BPF_MEM   0x60
#define BPF_LEN   0x80
#define BPF_MSH   0xa0
#define BPF_ADD   0x00
#define BPF_SUB   0x10
#define BPF_MUL   0x20
#define BPF_DIV   0x30
#define BPF_OR    0x40
#define BPF_AND   0x50
#define BPF_LSH   0x60
#define BPF_RSH   0x70
#define BPF_NEG   0x80
#define BPF_MOD   0x90
#define BPF_XOR   0xa0
#define BPF_JA    0x00
#define BPF_JEQ   0x10
#define BPF_JGT   0x20
#define BPF_JGE   0x30
#define BPF_JSET  0x40
#define BPF_K     0x00
#define BPF_X     0x08
#define BPF_A     0x10
#define BPF_TAX   0x00
#define BPF_TXA   0x80

#define BPF_MEMWORDS     16
#define BPF_MAXINSNS     256
#define SKF_AD_OFF       0xfffff000u
#define SKF_AD_PKTTYPE   4

#ifndef CONFIG_NET_PKT_MAX_CONNS
#  define CONFIG_NET_PKT_MAX_CONNS 0
#endif

struct sock_filter
{
  uint16_t code;
  uint8_t  jt;
  uint8_t  jf;
  uint32_t k;
};

struct sock_fprog
{
  unsigned short len;
  FAR struct sock_filter *filter;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The array containing all packet socket connections */

NET_BUFPOOL_DECLARE(g_pkt_connections, sizeof(struct pkt_conn_s),
                    CONFIG_NET_PKT_PREALLOC_CONNS,
                    CONFIG_NET_PKT_ALLOC_CONNS, CONFIG_NET_PKT_MAX_CONNS);

/* A list of all allocated packet socket connections */

static dq_queue_t g_active_pkt_connections;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool pkt_bpf_validate(FAR const struct pkt_bpf_insn_s *filter,
                             uint16_t len)
{
  uint16_t pc;

  if (filter == NULL || len == 0 || len > BPF_MAXINSNS)
    {
      return false;
    }

  for (pc = 0; pc < len; pc++)
    {
      uint16_t code = filter[pc].code;

      switch (BPF_CLASS(code))
        {
          case BPF_LD:
            switch (BPF_MODE(code))
              {
                case BPF_IMM:
                case BPF_ABS:
                case BPF_IND:
                case BPF_MEM:
                case BPF_LEN:
                case BPF_MSH:
                  break;

                default:
                  return false;
              }
            break;

          case BPF_LDX:
            switch (BPF_MODE(code))
              {
                case BPF_IMM:
                case BPF_MEM:
                case BPF_LEN:
                case BPF_MSH:
                  break;

                default:
                  return false;
              }
            break;

          case BPF_ST:
          case BPF_STX:
            if (filter[pc].k >= BPF_MEMWORDS)
              {
                return false;
              }
            break;

          case BPF_ALU:
            if (BPF_SRC(code) == BPF_K &&
                (BPF_OP(code) == BPF_DIV || BPF_OP(code) == BPF_MOD) &&
                filter[pc].k == 0)
              {
                return false;
              }
            break;

          case BPF_JMP:
            if (BPF_OP(code) == BPF_JA)
              {
                if (pc + 1 + filter[pc].k >= len)
                  {
                    return false;
                  }
              }
            else if (pc + 1 + filter[pc].jt >= len ||
                     pc + 1 + filter[pc].jf >= len)
              {
                return false;
              }
            break;

          case BPF_RET:
            if (BPF_RVAL(code) != BPF_K && BPF_RVAL(code) != BPF_A)
              {
                return false;
              }
            break;

          case BPF_MISC:
            if (BPF_OP(code) != BPF_TAX && BPF_OP(code) != BPF_TXA)
              {
                return false;
              }
            break;

          default:
            return false;
        }
    }

  return true;
}

static bool pkt_bpf_load(FAR struct iob_s *iob, int hdr_offset,
                         size_t framelen, uint32_t k, uint32_t size,
                         uint8_t pkttype, FAR uint32_t *val)
{
  uint8_t bytes[4];
  int ret;

  if (k >= SKF_AD_OFF)
    {
      if (k == SKF_AD_OFF + SKF_AD_PKTTYPE && size == BPF_B)
        {
          *val = pkttype;
          return true;
        }

      return false;
    }

  if (k >= framelen)
    {
      return false;
    }

  switch (size)
    {
      case BPF_B:
        ret = iob_copyout(bytes, iob, 1, hdr_offset + (int)k);
        if (ret != 1)
          {
            return false;
          }

        *val = bytes[0];
        return true;

      case BPF_H:
        if (k + 2 > framelen)
          {
            return false;
          }

        ret = iob_copyout(bytes, iob, 2, hdr_offset + (int)k);
        if (ret != 2)
          {
            return false;
          }

        *val = ((uint32_t)bytes[0] << 8) | bytes[1];
        return true;

      case BPF_W:
        if (k + 4 > framelen)
          {
            return false;
          }

        ret = iob_copyout(bytes, iob, 4, hdr_offset + (int)k);
        if (ret != 4)
          {
            return false;
          }

        *val = ((uint32_t)bytes[0] << 24) |
               ((uint32_t)bytes[1] << 16) |
               ((uint32_t)bytes[2] << 8) |
               bytes[3];
        return true;

      default:
        return false;
    }
}

static uint32_t pkt_bpf_run(FAR const struct pkt_bpf_insn_s *filter,
                            uint16_t len, FAR struct iob_s *iob,
                            size_t framelen, int hdr_offset,
                            uint8_t pkttype)
{
  uint32_t mem[BPF_MEMWORDS];
  uint32_t a = 0;
  uint32_t x = 0;
  uint16_t pc;

  memset(mem, 0, sizeof(mem));

  for (pc = 0; pc < len; pc++)
    {
      FAR const struct pkt_bpf_insn_s *insn = &filter[pc];
      uint32_t src;
      uint32_t v;

      switch (BPF_CLASS(insn->code))
        {
          case BPF_LD:
            switch (BPF_MODE(insn->code))
              {
                case BPF_IMM:
                  a = insn->k;
                  break;

                case BPF_LEN:
                  a = framelen;
                  break;

                case BPF_ABS:
                  if (!pkt_bpf_load(iob, hdr_offset, framelen, insn->k,
                                    BPF_SIZE(insn->code), pkttype, &a))
                    {
                      return 0;
                    }
                  break;

                case BPF_IND:
                  if (!pkt_bpf_load(iob, hdr_offset, framelen, x + insn->k,
                                    BPF_SIZE(insn->code), pkttype, &a))
                    {
                      return 0;
                    }
                  break;

                case BPF_MEM:
                  if (insn->k >= BPF_MEMWORDS)
                    {
                      return 0;
                    }
                  a = mem[insn->k];
                  break;

                default:
                  return 0;
              }
            break;

          case BPF_LDX:
            switch (BPF_MODE(insn->code))
              {
                case BPF_IMM:
                  x = insn->k;
                  break;

                case BPF_LEN:
                  x = framelen;
                  break;

                case BPF_MEM:
                  if (insn->k >= BPF_MEMWORDS)
                    {
                      return 0;
                    }
                  x = mem[insn->k];
                  break;

                case BPF_MSH:
                  if (!pkt_bpf_load(iob, hdr_offset, framelen, insn->k,
                                    BPF_B, pkttype, &v))
                    {
                      return 0;
                    }
                  x = (v & 0x0f) << 2;
                  break;

                default:
                  return 0;
              }
            break;

          case BPF_ST:
            mem[insn->k] = a;
            break;

          case BPF_STX:
            mem[insn->k] = x;
            break;

          case BPF_ALU:
            src = BPF_SRC(insn->code) == BPF_X ? x : insn->k;
            switch (BPF_OP(insn->code))
              {
                case BPF_ADD:
                  a += src;
                  break;
                case BPF_SUB:
                  a -= src;
                  break;
                case BPF_MUL:
                  a *= src;
                  break;
                case BPF_DIV:
                  if (src == 0)
                    {
                      return 0;
                    }
                  a /= src;
                  break;
                case BPF_OR:
                  a |= src;
                  break;
                case BPF_AND:
                  a &= src;
                  break;
                case BPF_LSH:
                  a <<= src;
                  break;
                case BPF_RSH:
                  a >>= src;
                  break;
                case BPF_NEG:
                  a = (uint32_t)(-(int32_t)a);
                  break;
                case BPF_MOD:
                  if (src == 0)
                    {
                      return 0;
                    }
                  a %= src;
                  break;
                case BPF_XOR:
                  a ^= src;
                  break;
                default:
                  return 0;
              }
            break;

          case BPF_JMP:
            src = BPF_SRC(insn->code) == BPF_X ? x : insn->k;
            switch (BPF_OP(insn->code))
              {
                case BPF_JA:
                  pc += insn->k;
                  break;
                case BPF_JEQ:
                  pc += (a == src) ? insn->jt : insn->jf;
                  break;
                case BPF_JGT:
                  pc += (a > src) ? insn->jt : insn->jf;
                  break;
                case BPF_JGE:
                  pc += (a >= src) ? insn->jt : insn->jf;
                  break;
                case BPF_JSET:
                  pc += (a & src) ? insn->jt : insn->jf;
                  break;
                default:
                  return 0;
              }
            break;

          case BPF_RET:
            return BPF_RVAL(insn->code) == BPF_A ? a : insn->k;

          case BPF_MISC:
            if (BPF_OP(insn->code) == BPF_TAX)
              {
                x = a;
              }
            else if (BPF_OP(insn->code) == BPF_TXA)
              {
                a = x;
              }
            else
              {
                return 0;
              }
            break;

          default:
            return 0;
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pkt_alloc()
 *
 * Description:
 *   Allocate a new, uninitialized packet socket connection structure. This
 *   is normally something done by the implementation of the socket() API
 *
 ****************************************************************************/

FAR struct pkt_conn_s *pkt_alloc(void)
{
  FAR struct pkt_conn_s *conn;

  /* The free list is protected by a mutex. */

  NET_BUFPOOL_LOCK(g_pkt_connections);

  conn = NET_BUFPOOL_TRYALLOC(g_pkt_connections);
  if (conn)
    {
      memset(conn, 0, sizeof(*conn));

      /* Enqueue the connection into the active list */

      dq_addlast(&conn->sconn.node, &g_active_pkt_connections);
    }

  NET_BUFPOOL_UNLOCK(g_pkt_connections);
  return conn;
}

/****************************************************************************
 * Name: pkt_free()
 *
 * Description:
 *   Free a packet socket connection structure that is no longer in use.
 *   This should be done by the implementation of close().
 *
 ****************************************************************************/

void pkt_free(FAR struct pkt_conn_s *conn)
{
  /* The free list is protected by a mutex. */

  DEBUGASSERT(conn->crefs == 0);

  NET_BUFPOOL_LOCK(g_pkt_connections);

  /* Remove the connection from the active list */

  dq_rem(&conn->sconn.node, &g_active_pkt_connections);
  nxrmutex_destroy(&conn->sconn.s_lock);
  pkt_detach_filter(conn);
  iob_free_queue(&conn->txstatus);

#ifdef CONFIG_NET_PKT_WRITE_BUFFERS
  /* Free the write queue */

  iob_free_queue(&conn->write_q);
#endif

  /* Free the connection. */

  NET_BUFPOOL_FREE(g_pkt_connections, conn);

  NET_BUFPOOL_UNLOCK(g_pkt_connections);
}

/****************************************************************************
 * Name: pkt_active()
 *
 * Description:
 *   Find a connection structure that is the appropriate connection to be
 *   used with the provided network device
 *
 * Assumptions:
 *   This function is called from network logic at with the network locked.
 *
 ****************************************************************************/

FAR struct pkt_conn_s *pkt_active(FAR struct net_driver_s *dev)
{
  FAR struct pkt_conn_s *conn =
    (FAR struct pkt_conn_s *)g_active_pkt_connections.head;
  FAR struct pkt_conn_s *match = NULL;
  uint16_t ethertype = 0;

  if (dev->d_lltype == NET_LL_ETHERNET || dev->d_lltype == NET_LL_IEEE80211)
    {
      FAR struct eth_hdr_s *ethhdr = NETLLBUF;
      ethertype = ethhdr->type;
    }

  while (conn)
    {
      bool type_match;

      type_match = conn->type == HTONS(ETH_P_ALL) ||
                   conn->type == ETH_P_ALL ||
                   conn->type == ethertype ||
                   HTONS(conn->type) == ethertype;

      if (dev->d_ifindex == conn->ifindex &&
          type_match)
        {
          /* Matching connection found.. return a reference to it */

          match = conn;
          break;
        }

      /* Look at the next active connection */

      conn = (FAR struct pkt_conn_s *)conn->sconn.node.flink;
    }

  return match;
}

/****************************************************************************
 * Name: pkt_nextconn()
 *
 * Description:
 *   Traverse the list of allocated packet connections
 *
 * Assumptions:
 *   This function is called from network logic at with the network locked.
 *
 ****************************************************************************/

FAR struct pkt_conn_s *pkt_nextconn(FAR struct pkt_conn_s *conn)
{
  if (!conn)
    {
      return (FAR struct pkt_conn_s *)g_active_pkt_connections.head;
    }
  else
    {
      return (FAR struct pkt_conn_s *)conn->sconn.node.flink;
    }
}

/****************************************************************************
 * Name: pkt_attach_filter
 ****************************************************************************/

int pkt_attach_filter(FAR struct pkt_conn_s *conn, FAR const void *value,
                      socklen_t value_len)
{
  FAR const struct sock_fprog *fprog;
  FAR struct pkt_bpf_insn_s *copy;
  uint16_t i;

  if (conn == NULL || value == NULL || value_len < sizeof(*fprog))
    {
      return -EINVAL;
    }

  fprog = (FAR const struct sock_fprog *)value;
  if (fprog->len == 0 || fprog->len > BPF_MAXINSNS ||
      fprog->filter == NULL)
    {
      return -EINVAL;
    }

  copy = kmm_malloc(fprog->len * sizeof(*copy));
  if (copy == NULL)
    {
      return -ENOMEM;
    }

  for (i = 0; i < fprog->len; i++)
    {
      copy[i].code = fprog->filter[i].code;
      copy[i].jt   = fprog->filter[i].jt;
      copy[i].jf   = fprog->filter[i].jf;
      copy[i].k    = fprog->filter[i].k;
    }

  if (!pkt_bpf_validate(copy, fprog->len))
    {
      kmm_free(copy);
      return -EINVAL;
    }

  conn_lock(&conn->sconn);
  pkt_detach_filter(conn);
  conn->filter = copy;
  conn->filter_len = fprog->len;
  conn_unlock(&conn->sconn);

  return OK;
}

/****************************************************************************
 * Name: pkt_detach_filter
 ****************************************************************************/

void pkt_detach_filter(FAR struct pkt_conn_s *conn)
{
  if (conn != NULL && conn->filter != NULL)
    {
      kmm_free(conn->filter);
      conn->filter = NULL;
      conn->filter_len = 0;
    }
}

/****************************************************************************
 * Name: pkt_eth_pkttype
 ****************************************************************************/

uint8_t pkt_eth_pkttype(FAR struct net_driver_s *dev,
                        FAR const struct eth_hdr_s *ethhdr)
{
  FAR const uint8_t *own;
  bool broadcast = true;
  int i;

  if (dev == NULL || ethhdr == NULL ||
      (dev->d_lltype != NET_LL_ETHERNET &&
       dev->d_lltype != NET_LL_IEEE80211))
    {
      return PACKET_HOST;
    }

  own = dev->d_mac.ether.ether_addr_octet;

  if (eth_addr_cmp(ethhdr->src, own))
    {
      return PACKET_OUTGOING;
    }

  if (eth_addr_cmp(ethhdr->dest, own))
    {
      return PACKET_HOST;
    }

  for (i = 0; i < ETHER_ADDR_LEN; i++)
    {
      if (ethhdr->dest[i] != 0xff)
        {
          broadcast = false;
          break;
        }
    }

  if (broadcast)
    {
      return PACKET_BROADCAST;
    }

  if ((ethhdr->dest[0] & 0x01) != 0)
    {
      return PACKET_MULTICAST;
    }

  return PACKET_OTHERHOST;
}

/****************************************************************************
 * Name: pkt_filter_accept
 ****************************************************************************/

bool pkt_filter_accept(FAR struct net_driver_s *dev,
                       FAR struct pkt_conn_s *conn,
                       FAR struct iob_s *iob, size_t framelen,
                       int hdr_offset, uint8_t pkttype)
{
  (void)dev;

  if (conn == NULL || conn->filter == NULL || conn->filter_len == 0)
    {
      return true;
    }

  if (iob == NULL)
    {
      return false;
    }

  return pkt_bpf_run(conn->filter, conn->filter_len, iob, framelen,
                     hdr_offset, pkttype) != 0;
}

/****************************************************************************
 * Name: pkt_sendmsg_is_valid
 *
 * Description:
 *   Validate the sendmsg() parameters for a packet socket.
 *
 * Input Parameters:
 *   psock - The socket structure to validate
 *   msg   - The message header containing the data to be sent
 *   dev   - The network device to be used to send the packet
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int pkt_sendmsg_is_valid(FAR struct socket *psock,
                         FAR const struct msghdr *msg,
                         FAR struct net_driver_s **dev)
{
  FAR struct sockaddr_ll *addr = msg->msg_name;

  /* Only single iov supported */

  if (msg->msg_iovlen != 1)
    {
      return -ENOTSUP;
    }

  /* Verify that the sockfd corresponds to valid, allocated socket */

  if (psock == NULL || psock->s_conn == NULL)
    {
      return -EBADF;
    }

  if (psock->s_type == SOCK_DGRAM)
    {
      if (msg->msg_name == NULL ||
          msg->msg_namelen < sizeof(struct sockaddr_ll) ||
          addr->sll_halen < ETHER_ADDR_LEN)
        {
          return -EINVAL;
        }

      /* Get the device driver that will service this transfer */

      *dev = netdev_findbyindex(addr->sll_ifindex);
    }
  else if (psock->s_type == SOCK_RAW)
    {
      if (msg->msg_name != NULL)
        {
          if (msg->msg_namelen < sizeof(struct sockaddr_ll) ||
              addr->sll_halen < ETHER_ADDR_LEN)
            {
              return -EINVAL;
            }

          *dev = netdev_findbyindex(addr->sll_ifindex);
        }
      else
        {
          /* Get the device driver that will service this transfer */

          *dev = pkt_find_device(psock->s_conn);
        }
    }
  else
    {
      return -ENOTSUP;
    }

  if (*dev == NULL)
    {
      return -ENODEV;
    }

  return OK;
}

/****************************************************************************
 * Name: pkt_conn_list_lock()
 *
 * Description:
 *   Lock the packet connection list
 *
 * Assumptions:
 *   This function must be called by driver thread.
 *
 ****************************************************************************/

void pkt_conn_list_lock(void)
{
  NET_BUFPOOL_LOCK(g_pkt_connections);
}

/****************************************************************************
 * Name: pkt_conn_list_unlock()
 *
 * Description:
 *   Unlock the packet connection list
 *
 * Assumptions:
 *   This function must be called by driver thread.
 *
 ****************************************************************************/

void pkt_conn_list_unlock(void)
{
  NET_BUFPOOL_UNLOCK(g_pkt_connections);
}

#endif /* CONFIG_NET && CONFIG_NET_PKT */
