/****************************************************************************
 * drivers/bluetooth/linux_bt_upstream_vhci.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/wireless/linux_bluetooth.h>

#include <linux/fs.h>
#include <linux/miscdevice.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_vhci_hwsim_bound;
static bool g_vhci_misc_registered;
static bool g_vhci_default_open;
static struct file g_vhci_default_file;
static struct inode g_vhci_default_inode;
static loff_t g_vhci_default_pos;
static struct miscdevice *g_vhci_miscdev;
static unsigned long g_vhci_medium_to_host;
static unsigned long g_vhci_host_to_medium;
static unsigned long g_vhci_read_empty;

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI
int linux_bt_compat_module_misc_register_vhci_miscdev(void);
struct hci_dev *linux_bt_compat_vhci_file_hdev(struct file *file);
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint16_t linux_bt_upstream_vhci_hwsim_type(uint8_t pkt_type)
{
  switch (pkt_type)
    {
      case LINUX_BT_HCI_ACL_PKT:
        return LINUX_BT_HWSIM_TYPE_ACL;

      case LINUX_BT_HCI_ISO_PKT:
        return LINUX_BT_HWSIM_TYPE_ISO;

      default:
        return 0;
    }
}

static uint16_t linux_bt_upstream_vhci_role(void)
{
#ifdef CONFIG_SIM_BTHWSIM_ROLE
  return CONFIG_SIM_BTHWSIM_ROLE;
#else
  return 0;
#endif
}

static void linux_bt_upstream_vhci_put_le16(uint8_t *p, uint16_t value)
{
  p[0] = (uint8_t)value;
  p[1] = (uint8_t)(value >> 8);
}

static uint16_t linux_bt_upstream_vhci_get_le16(const uint8_t *p)
{
  return (uint16_t)(p[0] | (p[1] << 8));
}

static uint16_t linux_bt_upstream_vhci_bredr_pair_dst(uint16_t handle)
{
  uint16_t pair;
  uint16_t low;
  uint16_t high;
  uint16_t role = linux_bt_upstream_vhci_role();

  if (handle < 0x0040)
    {
      return LINUX_BT_HWSIM_DST_BROADCAST;
    }

  pair = (uint16_t)(handle - 0x0040);
  low = (uint16_t)(pair / 16);
  high = (uint16_t)(pair % 16);

  if (low == role && high != 0)
    {
      return high;
    }

  if (high == role && low != 0)
    {
      return low;
    }

  return LINUX_BT_HWSIM_DST_BROADCAST;
}

static uint16_t linux_bt_upstream_vhci_medium_dst(uint8_t pkt_type,
                                                  const uint8_t *payload,
                                                  uint32_t payload_len)
{
  uint16_t handle;
  uint16_t peer;

  if (payload == NULL || payload_len < 2)
    {
      return LINUX_BT_HWSIM_DST_BROADCAST;
    }

  switch (pkt_type)
    {
      case LINUX_BT_HCI_ACL_PKT:
        handle = (uint16_t)(linux_bt_upstream_vhci_get_le16(payload) &
                            0x0fff);

        if (handle >= 0x0100)
          {
            peer = (uint16_t)(handle & 0x00ff);
            return peer != 0 ? peer : LINUX_BT_HWSIM_DST_BROADCAST;
          }

        peer = linux_bt_upstream_vhci_bredr_pair_dst(handle);
        if (peer != LINUX_BT_HWSIM_DST_BROADCAST)
          {
            return peer;
          }

        if (handle >= 0x0050)
          {
            peer = (uint16_t)(handle - 0x0050);
            if (peer > 0 && peer < 16)
              {
                return peer;
              }
          }

        return LINUX_BT_HWSIM_DST_BROADCAST;

      default:
        return LINUX_BT_HWSIM_DST_BROADCAST;
    }
}

static void linux_bt_upstream_vhci_addr_from_role(uint16_t role,
                                                  uint8_t *addr)
{
  addr[0] = (uint8_t)role;
  addr[1] = 0x00;
  addr[2] = 0x00;
  addr[3] = 0x00;
  addr[4] = 0xfe;
  addr[5] = 0x02;
}

static uint16_t linux_bt_upstream_vhci_handle(uint16_t src,
                                              const char *transport)
{
  if (transport != NULL && !strcmp(transport, "le"))
    {
      return (uint16_t)(0x0100 + src);
    }

  return (uint16_t)(0x0050 + src);
}

static int linux_bt_upstream_vhci_inject_conn_complete(uint16_t src,
                                                       uint16_t handle,
                                                       const char *transport)
{
  uint8_t evt[32];
  uint8_t addr[6];

  linux_bt_upstream_vhci_addr_from_role(src, addr);

  if (transport != NULL && !strcmp(transport, "le"))
    {
      memset(evt, 0, sizeof(evt));
      evt[0] = 0x3e;
      evt[1] = 19;
      evt[2] = 0x01;
      evt[3] = 0x00;
      linux_bt_upstream_vhci_put_le16(&evt[4], handle);
      evt[6] = 0x01;
      evt[7] = 0x00;
      memcpy(&evt[8], addr, sizeof(addr));
      linux_bt_upstream_vhci_put_le16(&evt[14], 0x0018);
      linux_bt_upstream_vhci_put_le16(&evt[16], 0x0000);
      linux_bt_upstream_vhci_put_le16(&evt[18], 0x01f4);
      evt[20] = 0x00;

      return linux_bt_upstream_vhci_write_default(LINUX_BT_HCI_EVENT_PKT,
                                                  evt, 21);
    }

  memset(evt, 0, sizeof(evt));
  evt[0] = 0x03;
  evt[1] = 11;
  evt[2] = 0x00;
  linux_bt_upstream_vhci_put_le16(&evt[3], handle);
  memcpy(&evt[5], addr, sizeof(addr));
  evt[11] = 0x01;
  evt[12] = 0x00;

  return linux_bt_upstream_vhci_write_default(LINUX_BT_HCI_EVENT_PKT,
                                              evt, 13);
}

static int linux_bt_upstream_vhci_inject_disconn_complete(uint16_t handle,
                                                          uint8_t reason)
{
  uint8_t evt[6];

  evt[0] = 0x05;
  evt[1] = 4;
  evt[2] = 0x00;
  linux_bt_upstream_vhci_put_le16(&evt[3], handle);
  evt[5] = reason;

  return linux_bt_upstream_vhci_write_default(LINUX_BT_HCI_EVENT_PKT,
                                              evt, 6);
}

static int linux_bt_upstream_vhci_poll_ctrl_medium(void)
{
  char payload[256];
  uint16_t src;
  uint16_t dst;
  uint32_t len;
  int total = 0;
  int read_ret;

  do
    {
      read_ret = linux_bt_hwsim_read_raw(LINUX_BT_HWSIM_TYPE_CTRL,
                                         &src, &dst, payload,
                                         sizeof(payload) - 1, &len);
      if (read_ret < 0)
        {
          return read_ret;
        }

      if (read_ret > 0)
        {
          uint16_t msg_src = src;
          uint16_t msg_dst = dst;
          unsigned int parsed_handle = 0;
          unsigned int parsed_reason = 0x13;
          char transport[16] = "bredr";
          int parsed;
          int ret;

          payload[len] = '\0';
          parsed = sscanf(payload,
                          "HCI_CMD_CONNECT src=%hu dst=%hu transport=%15s handle=%x",
                          &msg_src, &msg_dst, transport, &parsed_handle);
          if (parsed >= 3 && msg_dst == linux_bt_upstream_vhci_role())
            {
              if (parsed < 4)
                {
                  parsed_handle =
                    linux_bt_upstream_vhci_handle(msg_src, transport);
                }

              ret = linux_bt_upstream_vhci_inject_conn_complete(
                msg_src, (uint16_t)parsed_handle, transport);
              if (ret < 0)
                {
                  return ret;
                }

              total++;
            }

          parsed = sscanf(payload,
                          "HCI_CMD_DISCONNECT src=%hu dst=%hu transport=%15s handle=%x reason=%x",
                          &msg_src, &msg_dst, transport, &parsed_handle,
                          &parsed_reason);
          if (parsed >= 4 && msg_dst == linux_bt_upstream_vhci_role())
            {
              if (transport[0] != '\0' && !strcmp(transport, "le"))
                {
                  ret = linux_bt_upstream_vhci_inject_disconn_complete(
                    (uint16_t)parsed_handle, (uint8_t)parsed_reason);
                  if (ret < 0)
                    {
                      return ret;
                    }

                  total++;
                }
            }
        }
    }
  while (read_ret > 0);

  return total;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int linux_bt_upstream_vhci_status(char *out, size_t out_len)
{
  char af[4096];
  char hci[2048];
  int af_ret;
  int hci_ret;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  af_ret = linux_bt_upstream_af_status(af, sizeof(af));
  if (af_ret < 0)
    {
      af[0] = '\0';
    }

  hci_ret = linux_bt_upstream_hci_status(hci, sizeof(hci));
  if (hci_ret < 0)
    {
      hci[0] = '\0';
    }

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI
  snprintf(out, out_len,
           "upstream-vhci: source=drivers/bluetooth/hci_vhci.c "
           "selected=1 autostart=%u bound=%u misc=%u open=%u "
           "m2h=%lu h2m=%lu read-empty=%lu "
           "compat=hci-dev-wrapper\n%s%s",
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI_AUTOSTART
           1,
#else
           0,
#endif
           g_vhci_hwsim_bound ? 1 : 0,
           g_vhci_misc_registered ? 1 : 0,
           g_vhci_default_open ? 1 : 0,
           g_vhci_medium_to_host, g_vhci_host_to_medium,
           g_vhci_read_empty, af, hci);
#else
  snprintf(out, out_len,
           "upstream-vhci: source=drivers/bluetooth/hci_vhci.c "
           "selected=0 autostart=0 bound=%u misc=%u open=%u "
           "m2h=%lu h2m=%lu read-empty=%lu "
           "compat=hci-dev-wrapper\n%s%s",
           g_vhci_hwsim_bound ? 1 : 0,
           g_vhci_misc_registered ? 1 : 0,
           g_vhci_default_open ? 1 : 0,
           g_vhci_medium_to_host, g_vhci_host_to_medium,
           g_vhci_read_empty, af, hci);
#endif

  return 0;
}

int misc_register(struct miscdevice *misc)
{
  if (misc == NULL || misc->name == NULL || misc->fops == NULL)
    {
      return -EINVAL;
    }

  if (!strcmp(misc->name, "vhci"))
    {
      g_vhci_miscdev = misc;
      g_vhci_misc_registered = true;
      return 0;
    }

  return -ENODEV;
}

void misc_deregister(struct miscdevice *misc)
{
  if (misc != NULL && misc == g_vhci_miscdev)
    {
      g_vhci_miscdev = NULL;
      g_vhci_misc_registered = false;
    }
}

int linux_bt_upstream_vhci_bind_hwsim(void)
{
  /* Porting boundary:
   *
   * - Keep upstream/drivers_bluetooth/hci_vhci.c as the authoritative Linux
   *   virtual HCI device implementation.
   * - Add Linux kernel compat only where NuttX lacks the required Unix/kernel
   *   primitive.
   * - Bind the upstream HCI recv boundary to SIM_BTHWSIM records while the
   *   full upstream hci_core.c event and socket wakeup machinery is still
   *   being ported.
   *
   * This function intentionally does not reimplement HCI, mgmt, L2CAP, SMP,
   * ATT/GATT or ISO behavior.  Those paths should come from the imported
   * Linux net/bluetooth sources.
   */

  g_vhci_hwsim_bound = true;
  return 0;
}

