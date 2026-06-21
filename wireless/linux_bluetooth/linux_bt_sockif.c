/****************************************************************************
 * wireless/linux_bluetooth/linux_bt_sockif.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_NET_LINUX_BLUETOOTH

#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <nuttx/kmalloc.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/net/net.h>
#include <nuttx/wireless/linux_bluetooth.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef BTPROTO_L2CAP
#  define BTPROTO_L2CAP 0
#endif

#ifndef BTPROTO_HCI
#  define BTPROTO_HCI 1
#endif

#ifndef BTPROTO_SCO
#  define BTPROTO_SCO 2
#endif

#ifndef BTPROTO_RFCOMM
#  define BTPROTO_RFCOMM 3
#endif

#ifndef BTPROTO_BNEP
#  define BTPROTO_BNEP 4
#endif

#ifndef BTPROTO_CMTP
#  define BTPROTO_CMTP 5
#endif

#ifndef BTPROTO_HIDP
#  define BTPROTO_HIDP 6
#endif

#ifndef BTPROTO_ISO
#  define BTPROTO_ISO 8
#endif

#ifndef BNEPCONNADD
#  define BNEPCONNADD      0x42c8
#  define BNEPCONNDEL      0x42c9
#  define BNEPGETCONNLIST  0x42d2
#  define BNEPGETCONNINFO  0x42d3
#  define BNEPGETSUPPFEAT  0x42d4
#endif

#ifndef HCIDEVUP
#  define HCIDEVUP       0x400448c9
#  define HCIDEVDOWN     0x400448ca
#  define HCIDEVRESET    0x400448cb
#  define HCIDEVRESTAT   0x400448cc
#  define HCIGETDEVLIST  0x800448d2
#  define HCIGETDEVINFO  0x800448d3
#  define HCIGETCONNLIST 0x800448d4
#  define HCIGETCONNINFO 0x800448d5
#  define HCIGETAUTHINFO 0x800448d7
#  define HCISETSCAN     0x400448dd
#  define HCISETAUTH     0x400448de
#  define HCISETENCRYPT  0x400448df
#  define HCISETPTYPE    0x400448e0
#  define HCISETLINKPOL  0x400448e1
#  define HCISETLINKMODE 0x400448e2
#  define HCISETACLMTU   0x400448e3
#  define HCISETSCOMTU   0x400448e4
#  define HCIBLOCKADDR   0x400448e6
#  define HCIUNBLOCKADDR 0x400448e7
#endif

#define LINUX_BT_SOCKIF_DEFAULT_HANDLE 0x0052
#define LINUX_BT_SOCKIF_DEFAULT_ISO_HANDLE 0x0201
#define LINUX_BT_SOCKIF_MGMT_MAX       512
#define LINUX_BT_SOCKIF_MAX_LISTENERS  8

#ifndef HCI_CHANNEL_RAW
#  define HCI_CHANNEL_RAW 0
#endif

#ifndef HCI_CHANNEL_USER
#  define HCI_CHANNEL_USER 1
#endif

#ifndef HCI_CHANNEL_CONTROL
#  define HCI_CHANNEL_CONTROL 3
#endif

#ifndef HCI_CHANNEL_LOGGING
#  define HCI_CHANNEL_LOGGING 4
#endif

#ifndef HCI_CHANNEL_MONITOR
#  define HCI_CHANNEL_MONITOR 2
#endif

#ifndef HCI_DEV_NONE
#  define HCI_DEV_NONE 0xffff
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct linux_bt_sockif_conn_s
{
  struct socket_conn_s common;
  uint8_t refs;
  int proto;
  uint16_t psm;
  uint16_t cid;
  uint16_t handle;
  uint16_t hci_dev;
  uint16_t hci_channel;
  uint8_t rfcomm_channel;
  uint8_t l2_bdaddr_type;
  void *hci_raw;
  void *hci_upstream;
  void *l2cap_upstream;
  void *iso_upstream;
  void *rfcomm_upstream;
  void *sco_upstream;
  void *cmtp_upstream;
  void *hidp_upstream;
  uint8_t iso_addr_type;
  uint8_t pending_accepts;
  bool bound;
  bool connected;
};

struct linux_bt_sockif_sockaddr_l2_s
{
  sa_family_t l2_family;
  uint16_t l2_psm;
  uint8_t l2_bdaddr[6];
  uint16_t l2_cid;
  uint8_t l2_bdaddr_type;
};

struct linux_bt_sockif_connadd_req_s
{
  int sock;
  uint32_t flags;
  uint16_t role;
  char device[16];
};

struct linux_bt_sockif_sockaddr_hci_s
{
  sa_family_t hci_family;
  uint16_t hci_dev;
  uint16_t hci_channel;
};

struct linux_bt_sockif_sockaddr_rc_s
{
  sa_family_t rc_family;
  uint8_t rc_bdaddr[6];
  uint8_t rc_channel;
};

struct linux_bt_sockif_sockaddr_sco_s
{
  sa_family_t sco_family;
  uint8_t sco_bdaddr[6];
};

struct linux_bt_sockif_sockaddr_iso_s
{
  sa_family_t iso_family;
  uint8_t iso_bdaddr[6];
  uint8_t iso_bdaddr_type;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int linux_bt_sockif_setup(FAR struct socket *psock);
static sockcaps_t linux_bt_sockif_sockcaps(FAR struct socket *psock);
static void linux_bt_sockif_addref(FAR struct socket *psock);
static int linux_bt_sockif_bind(FAR struct socket *psock,
                                FAR const struct sockaddr *addr,
                                socklen_t addrlen);
static int linux_bt_sockif_getsockname(FAR struct socket *psock,
                                       FAR struct sockaddr *addr,
                                       FAR socklen_t *addrlen);
static int linux_bt_sockif_getpeername(FAR struct socket *psock,
                                       FAR struct sockaddr *addr,
                                       FAR socklen_t *addrlen);
static int linux_bt_sockif_listen(FAR struct socket *psock, int backlog);
static int linux_bt_sockif_connect(FAR struct socket *psock,
                                   FAR const struct sockaddr *addr,
                                   socklen_t addrlen);
static int linux_bt_sockif_accept(FAR struct socket *psock,
                                  FAR struct sockaddr *addr,
                                  FAR socklen_t *addrlen,
                                  FAR struct socket *newsock, int flags);
static int linux_bt_sockif_poll(FAR struct socket *psock,
                                FAR struct pollfd *fds, bool setup);
static ssize_t linux_bt_sockif_sendmsg(FAR struct socket *psock,
                                       FAR const struct msghdr *msg,
                                       int flags);
static ssize_t linux_bt_sockif_recvmsg(FAR struct socket *psock,
                                       FAR struct msghdr *msg,
                                       int flags);
static int linux_bt_sockif_close(FAR struct socket *psock);
static int linux_bt_sockif_ioctl(FAR struct socket *psock, int cmd,
                                 unsigned long arg);
static int linux_bt_sockif_shutdown(FAR struct socket *psock, int how);
#ifdef CONFIG_NET_SOCKOPTS
static int linux_bt_sockif_l2cap_ensure_upstream(
                                      FAR struct linux_bt_sockif_conn_s *conn);
static int linux_bt_sockif_iso_ensure_upstream(
                                      FAR struct linux_bt_sockif_conn_s *conn);
static int linux_bt_sockif_rfcomm_ensure_upstream(
                                      FAR struct linux_bt_sockif_conn_s *conn);
static int linux_bt_sockif_sco_ensure_upstream(
                                      FAR struct linux_bt_sockif_conn_s *conn);
static int linux_bt_sockif_getsockopt(FAR struct socket *psock, int level,
                                      int option, FAR void *value,
                                      FAR socklen_t *value_len);
static int linux_bt_sockif_setsockopt(FAR struct socket *psock, int level,
                                      int option, FAR const void *value,
                                      socklen_t value_len);
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

static FAR struct linux_bt_sockif_conn_s *
  g_linux_bt_sockif_listeners[LINUX_BT_SOCKIF_MAX_LISTENERS];

const struct sock_intf_s g_linux_bt_sockif =
{
  linux_bt_sockif_setup,    /* si_setup */
  linux_bt_sockif_sockcaps, /* si_sockcaps */
  linux_bt_sockif_addref,   /* si_addref */
  linux_bt_sockif_bind,     /* si_bind */
  linux_bt_sockif_getsockname, /* si_getsockname */
  linux_bt_sockif_getpeername, /* si_getpeername */
  linux_bt_sockif_listen,   /* si_listen */
  linux_bt_sockif_connect,  /* si_connect */
  linux_bt_sockif_accept,   /* si_accept */
  linux_bt_sockif_poll,     /* si_poll */
  linux_bt_sockif_sendmsg,  /* si_sendmsg */
  linux_bt_sockif_recvmsg,  /* si_recvmsg */
  linux_bt_sockif_close,    /* si_close */
  linux_bt_sockif_ioctl,    /* si_ioctl */
  NULL,                     /* si_socketpair */
  linux_bt_sockif_shutdown  /* si_shutdown */
