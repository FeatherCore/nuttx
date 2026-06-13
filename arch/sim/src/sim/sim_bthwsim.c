/****************************************************************************
 * arch/sim/src/sim/sim_bthwsim.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdint.h>
#include <syslog.h>

#include "sim_internal.h"
#include "sim_hostbthwsim.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int sim_bthwsim_register(int role_id)
{
  int ret;

  ret = host_bthwsim_init(CONFIG_SIM_BTHWSIM_MEDIUM_DIR, role_id,
                          CONFIG_SIM_BTHWSIM_MODE);
  if (ret < 0)
    {
      return ret;
    }

  syslog(LOG_INFO,
         "bthwsim: Linux Bluetooth hwsim skeleton registered role=%d "
         "mode=%s medium=%s\n",
         role_id, CONFIG_SIM_BTHWSIM_MODE, CONFIG_SIM_BTHWSIM_MEDIUM_DIR);

  return 0;
}

int sim_bthwsim_send(uint16_t type, uint16_t dst, const void *payload,
                     uint32_t payload_len)
{
  return host_bthwsim_append(CONFIG_SIM_BTHWSIM_MEDIUM_DIR, type,
                             CONFIG_SIM_BTHWSIM_ROLE, dst, payload,
                             payload_len);
}

int sim_bthwsim_read(uint16_t type, char *out, size_t out_len)
{
  return host_bthwsim_read(CONFIG_SIM_BTHWSIM_MEDIUM_DIR, type,
                           CONFIG_SIM_BTHWSIM_ROLE, out, out_len);
}

int sim_bthwsim_read_raw(uint16_t type, uint16_t *src, uint16_t *dst,
                         void *payload, uint32_t payload_len,
                         uint32_t *out_len)
{
  return host_bthwsim_read_raw(CONFIG_SIM_BTHWSIM_MEDIUM_DIR, type,
                               CONFIG_SIM_BTHWSIM_ROLE, src, dst,
                               payload, payload_len, out_len);
}

int sim_bthwsim_read_raw_named(uint16_t type, const char *consumer,
                               uint16_t *src, uint16_t *dst,
                               void *payload, uint32_t payload_len,
                               uint32_t *out_len)
{
  return host_bthwsim_read_raw_named(CONFIG_SIM_BTHWSIM_MEDIUM_DIR, type,
                                     CONFIG_SIM_BTHWSIM_ROLE, consumer,
                                     src, dst, payload, payload_len,
                                     out_len);
}

int sim_bthwsim_h4_read(const char *path, void *payload,
                        uint32_t payload_len, uint32_t *out_len)
{
  return host_bthwsim_h4_read(CONFIG_SIM_BTHWSIM_MEDIUM_DIR, path,
                              CONFIG_SIM_BTHWSIM_ROLE, payload,
                              payload_len, out_len);
}

int sim_bthwsim_h4_append(const char *path, const void *payload,
                          uint32_t payload_len)
{
  return host_bthwsim_h4_append(CONFIG_SIM_BTHWSIM_MEDIUM_DIR, path,
                                payload, payload_len);
}

int sim_bthwsim_conn_set(uint16_t peer, uint16_t handle,
                         const char *state)
{
  return host_bthwsim_conn_set(CONFIG_SIM_BTHWSIM_MEDIUM_DIR,
                               CONFIG_SIM_BTHWSIM_ROLE, peer, handle,
                               state);
}

int sim_bthwsim_conn_clear(uint16_t peer)
{
  return host_bthwsim_conn_clear(CONFIG_SIM_BTHWSIM_MEDIUM_DIR,
                                 CONFIG_SIM_BTHWSIM_ROLE, peer);
}
