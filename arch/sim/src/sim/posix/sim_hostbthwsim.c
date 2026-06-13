/****************************************************************************
 * arch/sim/src/sim/posix/sim_hostbthwsim.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/syscall.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sim_hostbthwsim.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BTHWSIM_MAGIC "BTHS"
#define BTHWSIM_MAGIC_U32 0x53485442u
#define BTHWSIM_VERSION 1
#define BTHWSIM_DST_BROADCAST 0xffff

#ifndef MSG_NOSIGNAL
#  define MSG_NOSIGNAL 0
#endif

struct bthwsim_record_s
{
  uint32_t magic;
  uint16_t version;
  uint16_t type;
  uint16_t src;
  uint16_t dst;
  uint32_t seq;
  uint64_t timestamp_us;
  uint32_t payload_len;
  uint32_t checksum;
};

static uint32_t g_bthwsim_seq;
static pthread_mutex_t g_bthwsim_raw_lock = PTHREAD_MUTEX_INITIALIZER;

struct host_bthwsim_h4_tcp_s
{
  int listen_fd;
  int client_fd;
  uint16_t port;
  uint8_t rx[4096];
  size_t rx_len;
  uint8_t tx[8192];
  size_t tx_len;
  pthread_t thread;
  int thread_started;
  int connect_mode;
};

static struct host_bthwsim_h4_tcp_s g_h4_tcp =
{
  -1, -1, 0, { 0 }, 0, { 0 }, 0, 0, 0, 0
};

static pthread_mutex_t g_h4_tcp_lock = PTHREAD_MUTEX_INITIALIZER;

static int host_bthwsim_h4_known(uint8_t type);
static size_t host_bthwsim_h4_len(const uint8_t *buf, size_t len);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int host_bthwsim_touch(const char *dir, const char *name)
{
  char path[512];
  int fd;
  int ret;

  ret = snprintf(path, sizeof(path), "%s/%s", dir, name);
  if (ret < 0 || (size_t)ret >= sizeof(path))
    {
      errno = ENAMETOOLONG;
      return -1;
    }

  fd = open(path, O_CREAT | O_RDWR | O_APPEND, 0666);
  if (fd < 0)
    {
      return -1;
    }

  close(fd);
  return 0;
}

static const char *host_bthwsim_file(uint16_t type)
{
  switch (type)
    {
      case 1:
        return "bt-hwsim-ctrl.bin";
      case 2:
        return "bt-hwsim-adv.bin";
      case 3:
        return "bt-hwsim-acl.bin";
      case 4:
        return "bt-hwsim-iso.bin";
      case 5:
        return "bt-hwsim-bnep.bin";
      default:
        return NULL;
    }
}

static uint32_t host_bthwsim_checksum(const uint8_t *data, uint32_t len)
{
  uint32_t hash = 2166136261u;
  uint32_t i;

  for (i = 0; i < len; i++)
    {
      hash ^= data[i];
      hash *= 16777619u;
    }

  return hash;
}

static uint64_t host_bthwsim_time_us(void)
{
  struct timespec ts;

  if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
    {
      return 0;
    }

  return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
}

static int host_bthwsim_path(char *path, size_t path_len,
                             const char *dir, uint16_t type)
{
  const char *name = host_bthwsim_file(type);
  int ret;

  if (name == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  ret = snprintf(path, path_len, "%s/%s", dir, name);
  if (ret < 0 || (size_t)ret >= path_len)
    {
      errno = ENAMETOOLONG;
      return -1;
    }

  return 0;
}

static int host_bthwsim_offset_path(char *path, size_t path_len,
                                    const char *dir, uint16_t type,
                                    uint16_t self)
{
  const char *name = host_bthwsim_file(type);
  int ret;

  if (name == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  ret = snprintf(path, path_len, "%s/%s.role%u.off", dir, name, self);
  if (ret < 0 || (size_t)ret >= path_len)
    {
      errno = ENAMETOOLONG;
      return -1;
    }

  return 0;
}

static int host_bthwsim_raw_offset_path_named(char *path, size_t path_len,
                                              const char *dir,
                                              uint16_t type,
                                              uint16_t self,
                                              const char *consumer)
{
  const char *name = host_bthwsim_file(type);
  int ret;

  if (name == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  if (consumer == NULL || consumer[0] == '\0')
    {
      consumer = "default";
    }

  ret = snprintf(path, path_len, "%s/%s.raw.%s.role%u.off", dir, name,
                 consumer, self);
  if (ret < 0 || (size_t)ret >= path_len)
    {
      errno = ENAMETOOLONG;
      return -1;
    }

  return 0;
}

static int host_bthwsim_raw_offset_path(char *path, size_t path_len,
                                        const char *dir, uint16_t type,
                                        uint16_t self)
{
  return host_bthwsim_raw_offset_path_named(path, path_len, dir, type,
                                            self, "default");
}

static int host_bthwsim_h4_path(char *path, size_t path_len,
                                const char *dir, const char *name)
{
  int ret;

  if (name == NULL || name[0] == '\0')
    {
      errno = EINVAL;
      return -1;
    }

  if (name[0] == '/')
    {
      ret = snprintf(path, path_len, "%s", name);
    }
  else
    {
      ret = snprintf(path, path_len, "%s/%s", dir, name);
    }

  if (ret < 0 || (size_t)ret >= path_len)
    {
      errno = ENAMETOOLONG;
      return -1;
    }

  return 0;
}

static int host_bthwsim_h4_offset_path(char *path, size_t path_len,
                                       const char *dir, uint16_t self)
{
  int ret;

  ret = snprintf(path, path_len, "%s/bt-h4-in.role%u.off", dir, self);
  if (ret < 0 || (size_t)ret >= path_len)
    {
      errno = ENAMETOOLONG;
      return -1;
    }

  return 0;
}

static int host_bthwsim_h4_is_tcp(const char *name, uint16_t *port)
{
  char *end = NULL;
  unsigned long value;

  if (name == NULL || strncmp(name, "tcp:", 4) != 0)
    {
      return 0;
    }

  value = strtoul(name + 4, &end, 0);
  if (end == name + 4 || value == 0 || value > 65535)
    {
      errno = EINVAL;
      return -1;
    }

  if (port != NULL)
    {
      *port = (uint16_t)value;
    }

  return 1;
}

static int host_bthwsim_h4_is_connect(const char *name, uint16_t *port)
{
  char *end = NULL;
  unsigned long value;

  if (name == NULL || strncmp(name, "connect:", 8) != 0)
    {
      return 0;
    }

  value = strtoul(name + 8, &end, 0);
  if (end == name + 8 || value == 0 || value > 65535)
    {
      errno = EINVAL;
      return -1;
    }

  if (port != NULL)
    {
      *port = (uint16_t)value;
    }

  return 1;
}

static int host_bthwsim_set_nonblock(int fd)
{
  int flags;

  flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    {
      return -errno;
    }

  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
      return -errno;
    }

  return 0;
}

static int host_bthwsim_socket(int domain, int type, int protocol)
{
  return (int)syscall(SYS_socket, domain, type, protocol);
}

static int host_bthwsim_bind(int fd, const struct sockaddr *addr,
                             socklen_t addrlen)
{
  return (int)syscall(SYS_bind, fd, addr, addrlen);
}

static int host_bthwsim_listen(int fd, int backlog)
{
  return (int)syscall(SYS_listen, fd, backlog);
}

static int host_bthwsim_accept(int fd)
{
  return (int)syscall(SYS_accept, fd, NULL, NULL);
}

static int host_bthwsim_connect(int fd, const struct sockaddr *addr,
                                socklen_t addrlen)
{
  return (int)syscall(SYS_connect, fd, addr, addrlen);
}

static void host_bthwsim_h4_tcp_close_client(void)
{
  pthread_mutex_lock(&g_h4_tcp_lock);
  if (g_h4_tcp.client_fd >= 0)
    {
      close(g_h4_tcp.client_fd);
      g_h4_tcp.client_fd = -1;
    }
  pthread_mutex_unlock(&g_h4_tcp_lock);
}

static void host_bthwsim_h4_tcp_reset(void)
{
  host_bthwsim_h4_tcp_close_client();

  if (g_h4_tcp.listen_fd >= 0)
    {
      close(g_h4_tcp.listen_fd);
      g_h4_tcp.listen_fd = -1;
    }

  g_h4_tcp.port = 0;
  g_h4_tcp.tx_len = 0;
  g_h4_tcp.connect_mode = 0;
}

static void host_bthwsim_h4_tcp_buffer_tx(const void *payload,
                                          uint32_t payload_len)
{
  const uint8_t *data = payload;
  size_t avail;
  size_t copy;

  if (payload == NULL || payload_len == 0)
    {
      return;
    }

  avail = sizeof(g_h4_tcp.tx) - g_h4_tcp.tx_len;
  copy = payload_len;
  if (copy > avail)
    {
      size_t drop = copy - avail;

      if (drop >= g_h4_tcp.tx_len)
        {
          g_h4_tcp.tx_len = 0;
        }
      else
        {
          memmove(g_h4_tcp.tx, &g_h4_tcp.tx[drop], g_h4_tcp.tx_len - drop);
          g_h4_tcp.tx_len -= drop;
        }

      avail = sizeof(g_h4_tcp.tx) - g_h4_tcp.tx_len;
      if (copy > avail)
        {
          copy = avail;
        }
    }

  if (copy > 0)
    {
      memcpy(&g_h4_tcp.tx[g_h4_tcp.tx_len], data, copy);
      g_h4_tcp.tx_len += copy;
      fprintf(stderr, "bthwsim: H4 TCP buffered tx bytes=%zu total=%zu\n",
              copy, g_h4_tcp.tx_len);
    }
}

static void host_bthwsim_h4_tcp_flush_tx_locked(void)
{
  ssize_t sent;

  if (g_h4_tcp.client_fd < 0 || g_h4_tcp.tx_len == 0)
    {
      return;
    }

  errno = 0;
  sent = write(g_h4_tcp.client_fd, g_h4_tcp.tx, g_h4_tcp.tx_len);
  if (sent > 0)
    {
      memmove(g_h4_tcp.tx, &g_h4_tcp.tx[sent], g_h4_tcp.tx_len - sent);
      g_h4_tcp.tx_len -= sent;
      fprintf(stderr, "bthwsim: H4 TCP flush bytes=%zd remaining=%zu\n",
              sent, g_h4_tcp.tx_len);
      return;
    }

  if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
      fprintf(stderr, "bthwsim: H4 TCP flush would block remaining=%zu\n",
              g_h4_tcp.tx_len);
      return;
    }

  fprintf(stderr, "bthwsim: H4 TCP flush failed sent=%zd errno=%d\n",
          sent, sent < 0 ? errno : 0);
  close(g_h4_tcp.client_fd);
  g_h4_tcp.client_fd = -1;
}

static void *host_bthwsim_h4_tcp_thread(void *arg)
{
  (void)arg;

  for (; ; )
    {
      int fd;

      fd = host_bthwsim_accept(g_h4_tcp.listen_fd);
      if (fd < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          usleep(10000);
          continue;
        }

      pthread_mutex_lock(&g_h4_tcp_lock);
      if (g_h4_tcp.client_fd >= 0)
        {
          close(g_h4_tcp.client_fd);
        }

      g_h4_tcp.client_fd = fd;
      g_h4_tcp.rx_len = 0;
      host_bthwsim_h4_tcp_flush_tx_locked();
      pthread_mutex_unlock(&g_h4_tcp_lock);

      fprintf(stderr, "bthwsim: H4 TCP client accepted port=%u\n",
              g_h4_tcp.port);

      for (; ; )
        {
          uint8_t tmp[512];
          ssize_t got;

          got = recv(fd, tmp, sizeof(tmp), 0);
          if (got > 0)
            {
              pthread_mutex_lock(&g_h4_tcp_lock);
              if (g_h4_tcp.client_fd == fd)
                {
                  size_t avail = sizeof(g_h4_tcp.rx) - g_h4_tcp.rx_len;
                  size_t copy = (size_t)got;

                  if (copy > avail)
                    {
                      copy = avail;
                    }

                  if (copy > 0)
                    {
                      memcpy(&g_h4_tcp.rx[g_h4_tcp.rx_len], tmp, copy);
                      g_h4_tcp.rx_len += copy;
                    }

                  fprintf(stderr,
                          "bthwsim: H4 TCP recv bytes=%zd buffered=%zu\n",
                          got, g_h4_tcp.rx_len);
                }

              pthread_mutex_unlock(&g_h4_tcp_lock);
              continue;
            }

          if (got == 0)
            {
              break;
            }

          if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
              usleep(10000);
              continue;
            }

          break;
        }

      pthread_mutex_lock(&g_h4_tcp_lock);
      if (g_h4_tcp.client_fd == fd)
        {
          g_h4_tcp.client_fd = -1;
        }

      pthread_mutex_unlock(&g_h4_tcp_lock);
      close(fd);
      fprintf(stderr, "bthwsim: H4 TCP client closed port=%u\n",
              g_h4_tcp.port);
    }

  return NULL;
}

static int host_bthwsim_h4_tcp_ensure(uint16_t port)
{
  struct sockaddr_in addr;
  int opt = 1;
  int ret;
  int fd;

  if (g_h4_tcp.listen_fd >= 0 && g_h4_tcp.port == port)
    {
      return 0;
    }

  host_bthwsim_h4_tcp_reset();

  fd = host_bthwsim_socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    {
      fprintf(stderr, "bthwsim: H4 TCP socket failed errno=%d\n", errno);
      return -errno;
    }

  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(port);

  if (host_bthwsim_bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      ret = -errno;
      fprintf(stderr, "bthwsim: H4 TCP bind failed port=%u errno=%d\n",
              port, errno);
      close(fd);
      return ret;
    }

  if (host_bthwsim_listen(fd, 1) < 0)
    {
      ret = -errno;
      fprintf(stderr, "bthwsim: H4 TCP listen failed port=%u errno=%d\n",
              port, errno);
      close(fd);
      return ret;
    }

  g_h4_tcp.listen_fd = fd;
  g_h4_tcp.port = port;
  fprintf(stderr, "bthwsim: H4 TCP listening on 127.0.0.1:%u\n", port);

  if (!g_h4_tcp.thread_started)
    {
      ret = pthread_create(&g_h4_tcp.thread, NULL,
                           host_bthwsim_h4_tcp_thread, NULL);
      if (ret != 0)
        {
          fprintf(stderr, "bthwsim: H4 TCP thread failed port=%u ret=%d\n",
                  port, ret);
          close(fd);
          g_h4_tcp.listen_fd = -1;
          g_h4_tcp.port = 0;
          return -ret;
        }

      pthread_detach(g_h4_tcp.thread);
      g_h4_tcp.thread_started = 1;
    }

  return 0;
}

static int host_bthwsim_h4_connect_ensure(uint16_t port)
{
  struct sockaddr_in addr;
  int ret;
  int fd;

  if (g_h4_tcp.connect_mode && g_h4_tcp.client_fd >= 0 &&
      g_h4_tcp.port == port)
    {
      return 0;
    }

  host_bthwsim_h4_tcp_reset();

  fd = host_bthwsim_socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    {
      return -errno;
    }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(port);

  if (host_bthwsim_connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      ret = -errno;
      close(fd);
      return ret;
    }

  ret = host_bthwsim_set_nonblock(fd);
  if (ret < 0)
    {
      close(fd);
      return ret;
    }

  pthread_mutex_lock(&g_h4_tcp_lock);
  g_h4_tcp.client_fd = fd;
  g_h4_tcp.port = port;
  g_h4_tcp.connect_mode = 1;
  g_h4_tcp.rx_len = 0;
  host_bthwsim_h4_tcp_flush_tx_locked();
  pthread_mutex_unlock(&g_h4_tcp_lock);

  fprintf(stderr, "bthwsim: H4 TCP connected to 127.0.0.1:%u\n", port);
  return 0;
}

static int host_bthwsim_h4_tcp_accept(void)
{
  struct timeval tv;
  fd_set rfds;
  int fd;
  int ret;

  if (g_h4_tcp.client_fd >= 0)
    {
      return 0;
    }

  FD_ZERO(&rfds);
  FD_SET(g_h4_tcp.listen_fd, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 20000;

  ret = select(g_h4_tcp.listen_fd + 1, &rfds, NULL, NULL, &tv);
  if (ret < 0)
    {
      return -errno;
    }

  if (ret == 0 || !FD_ISSET(g_h4_tcp.listen_fd, &rfds))
    {
      return 0;
    }

  fd = host_bthwsim_accept(g_h4_tcp.listen_fd);
  if (fd < 0)
    {
      return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : -errno;
    }

  ret = host_bthwsim_set_nonblock(fd);
  if (ret < 0)
    {
      close(fd);
      return ret;
    }

  pthread_mutex_lock(&g_h4_tcp_lock);
  if (g_h4_tcp.client_fd >= 0)
    {
      pthread_mutex_unlock(&g_h4_tcp_lock);
      close(fd);
      return 0;
    }

  g_h4_tcp.client_fd = fd;
  g_h4_tcp.rx_len = 0;
  pthread_mutex_unlock(&g_h4_tcp_lock);

  fprintf(stderr, "bthwsim: H4 TCP client accepted port=%u\n",
          g_h4_tcp.port);
  return 0;
}

static int host_bthwsim_h4_tcp_read(void *payload, uint32_t payload_len,
                                    uint32_t *out_len)
{
  ssize_t got;
  size_t frame_len;

  if (out_len != NULL)
    {
      *out_len = 0;
    }

  pthread_mutex_lock(&g_h4_tcp_lock);
  while (g_h4_tcp.connect_mode && g_h4_tcp.client_fd >= 0)
    {
      uint8_t tmp[512];

      got = recv(g_h4_tcp.client_fd, tmp, sizeof(tmp), 0);
      if (got > 0)
        {
          size_t avail = sizeof(g_h4_tcp.rx) - g_h4_tcp.rx_len;
          size_t copy = (size_t)got;

          if (copy > avail)
            {
              copy = avail;
            }

          if (copy > 0)
            {
              memcpy(&g_h4_tcp.rx[g_h4_tcp.rx_len], tmp, copy);
              g_h4_tcp.rx_len += copy;
            }

          fprintf(stderr, "bthwsim: H4 TCP recv bytes=%zd buffered=%zu\n",
                  got, g_h4_tcp.rx_len);
          continue;
        }

      if (got == 0)
        {
          close(g_h4_tcp.client_fd);
          g_h4_tcp.client_fd = -1;
          fprintf(stderr, "bthwsim: H4 TCP peer closed port=%u\n",
                  g_h4_tcp.port);
          break;
        }

      if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          break;
        }

      close(g_h4_tcp.client_fd);
      g_h4_tcp.client_fd = -1;
      break;
    }

  if (g_h4_tcp.client_fd < 0 && g_h4_tcp.rx_len == 0)
    {
      pthread_mutex_unlock(&g_h4_tcp_lock);
      return 0;
    }

  while (g_h4_tcp.rx_len > 0 &&
         !host_bthwsim_h4_known(g_h4_tcp.rx[0]))
    {
      memmove(g_h4_tcp.rx, &g_h4_tcp.rx[1], g_h4_tcp.rx_len - 1);
      g_h4_tcp.rx_len--;
    }

  frame_len = host_bthwsim_h4_len(g_h4_tcp.rx, g_h4_tcp.rx_len);
  if (frame_len == 0 || frame_len > g_h4_tcp.rx_len)
    {
      pthread_mutex_unlock(&g_h4_tcp_lock);
      return 0;
    }

  if (frame_len > payload_len)
    {
      pthread_mutex_unlock(&g_h4_tcp_lock);
      return -EMSGSIZE;
    }

  memcpy(payload, g_h4_tcp.rx, frame_len);
  memmove(g_h4_tcp.rx, &g_h4_tcp.rx[frame_len],
          g_h4_tcp.rx_len - frame_len);
  g_h4_tcp.rx_len -= frame_len;

  if (out_len != NULL)
    {
      *out_len = (uint32_t)frame_len;
    }

  pthread_mutex_unlock(&g_h4_tcp_lock);
  return 1;
}

static int host_bthwsim_h4_tcp_append(const void *payload,
                                      uint32_t payload_len)
{
  ssize_t sent;

  pthread_mutex_lock(&g_h4_tcp_lock);
  if (g_h4_tcp.client_fd < 0)
    {
      host_bthwsim_h4_tcp_buffer_tx(payload, payload_len);
      pthread_mutex_unlock(&g_h4_tcp_lock);
      return 0;
    }

  host_bthwsim_h4_tcp_flush_tx_locked();
  if (g_h4_tcp.client_fd < 0)
    {
      host_bthwsim_h4_tcp_buffer_tx(payload, payload_len);
      pthread_mutex_unlock(&g_h4_tcp_lock);
      return 0;
    }

  errno = 0;
  sent = write(g_h4_tcp.client_fd, payload, payload_len);
  pthread_mutex_unlock(&g_h4_tcp_lock);
  if (sent == (ssize_t)payload_len)
    {
      fprintf(stderr, "bthwsim: H4 TCP send bytes=%zd\n", sent);
      return 0;
    }

  if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
      return 0;
    }

  host_bthwsim_h4_tcp_close_client();
  return sent < 0 ? -errno : -EIO;
}

static off_t host_bthwsim_load_offset(const char *path)
{
  FILE *fp;
  long long value = 0;

  fp = fopen(path, "r");
  if (fp == NULL)
    {
      return 0;
    }

  if (fscanf(fp, "%lld", &value) != 1)
    {
      value = 0;
    }

  fclose(fp);
  return value < 0 ? 0 : (off_t)value;
}

static void host_bthwsim_save_offset(const char *path, off_t value)
{
  FILE *fp;

  fp = fopen(path, "w");
  if (fp == NULL)
    {
      return;
    }

  fprintf(fp, "%lld\n", (long long)value);
  fclose(fp);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int host_bthwsim_init(const char *medium_dir, int role_id,
                      const char *mode)
{
  static const char * const files[] =
  {
    "bt-hwsim-ctrl.bin",
    "bt-hwsim-adv.bin",
    "bt-hwsim-acl.bin",
    "bt-hwsim-iso.bin",
  };

  size_t i;

  if (mkdir(medium_dir, 0777) < 0 && errno != EEXIST)
    {
      return -errno;
    }

  for (i = 0; i < sizeof(files) / sizeof(files[0]); i++)
    {
      if (host_bthwsim_touch(medium_dir, files[i]) < 0)
        {
          return -errno;
        }
    }

  fprintf(stderr,
          "bthwsim: role=%d mode=%s medium=%s magic=%s files ready\n",
          role_id, mode, medium_dir, BTHWSIM_MAGIC);
  return 0;
}

static uint16_t host_bthwsim_get_le16(const uint8_t *p)
{
  return (uint16_t)(p[0] | (p[1] << 8));
}

static int host_bthwsim_h4_known(uint8_t type)
{
  return type == 0x01 || type == 0x02 || type == 0x03 ||
         type == 0x04 || type == 0x05 || type == 0xff;
}

static size_t host_bthwsim_h4_len(const uint8_t *buf, size_t len)
{
  uint16_t payload_len;

  if (buf == NULL || len == 0)
    {
      return 0;
    }

  switch (buf[0])
    {
      case 0x01:
        return len >= 4 ? (size_t)buf[3] + 4 : 0;

      case 0x02:
        return len >= 5 ? (size_t)host_bthwsim_get_le16(&buf[3]) + 5 : 0;

      case 0x03:
        return len >= 4 ? (size_t)buf[3] + 4 : 0;

      case 0x04:
        return len >= 3 ? (size_t)buf[2] + 3 : 0;

      case 0x05:
        if (len < 5)
          {
            return 0;
          }

        payload_len = (uint16_t)(host_bthwsim_get_le16(&buf[3]) & 0x3fff);
        return (size_t)payload_len + 5;

      case 0xff:
        return len >= 2 ? 2 : 0;

      default:
        return 1;
    }
}

int host_bthwsim_append(const char *medium_dir, uint16_t type,
                        uint16_t src, uint16_t dst,
                        const void *payload, uint32_t payload_len)
{
  struct bthwsim_record_s rec;
  char path[512];
  int fd;
  ssize_t written;

  if (host_bthwsim_path(path, sizeof(path), medium_dir, type) < 0)
    {
      return -errno;
    }

  fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0666);
  if (fd < 0)
    {
      return -errno;
    }

  memset(&rec, 0, sizeof(rec));
  rec.magic = BTHWSIM_MAGIC_U32;
  rec.version = BTHWSIM_VERSION;
  rec.type = type;
  rec.src = src;
  rec.dst = dst;
  rec.seq = ++g_bthwsim_seq;
  rec.timestamp_us = host_bthwsim_time_us();
  rec.payload_len = payload_len;
  rec.checksum = host_bthwsim_checksum(payload, payload_len);

  written = write(fd, &rec, sizeof(rec));
  if (written == (ssize_t)sizeof(rec) && payload_len > 0)
    {
      written = write(fd, payload, payload_len);
      if (written != (ssize_t)payload_len)
        {
          close(fd);
          return -EIO;
        }
    }

  close(fd);

  return written < 0 ? -errno : 0;
}

int host_bthwsim_read(const char *medium_dir, uint16_t type,
                      uint16_t self, char *out, size_t out_len)
{
  struct bthwsim_record_s rec;
  char path[512];
  char offset_path[512];
  char payload[256];
  size_t used = 0;
  int fd;
  int count = 0;
  off_t offset;

  if (out_len > 0)
    {
      out[0] = '\0';
    }

  if (host_bthwsim_path(path, sizeof(path), medium_dir, type) < 0)
    {
      return -errno;
    }

  if (host_bthwsim_offset_path(offset_path, sizeof(offset_path),
                               medium_dir, type, self) < 0)
    {
      return -errno;
    }

  fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      return errno == ENOENT ? 0 : -errno;
    }

  pthread_mutex_lock(&g_bthwsim_raw_lock);
  offset = host_bthwsim_load_offset(offset_path);
  if (offset > 0 && lseek(fd, offset, SEEK_SET) < 0)
    {
      pthread_mutex_unlock(&g_bthwsim_raw_lock);
      close(fd);
      return -errno;
    }

  while (read(fd, &rec, sizeof(rec)) == (ssize_t)sizeof(rec))
    {
      uint32_t payload_len = rec.payload_len;
      ssize_t got;

      if (payload_len >= sizeof(payload))
        {
          payload_len = sizeof(payload) - 1;
        }

      got = read(fd, payload, payload_len);
      if (got != (ssize_t)payload_len)
        {
          break;
        }

      if (rec.payload_len > payload_len)
        {
          if (lseek(fd, rec.payload_len - payload_len, SEEK_CUR) < 0)
            {
              break;
            }
        }

      payload[payload_len] = '\0';

      if (rec.magic != BTHWSIM_MAGIC_U32 ||
          rec.version != BTHWSIM_VERSION ||
          rec.type != type ||
          rec.src == self ||
          (rec.dst != self && rec.dst != BTHWSIM_DST_BROADCAST))
        {
          continue;
        }

      if (used < out_len)
        {
          int ret = snprintf(out + used, out_len - used,
                             "seq=%u src=%u dst=%u len=%u payload=%s\n",
                             rec.seq, rec.src, rec.dst,
                             rec.payload_len, payload);

          if (ret > 0)
            {
              used += (size_t)ret;
            }
        }

      count++;
    }

  offset = lseek(fd, 0, SEEK_CUR);
  if (offset >= 0)
    {
      host_bthwsim_save_offset(offset_path, offset);
    }

  close(fd);
  return count;
}

int host_bthwsim_read_raw(const char *medium_dir, uint16_t type,
                          uint16_t self, uint16_t *src, uint16_t *dst,
                          void *payload, uint32_t payload_len,
                          uint32_t *out_len)
{
  return host_bthwsim_read_raw_named(medium_dir, type, self, "default",
                                     src, dst, payload, payload_len,
                                     out_len);
}

int host_bthwsim_read_raw_named(const char *medium_dir, uint16_t type,
                                uint16_t self, const char *consumer,
                                uint16_t *src, uint16_t *dst,
                                void *payload, uint32_t payload_len,
                                uint32_t *out_len)
{
  struct bthwsim_record_s rec;
  char path[512];
  char offset_path[512];
  uint8_t scratch[256];
  int fd;
  off_t offset;
  ssize_t got;
  uint32_t read_len;
  int ret = 0;

  if (out_len != NULL)
    {
      *out_len = 0;
    }

  if (host_bthwsim_path(path, sizeof(path), medium_dir, type) < 0)
    {
      return -errno;
    }

  if (host_bthwsim_raw_offset_path_named(offset_path, sizeof(offset_path),
                                         medium_dir, type, self,
                                         consumer) < 0)
    {
      return -errno;
    }

  fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      return errno == ENOENT ? 0 : -errno;
    }

  offset = host_bthwsim_load_offset(offset_path);
  if (offset > 0 && lseek(fd, offset, SEEK_SET) < 0)
    {
      close(fd);
      return -errno;
    }

  while (read(fd, &rec, sizeof(rec)) == (ssize_t)sizeof(rec))
    {
      read_len = rec.payload_len;

      if (rec.magic != BTHWSIM_MAGIC_U32 ||
          rec.version != BTHWSIM_VERSION ||
          rec.type != type)
        {
          if (lseek(fd, read_len, SEEK_CUR) < 0)
            {
              ret = -errno;
              break;
            }

          continue;
        }

      if (rec.src == self ||
          (rec.dst != self && rec.dst != BTHWSIM_DST_BROADCAST))
        {
          if (lseek(fd, read_len, SEEK_CUR) < 0)
            {
              ret = -errno;
              break;
            }

          continue;
        }

      if (read_len > payload_len ||
          (payload == NULL && read_len > sizeof(scratch)))
        {
          ret = -EMSGSIZE;
          if (lseek(fd, read_len, SEEK_CUR) < 0)
            {
              ret = -errno;
            }

          break;
        }

      got = read(fd, payload != NULL ? payload : scratch, read_len);
      if (got != (ssize_t)read_len)
        {
          ret = got < 0 ? -errno : -EIO;
          break;
        }

      if (payload != NULL &&
          host_bthwsim_checksum(payload, read_len) != rec.checksum)
        {
          ret = -EBADMSG;
          break;
        }

      if (src != NULL)
        {
          *src = rec.src;
        }

      if (dst != NULL)
        {
          *dst = rec.dst;
        }

      if (out_len != NULL)
        {
          *out_len = read_len;
        }

      ret = 1;
      break;
    }

  offset = lseek(fd, 0, SEEK_CUR);
  if (offset >= 0)
    {
      host_bthwsim_save_offset(offset_path, offset);
    }

  pthread_mutex_unlock(&g_bthwsim_raw_lock);
  close(fd);
  return ret;
}

int host_bthwsim_h4_read(const char *medium_dir, const char *name,
                         uint16_t self, void *payload,
                         uint32_t payload_len, uint32_t *out_len)
{
  char path[512];
  char offset_path[512];
  uint8_t scratch[4096];
  uint16_t port;
  int fd;
  off_t offset;
  ssize_t got;
  size_t len;
  size_t pos = 0;
  int ret = 0;
  int is_tcp;
  int is_connect;

  if (out_len != NULL)
    {
      *out_len = 0;
    }

  if (payload == NULL || payload_len == 0)
    {
      return -EINVAL;
    }

  is_tcp = host_bthwsim_h4_is_tcp(name, &port);
  if (is_tcp < 0)
    {
      return -errno;
    }

  if (is_tcp > 0)
    {
      ret = host_bthwsim_h4_tcp_ensure(port);
      if (ret < 0)
        {
          return ret;
        }

      return host_bthwsim_h4_tcp_read(payload, payload_len, out_len);
    }

  is_connect = host_bthwsim_h4_is_connect(name, &port);
  if (is_connect < 0)
    {
      return -errno;
    }

  if (is_connect > 0)
    {
      ret = host_bthwsim_h4_connect_ensure(port);
      if (ret < 0)
        {
          return ret;
        }

      return host_bthwsim_h4_tcp_read(payload, payload_len, out_len);
    }

  if (host_bthwsim_h4_path(path, sizeof(path), medium_dir, name) < 0)
    {
      return -errno;
    }

  if (host_bthwsim_h4_offset_path(offset_path, sizeof(offset_path),
                                  medium_dir, self) < 0)
    {
      return -errno;
    }

  fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      return errno == ENOENT ? 0 : -errno;
    }

  offset = host_bthwsim_load_offset(offset_path);
  if (offset > 0 && lseek(fd, offset, SEEK_SET) < 0)
    {
      close(fd);
      return -errno;
    }

  got = read(fd, scratch, sizeof(scratch));
  if (got < 0)
    {
      ret = -errno;
      close(fd);
      return ret;
    }

  len = (size_t)got;
  while (pos < len)
    {
      size_t frame_len = host_bthwsim_h4_len(&scratch[pos], len - pos);

      if (frame_len == 0 || pos + frame_len > len)
        {
          break;
        }

      if (!host_bthwsim_h4_known(scratch[pos]))
        {
          pos++;
          continue;
        }

      if (frame_len > payload_len)
        {
          ret = -EMSGSIZE;
          break;
        }

      memcpy(payload, &scratch[pos], frame_len);
      if (out_len != NULL)
        {
          *out_len = (uint32_t)frame_len;
        }

      pos += frame_len;
      ret = 1;
      break;
    }

out:
  offset = lseek(fd, 0, SEEK_CUR);
  if (offset >= 0)
    {
      host_bthwsim_save_offset(offset_path, offset - (off_t)(len - pos));
    }

  close(fd);
  return ret;
}

int host_bthwsim_h4_append(const char *medium_dir, const char *name,
                           const void *payload, uint32_t payload_len)
{
  char path[512];
  uint16_t port;
  int fd;
  ssize_t written;
  int is_tcp;
  int is_connect;
  int ret;

  if (payload == NULL || payload_len == 0)
    {
      return -EINVAL;
    }

  is_tcp = host_bthwsim_h4_is_tcp(name, &port);
  if (is_tcp < 0)
    {
      return -errno;
    }

  if (is_tcp > 0)
    {
      ret = host_bthwsim_h4_tcp_ensure(port);
      if (ret < 0)
        {
          return ret;
        }

      return host_bthwsim_h4_tcp_append(payload, payload_len);
    }

  is_connect = host_bthwsim_h4_is_connect(name, &port);
  if (is_connect < 0)
    {
      return -errno;
    }

  if (is_connect > 0)
    {
      ret = host_bthwsim_h4_connect_ensure(port);
      if (ret < 0)
        {
          return ret;
        }

      return host_bthwsim_h4_tcp_append(payload, payload_len);
    }

  if (host_bthwsim_h4_path(path, sizeof(path), medium_dir, name) < 0)
    {
      return -errno;
    }

  fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0666);
  if (fd < 0)
    {
      return -errno;
    }

  written = write(fd, payload, payload_len);
  close(fd);

  return written == (ssize_t)payload_len ? 0 :
         written < 0 ? -errno : -EIO;
}

int host_bthwsim_conn_set(const char *medium_dir, uint16_t self,
                          uint16_t peer, uint16_t handle,
                          const char *state)
{
  char path[512];
  int fd;
  int ret;
  char line[96];

  ret = snprintf(path, sizeof(path), "%s/linux-bt-conn-%u-%u.state",
                 medium_dir, self, peer);
  if (ret < 0 || (size_t)ret >= sizeof(path))
    {
      return -ENAMETOOLONG;
    }

  fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0666);
  if (fd < 0)
    {
      return -errno;
    }

  ret = snprintf(line, sizeof(line), "peer=%u handle=%u state=%s\n",
                 peer, handle, state);
  if (ret > 0)
    {
      ret = write(fd, line, ret) < 0 ? -errno : 0;
    }

  close(fd);
  return ret < 0 ? ret : 0;
}

int host_bthwsim_conn_clear(const char *medium_dir, uint16_t self,
                            uint16_t peer)
{
  char path[512];
  int ret;

  ret = snprintf(path, sizeof(path), "%s/linux-bt-conn-%u-%u.state",
                 medium_dir, self, peer);
  if (ret < 0 || (size_t)ret >= sizeof(path))
    {
      return -ENAMETOOLONG;
    }

  if (unlink(path) < 0 && errno != ENOENT)
    {
      return -errno;
    }

  return 0;
}