#ifdef CONFIG_NET_SOCKOPTS
  , linux_bt_sockif_getsockopt /* si_getsockopt */
  , linux_bt_sockif_setsockopt /* si_setsockopt */
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void linux_bt_sockif_unregister_listener(
                                      FAR struct linux_bt_sockif_conn_s *conn)
{
  unsigned int i;

  for (i = 0; i < LINUX_BT_SOCKIF_MAX_LISTENERS; i++)
    {
      if (g_linux_bt_sockif_listeners[i] == conn)
        {
          g_linux_bt_sockif_listeners[i] = NULL;
        }
    }
}

static int linux_bt_sockif_register_listener(
                                      FAR struct linux_bt_sockif_conn_s *conn)
{
  unsigned int i;

  linux_bt_sockif_unregister_listener(conn);
  for (i = 0; i < LINUX_BT_SOCKIF_MAX_LISTENERS; i++)
    {
      if (g_linux_bt_sockif_listeners[i] == NULL)
        {
          g_linux_bt_sockif_listeners[i] = conn;
          return OK;
        }
    }

  return -ENOMEM;
}

static FAR struct linux_bt_sockif_conn_s *
linux_bt_sockif_find_listener(FAR const struct linux_bt_sockif_conn_s *conn)
{
  FAR struct linux_bt_sockif_conn_s *listener;
  unsigned int i;

  if (conn == NULL)
    {
      return NULL;
    }

  for (i = 0; i < LINUX_BT_SOCKIF_MAX_LISTENERS; i++)
    {
      listener = g_linux_bt_sockif_listeners[i];
      if (listener == NULL || listener == conn ||
          listener->proto != conn->proto)
        {
          continue;
        }

      if (conn->proto == BTPROTO_L2CAP &&
          listener->psm == conn->psm &&
          (listener->cid == 0 || conn->cid == 0 ||
           listener->cid == conn->cid))
        {
          return listener;
        }

      if (conn->proto == BTPROTO_ISO &&
          listener->iso_addr_type == conn->iso_addr_type)
        {
          return listener;
        }
    }

  return NULL;
}

static void linux_bt_sockif_note_pending_accept(
                                      FAR struct linux_bt_sockif_conn_s *conn)
{
  FAR struct linux_bt_sockif_conn_s *listener;

  listener = linux_bt_sockif_find_listener(conn);
  if (listener != NULL && listener->pending_accepts < UINT8_MAX)
    {
      listener->pending_accepts++;
    }
}

static int linux_bt_sockif_setup(FAR struct socket *psock)
{
  FAR struct linux_bt_sockif_conn_s *conn;

  if (psock == NULL)
    {
      return -ESOCKTNOSUPPORT;
    }

  if (psock->s_proto == BTPROTO_ISO)
    {
      if (psock->s_type != SOCK_SEQPACKET)
        {
          return -ESOCKTNOSUPPORT;
        }
    }
  else if (psock->s_proto == BTPROTO_L2CAP)
    {
      if (psock->s_type != SOCK_SEQPACKET &&
          psock->s_type != SOCK_STREAM &&
          psock->s_type != SOCK_DGRAM &&
          psock->s_type != SOCK_RAW)
        {
          return -ESOCKTNOSUPPORT;
        }
    }
  else if (psock->s_proto == BTPROTO_RFCOMM)
    {
      if (psock->s_type != SOCK_STREAM)
        {
          return -ESOCKTNOSUPPORT;
        }
    }
  else if (psock->s_proto == BTPROTO_SCO)
    {
      if (psock->s_type != SOCK_SEQPACKET)
        {
          return -ESOCKTNOSUPPORT;
        }
    }
  else if (psock->s_proto == BTPROTO_CMTP ||
           psock->s_proto == BTPROTO_HIDP)
    {
      if (psock->s_type != SOCK_RAW)
        {
          return -ESOCKTNOSUPPORT;
        }
    }
  else if (psock->s_type != SOCK_RAW)
    {
      return -ESOCKTNOSUPPORT;
    }

  if (psock->s_proto != BTPROTO_L2CAP &&
      psock->s_proto != BTPROTO_RFCOMM &&
      psock->s_proto != BTPROTO_SCO &&
      psock->s_proto != BTPROTO_BNEP &&
      psock->s_proto != BTPROTO_CMTP &&
      psock->s_proto != BTPROTO_HIDP &&
      psock->s_proto != BTPROTO_HCI &&
      psock->s_proto != BTPROTO_ISO)
    {
      return -EPROTONOSUPPORT;
    }

  conn = kmm_zalloc(sizeof(*conn));
  if (conn == NULL)
    {
      return -ENOMEM;
    }

  conn->refs = 1;
  conn->proto = psock->s_proto;
  conn->handle = LINUX_BT_SOCKIF_DEFAULT_HANDLE;
  conn->rfcomm_channel = 1;
  if (conn->proto == BTPROTO_ISO)
    {
      conn->handle = LINUX_BT_SOCKIF_DEFAULT_ISO_HANDLE;
      conn->iso_addr_type = 1;
    }

  psock->s_conn = conn;
  return OK;
}

static sockcaps_t linux_bt_sockif_sockcaps(FAR struct socket *psock)
{
  return 0;
}

static void linux_bt_sockif_addref(FAR struct socket *psock)
{
  FAR struct linux_bt_sockif_conn_s *conn;

  if (psock == NULL || psock->s_conn == NULL)
    {
      return;
    }

  conn = psock->s_conn;
  if (conn->refs < UINT8_MAX)
    {
      conn->refs++;
    }
}