int linux_bt_upstream_vhci_open_default(void)
{
  int ret;

#ifndef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI
  return -ENODEV;
#else
  if (!g_vhci_misc_registered)
    {
      ret = linux_bt_compat_module_misc_register_vhci_miscdev();
      if (ret < 0)
        {
          return ret;
        }
    }

  if (g_vhci_default_open)
    {
      return -EALREADY;
    }

  if (g_vhci_miscdev == NULL || g_vhci_miscdev->fops == NULL ||
      g_vhci_miscdev->fops->open == NULL)
    {
      return -ENODEV;
    }

  memset(&g_vhci_default_file, 0, sizeof(g_vhci_default_file));
  memset(&g_vhci_default_inode, 0, sizeof(g_vhci_default_inode));

  ret = g_vhci_miscdev->fops->open(&g_vhci_default_inode,
                                   &g_vhci_default_file);
  if (ret < 0)
    {
      return ret;
    }

  g_vhci_default_open = true;

  /* Linux hci_vhci.c normally creates a default virtual HCI device from its
   * delayed open timeout when userspace does not write an explicit vendor
   * create command.  The NuttX compat delayed-work shim does not run real
   * timers, so create the default hci0 device explicitly at the autostart
   * boundary.
   */

  {
    uint8_t opcode = 0;

    ret = linux_bt_upstream_vhci_write_default(LINUX_BT_HCI_VENDOR_PKT,
                                               &opcode, 1);
    if (ret < 0)
      {
        linux_bt_upstream_vhci_close_default();
        return ret;
      }
  }

  return linux_bt_upstream_vhci_bind_hwsim();
#endif
}

