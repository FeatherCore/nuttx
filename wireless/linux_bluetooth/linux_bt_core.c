/****************************************************************************
 * wireless/linux_bluetooth/linux_bt_core.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/wireless/linux_bluetooth.h>

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#ifdef CONFIG_SIM_BTHWSIM
int sim_bthwsim_send(uint16_t type, uint16_t dst, const void *payload,
                     uint32_t payload_len);
int sim_bthwsim_read(uint16_t type, char *out, size_t out_len);
int sim_bthwsim_read_raw(uint16_t type, uint16_t *src, uint16_t *dst,
                         void *payload, uint32_t payload_len,
                         uint32_t *out_len);
int sim_bthwsim_read_raw_named(uint16_t type, const char *consumer,
                               uint16_t *src, uint16_t *dst,
                               void *payload, uint32_t payload_len,
                               uint32_t *out_len);
int sim_bthwsim_h4_read(const char *path, void *payload,
                        uint32_t payload_len, uint32_t *out_len);
int sim_bthwsim_h4_append(const char *path, const void *payload,
                          uint32_t payload_len);
int sim_bthwsim_conn_set(uint16_t peer, uint16_t handle,
                         const char *state);
int sim_bthwsim_conn_clear(uint16_t peer);
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_NET_LINUX_BLUETOOTH_MAX_CONN
#  define LINUX_BT_MAX_CONN CONFIG_NET_LINUX_BLUETOOTH_MAX_CONN
#else
#  define LINUX_BT_MAX_CONN 4
#endif

#define LINUX_BT_MAX_L2CAP_CHAN (LINUX_BT_MAX_CONN * 4)
#define LINUX_BT_ATT_VALUE_LEN  48
#define LINUX_BT_EVENT_LOG_LEN  1024
#define LINUX_BT_MAX_ISO_PATHS  8

#define LINUX_BT_CONN_CONNECTING 1
#define LINUX_BT_CONN_CONNECTED  2

#define LINUX_BT_L2CAP_CONNECTING    1
#define LINUX_BT_L2CAP_CONFIG        2
#define LINUX_BT_L2CAP_OPEN          3
#define LINUX_BT_L2CAP_DISCONNECTING 4
#define LINUX_BT_L2CAP_PSM_IPSP      0x0023

#define LINUX_BT_ISO_SOURCE     1
#define LINUX_BT_ISO_SINK       2

#define LINUX_BT_ISO_CREATING   1
#define LINUX_BT_ISO_STREAMING  2
#define LINUX_BT_ISO_SYNCING    3
#define LINUX_BT_ISO_SYNCED     4

#define LINUX_BT_MGMT_SETTING_POWERED      (1u << 0)
#define LINUX_BT_MGMT_SETTING_CONNECTABLE  (1u << 1)
#define LINUX_BT_MGMT_SETTING_DISCOVERABLE (1u << 3)
#define LINUX_BT_MGMT_SETTING_BONDABLE     (1u << 4)
#define LINUX_BT_MGMT_SETTING_BREDR        (1u << 9)
#define LINUX_BT_MGMT_SETTING_LE           (1u << 13)
#define LINUX_BT_MGMT_SETTING_ADVERTISING  (1u << 14)

struct linux_bt_conn
{
  uint16_t peer;
  uint16_t handle;
  uint8_t state;
  uint8_t transport;
  uint8_t paired;
  uint8_t used;
};

struct linux_bt_l2cap_chan
{
  uint16_t peer;
  uint16_t handle;
  uint16_t cid;
  uint16_t psm;
  uint8_t ident;
  uint8_t state;
  uint8_t used;
};

struct linux_bt_att_attr
{
  uint16_t handle;
  const char *uuid;
  char value[LINUX_BT_ATT_VALUE_LEN];
  uint8_t readable;
  uint8_t writable;
};

struct linux_bt_iso_path
{
  uint16_t handle;
  uint32_t seq;
  uint8_t big;
  uint8_t bis;
  uint8_t direction;
  uint8_t state;
  uint8_t used;
  const char *codec;
};

struct linux_bt_mgmt_controller
{
  uint16_t index;
  uint32_t settings;
  uint32_t supported_settings;
};

struct linux_bt_mgmt_op
{
  uint16_t opcode;
  const char *name;
  int (*handler)(uint8_t param, char *out, size_t out_len);
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct linux_bt_conn g_linux_bt_conns[LINUX_BT_MAX_CONN];
static struct linux_bt_l2cap_chan g_linux_bt_l2cap[LINUX_BT_MAX_L2CAP_CHAN];
static struct linux_bt_iso_path g_linux_bt_iso_paths[LINUX_BT_MAX_ISO_PATHS];
static struct linux_bt_mgmt_controller g_linux_bt_mgmt =
{
  .index = 0,
  .settings = LINUX_BT_MGMT_SETTING_CONNECTABLE |
              LINUX_BT_MGMT_SETTING_BONDABLE |
              LINUX_BT_MGMT_SETTING_BREDR |
              LINUX_BT_MGMT_SETTING_LE,
  .supported_settings = LINUX_BT_MGMT_SETTING_POWERED |
                        LINUX_BT_MGMT_SETTING_CONNECTABLE |
                        LINUX_BT_MGMT_SETTING_DISCOVERABLE |
                        LINUX_BT_MGMT_SETTING_BONDABLE |
                        LINUX_BT_MGMT_SETTING_BREDR |
                        LINUX_BT_MGMT_SETTING_LE |
                        LINUX_BT_MGMT_SETTING_ADVERTISING,
};
static char g_linux_bt_events[LINUX_BT_EVENT_LOG_LEN];
static size_t g_linux_bt_events_used;
static uint16_t g_linux_bt_next_dynamic_cid = 0x0040;
static uint8_t g_linux_bt_next_ident = 1;

static struct linux_bt_att_attr g_linux_bt_att[] =
{
  {
    .handle = 0x0001,
    .uuid = "00002a00-0000-1000-8000-00805f9b34fb",
    .value = "linux-bt-hwsim",
    .readable = 1,
    .writable = 1,
  },
  {
    .handle = 0x0002,
    .uuid = "00002a01-0000-1000-8000-00805f9b34fb",
    .value = "0000",
    .readable = 1,
    .writable = 0,
  },
  {
    .handle = 0x0025,
    .uuid = "00002b7d-0000-1000-8000-00805f9b34fb",
    .value = "le-audio-ready",
    .readable = 1,
    .writable = 1,
  },
};

static int linux_bt_mgmt_dispatch_read_info(uint8_t param,
                                            char *out, size_t out_len);
static int linux_bt_mgmt_dispatch_powered(uint8_t param,
                                          char *out, size_t out_len);
static int linux_bt_mgmt_dispatch_connectable(uint8_t param,
                                              char *out, size_t out_len);
static int linux_bt_mgmt_dispatch_discoverable(uint8_t param,
                                               char *out, size_t out_len);
static int linux_bt_mgmt_dispatch_bondable(uint8_t param,
                                           char *out, size_t out_len);
static int linux_bt_mgmt_dispatch_le(uint8_t param,
                                     char *out, size_t out_len);
static int linux_bt_mgmt_dispatch_bredr(uint8_t param,
                                        char *out, size_t out_len);
static int linux_bt_mgmt_dispatch_advertising(uint8_t param,
                                              char *out, size_t out_len);

static const struct linux_bt_mgmt_op g_linux_bt_mgmt_ops[] =
{
  {
    .opcode = LINUX_BT_MGMT_OP_READ_INFO,
    .name = "Read Controller Information",
    .handler = linux_bt_mgmt_dispatch_read_info,
  },
  {
    .opcode = LINUX_BT_MGMT_OP_SET_POWERED,
    .name = "Set Powered",
    .handler = linux_bt_mgmt_dispatch_powered,
  },
  {
    .opcode = LINUX_BT_MGMT_OP_SET_DISCOVERABLE,
    .name = "Set Discoverable",
    .handler = linux_bt_mgmt_dispatch_discoverable,
  },
  {
    .opcode = LINUX_BT_MGMT_OP_SET_CONNECTABLE,
    .name = "Set Connectable",
    .handler = linux_bt_mgmt_dispatch_connectable,
  },
  {
    .opcode = LINUX_BT_MGMT_OP_SET_BONDABLE,
    .name = "Set Bondable",
    .handler = linux_bt_mgmt_dispatch_bondable,
  },
  {
    .opcode = LINUX_BT_MGMT_OP_SET_LE,
    .name = "Set LE",
    .handler = linux_bt_mgmt_dispatch_le,
  },
  {
    .opcode = LINUX_BT_MGMT_OP_SET_ADVERTISING,
    .name = "Set Advertising",
    .handler = linux_bt_mgmt_dispatch_advertising,
  },
  {
    .opcode = LINUX_BT_MGMT_OP_SET_BREDR,
    .name = "Set BR/EDR",
    .handler = linux_bt_mgmt_dispatch_bredr,
  },
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static struct linux_bt_l2cap_chan *linux_bt_l2cap_open(uint16_t peer,
                                                       uint16_t cid,
                                                       uint16_t psm);
static void linux_bt_event_push(const char *fmt, ...);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

void linux_bt_port_note_ready(void)
{
  syslog(LOG_INFO,
         "linux_bt: port namespace enabled hci=%d l2cap=%d a2dp=%d iso=%d\n",
#ifdef CONFIG_NET_LINUX_BLUETOOTH_HCI
         1,
#else
         0,
#endif
#ifdef CONFIG_NET_LINUX_BLUETOOTH_L2CAP
         1,
#else
         0,
#endif
#ifdef CONFIG_NET_LINUX_BLUETOOTH_A2DP
         1,
#else
         0,
#endif
#ifdef CONFIG_NET_LINUX_BLUETOOTH_ISO
         1
#else
         0
#endif
         );
}

static uint16_t linux_bt_role(void)
{
#ifdef CONFIG_SIM_BTHWSIM_ROLE
  return CONFIG_SIM_BTHWSIM_ROLE;
#else
  return 0;
#endif
}

static const char *linux_bt_conn_state_name(uint8_t state)
{
  switch (state)
    {
      case LINUX_BT_CONN_CONNECTING:
        return "connecting";

      case LINUX_BT_CONN_CONNECTED:
        return "connected";

      default:
        return "disconnected";
    }
}

static uint8_t linux_bt_conn_state_code(const char *state)
{
  if (state != NULL && !strcmp(state, "connected"))
    {
      return LINUX_BT_CONN_CONNECTED;
    }

  if (state != NULL && !strcmp(state, "connecting"))
    {
      return LINUX_BT_CONN_CONNECTING;
    }

  return 0;
}

static const char *linux_bt_l2cap_state_name(uint8_t state)
{
  switch (state)
    {
      case LINUX_BT_L2CAP_CONNECTING:
        return "connecting";

      case LINUX_BT_L2CAP_CONFIG:
        return "config";

      case LINUX_BT_L2CAP_OPEN:
        return "open";

      case LINUX_BT_L2CAP_DISCONNECTING:
        return "disconnecting";

      default:
        return "closed";
    }
}

static const char *linux_bt_iso_state_name(uint8_t state)
{
  switch (state)
    {
      case LINUX_BT_ISO_CREATING:
        return "creating";

      case LINUX_BT_ISO_STREAMING:
        return "streaming";

      case LINUX_BT_ISO_SYNCING:
        return "syncing";

      case LINUX_BT_ISO_SYNCED:
        return "synced";

      default:
        return "idle";
    }
}

static const char *linux_bt_iso_direction_name(uint8_t direction)
{
  return direction == LINUX_BT_ISO_SOURCE ? "source" : "sink";
}

static uint16_t linux_bt_iso_handle(uint8_t big, uint8_t bis)
{
  return 0x0200 + ((uint16_t)big * 16) + bis;
}

static void linux_bt_mgmt_set_setting(uint32_t setting, uint8_t enabled,
                                      const char *name)
{
  if (enabled)
    {
      g_linux_bt_mgmt.settings |= setting;
    }
  else
    {
      g_linux_bt_mgmt.settings &= ~setting;
    }

  linux_bt_event_push("MGMT_EV_NEW_SETTINGS index=%u settings=0x%08lx "
                      "changed=%s enabled=%u\n",
                      g_linux_bt_mgmt.index,
                      (unsigned long)g_linux_bt_mgmt.settings,
                      name, enabled ? 1 : 0);
}

static uint8_t linux_bt_mgmt_enabled(uint32_t setting)
{
  return (g_linux_bt_mgmt.settings & setting) != 0;
}

static uint8_t linux_bt_next_ident(void)
{
  uint8_t ident;

  ident = g_linux_bt_next_ident++;
  if (g_linux_bt_next_ident == 0)
    {
      g_linux_bt_next_ident = 1;
    }

  return ident;
}

static uint16_t linux_bt_next_dynamic_cid(void)
{
  uint16_t cid;

  cid = g_linux_bt_next_dynamic_cid++;
  if (g_linux_bt_next_dynamic_cid < 0x0040)
    {
      g_linux_bt_next_dynamic_cid = 0x0040;
    }

  return cid;
}

static uint16_t linux_bt_default_le_peer(void)
{
  uint16_t role = linux_bt_role();

  if (role == 3)
    {
      return 4;
    }

  if (role == 4)
    {
      return 3;
    }

  return role == 1 ? 2 : 1;
}

static struct linux_bt_conn *linux_bt_conn_lookup(uint16_t peer)
{
  int i;

  for (i = 0; i < LINUX_BT_MAX_CONN; i++)
    {
      if (g_linux_bt_conns[i].used && g_linux_bt_conns[i].peer == peer)
        {
          return &g_linux_bt_conns[i];
        }
    }

  return NULL;
}

static struct linux_bt_conn *linux_bt_conn_get(uint16_t peer)
{
  struct linux_bt_conn *conn;
  int i;

  conn = linux_bt_conn_lookup(peer);
  if (conn != NULL)
    {
      return conn;
    }

  for (i = 0; i < LINUX_BT_MAX_CONN; i++)
    {
      if (!g_linux_bt_conns[i].used)
        {
          g_linux_bt_conns[i].used = 1;
          g_linux_bt_conns[i].peer = peer;
          g_linux_bt_conns[i].handle = linux_bt_conn_handle(peer);
          g_linux_bt_conns[i].transport = 0;
          return &g_linux_bt_conns[i];
        }
    }

  return NULL;
}

uint16_t linux_bt_conn_handle(uint16_t peer)
{
  uint16_t low;
  uint16_t high;

  low = linux_bt_role() < peer ? linux_bt_role() : peer;
  high = linux_bt_role() < peer ? peer : linux_bt_role();

  return 0x0040 + (low * 16) + high;
}

static int linux_bt_conn_set(uint16_t peer, const char *state)
{
  struct linux_bt_conn *conn;
  uint8_t state_code;

  state_code = linux_bt_conn_state_code(state);
  conn = linux_bt_conn_get(peer);
  if (conn == NULL)
    {
      return -ENOMEM;
    }

  conn->state = state_code;
  conn->handle = linux_bt_conn_handle(peer);

  if (state_code == LINUX_BT_CONN_CONNECTED)
    {
      linux_bt_l2cap_open(peer, 0x0001, 0x0000);
      linux_bt_l2cap_open(peer, 0x0004, 0x0000);
    }

#ifdef CONFIG_SIM_BTHWSIM
  return sim_bthwsim_conn_set(peer, linux_bt_conn_handle(peer), state);
#else
  return 0;
#endif
}

static int linux_bt_conn_clear(uint16_t peer)
{
  int i;

  for (i = 0; i < LINUX_BT_MAX_CONN; i++)
    {
      if (g_linux_bt_conns[i].used && g_linux_bt_conns[i].peer == peer)
        {
          memset(&g_linux_bt_conns[i], 0, sizeof(g_linux_bt_conns[i]));
        }
    }

  for (i = 0; i < LINUX_BT_MAX_L2CAP_CHAN; i++)
    {
      if (g_linux_bt_l2cap[i].used && g_linux_bt_l2cap[i].peer == peer)
        {
          memset(&g_linux_bt_l2cap[i], 0, sizeof(g_linux_bt_l2cap[i]));
        }
    }

#ifdef CONFIG_SIM_BTHWSIM
  return sim_bthwsim_conn_clear(peer);
#else
  return 0;
#endif
}

static struct linux_bt_l2cap_chan *linux_bt_l2cap_lookup(uint16_t peer,
                                                         uint16_t cid)
{
  int i;

  for (i = 0; i < LINUX_BT_MAX_L2CAP_CHAN; i++)
    {
      if (g_linux_bt_l2cap[i].used &&
          g_linux_bt_l2cap[i].peer == peer &&
          g_linux_bt_l2cap[i].cid == cid)
        {
          return &g_linux_bt_l2cap[i];
        }
    }

  return NULL;
}

static struct linux_bt_l2cap_chan *linux_bt_l2cap_open(uint16_t peer,
                                                       uint16_t cid,
                                                       uint16_t psm)
{
  struct linux_bt_l2cap_chan *chan;
  int i;

  chan = linux_bt_l2cap_lookup(peer, cid);
  if (chan != NULL)
    {
      return chan;
    }

  for (i = 0; i < LINUX_BT_MAX_L2CAP_CHAN; i++)
    {
      if (!g_linux_bt_l2cap[i].used)
        {
          g_linux_bt_l2cap[i].used = 1;
          g_linux_bt_l2cap[i].peer = peer;
          g_linux_bt_l2cap[i].handle = linux_bt_conn_handle(peer);
          g_linux_bt_l2cap[i].cid = cid;
          g_linux_bt_l2cap[i].psm = psm;
          g_linux_bt_l2cap[i].ident = linux_bt_next_ident();
          g_linux_bt_l2cap[i].state = LINUX_BT_L2CAP_OPEN;
          return &g_linux_bt_l2cap[i];
        }
    }

  return NULL;
}

static struct linux_bt_l2cap_chan *linux_bt_l2cap_create(uint16_t peer,
                                                         uint16_t cid,
                                                         uint16_t psm,
                                                         uint8_t state)
{
  struct linux_bt_l2cap_chan *chan;

  chan = linux_bt_l2cap_open(peer, cid, psm);
  if (chan != NULL)
    {
      chan->state = state;
    }

  return chan;
}

static void linux_bt_l2cap_close(uint16_t peer, uint16_t cid)
{
  int i;

  for (i = 0; i < LINUX_BT_MAX_L2CAP_CHAN; i++)
    {
      if (g_linux_bt_l2cap[i].used &&
          g_linux_bt_l2cap[i].peer == peer &&
          g_linux_bt_l2cap[i].cid == cid)
        {
          memset(&g_linux_bt_l2cap[i], 0, sizeof(g_linux_bt_l2cap[i]));
        }
    }
}

int linux_bt_l2cap_ipsp_open(uint16_t cid, uint16_t *peer,
                             uint16_t *handle)
{
  struct linux_bt_l2cap_chan *chan;
  uint16_t dst;
  int ret;

  dst = linux_bt_default_le_peer();
  ret = linux_bt_conn_set(dst, "connected");
  if (ret < 0)
    {
      return ret;
    }

  chan = linux_bt_l2cap_create(dst, cid, LINUX_BT_L2CAP_PSM_IPSP,
                               LINUX_BT_L2CAP_OPEN);
  if (chan == NULL)
    {
      return -ENOMEM;
    }

  if (peer != NULL)
    {
      *peer = dst;
    }

  if (handle != NULL)
    {
      *handle = chan->handle;
    }

  return 0;
}

void linux_bt_l2cap_ipsp_close(uint16_t cid)
{
  linux_bt_l2cap_close(linux_bt_default_le_peer(), cid);
}

int linux_bt_l2cap_ipsp_status(uint16_t cid, char *out, size_t out_len)
{
  struct linux_bt_l2cap_chan *chan;
  uint16_t peer;

  if (out == NULL || out_len == 0)
    {
      return -EINVAL;
    }

  peer = linux_bt_default_le_peer();
  chan = linux_bt_l2cap_lookup(peer, cid);
  if (chan == NULL)
    {
      snprintf(out, out_len,
               "ipsp-peer=%u ipsp-psm=0x%04x ipsp-cid=0x%04x "
               "ipsp-state=closed ipsp-handle=0x0000",
               peer, LINUX_BT_L2CAP_PSM_IPSP, cid);
      return 0;
    }

  snprintf(out, out_len,
           "ipsp-peer=%u ipsp-psm=0x%04x ipsp-cid=0x%04x "
           "ipsp-state=%s ipsp-handle=0x%04x",
           chan->peer, chan->psm, chan->cid,
           linux_bt_l2cap_state_name(chan->state), chan->handle);
  return 0;
}

static struct linux_bt_iso_path *linux_bt_iso_lookup(uint8_t big,
                                                     uint8_t bis,
                                                     uint8_t direction)
{
  int i;

  for (i = 0; i < LINUX_BT_MAX_ISO_PATHS; i++)
    {
      if (g_linux_bt_iso_paths[i].used &&
          g_linux_bt_iso_paths[i].big == big &&
          g_linux_bt_iso_paths[i].bis == bis &&
          g_linux_bt_iso_paths[i].direction == direction)
        {
          return &g_linux_bt_iso_paths[i];
        }
    }

  return NULL;
}

static struct linux_bt_iso_path *linux_bt_iso_get(uint8_t big,
                                                  uint8_t bis,
                                                  uint8_t direction)
{
  struct linux_bt_iso_path *path;
  int i;

  path = linux_bt_iso_lookup(big, bis, direction);
  if (path != NULL)
    {
      return path;
    }

  for (i = 0; i < LINUX_BT_MAX_ISO_PATHS; i++)
    {
      if (!g_linux_bt_iso_paths[i].used)
        {
          g_linux_bt_iso_paths[i].used = 1;
          g_linux_bt_iso_paths[i].big = big;
          g_linux_bt_iso_paths[i].bis = bis;
          g_linux_bt_iso_paths[i].direction = direction;
          g_linux_bt_iso_paths[i].handle = linux_bt_iso_handle(big, bis);
          g_linux_bt_iso_paths[i].codec = "LC3";
          return &g_linux_bt_iso_paths[i];
        }
    }

  return NULL;
}

static void linux_bt_iso_clear(uint8_t big, uint8_t bis, uint8_t direction)
{
  int i;

  for (i = 0; i < LINUX_BT_MAX_ISO_PATHS; i++)
    {
      if (g_linux_bt_iso_paths[i].used &&
          g_linux_bt_iso_paths[i].big == big &&
          g_linux_bt_iso_paths[i].bis == bis &&
          g_linux_bt_iso_paths[i].direction == direction)
        {
          memset(&g_linux_bt_iso_paths[i], 0,
                 sizeof(g_linux_bt_iso_paths[i]));
        }
    }
}

static struct linux_bt_att_attr *linux_bt_att_lookup(uint16_t handle)
{
  unsigned int i;

  for (i = 0; i < sizeof(g_linux_bt_att) / sizeof(g_linux_bt_att[0]); i++)
    {
      if (g_linux_bt_att[i].handle == handle)
        {
          return &g_linux_bt_att[i];
        }
    }

  return NULL;
}

static void linux_bt_append(char *out, size_t out_len, size_t *used,
                            const char *fmt, ...)
{
  va_list ap;
  int ret;

  if (*used >= out_len)
    {
      return;
    }

  va_start(ap, fmt);
  ret = vsnprintf(out + *used, out_len - *used, fmt, ap);
  va_end(ap);

  if (ret < 0)
    {
      return;
    }

  if ((size_t)ret >= out_len - *used)
    {
      *used = out_len;
    }
  else
    {
      *used += ret;
    }
}

static void linux_bt_event_push(const char *fmt, ...)
{
  va_list ap;
  int ret;

  if (g_linux_bt_events_used >= sizeof(g_linux_bt_events) - 1)
    {
      g_linux_bt_events_used = 0;
      g_linux_bt_events[0] = '\0';
    }

  va_start(ap, fmt);
  ret = vsnprintf(g_linux_bt_events + g_linux_bt_events_used,
                  sizeof(g_linux_bt_events) - g_linux_bt_events_used,
                  fmt, ap);
  va_end(ap);

  if (ret < 0)
    {
      return;
    }

  if ((size_t)ret >= sizeof(g_linux_bt_events) - g_linux_bt_events_used)
    {
      g_linux_bt_events_used = sizeof(g_linux_bt_events) - 1;
      g_linux_bt_events[g_linux_bt_events_used] = '\0';
    }
  else
    {
      g_linux_bt_events_used += ret;
    }
}

static void linux_bt_ctrl_respond_connects(const char *records)
{
  const char *cursor = records;
  uint16_t src;
  uint16_t dst;
  unsigned int handle;
  char transport[16];
  char response[128];

  while ((cursor = strstr(cursor, "HCI_CMD_CONNECT")) != NULL)
    {
      src = 0;
      dst = 0;
      handle = 0;
      transport[0] = '\0';

      if (sscanf(cursor,
                 "HCI_CMD_CONNECT src=%hu dst=%hu transport=%15s "
                 "handle=0x%x",
                 &src, &dst, transport, &handle) >= 2 &&
          dst == linux_bt_role())
        {
          if (handle == 0)
            {
              handle = linux_bt_conn_handle(src);
            }

          linux_bt_conn_set(src, "connected");
          linux_bt_event_push("HCI_EVT_CONN_COMPLETE status=0 peer=%u "
                              "handle=%u role=acceptor\n",
                              src, handle);
          if (strcmp(transport, "le") == 0)
            {
              (void)linux_bt_upstream_hci_peer_le_complete(src,
                                                           (uint16_t)handle);
            }
          else if (strcmp(transport, "bredr") == 0)
            {
              (void)linux_bt_upstream_hci_peer_br_complete(
                src, (uint16_t)handle);
            }

          snprintf(response, sizeof(response),
                   "HCI_EVT_CONN_COMPLETE status=0 src=%u dst=%u "
                   "handle=%u role=acceptor",
                   linux_bt_role(), src, handle);
          linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_CTRL, src, response,
                              strlen(response));
        }

      cursor++;
    }
}

static void linux_bt_ctrl_process_disconnects(const char *records)
{
  const char *cursor = records;
  uint16_t src;
  uint16_t dst;
  unsigned int handle;
  char transport[16];
  char response[128];

  while ((cursor = strstr(cursor, "HCI_CMD_DISCONNECT")) != NULL)
    {
      src = 0;
      dst = 0;
      handle = 0;
      transport[0] = '\0';

      if (sscanf(cursor,
                 "HCI_CMD_DISCONNECT src=%hu dst=%hu transport=%15s "
                 "handle=0x%x",
                 &src, &dst, transport, &handle) >= 2 &&
          dst == linux_bt_role())
        {
          if (handle == 0)
            {
              handle = linux_bt_conn_handle(src);
            }

          linux_bt_conn_clear(src);
          linux_bt_event_push("HCI_EVT_DISCONN_COMPLETE status=0 peer=%u "
                              "handle=%u reason=0x13\n",
                              src, handle);
          if (strcmp(transport, "le") == 0)
            {
              (void)linux_bt_upstream_hci_peer_le_disconnect(
                (uint16_t)handle, 0x13);
            }
          else if (strcmp(transport, "bredr") == 0)
            {
              (void)linux_bt_upstream_hci_peer_br_disconnect(
                (uint16_t)handle, 0x13);
            }

          snprintf(response, sizeof(response),
                   "HCI_EVT_DISCONN_COMPLETE status=0 src=%u dst=%u "
                   "handle=%u reason=0x13",
                   linux_bt_role(), src, handle);
          linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_CTRL, src, response,
                              strlen(response));
        }

      cursor++;
    }
}

static void linux_bt_ctrl_process_completes(const char *records)
{
  const char *cursor = records;
  uint16_t src;
  uint16_t dst;

  while ((cursor = strstr(cursor, "HCI_EVT_CONN_COMPLETE")) != NULL)
    {
      src = 0;
      dst = 0;

      if (sscanf(cursor, "HCI_EVT_CONN_COMPLETE status=0 src=%hu dst=%hu",
                 &src, &dst) == 2 && dst == linux_bt_role())
        {
          linux_bt_conn_set(src, "connected");
          linux_bt_event_push("HCI_EVT_CONN_COMPLETE status=0 peer=%u "
                              "handle=%u role=initiator\n",
                              src, linux_bt_conn_handle(src));
        }

      cursor++;
    }

  cursor = records;
  while ((cursor = strstr(cursor, "HCI_EVT_DISCONN_COMPLETE")) != NULL)
    {
      src = 0;
      dst = 0;

      if (sscanf(cursor, "HCI_EVT_DISCONN_COMPLETE status=0 src=%hu dst=%hu",
                 &src, &dst) == 2 && dst == linux_bt_role())
        {
          linux_bt_conn_clear(src);
          linux_bt_event_push("HCI_EVT_DISCONN_COMPLETE status=0 peer=%u "
                              "handle=%u reason=0x13\n",
                              src, linux_bt_conn_handle(src));
        }

      cursor++;
    }
}

static void linux_bt_ctrl_process_records(const char *records)
{
  linux_bt_ctrl_respond_connects(records);
  linux_bt_ctrl_process_disconnects(records);
  linux_bt_ctrl_process_completes(records);
}

static int linux_bt_acl_line_peer(const char *line, uint16_t *peer)
{
  uint16_t src;
  uint16_t dst;

  if (sscanf(line, "seq=%*u src=%hu dst=%hu", &src, &dst) != 2)
    {
      return -EINVAL;
    }

  if (dst != linux_bt_role() && dst != LINUX_BT_HWSIM_DST_BROADCAST)
    {
      return -ENOENT;
    }

  *peer = src;
  return 0;
}

static void linux_bt_acl_send_l2cap_echo_rsp(uint16_t peer)
{
  char frame[160];

  linux_bt_l2cap_open(peer, 0x0001, 0x0000);

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u flags=PB_START cid=0x0001 "
           "L2CAP_ECHO_RSP ident=1 result=success payload=echo-rsp",
           linux_bt_conn_handle(peer));

  linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                      strlen(frame));
}

static void linux_bt_acl_send_l2cap_conn_rsp(uint16_t peer,
                                             uint8_t ident,
                                             uint16_t psm,
                                             uint16_t scid)
{
  struct linux_bt_l2cap_chan *chan;
  uint16_t dcid;
  char frame[192];

  dcid = linux_bt_next_dynamic_cid();
  chan = linux_bt_l2cap_create(peer, dcid, psm, LINUX_BT_L2CAP_CONFIG);
  if (chan != NULL)
    {
      chan->ident = ident;
    }

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0001 "
           "L2CAP_CONN_RSP ident=%u dcid=0x%04x scid=0x%04x "
           "psm=0x%04x result=success status=none",
           linux_bt_conn_handle(peer), ident, dcid, scid, psm);

  linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                      strlen(frame));

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0001 "
           "L2CAP_CONFIG_REQ ident=%u dcid=0x%04x flags=0 mtu=672",
           linux_bt_conn_handle(peer), linux_bt_next_ident(), scid);

  linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                      strlen(frame));
}

static void linux_bt_acl_send_l2cap_config_rsp(uint16_t peer,
                                               uint8_t ident,
                                               uint16_t dcid)
{
  struct linux_bt_l2cap_chan *chan;
  char frame[160];

  chan = linux_bt_l2cap_lookup(peer, dcid);
  if (chan != NULL)
    {
      chan->state = LINUX_BT_L2CAP_OPEN;
    }

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0001 "
           "L2CAP_CONFIG_RSP ident=%u scid=0x%04x flags=0 result=success",
           linux_bt_conn_handle(peer), ident, dcid);

  linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                      strlen(frame));
}

static void linux_bt_acl_send_l2cap_disconn_rsp(uint16_t peer,
                                                uint8_t ident,
                                                uint16_t dcid,
                                                uint16_t scid)
{
  char frame[160];

  linux_bt_l2cap_close(peer, dcid);

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0001 "
           "L2CAP_DISCONN_RSP ident=%u dcid=0x%04x scid=0x%04x",
           linux_bt_conn_handle(peer), ident, dcid, scid);

  linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                      strlen(frame));
}

static void linux_bt_acl_process_l2cap_signaling(uint16_t peer,
                                                 const char *line)
{
  struct linux_bt_l2cap_chan *chan;
  unsigned int ident;
  unsigned int psm;
  unsigned int scid;
  unsigned int dcid;

  ident = 0;
  psm = 0;
  scid = 0;
  dcid = 0;

  if (strstr(line, "L2CAP_CONN_REQ") != NULL &&
      sscanf(strstr(line, "L2CAP_CONN_REQ"),
             "L2CAP_CONN_REQ ident=%u psm=0x%x scid=0x%x",
             &ident, &psm, &scid) == 3)
    {
      linux_bt_acl_send_l2cap_conn_rsp(peer, (uint8_t)ident,
                                       (uint16_t)psm,
                                       (uint16_t)scid);
      return;
    }

  if (strstr(line, "L2CAP_CONN_RSP") != NULL &&
      sscanf(strstr(line, "L2CAP_CONN_RSP"),
             "L2CAP_CONN_RSP ident=%u dcid=0x%x scid=0x%x "
             "psm=0x%x result=success",
             &ident, &dcid, &scid, &psm) == 4)
    {
      chan = linux_bt_l2cap_lookup(peer, (uint16_t)scid);
      if (chan != NULL)
        {
          chan->state = LINUX_BT_L2CAP_CONFIG;
        }

      linux_bt_event_push("L2CAP_CONN_RSP peer=%u scid=0x%04x "
                          "dcid=0x%04x result=success\n",
                          peer, scid, dcid);
      return;
    }

  if (strstr(line, "L2CAP_CONFIG_REQ") != NULL &&
      sscanf(strstr(line, "L2CAP_CONFIG_REQ"),
             "L2CAP_CONFIG_REQ ident=%u dcid=0x%x",
             &ident, &dcid) == 2)
    {
      linux_bt_acl_send_l2cap_config_rsp(peer, (uint8_t)ident,
                                         (uint16_t)dcid);
      return;
    }

  if (strstr(line, "L2CAP_CONFIG_RSP") != NULL &&
      sscanf(strstr(line, "L2CAP_CONFIG_RSP"),
             "L2CAP_CONFIG_RSP ident=%u scid=0x%x",
             &ident, &scid) == 2)
    {
      chan = linux_bt_l2cap_lookup(peer, (uint16_t)scid);
      if (chan != NULL)
        {
          chan->state = LINUX_BT_L2CAP_OPEN;
        }

      linux_bt_event_push("L2CAP_CONFIG_RSP peer=%u scid=0x%04x "
                          "result=success\n",
                          peer, scid);
      return;
    }

  if (strstr(line, "L2CAP_DISCONN_REQ") != NULL &&
      sscanf(strstr(line, "L2CAP_DISCONN_REQ"),
             "L2CAP_DISCONN_REQ ident=%u dcid=0x%x scid=0x%x",
             &ident, &dcid, &scid) == 3)
    {
      linux_bt_acl_send_l2cap_disconn_rsp(peer, (uint8_t)ident,
                                          (uint16_t)dcid,
                                          (uint16_t)scid);
      return;
    }

  if (strstr(line, "L2CAP_DISCONN_RSP") != NULL &&
      sscanf(strstr(line, "L2CAP_DISCONN_RSP"),
             "L2CAP_DISCONN_RSP ident=%u dcid=0x%x scid=0x%x",
             &ident, &dcid, &scid) == 3)
    {
      linux_bt_l2cap_close(peer, (uint16_t)scid);
      linux_bt_event_push("L2CAP_DISCONN_RSP peer=%u scid=0x%04x\n",
                          peer, scid);
    }
}

static void linux_bt_conn_mark_paired(uint16_t peer)
{
  struct linux_bt_conn *conn;

  conn = linux_bt_conn_get(peer);
  if (conn != NULL)
    {
      conn->paired = 1;
      conn->state = LINUX_BT_CONN_CONNECTED;
    }
}

static void linux_bt_acl_send_smp_pairing_rsp(uint16_t peer)
{
  char frame[160];

  linux_bt_l2cap_open(peer, 0x0006, 0x0000);

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0006 "
           "SMP_PAIRING_RSP io_cap=noinput oob=0 auth=bonding "
           "max_key_size=16 init_key=enc_id resp_key=enc_id",
           linux_bt_conn_handle(peer));

  linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                      strlen(frame));
}

static void linux_bt_acl_send_smp_pairing_confirm(uint16_t peer)
{
  char frame[160];

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0006 "
           "SMP_PAIRING_CONFIRM value=00000000000000000000000000000000",
           linux_bt_conn_handle(peer));

  linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                      strlen(frame));
}

static void linux_bt_acl_send_smp_pairing_random(uint16_t peer)
{
  char frame[160];

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0006 "
           "SMP_PAIRING_RANDOM value=11111111111111111111111111111111",
           linux_bt_conn_handle(peer));

  linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                      strlen(frame));
}

static void linux_bt_acl_process_smp(uint16_t peer, const char *line)
{
  if (strstr(line, "SMP_PAIRING_REQ") != NULL)
    {
      linux_bt_l2cap_open(peer, 0x0006, 0x0000);
      linux_bt_event_push("SMP_PAIRING_REQ peer=%u method=justworks\n",
                          peer);
      linux_bt_acl_send_smp_pairing_rsp(peer);
      return;
    }

  if (strstr(line, "SMP_PAIRING_RSP") != NULL)
    {
      linux_bt_l2cap_open(peer, 0x0006, 0x0000);
      linux_bt_event_push("SMP_PAIRING_RSP peer=%u method=justworks\n",
                          peer);
      linux_bt_acl_send_smp_pairing_confirm(peer);
      return;
    }

  if (strstr(line, "SMP_PAIRING_CONFIRM") != NULL)
    {
      linux_bt_event_push("SMP_PAIRING_CONFIRM peer=%u\n", peer);
      linux_bt_acl_send_smp_pairing_random(peer);
      return;
    }

  if (strstr(line, "SMP_PAIRING_RANDOM") != NULL)
    {
      linux_bt_conn_mark_paired(peer);
      linux_bt_event_push("SMP_PAIRING_COMPLETE peer=%u status=0 "
                          "bonded=1\n",
                          peer);
    }
}

static void linux_bt_acl_send_att_read_rsp(uint16_t peer, uint16_t handle)
{
  struct linux_bt_att_attr *attr;
  char frame[160];

  linux_bt_l2cap_open(peer, 0x0004, 0x0000);

  attr = linux_bt_att_lookup(handle);
  if (attr == NULL || !attr->readable)
    {
      snprintf(frame, sizeof(frame),
               "HCI_ACL_DATA handle=%u cid=0x0004 ATT_ERROR_RSP "
               "opcode=read att_handle=0x%04x error=invalid_handle",
               linux_bt_conn_handle(peer), handle);
    }
  else
    {
      snprintf(frame, sizeof(frame),
               "HCI_ACL_DATA handle=%u cid=0x0004 ATT_READ_RSP "
               "att_handle=0x%04x uuid=%s value=%s",
               linux_bt_conn_handle(peer), handle, attr->uuid,
               attr->value);
    }

  linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                      strlen(frame));
}

static void linux_bt_acl_send_att_write_rsp(uint16_t peer, uint16_t handle,
                                            const char *value)
{
  struct linux_bt_att_attr *attr;
  char frame[160];

  linux_bt_l2cap_open(peer, 0x0004, 0x0000);

  attr = linux_bt_att_lookup(handle);
  if (attr == NULL || !attr->writable)
    {
      snprintf(frame, sizeof(frame),
               "HCI_ACL_DATA handle=%u cid=0x0004 ATT_ERROR_RSP "
               "opcode=write att_handle=0x%04x error=write_not_permitted",
               linux_bt_conn_handle(peer), handle);
    }
  else
    {
      snprintf(attr->value, sizeof(attr->value), "%s",
               value == NULL ? "" : value);
      snprintf(frame, sizeof(frame),
               "HCI_ACL_DATA handle=%u cid=0x0004 ATT_WRITE_RSP "
               "att_handle=0x%04x status=success",
               linux_bt_conn_handle(peer), handle);
    }

  linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                      strlen(frame));
}

static void linux_bt_acl_send_avdtp_start_rsp(uint16_t peer)
{
  char frame[160];

  linux_bt_l2cap_open(peer, 0x0041, 0x0019);

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0041 psm=0x0019 "
           "AVDTP_START_RSP state=streaming",
           linux_bt_conn_handle(peer));

  linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                      strlen(frame));
}

static uint16_t linux_bt_acl_att_handle(const char *line)
{
  uint16_t handle;
  const char *cursor;

  handle = 0;
  cursor = strstr(line, "att_handle=0x");
  if (cursor != NULL &&
      sscanf(cursor, "att_handle=0x%hx", &handle) == 1)
    {
      return handle;
    }

  cursor = strstr(line, "ATT_READ_REQ handle=0x");
  if (cursor != NULL &&
      sscanf(cursor, "ATT_READ_REQ handle=0x%hx", &handle) == 1)
    {
      return handle;
    }

  cursor = strstr(line, "ATT_WRITE_REQ handle=0x");
  if (cursor != NULL &&
      sscanf(cursor, "ATT_WRITE_REQ handle=0x%hx", &handle) == 1)
    {
      return handle;
    }

  return 0;
}

static const char *linux_bt_acl_payload(const char *line)
{
  const char *cursor;

  cursor = strstr(line, "payload=");
  if (cursor == NULL)
    {
      return "";
    }

  return cursor + strlen("payload=");
}

static void linux_bt_acl_process_line(const char *line)
{
  uint16_t peer;
  uint16_t handle;

  if (linux_bt_acl_line_peer(line, &peer) < 0)
    {
      return;
    }

  if (strstr(line, "L2CAP_ECHO_REQ") != NULL)
    {
      linux_bt_acl_send_l2cap_echo_rsp(peer);
    }

  if (strstr(line, "L2CAP_") != NULL)
    {
      linux_bt_acl_process_l2cap_signaling(peer, line);
    }

  if (strstr(line, "SMP_") != NULL)
    {
      linux_bt_acl_process_smp(peer, line);
    }

  if (strstr(line, "ATT_READ_REQ") != NULL)
    {
      handle = linux_bt_acl_att_handle(line);
      linux_bt_acl_send_att_read_rsp(peer, handle);
    }

  if (strstr(line, "ATT_WRITE_REQ") != NULL)
    {
      handle = linux_bt_acl_att_handle(line);
      linux_bt_acl_send_att_write_rsp(peer, handle,
                                      linux_bt_acl_payload(line));
    }

  if (strstr(line, "AVDTP_START") != NULL &&
      strstr(line, "AVDTP_START_RSP") == NULL)
    {
      linux_bt_acl_send_avdtp_start_rsp(peer);
    }
}

static void linux_bt_acl_process_records(const char *records)
{
  const char *cursor;
  const char *next;
  char line[384];
  size_t len;

  cursor = records;
  while (cursor != NULL && *cursor != '\0')
    {
      next = strchr(cursor, '\n');
      len = next == NULL ? strlen(cursor) : (size_t)(next - cursor);

      if (len >= sizeof(line))
        {
          len = sizeof(line) - 1;
        }

      memcpy(line, cursor, len);
      line[len] = '\0';

      if (strstr(line, "HCI_ACL_DATA") != NULL)
        {
          linux_bt_acl_process_line(line);
        }

      if (next == NULL)
        {
          break;
        }

      cursor = next + 1;
    }
}

int linux_bt_hwsim_send(uint16_t type, uint16_t dst,
                        const void *payload, size_t payload_len)
{
#ifdef CONFIG_SIM_BTHWSIM
  return sim_bthwsim_send(type, dst, payload, payload_len);
#else
  return -ENODEV;
#endif
}

int linux_bt_hwsim_read(uint16_t type, char *out, size_t out_len)
{
#ifdef CONFIG_SIM_BTHWSIM
  return sim_bthwsim_read(type, out, out_len);
#else
  if (out_len > 0)
    {
      out[0] = '\0';
    }

  return -ENODEV;
#endif
}

int linux_bt_hwsim_read_raw(uint16_t type, uint16_t *src, uint16_t *dst,
                            void *payload, uint32_t payload_len,
                            uint32_t *out_len)
{
#ifdef CONFIG_SIM_BTHWSIM
  return sim_bthwsim_read_raw(type, src, dst, payload, payload_len, out_len);
#else
  if (out_len != NULL)
    {
      *out_len = 0;
    }

  return -ENODEV;
#endif
}

int linux_bt_hwsim_read_raw_named(uint16_t type, const char *consumer,
                                  uint16_t *src, uint16_t *dst,
                                  void *payload, uint32_t payload_len,
                                  uint32_t *out_len)
{
#ifdef CONFIG_SIM_BTHWSIM
  return sim_bthwsim_read_raw_named(type, consumer, src, dst, payload,
                                    payload_len, out_len);
#else
  if (out_len != NULL)
    {
      *out_len = 0;
    }

  return -ENODEV;
#endif
}

int linux_bt_hwsim_h4_read(const char *path, void *payload,
                           uint32_t payload_len, uint32_t *out_len)
{
#ifdef CONFIG_SIM_BTHWSIM
  return sim_bthwsim_h4_read(path, payload, payload_len, out_len);
#else
  if (out_len != NULL)
    {
      *out_len = 0;
    }

  return -ENODEV;
#endif
}

int linux_bt_hwsim_h4_append(const char *path, const void *payload,
                             uint32_t payload_len)
{
#ifdef CONFIG_SIM_BTHWSIM
  return sim_bthwsim_h4_append(path, payload, payload_len);
#else
  return -ENODEV;
#endif
}

int linux_bt_info(char *out, size_t out_len)
{
  if (out_len == 0)
    {
      return 0;
    }

  snprintf(out, out_len,
           "linux_bt: role=%d mode=%s max_conn=%d settings=0x%08lx "
           "hci=1 mgmt=1 l2cap=1 smp=1 gatt=1 a2dp=1 iso=1\n",
#ifdef CONFIG_SIM_BTHWSIM_ROLE
           CONFIG_SIM_BTHWSIM_ROLE,
#else
           -1,
#endif
#ifdef CONFIG_SIM_BTHWSIM_MODE
           CONFIG_SIM_BTHWSIM_MODE,
#else
           "unknown",
#endif
#ifdef CONFIG_NET_LINUX_BLUETOOTH_MAX_CONN
           CONFIG_NET_LINUX_BLUETOOTH_MAX_CONN
#else
           0
#endif
           ,
           (unsigned long)g_linux_bt_mgmt.settings
           );

  return 0;
}

int linux_bt_mgmt_status(char *out, size_t out_len)
{
  if (out_len == 0)
    {
      return 0;
    }

  snprintf(out, out_len,
           "mgmt: index=%u settings=0x%08lx supported=0x%08lx "
           "powered=%u connectable=%u discoverable=%u bondable=%u "
           "bredr=%u le=%u advertising=%u\n",
           g_linux_bt_mgmt.index,
           (unsigned long)g_linux_bt_mgmt.settings,
           (unsigned long)g_linux_bt_mgmt.supported_settings,
           linux_bt_mgmt_enabled(LINUX_BT_MGMT_SETTING_POWERED),
           linux_bt_mgmt_enabled(LINUX_BT_MGMT_SETTING_CONNECTABLE),
           linux_bt_mgmt_enabled(LINUX_BT_MGMT_SETTING_DISCOVERABLE),
           linux_bt_mgmt_enabled(LINUX_BT_MGMT_SETTING_BONDABLE),
           linux_bt_mgmt_enabled(LINUX_BT_MGMT_SETTING_BREDR),
           linux_bt_mgmt_enabled(LINUX_BT_MGMT_SETTING_LE),
           linux_bt_mgmt_enabled(LINUX_BT_MGMT_SETTING_ADVERTISING));

  return 0;
}

int linux_bt_mgmt_set_powered(uint8_t enabled)
{
  linux_bt_mgmt_set_setting(LINUX_BT_MGMT_SETTING_POWERED, enabled,
                            "powered");
  linux_bt_event_push("MGMT_EV_INDEX_%s index=%u\n",
                      enabled ? "ADDED" : "REMOVED",
                      g_linux_bt_mgmt.index);
  return 0;
}

int linux_bt_mgmt_set_connectable(uint8_t enabled)
{
  linux_bt_mgmt_set_setting(LINUX_BT_MGMT_SETTING_CONNECTABLE, enabled,
                            "connectable");
  return 0;
}

int linux_bt_mgmt_set_discoverable(uint8_t enabled)
{
  linux_bt_mgmt_set_setting(LINUX_BT_MGMT_SETTING_DISCOVERABLE, enabled,
                            "discoverable");
  return 0;
}

int linux_bt_mgmt_set_bondable(uint8_t enabled)
{
  linux_bt_mgmt_set_setting(LINUX_BT_MGMT_SETTING_BONDABLE, enabled,
                            "bondable");
  return 0;
}

int linux_bt_mgmt_set_le(uint8_t enabled)
{
  linux_bt_mgmt_set_setting(LINUX_BT_MGMT_SETTING_LE, enabled, "le");
  return 0;
}

int linux_bt_mgmt_set_bredr(uint8_t enabled)
{
  linux_bt_mgmt_set_setting(LINUX_BT_MGMT_SETTING_BREDR, enabled,
                            "bredr");
  return 0;
}

int linux_bt_mgmt_set_advertising(uint8_t enabled)
{
  linux_bt_mgmt_set_setting(LINUX_BT_MGMT_SETTING_ADVERTISING, enabled,
                            "advertising");
  return 0;
}

static void linux_bt_mgmt_dispatch_reply(uint16_t opcode, const char *name,
                                         int status, char *out,
                                         size_t out_len)
{
  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len,
               "mgmt: opcode=0x%04x name=\"%s\" status=%d "
               "settings=0x%08lx\n",
               opcode, name, status,
               (unsigned long)g_linux_bt_mgmt.settings);
    }

  linux_bt_event_push("MGMT_EV_CMD_COMPLETE opcode=0x%04x name=\"%s\" "
                      "status=%d settings=0x%08lx\n",
                      opcode, name, status,
                      (unsigned long)g_linux_bt_mgmt.settings);
}

static int linux_bt_mgmt_dispatch_read_info(uint8_t param,
                                            char *out, size_t out_len)
{
  (void)param;

  return linux_bt_mgmt_status(out, out_len);
}

static int linux_bt_mgmt_dispatch_powered(uint8_t param,
                                          char *out, size_t out_len)
{
  int ret;

  ret = linux_bt_mgmt_set_powered(param);
  linux_bt_mgmt_dispatch_reply(LINUX_BT_MGMT_OP_SET_POWERED,
                               "Set Powered", ret, out, out_len);
  return ret;
}

static int linux_bt_mgmt_dispatch_connectable(uint8_t param,
                                              char *out, size_t out_len)
{
  int ret;

  ret = linux_bt_mgmt_set_connectable(param);
  linux_bt_mgmt_dispatch_reply(LINUX_BT_MGMT_OP_SET_CONNECTABLE,
                               "Set Connectable", ret, out, out_len);
  return ret;
}

static int linux_bt_mgmt_dispatch_discoverable(uint8_t param,
                                               char *out, size_t out_len)
{
  int ret;

  ret = linux_bt_mgmt_set_discoverable(param);
  linux_bt_mgmt_dispatch_reply(LINUX_BT_MGMT_OP_SET_DISCOVERABLE,
                               "Set Discoverable", ret, out, out_len);
  return ret;
}

static int linux_bt_mgmt_dispatch_bondable(uint8_t param,
                                           char *out, size_t out_len)
{
  int ret;

  ret = linux_bt_mgmt_set_bondable(param);
  linux_bt_mgmt_dispatch_reply(LINUX_BT_MGMT_OP_SET_BONDABLE,
                               "Set Bondable", ret, out, out_len);
  return ret;
}

static int linux_bt_mgmt_dispatch_le(uint8_t param,
                                     char *out, size_t out_len)
{
  int ret;

  ret = linux_bt_mgmt_set_le(param);
  linux_bt_mgmt_dispatch_reply(LINUX_BT_MGMT_OP_SET_LE,
                               "Set LE", ret, out, out_len);
  return ret;
}

static int linux_bt_mgmt_dispatch_bredr(uint8_t param,
                                        char *out, size_t out_len)
{
  int ret;

  ret = linux_bt_mgmt_set_bredr(param);
  linux_bt_mgmt_dispatch_reply(LINUX_BT_MGMT_OP_SET_BREDR,
                               "Set BR/EDR", ret, out, out_len);
  return ret;
}

static int linux_bt_mgmt_dispatch_advertising(uint8_t param,
                                              char *out, size_t out_len)
{
  int ret;

  ret = param ? linux_bt_advertise_start() : linux_bt_advertise_stop();
  linux_bt_mgmt_dispatch_reply(LINUX_BT_MGMT_OP_SET_ADVERTISING,
                               "Set Advertising", ret, out, out_len);
  return ret;
}

int linux_bt_mgmt_dispatch(uint16_t opcode, uint8_t param,
                           char *out, size_t out_len)
{
  unsigned int i;

  for (i = 0; i < sizeof(g_linux_bt_mgmt_ops) /
                  sizeof(g_linux_bt_mgmt_ops[0]); i++)
    {
      if (g_linux_bt_mgmt_ops[i].opcode == opcode)
        {
          return g_linux_bt_mgmt_ops[i].handler(param, out, out_len);
        }
    }

  if (out != NULL && out_len > 0)
    {
      snprintf(out, out_len,
               "mgmt: opcode=0x%04x status=-%d error=unknown_command\n",
               opcode, EOPNOTSUPP);
    }

  linux_bt_event_push("MGMT_EV_CMD_STATUS opcode=0x%04x status=-%d "
                      "error=unknown_command\n",
                      opcode, EOPNOTSUPP);
  return -EOPNOTSUPP;
}

uint32_t linux_bt_mgmt_settings(void)
{
  return (uint32_t)g_linux_bt_mgmt.settings;
}

int linux_bt_state(char *out, size_t out_len)
{
  size_t used;
  unsigned int i;

  if (out_len == 0)
    {
      return 0;
    }

  used = 0;
  out[0] = '\0';

  linux_bt_append(out, out_len, &used,
                  "linux_bt_state: role=%u settings=0x%08lx\n",
                  linux_bt_role(),
                  (unsigned long)g_linux_bt_mgmt.settings);

  linux_bt_append(out, out_len, &used, "connections:\n");
  for (i = 0; i < LINUX_BT_MAX_CONN; i++)
    {
      if (g_linux_bt_conns[i].used)
        {
          linux_bt_append(out, out_len, &used,
                          "  peer=%u handle=%u state=%s transport=%u "
                          "paired=%u\n",
                          g_linux_bt_conns[i].peer,
                          g_linux_bt_conns[i].handle,
                          linux_bt_conn_state_name(
                            g_linux_bt_conns[i].state),
                          g_linux_bt_conns[i].transport,
                          g_linux_bt_conns[i].paired);
        }
    }

  linux_bt_append(out, out_len, &used, "l2cap_channels:\n");
  for (i = 0; i < LINUX_BT_MAX_L2CAP_CHAN; i++)
    {
      if (g_linux_bt_l2cap[i].used)
        {
          linux_bt_append(out, out_len, &used,
                          "  peer=%u handle=%u cid=0x%04x psm=0x%04x "
                          "state=%s ident=%u\n",
                          g_linux_bt_l2cap[i].peer,
                          g_linux_bt_l2cap[i].handle,
                          g_linux_bt_l2cap[i].cid,
                          g_linux_bt_l2cap[i].psm,
                          linux_bt_l2cap_state_name(
                            g_linux_bt_l2cap[i].state),
                          g_linux_bt_l2cap[i].ident);
        }
    }

  linux_bt_append(out, out_len, &used, "att_db:\n");
  for (i = 0; i < sizeof(g_linux_bt_att) / sizeof(g_linux_bt_att[0]); i++)
    {
      linux_bt_append(out, out_len, &used,
                      "  handle=0x%04x uuid=%s value=%s r=%u w=%u\n",
                      g_linux_bt_att[i].handle,
                      g_linux_bt_att[i].uuid,
                      g_linux_bt_att[i].value,
                      g_linux_bt_att[i].readable,
                      g_linux_bt_att[i].writable);
    }

  linux_bt_append(out, out_len, &used, "iso_paths:\n");
  for (i = 0; i < LINUX_BT_MAX_ISO_PATHS; i++)
    {
      if (g_linux_bt_iso_paths[i].used)
        {
          linux_bt_append(out, out_len, &used,
                          "  dir=%s big=%u bis=%u handle=0x%04x "
                          "state=%s codec=%s seq=%lu\n",
                          linux_bt_iso_direction_name(
                            g_linux_bt_iso_paths[i].direction),
                          g_linux_bt_iso_paths[i].big,
                          g_linux_bt_iso_paths[i].bis,
                          g_linux_bt_iso_paths[i].handle,
                          linux_bt_iso_state_name(
                            g_linux_bt_iso_paths[i].state),
                          g_linux_bt_iso_paths[i].codec,
                          (unsigned long)g_linux_bt_iso_paths[i].seq);
        }
    }

  return 0;
}

int linux_bt_events(char *out, size_t out_len)
{
  if (out_len == 0)
    {
      return 0;
    }

  if (g_linux_bt_events_used == 0)
    {
      snprintf(out, out_len, "linux_bt_events: empty\n");
      return 0;
    }

  snprintf(out, out_len, "%s", g_linux_bt_events);
  g_linux_bt_events_used = 0;
  g_linux_bt_events[0] = '\0';
  return 0;
}

int linux_bt_scan_bredr(char *out, size_t out_len)
{
  int ret;

  ret = linux_bt_hwsim_read(LINUX_BT_HWSIM_TYPE_CTRL, out, out_len);
  if (ret > 0 && out != NULL)
    {
      linux_bt_ctrl_process_records(out);
    }

  return ret;
}

int linux_bt_ctrl_poll(char *out, size_t out_len)
{
  char payload[256];
  uint16_t src;
  uint16_t dst;
  uint32_t payload_len;
  size_t used = 0;
  int count = 0;
  int ret;

  if (out != NULL && out_len > 0)
    {
      out[0] = '\0';
    }

  for (; ; )
    {
      payload_len = 0;
      ret = linux_bt_hwsim_read_raw_named(LINUX_BT_HWSIM_TYPE_CTRL,
                                          "btctl-ctrl", &src, &dst,
                                          payload, sizeof(payload) - 1,
                                          &payload_len);
      if (ret <= 0)
        {
          return count > 0 ? count : ret;
        }

      payload[payload_len] = '\0';
      if (out != NULL && used < out_len)
        {
          linux_bt_append(out, out_len, &used,
                          "src=%u dst=%u len=%lu payload=%s\n",
                          src, dst, (unsigned long)payload_len, payload);
        }

      linux_bt_ctrl_process_records(payload);
      count++;
    }
}

int linux_bt_scan_le(char *out, size_t out_len)
{
  char ctrl[256];
  int adv_ret;
  int ctrl_ret;

  adv_ret = linux_bt_hwsim_read(LINUX_BT_HWSIM_TYPE_ADV, out, out_len);

  ctrl_ret = linux_bt_hwsim_read(LINUX_BT_HWSIM_TYPE_CTRL, ctrl,
                                 sizeof(ctrl));
  if (ctrl_ret > 0)
    {
      linux_bt_ctrl_process_records(ctrl);
      if (adv_ret <= 0 && out != NULL && out_len > 0)
        {
          snprintf(out, out_len, "%s", ctrl);
        }
    }

  return adv_ret > 0 ? adv_ret : ctrl_ret;
}

int linux_bt_advertise_start(void)
{
  char payload[96];
  int ret;

  snprintf(payload, sizeof(payload),
           "HCI_LE_ADV_REPORT role=%d event=ADV_IND name=linux-bt-hwsim",
#ifdef CONFIG_SIM_BTHWSIM_ROLE
           CONFIG_SIM_BTHWSIM_ROLE
#else
           -1
#endif
           );

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ADV,
                            LINUX_BT_HWSIM_DST_BROADCAST,
                            payload, strlen(payload));
  if (ret == 0)
    {
      linux_bt_mgmt_set_setting(LINUX_BT_MGMT_SETTING_ADVERTISING, 1,
                                "advertising");
      linux_bt_event_push("HCI_EVT_CMD_COMPLETE opcode=LE_Set_Adv_Enable "
                          "status=0 enable=1\n");
    }

  return ret;
}

int linux_bt_advertise_stop(void)
{
  char payload[80];
  int ret;

  snprintf(payload, sizeof(payload),
           "HCI_LE_ADV_STOP role=%d",
#ifdef CONFIG_SIM_BTHWSIM_ROLE
           CONFIG_SIM_BTHWSIM_ROLE
#else
           -1
#endif
           );

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ADV,
                            LINUX_BT_HWSIM_DST_BROADCAST,
                            payload, strlen(payload));
  if (ret == 0)
    {
      linux_bt_mgmt_set_setting(LINUX_BT_MGMT_SETTING_ADVERTISING, 0,
                                "advertising");
      linux_bt_event_push("HCI_EVT_CMD_COMPLETE opcode=LE_Set_Adv_Enable "
                          "status=0 enable=0\n");
    }

  return ret;
}

int linux_bt_connect(uint16_t peer)
{
  char payload[96];
  int ret;

  snprintf(payload, sizeof(payload),
           "HCI_CMD_CONNECT src=%d dst=%u transport=auto",
           linux_bt_role(),
           peer);

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_CTRL, peer, payload,
                            strlen(payload));
  if (ret == 0)
    {
      linux_bt_conn_set(peer, "connecting");
      linux_bt_event_push("HCI_EVT_CMD_STATUS opcode=Create_Connection "
                          "status=0 peer=%u\n",
                          peer);
    }

  return ret;
}

int linux_bt_disconnect(uint16_t peer)
{
  char payload[96];
  int ret;

  snprintf(payload, sizeof(payload),
           "HCI_CMD_DISCONNECT src=%d dst=%u handle=%u reason=0x13",
           linux_bt_role(),
           peer, linux_bt_conn_handle(peer));

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_CTRL, peer, payload,
                            strlen(payload));
  if (ret == 0)
    {
      linux_bt_event_push("HCI_EVT_CMD_STATUS opcode=Disconnect "
                          "status=0 peer=%u handle=%u\n",
                          peer, linux_bt_conn_handle(peer));
      linux_bt_conn_clear(peer);
    }

  return ret;
}

int linux_bt_pair(uint16_t peer)
{
  char frame[192];
  int ret;

  linux_bt_l2cap_open(peer, 0x0006, 0x0000);

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0006 "
           "SMP_PAIRING_REQ io_cap=noinput oob=0 auth=bonding "
           "max_key_size=16 init_key=enc_id resp_key=enc_id",
           linux_bt_conn_handle(peer));

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                            strlen(frame));
  if (ret == 0)
    {
      linux_bt_event_push("SMP_PAIRING_REQ peer=%u method=justworks\n",
                          peer);
    }

  return ret;
}

int linux_bt_l2cap_connect(uint16_t peer, uint16_t psm)
{
  struct linux_bt_l2cap_chan *chan;
  uint16_t scid;
  char frame[160];
  int ret;

  scid = linux_bt_next_dynamic_cid();
  chan = linux_bt_l2cap_create(peer, scid, psm, LINUX_BT_L2CAP_CONNECTING);
  if (chan == NULL)
    {
      return -ENOMEM;
    }

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0001 "
           "L2CAP_CONN_REQ ident=%u psm=0x%04x scid=0x%04x",
           linux_bt_conn_handle(peer), chan->ident, psm, scid);

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                            strlen(frame));
  if (ret == 0)
    {
      linux_bt_event_push("L2CAP_CONN_REQ peer=%u psm=0x%04x "
                          "scid=0x%04x ident=%u\n",
                          peer, psm, scid, chan->ident);
    }

  return ret;
}

int linux_bt_l2cap_disconnect(uint16_t peer, uint16_t cid)
{
  struct linux_bt_l2cap_chan *chan;
  uint8_t ident;
  char frame[160];
  int ret;

  ident = linux_bt_next_ident();
  chan = linux_bt_l2cap_lookup(peer, cid);
  if (chan != NULL)
    {
      chan->state = LINUX_BT_L2CAP_DISCONNECTING;
      ident = chan->ident;
    }

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0001 "
           "L2CAP_DISCONN_REQ ident=%u dcid=0x%04x scid=0x%04x",
           linux_bt_conn_handle(peer), ident, cid, cid);

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                            strlen(frame));
  if (ret == 0)
    {
      linux_bt_event_push("L2CAP_DISCONN_REQ peer=%u cid=0x%04x "
                          "ident=%u\n",
                          peer, cid, ident);
    }

  return ret;
}

int linux_bt_l2cap_send(uint16_t peer, const char *payload)
{
  char frame[192];
  int ret;

  linux_bt_l2cap_open(peer, 0x0040, 0x1001);

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u flags=PB_START l2cap_len=%zu "
           "cid=0x0040 psm=0x1001 payload=%s",
           linux_bt_conn_handle(peer), strlen(payload), payload);

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                            strlen(frame));
  if (ret == 0)
    {
      linux_bt_event_push("HCI_EVT_NUM_COMPLETED_PACKETS handle=%u "
                          "count=1 cid=0x0040\n",
                          linux_bt_conn_handle(peer));
    }

  return ret;
}

int linux_bt_l2cap_echo(uint16_t peer, const char *payload)
{
  char frame[192];
  int ret;

  linux_bt_l2cap_open(peer, 0x0001, 0x0000);

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u flags=PB_START cid=0x0001 "
           "L2CAP_ECHO_REQ ident=1 payload=%s",
           linux_bt_conn_handle(peer), payload);

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, peer, frame,
                            strlen(frame));
  if (ret == 0)
    {
      linux_bt_event_push("HCI_EVT_NUM_COMPLETED_PACKETS handle=%u "
                          "count=1 cid=0x0001\n",
                          linux_bt_conn_handle(peer));
    }

  return ret;
}

int linux_bt_acl_poll(char *out, size_t out_len)
{
  int ret;

  ret = linux_bt_hwsim_read(LINUX_BT_HWSIM_TYPE_ACL, out, out_len);
  if (ret > 0 && out != NULL)
    {
      linux_bt_acl_process_records(out);
      linux_bt_event_push("HCI_EVT_NUM_COMPLETED_PACKETS count=%d "
                          "direction=rx\n",
                          ret);
    }

  return ret;
}

static int linux_bt_gatt_read_emit(uint16_t dst, uint16_t conn_handle,
                                   uint16_t handle)
{
  char frame[128];
  int ret;

  if (dst != LINUX_BT_HWSIM_DST_BROADCAST)
    {
      linux_bt_l2cap_open(dst, 0x0004, 0x0000);
    }

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0004 ATT_READ_REQ "
           "att_handle=0x%04x",
           conn_handle, handle);

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, dst, frame,
                            strlen(frame));
  if (ret == 0)
    {
      linux_bt_event_push("HCI_EVT_NUM_COMPLETED_PACKETS handle=%u "
                          "count=1 cid=0x0004 att=read\n",
                          conn_handle);
    }

  return ret;
}

int linux_bt_gatt_read(uint16_t handle)
{
  return linux_bt_gatt_read_emit(LINUX_BT_HWSIM_DST_BROADCAST, 0, handle);
}

int linux_bt_gatt_read_peer(uint16_t peer, uint16_t handle)
{
  return linux_bt_gatt_read_emit(peer, linux_bt_conn_handle(peer), handle);
}

static int linux_bt_gatt_write_emit(uint16_t dst, uint16_t conn_handle,
                                    uint16_t handle, const char *payload)
{
  char frame[192];
  int ret;

  if (dst != LINUX_BT_HWSIM_DST_BROADCAST)
    {
      linux_bt_l2cap_open(dst, 0x0004, 0x0000);
    }

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0004 ATT_WRITE_REQ "
           "att_handle=0x%04x payload=%s",
           conn_handle, handle, payload);

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, dst, frame,
                            strlen(frame));
  if (ret == 0)
    {
      linux_bt_event_push("HCI_EVT_NUM_COMPLETED_PACKETS handle=%u "
                          "count=1 cid=0x0004 att=write\n",
                          conn_handle);
    }

  return ret;
}

int linux_bt_gatt_write(uint16_t handle, const char *payload)
{
  return linux_bt_gatt_write_emit(LINUX_BT_HWSIM_DST_BROADCAST, 0,
                                  handle, payload);
}

int linux_bt_gatt_write_peer(uint16_t peer, uint16_t handle,
                             const char *payload)
{
  return linux_bt_gatt_write_emit(peer, linux_bt_conn_handle(peer),
                                  handle, payload);
}

static int linux_bt_a2dp_source_emit(uint16_t dst, uint16_t handle)
{
  char frame[160];
  int ret;

  if (dst != LINUX_BT_HWSIM_DST_BROADCAST)
    {
      linux_bt_l2cap_open(dst, 0x0041, 0x0019);
    }

  snprintf(frame, sizeof(frame),
           "HCI_ACL_DATA handle=%u cid=0x0041 psm=0x0019 "
           "AVDTP_START A2DP_MEDIA codec=SBC payload=BTAU "
           "synthetic-frame",
           handle);

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ACL, dst, frame,
                            strlen(frame));
  if (ret == 0)
    {
      linux_bt_event_push("HCI_EVT_NUM_COMPLETED_PACKETS handle=%u "
                          "count=1 cid=0x0041 profile=a2dp\n",
                          handle);
    }

  return ret;
}

int linux_bt_a2dp_source_start(void)
{
  return linux_bt_a2dp_source_emit(LINUX_BT_HWSIM_DST_BROADCAST, 0x0040);
}

int linux_bt_a2dp_source_start_peer(uint16_t peer)
{
  return linux_bt_a2dp_source_emit(peer, linux_bt_conn_handle(peer));
}

int linux_bt_a2dp_sink_poll(char *out, size_t out_len)
{
  return linux_bt_acl_poll(out, out_len);
}

int linux_bt_le_broadcast_source_create(uint8_t big, uint8_t bis)
{
  struct linux_bt_iso_path *path;

  path = linux_bt_iso_get(big, bis, LINUX_BT_ISO_SOURCE);
  if (path == NULL)
    {
      return -ENOMEM;
    }

  path->state = LINUX_BT_ISO_STREAMING;
  path->handle = linux_bt_iso_handle(big, bis);
  path->codec = "LC3";

  linux_bt_event_push("HCI_EVT_LE_CREATE_BIG_COMPLETE status=0 "
                      "big=%u bis=%u handle=0x%04x codec=%s\n",
                      big, bis, path->handle, path->codec);

  return 0;
}

int linux_bt_le_broadcast_source_stop(uint8_t big, uint8_t bis)
{
  linux_bt_iso_clear(big, bis, LINUX_BT_ISO_SOURCE);
  linux_bt_event_push("HCI_EVT_LE_TERMINATE_BIG_COMPLETE status=0 "
                      "big=%u bis=%u reason=local\n",
                      big, bis);
  return 0;
}

int linux_bt_le_broadcast_source_start_path(uint8_t big, uint8_t bis)
{
  struct linux_bt_iso_path *path;
  char frame[176];
  int ret;

  ret = linux_bt_le_broadcast_source_create(big, bis);
  if (ret < 0)
    {
      return ret;
    }

  path = linux_bt_iso_get(big, bis, LINUX_BT_ISO_SOURCE);
  if (path == NULL)
    {
      return -ENOMEM;
    }

  path->seq++;

  snprintf(frame, sizeof(frame),
           "HCI_ISO_DATA handle=0x%04x BIG=%u BIS=%u seq=%lu "
           "ts=present LE_AUDIO_ISO codec=%s payload=BTAU "
           "synthetic-frame",
           path->handle, big, bis, (unsigned long)path->seq,
           path->codec);

  ret = linux_bt_hwsim_send(LINUX_BT_HWSIM_TYPE_ISO,
                            LINUX_BT_HWSIM_DST_BROADCAST,
                            frame, strlen(frame));
  if (ret == 0)
    {
      linux_bt_event_push("HCI_EVT_LE_ISO_TX_COMPLETE status=0 big=%u "
                          "bis=%u handle=0x%04x seq=%lu\n",
                          big, bis, path->handle,
                          (unsigned long)path->seq);
    }

  return ret;
}

int linux_bt_le_broadcast_source_start(void)
{
  return linux_bt_le_broadcast_source_start_path(0, 1);
}

int linux_bt_le_broadcast_sink_sync(uint8_t big, uint8_t bis)
{
  struct linux_bt_iso_path *path;

  path = linux_bt_iso_get(big, bis, LINUX_BT_ISO_SINK);
  if (path == NULL)
    {
      return -ENOMEM;
    }

  path->state = LINUX_BT_ISO_SYNCING;
  path->handle = linux_bt_iso_handle(big, bis);
  path->codec = "LC3";

  linux_bt_event_push("HCI_EVT_LE_BIG_SYNC_REQUEST big=%u bis=%u "
                      "handle=0x%04x codec=%s\n",
                      big, bis, path->handle, path->codec);

  return 0;
}

int linux_bt_le_broadcast_sink_stop(uint8_t big, uint8_t bis)
{
  linux_bt_iso_clear(big, bis, LINUX_BT_ISO_SINK);
  linux_bt_event_push("HCI_EVT_LE_BIG_SYNC_LOST big=%u bis=%u "
                      "reason=local\n",
                      big, bis);
  return 0;
}

int linux_bt_le_broadcast_sink_poll_path(uint8_t big, uint8_t bis,
                                         char *out, size_t out_len)
{
  struct linux_bt_iso_path *path;
  int ret;

  ret = linux_bt_hwsim_read(LINUX_BT_HWSIM_TYPE_ISO, out, out_len);
  if (ret > 0)
    {
      path = linux_bt_iso_get(big, bis, LINUX_BT_ISO_SINK);
      if (path != NULL)
        {
          path->state = LINUX_BT_ISO_SYNCED;
          path->handle = linux_bt_iso_handle(big, bis);
          path->codec = "LC3";
          path->seq += ret;
        }

      linux_bt_event_push("HCI_EVT_LE_BIG_SYNC_ESTABLISHED status=0 "
                          "big=%u bis=%u records=%d\n",
                          big, bis, ret);
    }

  return ret;
}

int linux_bt_le_broadcast_sink_poll(char *out, size_t out_len)
{
  return linux_bt_le_broadcast_sink_poll_path(0, 1, out, out_len);
}