static int linux_bt_sockif_bind(FAR struct socket *psock,
                                FAR const struct sockaddr *addr,
                                socklen_t addrlen)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  FAR const struct linux_bt_sockif_sockaddr_l2_s *l2addr;
  FAR const struct linux_bt_sockif_sockaddr_hci_s *hciaddr;
  FAR const struct linux_bt_sockif_sockaddr_iso_s *isoaddr;
  FAR const struct linux_bt_sockif_sockaddr_rc_s *rcaddr;
  char out[256];
  int ret;

  if (psock == NULL || psock->s_conn == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
  if (conn->proto == BTPROTO_HCI)
    {
      if (addrlen < sizeof(*hciaddr) || addr->sa_family != AF_BLUETOOTH)
        {
          return -EINVAL;
        }

      hciaddr = (FAR const struct linux_bt_sockif_sockaddr_hci_s *)addr;
      if (hciaddr->hci_channel == HCI_CHANNEL_RAW)
        {
          ret = linux_bt_upstream_hci_socket_open(hciaddr->hci_channel,
                                                  hciaddr->hci_dev,
                                                  &conn->hci_upstream);
          if (ret < 0)
            {
              ret = linux_bt_upstream_hci_raw_open_probe(hciaddr->hci_dev,
                                                         &conn->hci_raw);
            }
        }
      else if (hciaddr->hci_channel == HCI_CHANNEL_USER)
        {
          ret = linux_bt_upstream_hci_socket_open(hciaddr->hci_channel,
                                                  hciaddr->hci_dev,
                                                  &conn->hci_upstream);
        }
      else if (hciaddr->hci_channel == HCI_CHANNEL_CONTROL)
        {
          ret = linux_bt_upstream_hci_socket_open(hciaddr->hci_channel,
                                                  hciaddr->hci_dev,
                                                  &conn->hci_upstream);
          if (ret < 0)
            {
              ret = linux_bt_upstream_mgmt_listen_probe(out, sizeof(out));
            }
        }
      else if (hciaddr->hci_channel == HCI_CHANNEL_MONITOR)
        {
          ret = linux_bt_upstream_hci_socket_open(hciaddr->hci_channel,
                                                  hciaddr->hci_dev,
                                                  &conn->hci_upstream);
          if (ret < 0)
            {
              ret = linux_bt_upstream_monitor_listen_probe(out,
                                                           sizeof(out));
            }
        }
      else if (hciaddr->hci_channel == HCI_CHANNEL_LOGGING)
        {
          ret = linux_bt_upstream_hci_socket_open(hciaddr->hci_channel,
                                                  hciaddr->hci_dev,
                                                  &conn->hci_upstream);
        }
      else
        {
          return -EPROTONOSUPPORT;
        }

      if (ret < 0 && ret != -EALREADY)
        {
          return ret;
        }

      conn->hci_dev = hciaddr->hci_dev;
      conn->hci_channel = hciaddr->hci_channel;
      conn->bound = true;
      conn->connected = true;
      return OK;
    }

  if (conn->proto == BTPROTO_RFCOMM)
    {
      if (addrlen < sizeof(*rcaddr) || addr->sa_family != AF_BLUETOOTH)
        {
          return -EINVAL;
        }

      rcaddr = (FAR const struct linux_bt_sockif_sockaddr_rc_s *)addr;
      conn->rfcomm_channel = rcaddr->rc_channel != 0 ?
                             rcaddr->rc_channel : 1;
      if (conn->rfcomm_upstream == NULL)
        {
          ret = linux_bt_upstream_rfcomm_socket_open(
                  conn->rfcomm_channel, conn->handle,
                  &conn->rfcomm_upstream);
          if (ret < 0)
            {
              return ret;
            }
        }

      conn->bound = true;
      return OK;
    }

  if (conn->proto == BTPROTO_SCO)
    {
      if (addrlen < sizeof(struct linux_bt_sockif_sockaddr_sco_s) ||
          addr->sa_family != AF_BLUETOOTH)
        {
          return -EINVAL;
        }

      if (conn->sco_upstream == NULL)
        {
          ret = linux_bt_upstream_sco_socket_open(conn->handle,
                                                  &conn->sco_upstream);
          if (ret < 0)
            {
              return ret;
            }
        }

      conn->bound = true;
      return OK;
    }

  if (conn->proto == BTPROTO_ISO)
    {
      if (addrlen < sizeof(*isoaddr) || addr->sa_family != AF_BLUETOOTH)
        {
          return -EINVAL;
        }

      isoaddr = (FAR const struct linux_bt_sockif_sockaddr_iso_s *)addr;
      conn->iso_addr_type = isoaddr->iso_bdaddr_type;
      if (conn->iso_upstream == NULL)
        {
          ret = linux_bt_upstream_iso_socket_open(conn->iso_addr_type,
                                                  conn->handle,
                                                  &conn->iso_upstream);
          if (ret < 0)
            {
              return ret;
            }
        }

      conn->bound = true;
      return OK;
    }

  if (conn->proto != BTPROTO_L2CAP)
    {
      return -EOPNOTSUPP;
    }

  if (addrlen < sizeof(*l2addr) || addr->sa_family != AF_BLUETOOTH)
    {
      return -EINVAL;
    }

  l2addr = (FAR const struct linux_bt_sockif_sockaddr_l2_s *)addr;
      conn->psm = l2addr->l2_psm;
      conn->cid = l2addr->l2_cid;
      conn->l2_bdaddr_type = l2addr->l2_bdaddr_type;
      conn->handle = LINUX_BT_SOCKIF_DEFAULT_HANDLE;
  if (conn->l2cap_upstream == NULL)
    {
      ret = linux_bt_upstream_l2cap_socket_open(conn->psm, conn->cid,
                                                conn->handle,
                                                &conn->l2cap_upstream);
      if (ret < 0)
        {
          ret = linux_bt_upstream_l2cap_socket_bind_probe(
            conn->psm, conn->cid, conn->handle, out, sizeof(out));
        }
      if (ret < 0)
        {
          return ret;
        }
    }

  conn->bound = true;
  return OK;
}