int linux_bt_upstream_vhci_close_default(void)
{
  int ret = 0;

  if (!g_vhci_default_open)
    {
      return -ENODEV;
    }

  if (g_vhci_miscdev != NULL && g_vhci_miscdev->fops != NULL &&
      g_vhci_miscdev->fops->release != NULL)
    {
      ret = g_vhci_miscdev->fops->release(&g_vhci_default_inode,
                                           &g_vhci_default_file);
    }

  memset(&g_vhci_default_file, 0, sizeof(g_vhci_default_file));
  memset(&g_vhci_default_inode, 0, sizeof(g_vhci_default_inode));
  g_vhci_default_pos = 0;
  g_vhci_default_open = false;
  return ret;
}

int linux_bt_upstream_vhci_create_default(uint8_t opcode)
{
  int ret;

  if (!g_vhci_default_open)
    {
      ret = linux_bt_upstream_vhci_open_default();
      if (ret < 0)
        {
          return ret;
        }
    }

  return linux_bt_upstream_vhci_write_default(LINUX_BT_HCI_VENDOR_PKT,
                                              &opcode, 1);
}

int linux_bt_upstream_vhci_write_default(uint8_t pkt_type,
                                         const void *payload,
                                         size_t payload_len)
{
  uint8_t h4[1025];
  struct iov_iter iter;
  struct kiocb iocb;
  int ret;

  if (!g_vhci_default_open)
    {
      return -ENODEV;
    }

  if (payload == NULL && payload_len > 0)
    {
      return -EINVAL;
    }

  if (payload_len + 1 > sizeof(h4))
    {
      return -EMSGSIZE;
    }

  if (g_vhci_miscdev == NULL || g_vhci_miscdev->fops == NULL ||
      g_vhci_miscdev->fops->write_iter == NULL)
    {
      return -ENODEV;
    }

  h4[0] = pkt_type;
  if (payload_len > 0)
    {
      memcpy(&h4[1], payload, payload_len);
    }

  iter.data = (const char *)h4;
  iter.count = payload_len + 1;
  iocb.ki_filp = &g_vhci_default_file;

  ret = g_vhci_miscdev->fops->write_iter(&iocb, &iter);
  if (ret >= 0)
    {
      g_vhci_medium_to_host++;
    }

  return ret;
}

