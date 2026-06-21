/****************************************************************************
 * include/nuttx/wireless/linux_bluetooth.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_WIRELESS_LINUX_BLUETOOTH_H
#define __INCLUDE_NUTTX_WIRELESS_LINUX_BLUETOOTH_H

#include <stddef.h>
#include <stdint.h>

#define LINUX_BT_HWSIM_TYPE_CTRL 1
#define LINUX_BT_HWSIM_TYPE_ADV  2
#define LINUX_BT_HWSIM_TYPE_ACL  3
#define LINUX_BT_HWSIM_TYPE_ISO  4
#define LINUX_BT_HWSIM_TYPE_BNEP 5

#define LINUX_BT_HWSIM_DST_BROADCAST 0xffff

#define LINUX_BT_HCI_COMMAND_PKT 0x01
#define LINUX_BT_HCI_ACL_PKT     0x02
#define LINUX_BT_HCI_SCO_PKT     0x03
#define LINUX_BT_HCI_EVENT_PKT   0x04
#define LINUX_BT_HCI_ISO_PKT     0x05
#define LINUX_BT_HCI_VENDOR_PKT  0xff

#define LINUX_BT_MGMT_OP_READ_INFO        0x0004
#define LINUX_BT_MGMT_OP_SET_POWERED      0x0005
#define LINUX_BT_MGMT_OP_SET_DISCOVERABLE 0x0006
#define LINUX_BT_MGMT_OP_SET_CONNECTABLE  0x0007
#define LINUX_BT_MGMT_OP_SET_BONDABLE     0x0009
#define LINUX_BT_MGMT_OP_SET_LE           0x000d
#define LINUX_BT_MGMT_OP_SET_ADVERTISING  0x0029
#define LINUX_BT_MGMT_OP_SET_BREDR        0x002a
#define LINUX_BT_MGMT_INDEX_NONE          0xffff

#define LINUX_BT_BTPROTO_L2CAP 0
#define LINUX_BT_BTPROTO_HCI   1
#define LINUX_BT_BTPROTO_BNEP  4
#define LINUX_BT_BTPROTO_CMTP  5
#define LINUX_BT_BTPROTO_HIDP  6
#define LINUX_BT_BTPROTO_ISO   8

#define LINUX_BT_IOCTL_NRBITS    8
#define LINUX_BT_IOCTL_TYPEBITS  8
#define LINUX_BT_IOCTL_SIZEBITS  14
#define LINUX_BT_IOCTL_DIRBITS   2
#define LINUX_BT_IOCTL_NRSHIFT   0
#define LINUX_BT_IOCTL_TYPESHIFT \
  (LINUX_BT_IOCTL_NRSHIFT + LINUX_BT_IOCTL_NRBITS)
#define LINUX_BT_IOCTL_SIZESHIFT \
  (LINUX_BT_IOCTL_TYPESHIFT + LINUX_BT_IOCTL_TYPEBITS)
#define LINUX_BT_IOCTL_DIRSHIFT \
  (LINUX_BT_IOCTL_SIZESHIFT + LINUX_BT_IOCTL_SIZEBITS)
#define LINUX_BT_IOCTL_WRITE     1U
#define LINUX_BT_IOCTL_READ      2U
#define LINUX_BT_IOCTL(dir, type, nr, size) \
  (((dir) << LINUX_BT_IOCTL_DIRSHIFT) | \
   ((type) << LINUX_BT_IOCTL_TYPESHIFT) | \
   ((nr) << LINUX_BT_IOCTL_NRSHIFT) | \
   ((size) << LINUX_BT_IOCTL_SIZESHIFT))
#define LINUX_BT_IOW(type, nr, size) \
  LINUX_BT_IOCTL(LINUX_BT_IOCTL_WRITE, (type), (nr), sizeof(size))
#define LINUX_BT_IOR(type, nr, size) \
  LINUX_BT_IOCTL(LINUX_BT_IOCTL_READ, (type), (nr), sizeof(size))

#define LINUX_BT_NATIVE_CMTPCONNADD \
  LINUX_BT_IOW('C', 200, int)
#define LINUX_BT_NATIVE_CMTPCONNDEL \
  LINUX_BT_IOW('C', 201, int)
#define LINUX_BT_NATIVE_CMTPGETCONNLIST \
  LINUX_BT_IOR('C', 210, int)
#define LINUX_BT_NATIVE_CMTPGETCONNINFO \
  LINUX_BT_IOR('C', 211, int)
#define LINUX_BT_NATIVE_HIDPCONNADD \
  LINUX_BT_IOW('H', 200, int)
#define LINUX_BT_NATIVE_HIDPCONNDEL \
  LINUX_BT_IOW('H', 201, int)
#define LINUX_BT_NATIVE_HIDPGETCONNLIST \
  LINUX_BT_IOR('H', 210, int)
#define LINUX_BT_NATIVE_HIDPGETCONNINFO \
  LINUX_BT_IOR('H', 211, int)

#define LINUX_BT_BNEP_IOCTL_GETSUPPFEAT 1
#define LINUX_BT_BNEP_IOCTL_GETCONNLIST 2
#define LINUX_BT_BNEP_IOCTL_GETCONNINFO 3
#define LINUX_BT_BNEP_IOCTL_CONNADD     4
#define LINUX_BT_BNEP_IOCTL_CONNDEL     5

#define LINUX_BT_HCI_CHANNEL_RAW     0
#define LINUX_BT_HCI_CHANNEL_USER    1
#define LINUX_BT_HCI_CHANNEL_MONITOR 2
#define LINUX_BT_HCI_CHANNEL_CONTROL 3
#define LINUX_BT_HCI_CHANNEL_LOGGING 4
#define LINUX_BT_HCI_DEV_NONE        0xffff

struct hci_dev;
struct file;
struct socket;

struct linux_bt_sockif_l2cap_fd_state
{
  uint16_t psm;
  uint16_t cid;
  uint16_t handle;
  uint8_t bdaddr_type;
  uint8_t bdaddr[6];
};

int linux_bt_hwsim_send(uint16_t type, uint16_t dst,
                        const void *payload, size_t payload_len);
int linux_bt_hwsim_read(uint16_t type, char *out, size_t out_len);
int linux_bt_hwsim_read_raw(uint16_t type, uint16_t *src, uint16_t *dst,
                            void *payload, uint32_t payload_len,
                            uint32_t *out_len);
int linux_bt_hwsim_read_raw_named(uint16_t type, const char *consumer,
                                  uint16_t *src, uint16_t *dst,
                                  void *payload, uint32_t payload_len,
                                  uint32_t *out_len);
int linux_bt_hwsim_h4_read(const char *path, void *payload,
                           uint32_t payload_len, uint32_t *out_len);
int linux_bt_hwsim_h4_append(const char *path, const void *payload,
                             uint32_t payload_len);

int linux_bt_bnep_hwsim_netdev_register(const char *name,
                                        char *actual_name,
                                        size_t actual_name_len);
void linux_bt_bnep_hwsim_netdev_unregister(void);
int linux_bt_6lowpan_netdev_register(const char *name,
                                     char *actual_name,
                                     size_t actual_name_len);
void linux_bt_6lowpan_netdev_unregister(void);
int linux_bt_6lowpan_recv_l2cap_payload(uint16_t cid, const void *payload,
                                        size_t payload_len);
int linux_bt_6lowpan_status(char *out, size_t out_len);

int linux_bt_upstream_vhci_status(char *out, size_t out_len);
int linux_bt_upstream_vhci_bind_hwsim(void);
int linux_bt_upstream_vhci_open_default(void);
int linux_bt_upstream_vhci_close_default(void);
int linux_bt_upstream_vhci_create_default(uint8_t opcode);
int linux_bt_upstream_vhci_write_default(uint8_t pkt_type,
                                         const void *payload,
                                         size_t payload_len);
int linux_bt_upstream_vhci_read_default(void *payload,
                                        uint32_t payload_len,
                                        uint32_t *out_len);
int linux_bt_upstream_vhci_drain_default(void);
int linux_bt_upstream_vhci_drain_default_trace(size_t max_records,
                                               char *out,
                                               size_t out_len);
int linux_bt_upstream_vhci_drain_default_trace_tee(size_t max_records,
                                                   const char *h4_out,
                                                   char *out,
                                                   size_t out_len);
struct hci_dev *linux_bt_upstream_vhci_default_hdev(void);
int linux_bt_upstream_vhci_recv_frame(uint8_t pkt_type,
                                      const void *payload,
                                      size_t payload_len);
int linux_bt_upstream_vhci_poll_medium(void);
int linux_bt_upstream_hci_status(char *out, size_t out_len);
int linux_bt_upstream_hci_send(uint8_t pkt_type,
                               const void *payload,
                               size_t payload_len);
int linux_bt_upstream_hci_hwsim_pump(unsigned int rounds);
int linux_bt_upstream_hci_connect_br_probe(uint16_t peer,
                                           char *out, size_t out_len);
int linux_bt_upstream_hci_disconnect_br_probe(uint16_t peer,
                                              char *out, size_t out_len);
int linux_bt_upstream_hci_connect_le_probe(uint16_t peer,
                                           char *out, size_t out_len);
int linux_bt_upstream_hci_disconnect_le_probe(uint16_t peer,
                                              char *out, size_t out_len);
int linux_bt_upstream_hci_sim_br_complete(struct hci_dev *hdev,
                                          const void *bdaddr,
                                          uint8_t role,
                                          uint16_t handle);
int linux_bt_upstream_hci_sim_le_complete(struct hci_dev *hdev,
                                          const void *bdaddr,
                                          uint8_t bdaddr_type,
                                          uint8_t role,
                                          uint16_t handle);
int linux_bt_upstream_hci_sim_le_disconnect(struct hci_dev *hdev,
                                            uint16_t handle,
                                            uint8_t reason);
int linux_bt_upstream_hci_peer_le_complete(uint16_t peer,
                                           uint16_t handle);
int linux_bt_upstream_hci_peer_le_disconnect(uint16_t handle,
                                             uint8_t reason);
int linux_bt_upstream_hci_peer_br_complete(uint16_t peer,
                                           uint16_t handle);
int linux_bt_upstream_hci_peer_br_disconnect(uint16_t handle,
                                             uint8_t reason);
int linux_bt_upstream_hci_recv_medium(uint8_t pkt_type,
                                      const void *payload,
                                      size_t payload_len);
int linux_bt_upstream_proto_sock_recv(uint8_t pkt_type,
                                      const void *payload,
                                      size_t payload_len);
int linux_bt_upstream_a2dp_source_sample(void);
int linux_bt_upstream_a2dp_source_sample_peer(uint16_t peer);
int linux_bt_upstream_le_audio_source_sample(uint8_t big, uint8_t bis);
int linux_bt_upstream_af_init(void);
int linux_bt_upstream_af_status(char *out, size_t out_len);
int linux_bt_sockif_l2cap_state_from_fd(
  int fd, struct linux_bt_sockif_l2cap_fd_state *state);
int linux_bt_sockif_l2cap_upstream_from_fd(int fd, void **handle);
int linux_bt_upstream_socket_probe(int proto, char *out, size_t out_len);
int linux_bt_upstream_socket_probe_bind(int proto, int channel, int dev,
                                        char *out, size_t out_len);
int linux_bt_upstream_socket_send_probe(int channel, int dev,
                                        uint8_t pkt_type,
                                        const void *payload,
                                        size_t payload_len,
                                        char *out, size_t out_len);
int linux_bt_upstream_bnep_ioctl_raw(unsigned int cmd, unsigned long arg);
int linux_bt_upstream_bnep_ioctl_probe(int action, uint32_t param,
                                       char *out, size_t out_len);
void linux_bt_upstream_bnep_note_native_ioctl(unsigned int cmd, int ret);
void linux_bt_upstream_bnep_note_native_netdev_register(void);
void linux_bt_upstream_bnep_note_native_netdev_unregister(void);
void linux_bt_upstream_bnep_note_native_netdev_xmit(size_t len);
void linux_bt_upstream_bnep_note_native_tx_frame(size_t len, int ret);
void linux_bt_upstream_bnep_note_native_l2cap_rx(size_t len, int delivered);
void linux_bt_upstream_bnep_note_native_rx_frame(size_t len, int ret);
void linux_bt_upstream_bnep_note_native_netif_rx(size_t len);
void linux_bt_upstream_bnep_note_native_session_create(void);
void linux_bt_upstream_bnep_note_native_session_start(void);
void linux_bt_upstream_bnep_note_native_session_stop(void);
void linux_bt_upstream_bnep_note_native_session_terminate(void);
void linux_bt_upstream_bnep_note_native_netdev_setup(void);
void linux_bt_upstream_bnep_note_native_ndo_start_xmit(void);
void linux_bt_upstream_bnep_note_native_session_link(void);
void linux_bt_upstream_bnep_note_native_session_unlink(void);
void linux_bt_upstream_bnep_note_native_session_thread(void);
void linux_bt_upstream_bnep_note_native_kthread_run(void);
void linux_bt_upstream_bnep_note_native_session_rx_dequeue(void);
void linux_bt_upstream_bnep_note_native_session_tx_dequeue(void);
void linux_bt_upstream_bnep_note_native_fd_handoff(int fd, uint16_t role,
                                                   int err);
int linux_bt_upstream_bnep_drain_pending_l2cap(uint16_t cid,
                                                unsigned int max_packets);
void linux_bt_upstream_bnep_note_native_sock_ioctl_connadd(int err);
void linux_bt_upstream_bnep_note_native_sock_ioctl_conndel(int err);
void linux_bt_upstream_bnep_note_native_sock_ioctl_getconnlist(int err);
void linux_bt_upstream_bnep_note_native_sock_ioctl_getconninfo(int err);
void linux_bt_upstream_bnep_note_native_sock_ioctl_getsuppfeat(int err);
int linux_bt_upstream_hci_filter_probe(int dev, uint32_t type_mask,
                                       uint32_t event_mask0,
                                       uint32_t event_mask1,
                                       uint16_t opcode,
                                       char *out, size_t out_len);
int linux_bt_upstream_hci_ioctl_probe(uint16_t dev, int action,
                                      uint32_t dev_opt,
                                      char *out, size_t out_len);
int linux_bt_upstream_hci_ioctl_raw(unsigned int cmd, unsigned long arg);
int linux_bt_upstream_hci_socket_create(void **handle);
int linux_bt_upstream_hci_socket_open(uint16_t channel, uint16_t dev,
                                      void **handle);
int linux_bt_upstream_hci_socket_ioctl(void *handle, unsigned int cmd,
                                       unsigned long arg);
int linux_bt_upstream_hci_socket_send(void *handle, const void *payload,
                                      size_t payload_len);
int linux_bt_upstream_hci_socket_recv(void *handle, void *payload,
                                      size_t payload_len, int *flags);
int linux_bt_upstream_hci_socket_setsockopt(void *handle, int level,
                                            int option,
                                            const void *value,
                                            unsigned int value_len);
int linux_bt_upstream_hci_socket_getsockopt(void *handle, int level,
                                            int option, void *value,
                                            int *value_len);
int linux_bt_upstream_hci_socket_close(void *handle);
int linux_bt_upstream_l2cap_socket_open(uint16_t psm, uint16_t cid,
                                        uint16_t handle, void **out_handle);
int linux_bt_upstream_l2cap_socket_connect_handle(void *handle,
                                                  uint16_t psm,
                                                  uint16_t cid);
int linux_bt_upstream_l2cap_socket_listen_handle(void *handle,
                                                 int backlog);
int linux_bt_upstream_l2cap_socket_accept_handle(void *handle,
                                                 void **out_handle);
int linux_bt_upstream_l2cap_socket_getsockopt_handle(void *handle,
                                                     int level,
                                                     int optname,
                                                     void *optval,
                                                     int *optlen);
int linux_bt_upstream_l2cap_socket_setsockopt_handle(void *handle,
                                                     int level,
                                                     int optname,
                                                     const void *optval,
                                                     unsigned int optlen);
int linux_bt_upstream_l2cap_socket_option_probe(void *handle,
                                                char *out,
                                                size_t out_len);
int linux_bt_upstream_l2cap_socket_connected_option_probe(void *handle,
                                                          char *out,
                                                          size_t out_len);
int linux_bt_upstream_l2cap_socket_write_handle(void *handle,
                                                const void *payload,
                                                size_t payload_len,
                                                char *out,
                                                size_t out_len);
int linux_bt_upstream_l2cap_socket_recv_handle(void *handle, size_t max_len,
                                               char *out, size_t out_len);
int linux_bt_upstream_l2cap_socket_poll_handle(void *handle, short events,
                                               short *revents);
int linux_bt_upstream_l2cap_socket_shutdown_handle(void *handle, int how);
int linux_bt_upstream_l2cap_socket_ioctl_handle(void *handle,
                                                unsigned int cmd,
                                                unsigned long arg);
int linux_bt_upstream_l2cap_socket_ioctl_probe(void *handle,
                                               char *out,
                                               size_t out_len);
int linux_bt_upstream_l2cap_socket_poll_probe(void *handle,
                                              int want_write,
                                              char *out,
                                              size_t out_len);
int linux_bt_upstream_l2cap_socket_timestamp_probe(void *handle,
                                                   char *out,
                                                   size_t out_len);
int linux_bt_upstream_l2cap_socket_close_handle(void *handle);
int linux_bt_upstream_rfcomm_socket_open(uint8_t channel, uint16_t handle,
                                         void **out_handle);
int linux_bt_upstream_rfcomm_socket_connect_handle(void *handle,
                                                   uint8_t channel);
int linux_bt_upstream_rfcomm_socket_listen_handle(void *handle,
                                                  int backlog);
int linux_bt_upstream_rfcomm_socket_accept_handle(void *handle,
                                                  void **out_handle);
int linux_bt_upstream_rfcomm_socket_getsockopt_handle(void *handle,
                                                      int level,
                                                      int optname,
                                                      void *optval,
                                                      int *optlen);
int linux_bt_upstream_rfcomm_socket_setsockopt_handle(void *handle,
                                                      int level,
                                                      int optname,
                                                      const void *optval,
                                                      unsigned int optlen);
int linux_bt_upstream_rfcomm_socket_listen_accept_probe(uint8_t channel,
                                                        uint16_t handle,
                                                        char *out,
                                                        size_t out_len);
int linux_bt_upstream_rfcomm_socket_write_handle(void *handle,
                                                const void *payload,
                                                size_t payload_len,
                                                char *out,
                                                size_t out_len);
int linux_bt_upstream_rfcomm_socket_recv_handle(void *handle, void *payload,
                                               size_t max_len,
                                               int *msg_flags,
                                               char *out, size_t out_len);
int linux_bt_upstream_rfcomm_socket_poll_handle(void *handle, short events,
                                                short *revents);
int linux_bt_upstream_rfcomm_socket_shutdown_handle(void *handle, int how);
int linux_bt_upstream_rfcomm_socket_ioctl_handle(void *handle,
                                                 unsigned int cmd,
                                                 unsigned long arg);
int linux_bt_upstream_rfcomm_socket_ioctl_probe(void *handle,
                                                char *out,
                                                size_t out_len);
int linux_bt_upstream_rfcomm_socket_poll_probe(void *handle,
                                               int want_write,
                                               char *out,
                                               size_t out_len);
int linux_bt_upstream_rfcomm_socket_timestamp_probe(void *handle,
                                                    char *out,
                                                    size_t out_len);
int linux_bt_upstream_rfcomm_socket_close_handle(void *handle);
int linux_bt_upstream_sco_socket_open(uint16_t handle, void **out_handle);
int linux_bt_upstream_sco_socket_connect_handle(void *handle,
                                                uint16_t handle_id);
int linux_bt_upstream_sco_socket_listen_handle(void *handle, int backlog);
int linux_bt_upstream_sco_socket_accept_handle(void *handle,
                                               void **out_handle);
int linux_bt_upstream_sco_socket_getsockopt_handle(void *handle,
                                                   int level,
                                                   int optname,
                                                   void *optval,
                                                   int *optlen);
int linux_bt_upstream_sco_socket_setsockopt_handle(void *handle,
                                                   int level,
                                                   int optname,
                                                   const void *optval,
                                                   unsigned int optlen);
int linux_bt_upstream_sco_socket_listen_accept_probe(uint16_t handle,
                                                     char *out,
                                                     size_t out_len);
int linux_bt_upstream_sco_socket_write_handle(void *handle,
                                              const void *payload,
                                              size_t payload_len,
                                              char *out,
                                              size_t out_len);
int linux_bt_upstream_sco_socket_recv_handle(void *handle, void *payload,
                                             size_t max_len,
                                             int *msg_flags,
                                             char *out, size_t out_len);
int linux_bt_upstream_sco_socket_poll_handle(void *handle, short events,
                                             short *revents);
int linux_bt_upstream_sco_socket_shutdown_handle(void *handle, int how);
int linux_bt_upstream_sco_socket_ioctl_handle(void *handle,
                                              unsigned int cmd,
                                              unsigned long arg);
int linux_bt_upstream_sco_socket_ioctl_probe(void *handle,
                                             char *out,
                                             size_t out_len);
int linux_bt_upstream_sco_socket_poll_probe(void *handle,
                                            int want_write,
                                            char *out,
                                            size_t out_len);
int linux_bt_upstream_sco_socket_timestamp_probe(void *handle,
                                                 char *out,
                                                 size_t out_len);
int linux_bt_upstream_sco_socket_close_handle(void *handle);
int linux_bt_upstream_cmtp_socket_open(void **out_handle);
int linux_bt_upstream_cmtp_socket_ioctl_handle(void *handle,
                                               unsigned int cmd,
                                               unsigned long arg);
int linux_bt_upstream_cmtp_socket_close_handle(void *handle);
int linux_bt_upstream_hidp_socket_open(void **out_handle);
int linux_bt_upstream_hidp_socket_ioctl_handle(void *handle,
                                               unsigned int cmd,
                                               unsigned long arg);
int linux_bt_upstream_hidp_socket_close_handle(void *handle);
int linux_bt_upstream_cmtp_socket_session_probe(uint16_t handle,
                                                char *out,
                                                size_t out_len);
int linux_bt_upstream_hidp_socket_session_probe(const char *role,
                                                uint16_t handle,
                                                char *out,
                                                size_t out_len);
struct socket *linux_bt_upstream_l2cap_socket_kernel_socket(void *handle);
int linux_bt_upstream_l2cap_socket_mark_bnep_owner(void *handle);
int linux_bt_upstream_l2cap_socket_close_bnep_file(struct file *file);
int linux_bt_upstream_l2cap_socket_send_probe(uint16_t psm, uint16_t cid,
                                              uint16_t handle,
                                              const void *payload,
                                              size_t payload_len,
                                              char *out, size_t out_len);
int linux_bt_upstream_l2cap_socket_bind_probe(uint16_t psm, uint16_t cid,
                                              uint16_t handle,
                                              char *out, size_t out_len);
int linux_bt_upstream_l2cap_socket_connect_probe(uint16_t psm, uint16_t cid,
                                                 char *out, size_t out_len);
int linux_bt_upstream_l2cap_socket_listen_probe(int backlog,
                                                char *out, size_t out_len);
int linux_bt_upstream_l2cap_socket_recv_probe(size_t max_len,
                                              char *out, size_t out_len);
int linux_bt_upstream_l2cap_socket_recv_raw(void *payload, size_t max_len,
                                            size_t *payload_len,
                                            char *out, size_t out_len);
int linux_bt_upstream_l2cap_socket_native_control_probe(int enable,
                                                        char *out,
                                                        size_t out_len);
int linux_bt_upstream_l2cap_socket_write_probe(const void *payload,
                                               size_t payload_len,
                                               char *out, size_t out_len);
int linux_bt_upstream_l2cap_socket_close_probe(char *out, size_t out_len);
int linux_bt_upstream_l2cap_socket_clear_probe(uint16_t psm, uint16_t cid,
                                               uint16_t handle,
                                               char *out, size_t out_len);
int linux_bt_upstream_iso_socket_send_probe(uint8_t addr_type,
                                            uint16_t handle,
                                            const void *payload,
                                            size_t payload_len,
                                            char *out, size_t out_len);
int linux_bt_upstream_iso_socket_open(uint8_t addr_type, uint16_t handle,
                                      void **out_handle);
int linux_bt_upstream_iso_socket_connect_handle(void *handle,
                                                uint8_t addr_type);
int linux_bt_upstream_iso_socket_listen_handle(void *handle, int backlog);
int linux_bt_upstream_iso_socket_accept_handle(void *handle,
                                               void **out_handle);
int linux_bt_upstream_iso_socket_getsockopt_handle(void *handle,
                                                   int level,
                                                   int optname,
                                                   void *optval,
                                                   int *optlen);
int linux_bt_upstream_iso_socket_setsockopt_handle(void *handle,
                                                   int level,
                                                   int optname,
                                                   const void *optval,
                                                   unsigned int optlen);
int linux_bt_upstream_iso_socket_write_handle(void *handle,
                                              const void *payload,
                                              size_t payload_len,
                                              char *out,
                                              size_t out_len);
int linux_bt_upstream_iso_socket_recv_handle(void *handle, void *payload,
                                             size_t max_len, int *flags);
int linux_bt_upstream_iso_socket_poll_handle(void *handle, short events,
                                             short *revents);
int linux_bt_upstream_iso_socket_shutdown_handle(void *handle, int how);
int linux_bt_upstream_iso_socket_ioctl_handle(void *handle,
                                              unsigned int cmd,
                                              unsigned long arg);
int linux_bt_upstream_iso_socket_close_handle(void *handle);
int linux_bt_upstream_iso_socket_bind_probe(uint8_t addr_type,
                                            uint16_t handle,
                                            char *out, size_t out_len);
int linux_bt_upstream_iso_socket_option_probe(uint8_t cig, uint8_t cis,
                                              uint16_t sdu,
                                              char *out, size_t out_len);
int linux_bt_upstream_iso_socket_connect_probe(uint8_t addr_type,
                                               char *out, size_t out_len);
int linux_bt_upstream_iso_socket_listen_probe(int backlog,
                                              char *out, size_t out_len);
int linux_bt_upstream_iso_socket_accept_probe(char *out, size_t out_len);
int linux_bt_upstream_iso_socket_active_probe(void);
int linux_bt_upstream_iso_socket_poll_probe(int want_write,
                                            char *out, size_t out_len);
int linux_bt_upstream_iso_socket_recv_probe(size_t max_len,
                                            char *out, size_t out_len);
int linux_bt_upstream_iso_socket_write_probe(const void *payload,
                                             size_t payload_len,
                                             char *out, size_t out_len);
int linux_bt_upstream_iso_socket_shutdown_probe(int how,
                                                char *out, size_t out_len);
int linux_bt_upstream_iso_socket_ioctl_probe(char *out, size_t out_len);
int linux_bt_upstream_iso_socket_close_probe(char *out, size_t out_len);
int linux_bt_upstream_mgmt_socket_probe(uint16_t opcode, uint16_t index,
                                        uint8_t param, char *out,
                                        size_t out_len);
int linux_bt_upstream_mgmt_listen_probe(char *out, size_t out_len);
int linux_bt_upstream_mgmt_read_probe(size_t max_len,
                                      char *out, size_t out_len);
int linux_bt_upstream_mgmt_send_probe(uint16_t opcode, uint16_t index,
                                      uint8_t param, char *out,
                                      size_t out_len);
int linux_bt_upstream_mgmt_send_raw_probe(const void *payload,
                                          size_t payload_len);
int linux_bt_upstream_mgmt_recv_raw_probe(void *payload, size_t payload_len,
                                          int *flags);
void linux_bt_upstream_mgmt_note_socket_send(const void *payload,
                                             size_t payload_len);
void linux_bt_upstream_mgmt_note_socket_recv(const void *payload,
                                             size_t payload_len);
int linux_bt_upstream_mgmt_close_probe(char *out, size_t out_len);
int linux_bt_upstream_mgmt_poll_discovery_probe(size_t max_records,
                                                char *out, size_t out_len);
int linux_bt_upstream_monitor_listen_probe(char *out, size_t out_len);
int linux_bt_upstream_monitor_recv_raw_probe(void *payload,
                                             size_t payload_len,
                                             int *flags);
int linux_bt_upstream_monitor_close_probe(char *out, size_t out_len);
int linux_bt_upstream_hci_raw_open_probe(uint16_t dev, void **handle);
int linux_bt_upstream_hci_raw_send_probe(void *handle, const void *payload,
                                         size_t payload_len);
int linux_bt_upstream_hci_raw_recv_probe(void *handle, void *payload,
                                         size_t payload_len, int *flags);
int linux_bt_upstream_hci_raw_setsockopt_probe(void *handle, int level,
                                               int option,
                                               const void *value,
                                               unsigned int value_len);
int linux_bt_upstream_hci_raw_getsockopt_probe(void *handle, int level,
                                               int option, void *value,
                                               int *value_len);
int linux_bt_upstream_hci_raw_close_probe(void *handle);

int linux_bt_info(char *out, size_t out_len);
int linux_bt_state(char *out, size_t out_len);
int linux_bt_events(char *out, size_t out_len);
int linux_bt_mgmt_status(char *out, size_t out_len);
int linux_bt_mgmt_set_powered(uint8_t enabled);
int linux_bt_mgmt_set_connectable(uint8_t enabled);
int linux_bt_mgmt_set_discoverable(uint8_t enabled);
int linux_bt_mgmt_set_bondable(uint8_t enabled);
int linux_bt_mgmt_set_le(uint8_t enabled);
int linux_bt_mgmt_set_bredr(uint8_t enabled);
int linux_bt_mgmt_set_advertising(uint8_t enabled);
int linux_bt_mgmt_dispatch(uint16_t opcode, uint8_t param,
                           char *out, size_t out_len);
uint32_t linux_bt_mgmt_settings(void);
int linux_bt_scan_bredr(char *out, size_t out_len);
int linux_bt_scan_le(char *out, size_t out_len);
int linux_bt_ctrl_poll(char *out, size_t out_len);
int linux_bt_advertise_start(void);
int linux_bt_advertise_stop(void);
int linux_bt_connect(uint16_t peer);
int linux_bt_disconnect(uint16_t peer);
int linux_bt_pair(uint16_t peer);
int linux_bt_l2cap_connect(uint16_t peer, uint16_t psm);
int linux_bt_l2cap_disconnect(uint16_t peer, uint16_t cid);
int linux_bt_l2cap_send(uint16_t peer, const char *payload);
int linux_bt_l2cap_echo(uint16_t peer, const char *payload);
int linux_bt_l2cap_ipsp_open(uint16_t cid, uint16_t *peer,
                             uint16_t *handle);
void linux_bt_l2cap_ipsp_close(uint16_t cid);
int linux_bt_l2cap_ipsp_status(uint16_t cid, char *out, size_t out_len);
int linux_bt_acl_poll(char *out, size_t out_len);
int linux_bt_gatt_read(uint16_t handle);
int linux_bt_gatt_read_peer(uint16_t peer, uint16_t handle);
int linux_bt_gatt_write(uint16_t handle, const char *payload);
int linux_bt_gatt_write_peer(uint16_t peer, uint16_t handle,
                             const char *payload);
int linux_bt_a2dp_source_start(void);
int linux_bt_a2dp_source_start_peer(uint16_t peer);
int linux_bt_a2dp_sink_poll(char *out, size_t out_len);
int linux_bt_le_broadcast_source_create(uint8_t big, uint8_t bis);
int linux_bt_le_broadcast_source_stop(uint8_t big, uint8_t bis);
int linux_bt_le_broadcast_source_start(void);
int linux_bt_le_broadcast_source_start_path(uint8_t big, uint8_t bis);
int linux_bt_le_broadcast_sink_sync(uint8_t big, uint8_t bis);
int linux_bt_le_broadcast_sink_stop(uint8_t big, uint8_t bis);
int linux_bt_le_broadcast_sink_poll_path(uint8_t big, uint8_t bis,
                                         char *out, size_t out_len);
int linux_bt_le_broadcast_sink_poll(char *out, size_t out_len);
uint16_t linux_bt_conn_handle(uint16_t peer);

#endif /* __INCLUDE_NUTTX_WIRELESS_LINUX_BLUETOOTH_H */