static int linux_bt_sockif_getname_common(FAR struct socket *psock,
                                          FAR struct sockaddr *addr,
                                          FAR socklen_t *addrlen,
                                          bool peer)
{
  FAR struct linux_bt_sockif_conn_s *conn;

  if (psock == NULL || psock->s_conn == NULL ||
      addr == NULL || addrlen == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
  if (conn->proto == BTPROTO_BNEP ||
      conn->proto == BTPROTO_CMTP ||
      conn->proto == BTPROTO_HIDP)
    {
      return -EOPNOTSUPP;
    }

  if (peer && !conn->connected)
    {
      return -ENOTCONN;
    }

  memset(addr, 0, *addrlen);
  if (conn->proto == BTPROTO_L2CAP)
    {
      FAR struct linux_bt_sockif_sockaddr_l2_s *l2addr =
        (FAR struct linux_bt_sockif_sockaddr_l2_s *)addr;

      if (*addrlen < sizeof(*l2addr))
        {
          return -EINVAL;
        }

      l2addr->l2_family = AF_BLUETOOTH;
      l2addr->l2_psm = conn->psm;
      l2addr->l2_cid = conn->cid;
      l2addr->l2_bdaddr_type = conn->l2_bdaddr_type;
      *addrlen = sizeof(*l2addr);
      return OK;
    }

  if (conn->proto == BTPROTO_RFCOMM)
    {
      FAR struct linux_bt_sockif_sockaddr_rc_s *rcaddr =
        (FAR struct linux_bt_sockif_sockaddr_rc_s *)addr;

      if (*addrlen < sizeof(*rcaddr))
        {
          return -EINVAL;
        }

      rcaddr->rc_family = AF_BLUETOOTH;
      rcaddr->rc_channel = conn->rfcomm_channel;
      *addrlen = sizeof(*rcaddr);
      return OK;
    }

  if (conn->proto == BTPROTO_SCO)
    {
      FAR struct linux_bt_sockif_sockaddr_sco_s *scoaddr =
        (FAR struct linux_bt_sockif_sockaddr_sco_s *)addr;

      if (*addrlen < sizeof(*scoaddr))
        {
          return -EINVAL;
        }

      scoaddr->sco_family = AF_BLUETOOTH;
      *addrlen = sizeof(*scoaddr);
      return OK;
    }

  if (conn->proto == BTPROTO_ISO)
    {
      FAR struct linux_bt_sockif_sockaddr_iso_s *isoaddr =
        (FAR struct linux_bt_sockif_sockaddr_iso_s *)addr;

      if (*addrlen < sizeof(*isoaddr))
        {
          return -EINVAL;
        }

      isoaddr->iso_family = AF_BLUETOOTH;
      isoaddr->iso_bdaddr_type = conn->iso_addr_type;
      *addrlen = sizeof(*isoaddr);
      return OK;
    }

  if (conn->proto == BTPROTO_HCI)
    {
      FAR struct linux_bt_sockif_sockaddr_hci_s *hciaddr =
        (FAR struct linux_bt_sockif_sockaddr_hci_s *)addr;

      if (peer)
        {
          return -EOPNOTSUPP;
        }

      if (*addrlen < sizeof(*hciaddr))
        {
          return -EINVAL;
        }

      hciaddr->hci_family = AF_BLUETOOTH;
      hciaddr->hci_dev = conn->hci_dev;
      hciaddr->hci_channel = conn->hci_channel;
      *addrlen = sizeof(*hciaddr);
      return OK;
    }

  return -EOPNOTSUPP;
}

static int linux_bt_sockif_getsockname(FAR struct socket *psock,
                                       FAR struct sockaddr *addr,
                                       FAR socklen_t *addrlen)
{
  return linux_bt_sockif_getname_common(psock, addr, addrlen, false);
}

static int linux_bt_sockif_getpeername(FAR struct socket *psock,
                                       FAR struct sockaddr *addr,
                                       FAR socklen_t *addrlen)
{
  return linux_bt_sockif_getname_common(psock, addr, addrlen, true);
}

static int linux_bt_sockif_listen(FAR struct socket *psock, int backlog)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  int ret;

  if (psock == NULL || psock->s_conn == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
  if (conn->proto == BTPROTO_L2CAP)
    {
      if (conn->l2cap_upstream == NULL)
        {
          ret = linux_bt_upstream_l2cap_socket_open(
                  conn->psm, conn->cid, conn->handle,
                  &conn->l2cap_upstream);
          if (ret < 0)
            {
              return ret;
            }
        }

      ret = linux_bt_upstream_l2cap_socket_listen_handle(
              conn->l2cap_upstream, backlog);
      if (ret >= 0)
        {
          conn->bound = true;
          (void)linux_bt_sockif_register_listener(conn);
        }

      return ret;
    }

  if (conn->proto == BTPROTO_ISO)
    {
      if (conn->iso_upstream == NULL)
        {
          ret = linux_bt_upstream_iso_socket_open(conn->iso_addr_type,
                                                  conn->handle,
                                                  &conn->iso_upstream);
          if (ret < 0)
            {
              return ret;
            }
        }

      ret = linux_bt_upstream_iso_socket_listen_handle(conn->iso_upstream,
                                                       backlog);
      if (ret >= 0)
        {
          conn->bound = true;
          (void)linux_bt_sockif_register_listener(conn);
        }

      return ret;
    }

  if (conn->proto == BTPROTO_RFCOMM)
    {
      if (conn->rfcomm_upstream == NULL)
        {
          ret = linux_bt_upstream_rfcomm_socket_open(
                  conn->rfcomm_channel, conn->handle,
                  &conn->rfcomm_upstream);
          if (ret < 0)
            {
              return ret;
            }
        }

      ret = linux_bt_upstream_rfcomm_socket_listen_handle(
              conn->rfcomm_upstream, backlog);
      if (ret >= 0)
        {
          conn->bound = true;
        }

      return ret;
    }

  if (conn->proto == BTPROTO_SCO)
    {
      if (conn->sco_upstream == NULL)
        {
          ret = linux_bt_upstream_sco_socket_open(conn->handle,
                                                  &conn->sco_upstream);
          if (ret < 0)
            {
              return ret;
            }
        }

      ret = linux_bt_upstream_sco_socket_listen_handle(conn->sco_upstream,
                                                       backlog);
      if (ret >= 0)
        {
          conn->bound = true;
        }

      return ret;
    }

  return -EOPNOTSUPP;
}

static int linux_bt_sockif_accept(FAR struct socket *psock,
                                  FAR struct sockaddr *addr,
                                  FAR socklen_t *addrlen,
                                  FAR struct socket *newsock, int flags)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  FAR struct socket_conn_s *common;
  FAR struct linux_bt_sockif_conn_s *newconn;
  void *accepted = NULL;
  int ret;

  if (psock == NULL || psock->s_conn == NULL || newsock == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
  common = psock->s_conn;
  if ((conn->proto == BTPROTO_L2CAP || conn->proto == BTPROTO_ISO ||
       conn->proto == BTPROTO_RFCOMM || conn->proto == BTPROTO_SCO) &&
      !_SS_ISLISTENING(common->s_flags))
    {
      return -EINVAL;
    }

  if ((conn->proto == BTPROTO_L2CAP || conn->proto == BTPROTO_ISO) &&
      conn->pending_accepts == 0)
    {
      return -EAGAIN;
    }

  newconn = kmm_zalloc(sizeof(*newconn));
  if (newconn == NULL)
    {
      return -ENOMEM;
    }

  if (conn->proto == BTPROTO_L2CAP)
    {
      ret = conn->l2cap_upstream != NULL ?
            linux_bt_upstream_l2cap_socket_accept_handle(
              conn->l2cap_upstream, &accepted) : -ENOTCONN;
      if (ret >= 0)
        {
          newconn->l2cap_upstream = accepted;
        }
    }
  else if (conn->proto == BTPROTO_ISO)
    {
      ret = conn->iso_upstream != NULL ?
            linux_bt_upstream_iso_socket_accept_handle(
              conn->iso_upstream, &accepted) : -ENOTCONN;
      if (ret >= 0)
        {
          newconn->iso_upstream = accepted;
        }
    }
  else if (conn->proto == BTPROTO_RFCOMM)
    {
      ret = conn->rfcomm_upstream != NULL ?
            linux_bt_upstream_rfcomm_socket_accept_handle(
              conn->rfcomm_upstream, &accepted) : -ENOTCONN;
      if (ret >= 0)
        {
          newconn->rfcomm_upstream = accepted;
          newconn->rfcomm_channel = conn->rfcomm_channel;
        }
    }
  else if (conn->proto == BTPROTO_SCO)
    {
      ret = conn->sco_upstream != NULL ?
            linux_bt_upstream_sco_socket_accept_handle(
              conn->sco_upstream, &accepted) : -ENOTCONN;
      if (ret >= 0)
        {
          newconn->sco_upstream = accepted;
        }
    }
  else
    {
      ret = -EOPNOTSUPP;
    }

  if (ret < 0)
    {
      kmm_free(newconn);
      return ret;
    }

  newconn->refs = 1;
  newconn->proto = conn->proto;
  newconn->psm = conn->psm;
  newconn->cid = conn->cid;
  newconn->handle = conn->handle;
  newconn->hci_dev = conn->hci_dev;
  newconn->hci_channel = conn->hci_channel;
  newconn->l2_bdaddr_type = conn->l2_bdaddr_type;
  newconn->iso_addr_type = conn->iso_addr_type;
  newconn->bound = true;
  newconn->connected = true;
  if ((conn->proto == BTPROTO_L2CAP || conn->proto == BTPROTO_ISO) &&
      conn->pending_accepts > 0)
    {
      conn->pending_accepts--;
    }

  newsock->s_sockif = psock->s_sockif;
  newsock->s_conn = (FAR struct socket_conn_s *)newconn;

  if (addr != NULL && addrlen != NULL && *addrlen > 0)
    {
      (void)linux_bt_sockif_getname_common(newsock, addr, addrlen, true);
    }

  (void)flags;
  return OK;
}

static int linux_bt_sockif_connect(FAR struct socket *psock,
                                   FAR const struct sockaddr *addr,
                                   socklen_t addrlen)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  FAR const struct linux_bt_sockif_sockaddr_l2_s *l2addr;
  FAR const struct linux_bt_sockif_sockaddr_iso_s *isoaddr;
  FAR const struct linux_bt_sockif_sockaddr_rc_s *rcaddr;
  char out[256];
  int ret;

  if (psock == NULL || psock->s_conn == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
  if (conn->proto == BTPROTO_ISO)
    {
      if (addrlen < sizeof(*isoaddr) || addr->sa_family != AF_BLUETOOTH)
        {
          return -EINVAL;
        }

      isoaddr = (FAR const struct linux_bt_sockif_sockaddr_iso_s *)addr;
      conn->iso_addr_type = isoaddr->iso_bdaddr_type;
      if (!conn->bound)
        {
          ret = linux_bt_upstream_iso_socket_open(conn->iso_addr_type,
                                                  conn->handle,
                                                  &conn->iso_upstream);
          if (ret < 0)
            {
              return ret;
            }

          conn->bound = true;
        }

      ret = linux_bt_upstream_iso_socket_connect_handle(conn->iso_upstream,
                                                        conn->iso_addr_type);
      if (ret < 0)
        {
          return ret;
        }

      conn->connected = true;
      linux_bt_sockif_note_pending_accept(conn);
      return OK;
    }

  if (conn->proto == BTPROTO_RFCOMM)
    {
      if (addrlen < sizeof(*rcaddr) || addr->sa_family != AF_BLUETOOTH)
        {
          return -EINVAL;
        }

      rcaddr = (FAR const struct linux_bt_sockif_sockaddr_rc_s *)addr;
      conn->rfcomm_channel = rcaddr->rc_channel != 0 ?
                             rcaddr->rc_channel : conn->rfcomm_channel;
      if (!conn->bound)
        {
          ret = linux_bt_upstream_rfcomm_socket_open(
                  conn->rfcomm_channel, conn->handle,
                  &conn->rfcomm_upstream);
          if (ret < 0)
            {
              return ret;
            }

          conn->bound = true;
        }

      ret = linux_bt_upstream_rfcomm_socket_connect_handle(
              conn->rfcomm_upstream, conn->rfcomm_channel);
      if (ret < 0)
        {
          return ret;
        }

      conn->connected = true;
      return OK;
    }

  if (conn->proto == BTPROTO_SCO)
    {
      if (addrlen < sizeof(struct linux_bt_sockif_sockaddr_sco_s) ||
          addr->sa_family != AF_BLUETOOTH)
        {
          return -EINVAL;
        }

      if (!conn->bound)
        {
          ret = linux_bt_upstream_sco_socket_open(conn->handle,
                                                  &conn->sco_upstream);
          if (ret < 0)
            {
              return ret;
            }

          conn->bound = true;
        }

      ret = linux_bt_upstream_sco_socket_connect_handle(conn->sco_upstream,
                                                        conn->handle);
      if (ret < 0)
        {
          return ret;
        }

      conn->connected = true;
      return OK;
    }

  if (conn->proto != BTPROTO_L2CAP)
    {
      return -EOPNOTSUPP;
    }

  if (addrlen < sizeof(*l2addr) || addr->sa_family != AF_BLUETOOTH)
    {
      return -EINVAL;
    }

  l2addr = (FAR const struct linux_bt_sockif_sockaddr_l2_s *)addr;
  if (l2addr->l2_psm != 0)
    {
      conn->psm = l2addr->l2_psm;
    }

      if (l2addr->l2_cid != 0)
        {
          conn->cid = l2addr->l2_cid;
        }

      conn->l2_bdaddr_type = l2addr->l2_bdaddr_type;

  if (!conn->bound)
    {
      ret = linux_bt_upstream_l2cap_socket_open(conn->psm, conn->cid,
                                                conn->handle,
                                                &conn->l2cap_upstream);
      if (ret < 0)
        {
          ret = linux_bt_upstream_l2cap_socket_bind_probe(
            conn->psm, conn->cid, conn->handle, out, sizeof(out));
        }
      if (ret < 0)
        {
          return ret;
        }

      conn->bound = true;
    }

  if (conn->l2cap_upstream != NULL)
    {
      ret = linux_bt_upstream_l2cap_socket_connect_handle(
        conn->l2cap_upstream, conn->psm, conn->cid);
    }
  else
    {
      ret = linux_bt_upstream_l2cap_socket_connect_probe(conn->psm,
                                                         conn->cid, out,
                                                         sizeof(out));
    }
  if (ret < 0)
    {
      return ret;
    }

  conn->connected = true;
  linux_bt_sockif_note_pending_accept(conn);
  return OK;
}

static int linux_bt_sockif_poll(FAR struct socket *psock,
                                FAR struct pollfd *fds, bool setup)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  short revents = 0;
  int ret;

  if (psock == NULL || psock->s_conn == NULL || fds == NULL)
    {
      return -EINVAL;
    }

  if (!setup)
    {
      return OK;
    }

  conn = psock->s_conn;
  if (conn->proto == BTPROTO_L2CAP)
    {
      ret = conn->l2cap_upstream != NULL ?
            linux_bt_upstream_l2cap_socket_poll_handle(
              conn->l2cap_upstream, fds->events, &revents) : -ENOTCONN;
    }
  else if (conn->proto == BTPROTO_ISO)
    {
      ret = conn->iso_upstream != NULL ?
            linux_bt_upstream_iso_socket_poll_handle(
              conn->iso_upstream, fds->events, &revents) : -ENOTCONN;
    }
  else if (conn->proto == BTPROTO_RFCOMM)
    {
      ret = conn->rfcomm_upstream != NULL ?
            linux_bt_upstream_rfcomm_socket_poll_handle(
              conn->rfcomm_upstream, fds->events, &revents) : -ENOTCONN;
    }
  else if (conn->proto == BTPROTO_SCO)
    {
      ret = conn->sco_upstream != NULL ?
            linux_bt_upstream_sco_socket_poll_handle(
              conn->sco_upstream, fds->events, &revents) : -ENOTCONN;
    }
  else
    {
      ret = -EOPNOTSUPP;
    }

  if (ret < 0)
    {
      return ret;
    }

  if (revents != 0)
    {
      poll_notify(&fds, 1, revents);
    }

  return OK;
}

