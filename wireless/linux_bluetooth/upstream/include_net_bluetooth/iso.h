#ifndef __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_ISO_H
#define __WIRELESS_LINUX_BLUETOOTH_COMPAT_NET_BLUETOOTH_ISO_H

#include <linux/types.h>
#include <linux/kref.h>

#include <net/bluetooth/bluetooth.h>

#define ISO_DEFAULT_MTU 251
#define ISO_MAX_NUM_BIS 0x1f

struct sockaddr_iso_bc
{
  bdaddr_t bc_bdaddr;
  uint8_t bc_bdaddr_type;
  uint8_t bc_sid;
  uint8_t bc_num_bis;
  uint8_t bc_bis[ISO_MAX_NUM_BIS];
};

struct sockaddr_iso
{
  sa_family_t iso_family;
  bdaddr_t iso_bdaddr;
  uint8_t iso_bdaddr_type;
  struct sockaddr_iso_bc iso_bc[];
};

#if defined(CONFIG_SIM_BTHWSIM) && \
    defined(CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_ISO)
struct socket;
int iso_sim_attach_connected(struct socket *sock, uint16_t handle);
int iso_sim_detach_connected(uint16_t handle);
#endif

#endif
