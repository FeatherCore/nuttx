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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <nuttx/kmalloc.h>
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

#ifndef BTPROTO_BNEP
#  define BTPROTO_BNEP 4
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

#define LINUX_BT_SOCKIF_DEFAULT_HANDLE 0x0040
#define LINUX_BT_SOCKIF_MGMT_MAX       512

#ifndef HCI_CHANNEL_RAW
#  define HCI_CHANNEL_RAW 0
#endif

#ifndef HCI_CHANNEL_USER
#  define HCI_CHANNEL_USER 1
#endif

#ifndef HCI_CHANNEL_CONTROL
#  define HCI_CHANNEL_CONTROL 3
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
  void *hci_raw;
  void *hci_upstream;
  void *l2cap_upstream;
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

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int linux_bt_sockif_setup(FAR struct socket *psock);
static sockcaps_t linux_bt_sockif_sockcaps(FAR struct socket *psock);
static void linux_bt_sockif_addref(FAR struct socket *psock);
static int linux_bt_sockif_bind(FAR struct socket *psock,
                                FAR const struct sockaddr *addr,
                                socklen_t addrlen);
static int linux_bt_sockif_connect(FAR struct socket *psock,
                                   FAR const struct sockaddr *addr,
                                   socklen_t addrlen);
static ssize_t linux_bt_sockif_sendmsg(FAR struct socket *psock,
                                       FAR const struct msghdr *msg,
                                       int flags);
static ssize_t linux_bt_sockif_recvmsg(FAR struct socket *psock,
                                       FAR struct msghdr *msg,
                                       int flags);
static int linux_bt_sockif_close(FAR struct socket *psock);
static int linux_bt_sockif_ioctl(FAR struct socket *psock, int cmd,
                                 unsigned long arg);
#ifdef CONFIG_NET_SOCKOPTS
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

const struct sock_intf_s g_linux_bt_sockif =
{
  linux_bt_sockif_setup,    /* si_setup */
  linux_bt_sockif_sockcaps, /* si_sockcaps */
  linux_bt_sockif_addref,   /* si_addref */
  linux_bt_sockif_bind,     /* si_bind */
  NULL,                     /* si_getsockname */
  NULL,                     /* si_getpeername */
  NULL,                     /* si_listen */
  linux_bt_sockif_connect,  /* si_connect */
  NULL,                     /* si_accept */
  NULL,                     /* si_poll */
  linux_bt_sockif_sendmsg,  /* si_sendmsg */
  linux_bt_sockif_recvmsg,  /* si_recvmsg */
  linux_bt_sockif_close,    /* si_close */
  linux_bt_sockif_ioctl,    /* si_ioctl */
  NULL,                     /* si_socketpair */
  NULL                      /* si_shutdown */
#ifdef CONFIG_NET_SOCKOPTS
  , linux_bt_sockif_getsockopt /* si_getsockopt */
  , linux_bt_sockif_setsockopt /* si_setsockopt */
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int linux_bt_sockif_setup(FAR struct socket *psock)
{
  FAR struct linux_bt_sockif_conn_s *conn;

  if (psock == NULL || psock->s_type != SOCK_RAW)
    {
      return -ESOCKTNOSUPPORT;
    }

  if (psock->s_proto != BTPROTO_L2CAP &&
      psock->s_proto != BTPROTO_BNEP &&
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

  if (conn->proto != BTPROTO_L2CAP)
    {
      return -EPFNOSUPPORT;
    }

  if (addrlen < sizeof(*l2addr) || addr->sa_family != AF_BLUETOOTH)
    {
      return -EINVAL;
    }

  l2addr = (FAR const struct linux_bt_sockif_sockaddr_l2_s *)addr;
  conn->psm = l2addr->l2_psm;
  conn->cid = l2addr->l2_cid;
  conn->handle = LINUX_BT_SOCKIF_DEFAULT_HANDLE;
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
  return OK;
}

static int linux_bt_sockif_connect(FAR struct socket *psock,
                                   FAR const struct sockaddr *addr,
                                   socklen_t addrlen)
{
  FAR struct linux_bt_sockif_conn_s *conn;
  FAR const struct linux_bt_sockif_sockaddr_l2_s *l2addr;
  char out[256];
  int ret;

  if (psock == NULL || psock->s_conn == NULL || addr == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
  if (conn->proto != BTPROTO_L2CAP)
    {
      return -EPFNOSUPPORT;
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
  if (conn->proto != BTPROTO_HCI)
    {
      return -EOPNOTSUPP;
    }

  if (!conn->bound ||
      (conn->hci_channel != HCI_CHANNEL_CONTROL &&
       conn->hci_channel != HCI_CHANNEL_RAW &&
       conn->hci_channel != HCI_CHANNEL_USER))
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
  if (conn->proto != BTPROTO_HCI)
    {
      return -EOPNOTSUPP;
    }

  if (!conn->bound ||
      (conn->hci_channel != HCI_CHANNEL_CONTROL &&
       conn->hci_channel != HCI_CHANNEL_RAW &&
       conn->hci_channel != HCI_CHANNEL_USER &&
       conn->hci_channel != HCI_CHANNEL_MONITOR))
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

#ifdef CONFIG_NET_SOCKOPTS
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

  if (psock == NULL || psock->s_conn == NULL)
    {
      return -EINVAL;
    }

  conn = psock->s_conn;
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