static ssize_t linux_bt_sockif_sendmsg(FAR struct socket *psock,
                                       FAR const struct msghdr *msg,
                                       int flags)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  uint8_t payload[LINUX_BT_SOCKIF_MGMT_MAX];
  size_t copied = 0;
  int ret;
  int i;

  if (psock == NULL || psock->s_conn == NULL || msg == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
  if (conn->proto == BTPROTO_ISO)
    {
      char out[192];

      if (!conn->bound || conn->iso_upstream == NULL)
        {
          return -ENOTCONN;
        }

      for (i = 0; i < msg->msg_iovlen; i++)
        {
          FAR const struct iovec *iov = &msg->msg_iov[i];

          if (iov->iov_len == 0)
            {
              continue;
            }

          if (iov->iov_base == NULL)
            {
              return -EINVAL;
            }

          if (copied + iov->iov_len > sizeof(payload))
            {
              return -EMSGSIZE;
            }

          memcpy(payload + copied, iov->iov_base, iov->iov_len);
          copied += iov->iov_len;
        }

      ret = linux_bt_upstream_iso_socket_write_handle(conn->iso_upstream,
                                                      payload, copied,
                                                      out, sizeof(out));
      return ret < 0 ? ret : (ssize_t)copied;
    }

  if (conn->proto == BTPROTO_RFCOMM || conn->proto == BTPROTO_SCO)
    {
      char out[256];

      if (!conn->bound)
        {
          return -ENOTCONN;
        }

      for (i = 0; i < msg->msg_iovlen; i++)
        {
          FAR const struct iovec *iov = &msg->msg_iov[i];

          if (iov->iov_len == 0)
            {
              continue;
            }

          if (iov->iov_base == NULL)
            {
              return -EINVAL;
            }

          if (copied + iov->iov_len > sizeof(payload))
            {
              return -EMSGSIZE;
            }

          memcpy(payload + copied, iov->iov_base, iov->iov_len);
          copied += iov->iov_len;
        }

      if (conn->proto == BTPROTO_RFCOMM)
        {
          ret = conn->rfcomm_upstream != NULL ?
                linux_bt_upstream_rfcomm_socket_write_handle(
                  conn->rfcomm_upstream, payload, copied, out,
                  sizeof(out)) : -ENOTCONN;
        }
      else
        {
          ret = conn->sco_upstream != NULL ?
                linux_bt_upstream_sco_socket_write_handle(
                  conn->sco_upstream, payload, copied, out,
                  sizeof(out)) : -ENOTCONN;
        }

      return ret < 0 ? ret : (ssize_t)copied;
    }

  if (conn->proto != BTPROTO_HCI)
    {
      return -EOPNOTSUPP;
    }

  if (!conn->bound ||
      (conn->hci_channel != HCI_CHANNEL_CONTROL &&
       conn->hci_channel != HCI_CHANNEL_RAW &&
       conn->hci_channel != HCI_CHANNEL_USER &&
       conn->hci_channel != HCI_CHANNEL_LOGGING))
    {
      return conn->hci_channel == HCI_CHANNEL_MONITOR ? -EOPNOTSUPP :
             -ENOTCONN;
    }

  for (i = 0; i < msg->msg_iovlen; i++)
    {
      FAR const struct iovec *iov = &msg->msg_iov[i];

      if (iov->iov_len == 0)
        {
          continue;
        }

      if (iov->iov_base == NULL)
        {
          return -EINVAL;
        }

      if (copied + iov->iov_len > sizeof(payload))
        {
          return -EMSGSIZE;
        }

      memcpy(payload + copied, iov->iov_base, iov->iov_len);
      copied += iov->iov_len;
    }

  if (conn->hci_channel == HCI_CHANNEL_RAW)
    {
      if (conn->hci_upstream != NULL)
        {
          ret = linux_bt_upstream_hci_socket_send(conn->hci_upstream,
                                                  payload, copied);
        }
      else
        {
          ret = linux_bt_upstream_hci_raw_send_probe(conn->hci_raw, payload,
                                                     copied);
        }
    }
  else if (conn->hci_upstream != NULL)
    {
      ret = linux_bt_upstream_hci_socket_send(conn->hci_upstream, payload,
                                              copied);
      if (ret >= 0 && conn->hci_channel == HCI_CHANNEL_CONTROL)
        {
          linux_bt_upstream_mgmt_note_socket_send(payload, copied);
        }
    }
  else
    {
      ret = linux_bt_upstream_mgmt_send_raw_probe(payload, copied);
    }

  return ret < 0 ? ret : (ssize_t)copied;
}