int linux_bt_upstream_vhci_read_default(void *payload,
                                        uint32_t payload_len,
                                        uint32_t *out_len)
{
  ssize_t ret;

  if (out_len != NULL)
    {
      *out_len = 0;
    }

  if (!g_vhci_default_open)
    {
      return -ENODEV;
    }

  if (payload == NULL || payload_len == 0)
    {
      return -EINVAL;
    }

  if (g_vhci_miscdev == NULL || g_vhci_miscdev->fops == NULL ||
      g_vhci_miscdev->fops->read == NULL)
    {
      return -ENODEV;
    }

  ret = g_vhci_miscdev->fops->read(&g_vhci_default_file, payload,
                                   payload_len, &g_vhci_default_pos);
  if (ret == -EAGAIN)
    {
      g_vhci_read_empty++;
      return 0;
    }

  if (ret < 0)
    {
      return (int)ret;
    }

  if (out_len != NULL)
    {
      *out_len = (uint32_t)ret;
    }

  return ret > 0 ? 1 : 0;
}

int linux_bt_upstream_vhci_drain_default(void)
{
  uint8_t h4[1025];
  uint32_t len;
  uint16_t type;
  int total = 0;
  int ret;
  int read_ret;

  do
    {
      read_ret = linux_bt_upstream_vhci_read_default(h4, sizeof(h4), &len);
      if (read_ret < 0)
        {
          return read_ret;
        }

      if (read_ret > 0 && len > 0)
        {
          uint16_t dst;

          type = linux_bt_upstream_vhci_hwsim_type(h4[0]);
          if (type == 0)
            {
              continue;
            }

          dst = linux_bt_upstream_vhci_medium_dst(h4[0], &h4[1], len - 1);
          ret = linux_bt_hwsim_send(type, dst, &h4[1], len - 1);
          if (ret < 0)
            {
              return ret;
            }

          g_vhci_host_to_medium++;
          total++;
        }
    }
  while (read_ret > 0);

  return total;
}

