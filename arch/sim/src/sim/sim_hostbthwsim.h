/****************************************************************************
 * arch/sim/src/sim/sim_hostbthwsim.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __ARCH_SIM_SRC_SIM_SIM_HOSTBTHWSIM_H
#define __ARCH_SIM_SRC_SIM_SIM_HOSTBTHWSIM_H

#include <stddef.h>
#include <stdint.h>

int host_bthwsim_init(const char *medium_dir, int role_id,
                      const char *mode);
int host_bthwsim_append(const char *medium_dir, uint16_t type,
                        uint16_t src, uint16_t dst,
                        const void *payload, uint32_t payload_len);
int host_bthwsim_read(const char *medium_dir, uint16_t type,
                      uint16_t self, char *out, size_t out_len);
int host_bthwsim_read_raw(const char *medium_dir, uint16_t type,
                          uint16_t self, uint16_t *src, uint16_t *dst,
                          void *payload, uint32_t payload_len,
                          uint32_t *out_len);
int host_bthwsim_read_raw_named(const char *medium_dir, uint16_t type,
                                uint16_t self, const char *consumer,
                                uint16_t *src, uint16_t *dst,
                                void *payload, uint32_t payload_len,
                                uint32_t *out_len);
int host_bthwsim_h4_read(const char *medium_dir, const char *path,
                         uint16_t self, void *payload,
                         uint32_t payload_len, uint32_t *out_len);
int host_bthwsim_h4_append(const char *medium_dir, const char *path,
                           const void *payload, uint32_t payload_len);
int host_bthwsim_conn_set(const char *medium_dir, uint16_t self,
                          uint16_t peer, uint16_t handle,
                          const char *state);
int host_bthwsim_conn_clear(const char *medium_dir, uint16_t self,
                            uint16_t peer);

#endif /* __ARCH_SIM_SRC_SIM_SIM_HOSTBTHWSIM_H */