static ssize_t linux_bt_sockif_recvmsg(FAR struct socket *psock,
                                       FAR struct msghdr *msg,
                                       int flags)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  uint8_t payload[LINUX_BT_SOCKIF_MGMT_MAX];
  size_t capacity = 0;
  size_t copied = 0;
  int msg_flags = 0;
  int ret;
  int i;

  if (psock == NULL || psock->s_conn == NULL || msg == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
  if (conn->proto == BTPROTO_ISO)
    {
      if (!conn->bound || conn->iso_upstream == NULL)
        {
          return -ENOTCONN;
        }

      for (i = 0; i < msg->msg_iovlen; i++)
        {
          capacity += msg->msg_iov[i].iov_len;
        }

      if (capacity == 0)
        {
          return 0;
        }

      if (capacity > sizeof(payload))
        {
          capacity = sizeof(payload);
        }

      ret = linux_bt_upstream_iso_socket_recv_handle(conn->iso_upstream,
                                                     payload, capacity,
                                                     &msg_flags);
      if (ret < 0)
        {
          return ret;
        }

      msg->msg_flags = msg_flags;
      for (i = 0; i < msg->msg_iovlen && copied < (size_t)ret; i++)
        {
          FAR struct iovec *iov = &msg->msg_iov[i];
          size_t chunk;

          if (iov->iov_len == 0)
            {
              continue;
            }

          if (iov->iov_base == NULL)
            {
              return -EINVAL;
            }

          chunk = (size_t)ret - copied;
          if (chunk > iov->iov_len)
            {
              chunk = iov->iov_len;
            }

          memcpy(iov->iov_base, payload + copied, chunk);
          copied += chunk;
        }

      return ret;
    }

  if (conn->proto == BTPROTO_RFCOMM || conn->proto == BTPROTO_SCO)
    {
      char out[256];

      if (!conn->bound)
        {
          return -ENOTCONN;
        }

      for (i = 0; i < msg->msg_iovlen; i++)
        {
          capacity += msg->msg_iov[i].iov_len;
        }

      if (capacity == 0)
        {
          return 0;
        }

      if (capacity > sizeof(payload))
        {
          capacity = sizeof(payload);
        }

      if (conn->proto == BTPROTO_RFCOMM)
        {
          ret = conn->rfcomm_upstream != NULL ?
                linux_bt_upstream_rfcomm_socket_recv_handle(
                  conn->rfcomm_upstream, payload, capacity, &msg_flags,
                  out, sizeof(out)) : -ENOTCONN;
        }
      else
        {
          ret = conn->sco_upstream != NULL ?
                linux_bt_upstream_sco_socket_recv_handle(
                  conn->sco_upstream, payload, capacity, &msg_flags,
                  out, sizeof(out)) : -ENOTCONN;
        }

      if (ret < 0)
        {
          return ret;
        }

      msg->msg_flags = msg_flags;
      for (i = 0; i < msg->msg_iovlen && copied < (size_t)ret; i++)
        {
          FAR struct iovec *iov = &msg->msg_iov[i];
          size_t chunk;

          if (iov->iov_len == 0)
            {
              continue;
            }

          if (iov->iov_base == NULL)
            {
              return -EINVAL;
            }

          chunk = (size_t)ret - copied;
          if (chunk > iov->iov_len)
            {
              chunk = iov->iov_len;
            }

          memcpy(iov->iov_base, payload + copied, chunk);
          copied += chunk;
        }

      return ret;
    }

  if (conn->proto != BTPROTO_HCI)
    {
      return -EOPNOTSUPP;
    }

  if (!conn->bound ||
      (conn->hci_channel != HCI_CHANNEL_CONTROL &&
       conn->hci_channel != HCI_CHANNEL_RAW &&
       conn->hci_channel != HCI_CHANNEL_USER &&
       conn->hci_channel != HCI_CHANNEL_MONITOR &&
       conn->hci_channel != HCI_CHANNEL_LOGGING))
    {
      return -ENOTCONN;
    }

  for (i = 0; i < msg->msg_iovlen; i++)
    {
      capacity += msg->msg_iov[i].iov_len;
    }

  if (capacity == 0)
    {
      return 0;
    }

  if (capacity > sizeof(payload))
    {
      capacity = sizeof(payload);
    }

  if (conn->hci_channel == HCI_CHANNEL_CONTROL)
    {
      if (conn->hci_upstream != NULL)
        {
          ret = linux_bt_upstream_hci_socket_recv(conn->hci_upstream,
                                                  payload, capacity,
                                                  &msg_flags);
        }
      else
        {
          ret = linux_bt_upstream_mgmt_recv_raw_probe(payload, capacity,
                                                      &msg_flags);
        }
    }
  else if (conn->hci_channel == HCI_CHANNEL_RAW)
    {
      if (conn->hci_upstream != NULL)
        {
          ret = linux_bt_upstream_hci_socket_recv(conn->hci_upstream,
                                                  payload, capacity,
                                                  &msg_flags);
        }
      else
        {
          ret = linux_bt_upstream_hci_raw_recv_probe(conn->hci_raw, payload,
                                                     capacity, &msg_flags);
        }
    }
  else if (conn->hci_channel == HCI_CHANNEL_USER)
    {
      ret = conn->hci_upstream != NULL ?
            linux_bt_upstream_hci_socket_recv(conn->hci_upstream,
                                              payload, capacity,
                                              &msg_flags) : -ENOTCONN;
    }
  else if (conn->hci_channel == HCI_CHANNEL_MONITOR)
    {
      if (conn->hci_upstream != NULL)
        {
          ret = linux_bt_upstream_hci_socket_recv(conn->hci_upstream,
                                                  payload, capacity,
                                                  &msg_flags);
        }
      else
        {
          ret = linux_bt_upstream_monitor_recv_raw_probe(payload, capacity,
                                                         &msg_flags);
        }
    }
  else
    {
      return -ENOTCONN;
    }
  if (ret < 0)
    {
      return ret;
    }

  if (conn->hci_channel == HCI_CHANNEL_CONTROL && conn->hci_upstream != NULL)
    {
      linux_bt_upstream_mgmt_note_socket_recv(payload, ret);
    }

  msg->msg_flags = msg_flags;
  for (i = 0; i < msg->msg_iovlen && copied < (size_t)ret; i++)
    {
      FAR struct iovec *iov = &msg->msg_iov[i];
      size_t chunk;

      if (iov->iov_len == 0)
        {
          continue;
        }

      if (iov->iov_base == NULL)
        {
          return -EINVAL;
        }

      chunk = (size_t)ret - copied;
      if (chunk > iov->iov_len)
        {
          chunk = iov->iov_len;
        }

      memcpy(iov->iov_base, payload + copied, chunk);
      copied += chunk;
    }

  return ret;
}