int linux_bt_upstream_vhci_drain_default_trace(size_t max_records,
                                               char *out,
                                               size_t out_len)
{
  return linux_bt_upstream_vhci_drain_default_trace_tee(max_records, NULL,
                                                        out, out_len);
}

int linux_bt_upstream_vhci_drain_default_trace_tee(size_t max_records,
                                                   const char *h4_out,
                                                   char *out,
                                                   size_t out_len)
{
  uint8_t h4[1025];
  uint32_t len;
  uint16_t dst;
  uint16_t type;
  size_t used = 0;
  size_t hex_len;
  int seen = 0;
  int total = 0;
  int ret;
  int read_ret;
  unsigned int i;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  out[0] = '\0';
  if (max_records == 0)
    {
      max_records = 32;
    }

  do
    {
      read_ret = linux_bt_upstream_vhci_read_default(h4, sizeof(h4), &len);
      if (read_ret < 0)
        {
          return read_ret;
        }

      if (read_ret > 0 && len > 0)
        {
          if (h4_out != NULL &&
              (h4[0] == 0x01 || h4[0] == 0x02 || h4[0] == 0x03 ||
               h4[0] == 0x04 || h4[0] == 0x05))
            {
              ret = linux_bt_hwsim_h4_append(h4_out, h4, len);
              if (ret < 0)
                {
                  return ret;
                }
            }

          type = linux_bt_upstream_vhci_hwsim_type(h4[0]);
          if (type == 0)
            {
              ret = snprintf(out + used, out_len - used,
                             "vhci-drain-trace[%d]: hci=0x%02x "
                             "unsupported len=%u\n",
                             seen, h4[0], (unsigned int)len);
              if (ret > 0 && (size_t)ret < out_len - used)
                {
                  used += ret;
                }

              seen++;
              continue;
            }

          dst = linux_bt_upstream_vhci_medium_dst(h4[0], &h4[1], len - 1);
          ret = linux_bt_hwsim_send(type, dst, &h4[1], len - 1);
          if (ret < 0)
            {
              return ret;
            }

          g_vhci_host_to_medium++;
          hex_len = len - 1;
          if (hex_len > 24)
            {
              hex_len = 24;
            }

          ret = snprintf(out + used, out_len - used,
                         "vhci-drain-trace[%d]: hci=0x%02x hwsim=%u "
                         "dst=0x%04x len=%u payload=",
                         seen, h4[0], type, dst,
                         (unsigned int)(len - 1));
          if (ret > 0 && (size_t)ret < out_len - used)
            {
              used += ret;
            }

          for (i = 0; i < hex_len && used + 4 < out_len; i++)
            {
              ret = snprintf(out + used, out_len - used, "%s%02x",
                             i == 0 ? "" : " ", h4[i + 1]);
              if (ret > 0 && (size_t)ret < out_len - used)
                {
                  used += ret;
                }
            }

          if (hex_len < len - 1 && used + 5 < out_len)
            {
              ret = snprintf(out + used, out_len - used, " ...");
              if (ret > 0 && (size_t)ret < out_len - used)
                {
                  used += ret;
                }
            }

          if (used + 2 < out_len)
            {
              out[used++] = '\n';
              out[used] = '\0';
            }

          total++;
          seen++;
        }
    }
  while (read_ret > 0 && (size_t)seen < max_records);

  if (seen == 0)
    {
      snprintf(out, out_len, "vhci-drain-trace: no-records\n");
    }

  return total;
}