static int linux_bt_sockif_close(FAR struct socket *psock)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  char out[128];

  if (psock == NULL || psock->s_conn == NULL)
    {
      return OK;
    }

  conn = psock->s_conn;
  linux_bt_sockif_unregister_listener(conn);
  if (conn->refs > 1)
    {
      conn->refs--;
      return OK;
    }

  if (conn->proto == BTPROTO_L2CAP)
    {
      if (conn->l2cap_upstream != NULL)
        {
          (void)linux_bt_upstream_l2cap_socket_close_handle(
            conn->l2cap_upstream);
          conn->l2cap_upstream = NULL;
        }
      else if (conn->connected)
        {
          (void)linux_bt_upstream_l2cap_socket_close_probe(out,
                                                           sizeof(out));
        }
    }
  else if (conn->proto == BTPROTO_HCI)
    {
      if (conn->hci_upstream != NULL)
        {
          (void)linux_bt_upstream_hci_socket_close(conn->hci_upstream);
          conn->hci_upstream = NULL;
        }
      else if (conn->bound && conn->hci_channel == HCI_CHANNEL_CONTROL)
        {
          (void)linux_bt_upstream_mgmt_close_probe(out, sizeof(out));
        }
      else if (conn->bound && conn->hci_channel == HCI_CHANNEL_MONITOR)
        {
          (void)linux_bt_upstream_monitor_close_probe(out, sizeof(out));
        }
      else if (conn->bound && conn->hci_channel == HCI_CHANNEL_RAW)
        {
            (void)linux_bt_upstream_hci_raw_close_probe(conn->hci_raw);
        }
    }
  else if (conn->proto == BTPROTO_ISO)
    {
      if (conn->iso_upstream != NULL)
        {
          (void)linux_bt_upstream_iso_socket_close_handle(
            conn->iso_upstream);
          conn->iso_upstream = NULL;
        }
    }
  else if (conn->proto == BTPROTO_RFCOMM)
    {
      if (conn->rfcomm_upstream != NULL)
        {
          (void)linux_bt_upstream_rfcomm_socket_close_handle(
            conn->rfcomm_upstream);
          conn->rfcomm_upstream = NULL;
        }
    }
  else if (conn->proto == BTPROTO_SCO)
    {
      if (conn->sco_upstream != NULL)
        {
          (void)linux_bt_upstream_sco_socket_close_handle(
            conn->sco_upstream);
          conn->sco_upstream = NULL;
        }
    }
  else if (conn->proto == BTPROTO_CMTP)
    {
      if (conn->cmtp_upstream != NULL)
        {
          (void)linux_bt_upstream_cmtp_socket_close_handle(
            conn->cmtp_upstream);
          conn->cmtp_upstream = NULL;
        }
    }
  else if (conn->proto == BTPROTO_HIDP)
    {
      if (conn->hidp_upstream != NULL)
        {
          (void)linux_bt_upstream_hidp_socket_close_handle(
            conn->hidp_upstream);
          conn->hidp_upstream = NULL;
        }
    }

  kmm_free(conn);
  psock->s_conn = NULL;
  return OK;
}

static int linux_bt_sockif_ioctl(FAR struct socket *psock, int cmd,
                                 unsigned long arg)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  FAR struct linux_bt_sockif_connadd_req_s *ca;
  int ret;

  if (psock == NULL || psock->s_conn == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
  if (cmd == FIONBIO)
    {
      FAR int *nonblock = (FAR int *)(uintptr_t)arg;

      if (nonblock != NULL && *nonblock)
        {
          conn->common.s_flags |= _SF_NONBLOCK;
        }
      else
        {
          conn->common.s_flags &= ~_SF_NONBLOCK;
        }

      return OK;
    }

  if (conn->proto == BTPROTO_HCI)
    {
      if (conn->hci_upstream == NULL)
        {
          ret = linux_bt_upstream_hci_socket_create(&conn->hci_upstream);
          if (ret < 0)
            {
              return linux_bt_upstream_hci_ioctl_raw((unsigned int)cmd,
                                                     arg);
            }
        }

      return linux_bt_upstream_hci_socket_ioctl(conn->hci_upstream,
                                                (unsigned int)cmd, arg);
    }

  if (conn->proto == BTPROTO_L2CAP)
    {
      return conn->l2cap_upstream != NULL ?
             linux_bt_upstream_l2cap_socket_ioctl_handle(
               conn->l2cap_upstream, (unsigned int)cmd, arg) : -ENOTCONN;
    }

  if (conn->proto == BTPROTO_ISO)
    {
      return conn->iso_upstream != NULL ?
             linux_bt_upstream_iso_socket_ioctl_handle(
               conn->iso_upstream, (unsigned int)cmd, arg) : -ENOTCONN;
    }

  if (conn->proto == BTPROTO_RFCOMM)
    {
      return conn->rfcomm_upstream != NULL ?
             linux_bt_upstream_rfcomm_socket_ioctl_handle(
               conn->rfcomm_upstream, (unsigned int)cmd, arg) : -ENOTCONN;
    }

  if (conn->proto == BTPROTO_SCO)
    {
      return conn->sco_upstream != NULL ?
             linux_bt_upstream_sco_socket_ioctl_handle(
               conn->sco_upstream, (unsigned int)cmd, arg) : -ENOTCONN;
    }

  if (conn->proto == BTPROTO_CMTP)
    {
      if (conn->cmtp_upstream == NULL)
        {
          ret = linux_bt_upstream_cmtp_socket_open(&conn->cmtp_upstream);
          if (ret < 0)
            {
              return ret;
            }
        }

      return linux_bt_upstream_cmtp_socket_ioctl_handle(
               conn->cmtp_upstream, (unsigned int)cmd, arg);
    }

  if (conn->proto == BTPROTO_HIDP)
    {
      if (conn->hidp_upstream == NULL)
        {
          ret = linux_bt_upstream_hidp_socket_open(&conn->hidp_upstream);
          if (ret < 0)
            {
              return ret;
            }
        }

      return linux_bt_upstream_hidp_socket_ioctl_handle(
               conn->hidp_upstream, (unsigned int)cmd, arg);
    }

  if (conn->proto != BTPROTO_BNEP)
    {
      return -ENOTTY;
    }

  switch (cmd)
    {
      case BNEPCONNADD:
        ca = (FAR struct linux_bt_sockif_connadd_req_s *)arg;
        if (ca == NULL)
          {
            return -EFAULT;
          }

        if (ca->sock < 0)
          {
            return -EBADFD;
          }

        ret = linux_bt_upstream_bnep_ioctl_raw(cmd, arg);
        if (ret >= 0)
          {
            snprintf(ca->device, sizeof(ca->device), "btn0");
          }

        return ret;

      case BNEPCONNDEL:
      case BNEPGETSUPPFEAT:
      case BNEPGETCONNLIST:
      case BNEPGETCONNINFO:
        return linux_bt_upstream_bnep_ioctl_raw(cmd, arg);

      default:
        return -EINVAL;
    }
}

static int linux_bt_sockif_shutdown(FAR struct socket *psock, int how)
{
  FAR struct linux_bt_sockif_conn_s *conn;

  if (psock == NULL || psock->s_conn == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
  if (conn->proto == BTPROTO_L2CAP)
    {
      return conn->l2cap_upstream != NULL ?
             linux_bt_upstream_l2cap_socket_shutdown_handle(
               conn->l2cap_upstream, how) : -ENOTCONN;
    }

  if (conn->proto == BTPROTO_ISO)
    {
      return conn->iso_upstream != NULL ?
             linux_bt_upstream_iso_socket_shutdown_handle(
               conn->iso_upstream, how) : -ENOTCONN;
    }

  if (conn->proto == BTPROTO_RFCOMM)
    {
      return conn->rfcomm_upstream != NULL ?
             linux_bt_upstream_rfcomm_socket_shutdown_handle(
               conn->rfcomm_upstream, how) : -ENOTCONN;
    }

  if (conn->proto == BTPROTO_SCO)
    {
      return conn->sco_upstream != NULL ?
             linux_bt_upstream_sco_socket_shutdown_handle(
               conn->sco_upstream, how) : -ENOTCONN;
    }

  return -EOPNOTSUPP;
}

#ifdef CONFIG_NET_SOCKOPTS
static int linux_bt_sockif_l2cap_ensure_upstream(
                                      FAR struct linux_bt_sockif_conn_s *conn)
{
  if (conn == NULL || conn->proto != BTPROTO_L2CAP)
    {
      return -EINVAL;
    }

  if (conn->l2cap_upstream != NULL)
    {
      return OK;
    }

  return linux_bt_upstream_l2cap_socket_open(conn->psm, conn->cid,
                                             conn->handle,
                                             &conn->l2cap_upstream);
}

static int linux_bt_sockif_iso_ensure_upstream(
                                      FAR struct linux_bt_sockif_conn_s *conn)
{
  if (conn == NULL || conn->proto != BTPROTO_ISO)
    {
      return -EINVAL;
    }

  if (conn->iso_upstream != NULL)
    {
      return OK;
    }

  return linux_bt_upstream_iso_socket_open(conn->iso_addr_type,
                                           conn->handle,
                                           &conn->iso_upstream);
}

static int linux_bt_sockif_rfcomm_ensure_upstream(
                                      FAR struct linux_bt_sockif_conn_s *conn)
{
  if (conn == NULL || conn->proto != BTPROTO_RFCOMM)
    {
      return -EINVAL;
    }

  if (conn->rfcomm_upstream != NULL)
    {
      return OK;
    }

  return linux_bt_upstream_rfcomm_socket_open(conn->rfcomm_channel,
                                              conn->handle,
                                              &conn->rfcomm_upstream);
}

static int linux_bt_sockif_sco_ensure_upstream(
                                      FAR struct linux_bt_sockif_conn_s *conn)
{
  if (conn == NULL || conn->proto != BTPROTO_SCO)
    {
      return -EINVAL;
    }

  if (conn->sco_upstream != NULL)
    {
      return OK;
    }

  return linux_bt_upstream_sco_socket_open(conn->handle,
                                           &conn->sco_upstream);
}

static int linux_bt_sockif_getsockopt(FAR struct socket *psock, int level,
                                      int option, FAR void *value,
                                      FAR socklen_t *value_len)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  int len;
  int ret;

  if (psock == NULL || psock->s_conn == NULL || value_len == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
  if (conn->proto == BTPROTO_L2CAP)
    {
      ret = linux_bt_sockif_l2cap_ensure_upstream(conn);
      if (ret < 0)
        {
          return ret;
        }

      len = (int)*value_len;
      ret = linux_bt_upstream_l2cap_socket_getsockopt_handle(
              conn->l2cap_upstream, level, option, value, &len);
      if (ret >= 0)
        {
          *value_len = (socklen_t)len;
        }

      return ret;
    }

  if (conn->proto == BTPROTO_ISO)
    {
      ret = linux_bt_sockif_iso_ensure_upstream(conn);
      if (ret < 0)
        {
          return ret;
        }

      len = (int)*value_len;
      ret = linux_bt_upstream_iso_socket_getsockopt_handle(
              conn->iso_upstream, level, option, value, &len);
      if (ret >= 0)
        {
          *value_len = (socklen_t)len;
        }

      return ret;
    }

  if (conn->proto == BTPROTO_RFCOMM)
    {
      ret = linux_bt_sockif_rfcomm_ensure_upstream(conn);
      if (ret < 0)
        {
          return ret;
        }

      len = (int)*value_len;
      ret = linux_bt_upstream_rfcomm_socket_getsockopt_handle(
              conn->rfcomm_upstream, level, option, value, &len);
      if (ret >= 0)
        {
          *value_len = (socklen_t)len;
        }

      return ret;
    }

  if (conn->proto == BTPROTO_SCO)
    {
      ret = linux_bt_sockif_sco_ensure_upstream(conn);
      if (ret < 0)
        {
          return ret;
        }

      len = (int)*value_len;
      ret = linux_bt_upstream_sco_socket_getsockopt_handle(
              conn->sco_upstream, level, option, value, &len);
      if (ret >= 0)
        {
          *value_len = (socklen_t)len;
        }

      return ret;
    }

  if (conn->proto != BTPROTO_HCI || conn->hci_channel != HCI_CHANNEL_RAW)
    {
      return -ENOPROTOOPT;
    }

  len = (int)*value_len;
  if (conn->hci_upstream != NULL)
    {
      ret = linux_bt_upstream_hci_socket_getsockopt(conn->hci_upstream,
                                                    level, option, value,
                                                    &len);
    }
  else
    {
      ret = linux_bt_upstream_hci_raw_getsockopt_probe(conn->hci_raw, level,
                                                       option, value, &len);
    }
  if (ret >= 0)
    {
      *value_len = (socklen_t)len;
    }

  return ret;
}

static int linux_bt_sockif_setsockopt(FAR struct socket *psock, int level,
                                      int option, FAR const void *value,
                                      socklen_t value_len)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  int ret;

  if (psock == NULL || psock->s_conn == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
  if (conn->proto == BTPROTO_L2CAP)
    {
      ret = linux_bt_sockif_l2cap_ensure_upstream(conn);
      if (ret < 0)
        {
          return ret;
        }

      return linux_bt_upstream_l2cap_socket_setsockopt_handle(
               conn->l2cap_upstream, level, option, value, value_len);
    }

  if (conn->proto == BTPROTO_ISO)
    {
      ret = linux_bt_sockif_iso_ensure_upstream(conn);
      if (ret < 0)
        {
          return ret;
        }

      return linux_bt_upstream_iso_socket_setsockopt_handle(
               conn->iso_upstream, level, option, value, value_len);
    }

  if (conn->proto == BTPROTO_RFCOMM)
    {
      ret = linux_bt_sockif_rfcomm_ensure_upstream(conn);
      if (ret < 0)
        {
          return ret;
        }

      return linux_bt_upstream_rfcomm_socket_setsockopt_handle(
               conn->rfcomm_upstream, level, option, value, value_len);
    }

  if (conn->proto == BTPROTO_SCO)
    {
      ret = linux_bt_sockif_sco_ensure_upstream(conn);
      if (ret < 0)
        {
          return ret;
        }

      return linux_bt_upstream_sco_socket_setsockopt_handle(
               conn->sco_upstream, level, option, value, value_len);
    }

  if (conn->proto != BTPROTO_HCI || conn->hci_channel != HCI_CHANNEL_RAW)
    {
      return -ENOPROTOOPT;
    }

  if (conn->hci_upstream != NULL)
    {
      return linux_bt_upstream_hci_socket_setsockopt(conn->hci_upstream,
                                                     level, option, value,
                                                     value_len);
    }

  return linux_bt_upstream_hci_raw_setsockopt_probe(conn->hci_raw, level,
                                                    option, value,
                                                    value_len);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int linux_bt_sockif_l2cap_state_from_fd(
  int fd, FAR struct linux_bt_sockif_l2cap_fd_state *state)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  FAR struct socket *psock;
  FAR struct file *filep;
  int ret;

  if (state == NULL)
    {
      return -EINVAL;
    }

  memset(state, 0, sizeof(*state));
  ret = sockfd_socket(fd, &filep, &psock);
  if (ret < 0)
    {
      return ret;
    }

  if (psock == NULL || psock->s_conn == NULL ||
      psock->s_sockif != &g_linux_bt_sockif)
    {
      ret = -ENOTSOCK;
      goto out;
    }

  conn = psock->s_conn;
  if (conn->proto != BTPROTO_L2CAP || !conn->connected)
    {
      ret = -EBADFD;
      goto out;
    }

  state->psm = conn->psm;
  state->cid = conn->cid;
  state->handle = conn->handle;
  ret = OK;

out:
  file_put(filep);
  return ret;
}

int linux_bt_sockif_l2cap_upstream_from_fd(int fd, FAR void **handle)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  FAR struct socket *psock;
  FAR struct file *filep;
  int ret;

  if (handle == NULL)
    {
      return -EINVAL;
    }

  *handle = NULL;
  ret = sockfd_socket(fd, &filep, &psock);
  if (ret < 0)
    {
      return ret;
    }

  if (psock == NULL || psock->s_conn == NULL ||
      psock->s_sockif != &g_linux_bt_sockif)
    {
      ret = -ENOTSOCK;
      goto out;
    }

  conn = psock->s_conn;
  if (conn->proto != BTPROTO_L2CAP || !conn->connected ||
      conn->l2cap_upstream == NULL)
    {
      ret = -EBADFD;
      goto out;
    }

  *handle = conn->l2cap_upstream;
  ret = OK;

out:
  file_put(filep);
  return ret;
}

#endif /* CONFIG_NET_LINUX_BLUETOOTH */