struct hci_dev *linux_bt_upstream_vhci_default_hdev(void)
{
#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI
  if (!g_vhci_default_open)
    {
      return NULL;
    }

  return linux_bt_compat_vhci_file_hdev(&g_vhci_default_file);
#else
  return NULL;
#endif
}

int linux_bt_upstream_vhci_recv_frame(uint8_t pkt_type,
                                      const void *payload,
                                      size_t payload_len)
{
  uint16_t type;

  if (!g_vhci_hwsim_bound)
    {
      linux_bt_upstream_vhci_bind_hwsim();
    }

  type = linux_bt_upstream_vhci_hwsim_type(pkt_type);
  if (type == 0)
    {
      return -EOPNOTSUPP;
    }

  return linux_bt_hwsim_send(type, LINUX_BT_HWSIM_DST_BROADCAST,
                             payload, payload_len);
}

int linux_bt_upstream_vhci_poll_medium(void)
{
  struct
  {
    uint16_t hwsim_type;
    uint8_t hci_type;
  } map[] =
  {
    { LINUX_BT_HWSIM_TYPE_ACL,  LINUX_BT_HCI_ACL_PKT },
    { LINUX_BT_HWSIM_TYPE_ISO,  LINUX_BT_HCI_ISO_PKT },
  };
  uint8_t payload[1024];
  uint16_t src;
  uint16_t dst;
  uint32_t len;
  int total = 0;
  int ret;
  int read_ret;
  unsigned int i;

  ret = linux_bt_upstream_vhci_poll_ctrl_medium();
  if (ret < 0)
    {
      return ret;
    }

  total += ret;

  for (i = 0; i < sizeof(map) / sizeof(map[0]); i++)
    {
      do
        {
          read_ret = linux_bt_hwsim_read_raw_named(map[i].hwsim_type,
                                                   "vhci", &src, &dst,
                                                   payload,
                                                   sizeof(payload), &len);
          if (read_ret < 0)
            {
              return read_ret;
            }

          if (read_ret > 0)
            {
              ret = linux_bt_upstream_hci_recv_medium(map[i].hci_type,
                                                      payload, len);
              if (ret < 0)
                {
                  return ret;
                }

              total++;
            }
        }
      while (read_ret > 0);
    }

  return total;
}
