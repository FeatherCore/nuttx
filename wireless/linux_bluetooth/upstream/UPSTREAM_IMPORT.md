# Linux Bluetooth upstream import

This directory is the authoritative upstream-first port area for the
FeatherCore/NuttX Linux Bluetooth work.

Source snapshot:

```text
/home/uan/Feather-develop-BT/third/linux-hwe-6.17-6.17.0
```

Imported Linux host stack files:

```text
net/bluetooth/af_bluetooth.c
net/bluetooth/hci_core.c
net/bluetooth/hci_conn.c
net/bluetooth/hci_event.c
net/bluetooth/hci_sock.c
net/bluetooth/hci_sync.c
net/bluetooth/iso.c
net/bluetooth/l2cap_core.c
net/bluetooth/l2cap_sock.c
net/bluetooth/mgmt.c
net/bluetooth/mgmt_config.c
net/bluetooth/mgmt_config.h
net/bluetooth/mgmt_util.c
net/bluetooth/mgmt_util.h
net/bluetooth/smp.c
net/bluetooth/smp.h
net/bluetooth/eir.c
net/bluetooth/eir.h
net/bluetooth/lib.c
net/bluetooth/hci_codec.c
net/bluetooth/hci_codec.h
net/bluetooth/6lowpan.c
net/6lowpan/core.c
net/6lowpan/iphc.c
net/6lowpan/ndisc.c
net/6lowpan/nhc.c
net/6lowpan/nhc.h
net/6lowpan/nhc_dest.c
net/6lowpan/nhc_fragment.c
net/6lowpan/nhc_hop.c
net/6lowpan/nhc_ipv6.c
net/6lowpan/nhc_mobility.c
net/6lowpan/nhc_routing.c
net/6lowpan/nhc_udp.c
include/net/6lowpan.h
```

The Bluetooth 6LoWPAN import is intentionally guarded by
`CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN` and remains disabled by
default.  It is the required upstream path for BLE IP networking validation,
but it still needs Linux IPv6 route/neighbour, lowpan_dev, LE L2CAP CoC
callback and net_device compatibility work before `BLE1 <-> BLE2` IPv6 ping
can be considered a Linux-semantics result.

Imported Linux virtual controller driver:

```text
drivers/bluetooth/hci_vhci.c
```

This is the Bluetooth-side upstream counterpart to the Wi-Fi port's imported
`drivers/wireless/virtual/mac80211_hwsim_linux.c`.  The expected NuttX shape is
the same: keep the upstream Linux driver source close to original, provide
compatibility for Linux kernel primitives, and add only a small NuttX-facing
initialize/bind layer.

NuttX build ownership follows the same Linux split:

```text
Linux net/bluetooth      -> FeatherCore/nuttx/wireless/linux_bluetooth
Linux drivers/bluetooth  -> FeatherCore/nuttx/drivers/bluetooth
Linux/BlueZ-like tools   -> FeatherCore/apps/wireless/linux_bluetooth
```

The staged upstream copy of `drivers/bluetooth/hci_vhci.c` lives here for
source tracking, but the file is built from `nuttx/drivers/bluetooth` so driver
code does not drift into the host stack directory.

The local VHCI open/read/write/pump bridge is also driver-owned:

```text
FeatherCore/nuttx/drivers/bluetooth/linux_bt_upstream_vhci.c
```

Imported Linux Bluetooth headers:

```text
include/net/bluetooth/bluetooth.h
include/net/bluetooth/hci.h
include/net/bluetooth/hci_core.h
include/net/bluetooth/hci_mon.h
include/net/bluetooth/hci_sock.h
include/net/bluetooth/hci_sync.h
include/net/bluetooth/iso.h
include/net/bluetooth/l2cap.h
include/net/bluetooth/mgmt.h
include/net/bluetooth/sco.h
```

Current native-facing socket parity surfaces:

- `BTPROTO_RFCOMM`: registered from `linux_bt_upstream_af_init()` and used by
  HFP/HSP closeout as an RFCOMM socket boundary.
- `BTPROTO_SCO`: registered from `linux_bt_upstream_af_init()` and used by
  HFP/HSP closeout as a SCO audio socket boundary.
- `BTPROTO_CMTP`: registered from `linux_bt_upstream_af_init()` and used by
  the HCI/mgmt/socket closeout for `CMTPCONNADD`, `CMTPGETCONNLIST`,
  `CMTPGETCONNINFO`, and `CMTPCONNDEL` lifecycle evidence.
- `BTPROTO_HIDP`: registered from `linux_bt_upstream_af_init()` and used by
  Classic HID closeout for `HIDPCONNADD`, `HIDPGETCONNLIST`,
  `HIDPGETCONNINFO`, and `HIDPCONNDEL` lifecycle evidence.

These surfaces are intentionally native-facing socket parity layers.  They
close the Linux socket/API observation gaps required by current hwsim gates, but
they do not yet replace the full upstream Linux `rfcomm/core.c`, `sco.c`, or
`cmtp/core.c` / `hidp/core.c` session implementations.

Configuration note:

- `CONFIG_NET_LINUX_BLUETOOTH_RFCOMM_SOCKET_PARITY` and
  `CONFIG_NET_LINUX_BLUETOOTH_SCO_SOCKET_PARITY` are the active build gates.
- The old `*_SOCKET_TRANSITIONAL` Kconfig symbols are deprecated aliases kept
  only to select the new parity symbols for existing local configurations.

Porting rule:

- Prefer adapting these upstream files over reimplementing behavior in
  `linux_bt_core.c`.
- Keep upstream files as close to Linux as possible.
- Reuse the Wi-Fi/mac80211 Linux compatibility layer first.  The Bluetooth
  Make/CMake paths include `wireless/ieee80211/include` as a fallback after
  Bluetooth-local compat headers, so already-ported generic Linux APIs should
  not be reimplemented under `linux_bluetooth/upstream/compat`.
- Add NuttX/Linux compatibility in a separate compat layer rather than editing
  protocol logic first.  Bluetooth-local compat should cover Bluetooth-specific
  wrappers or gaps not already provided by the IEEE80211/mac80211 Linux compat
  layer.
- Use the current `linux_bt_core.c` only as the temporary NuttX sim/hwsim
  bring-up shim until the upstream HCI, mgmt, L2CAP, SMP, ATT/GATT, ISO paths
  are compilable.
- When an upstream source must be changed, document the reason and keep the
  semantic behavior aligned with Linux.

Initial compat areas expected:

```text
linux/list.h, linux/skbuff.h, linux/socket.h, linux/mutex.h, linux/workqueue.h
linux/refcount.h, linux/atomic.h, linux/errno.h, linux/bitops.h
net/sock.h, net/bluetooth/* include layout, kernel module/init annotations
```

Current compat skeleton:

```text
shared generic compat: wireless/linux_compat/include/linux/*.h
legacy Wi-Fi fallback compat: wireless/ieee80211/include/linux/*.h
compat/include/linux/types.h
compat/include/linux/kernel.h
compat/include/linux/export.h
compat/include/linux/err.h
compat/include/linux/slab.h
compat/include/linux/list.h
compat/include/linux/skbuff.h
compat/include/linux/spinlock.h
compat/include/linux/mutex.h
compat/include/linux/atomic.h
compat/include/linux/refcount.h
compat/include/linux/workqueue.h
compat/include/linux/wait.h
compat/include/linux/idr.h
compat/include/linux/rculist.h
compat/include/linux/srcu.h
compat/include/linux/leds.h
compat/include/linux/module.h
compat/include/linux/init.h
compat/include/linux/sched.h
compat/include/linux/sched/signal.h
compat/include/linux/fs.h
compat/include/linux/miscdevice.h
compat/include/linux/debugfs.h
compat/include/linux/proc_fs.h
compat/include/linux/stringify.h
compat/include/linux/sockios.h
compat/include/linux/unaligned.h
compat/include/linux/string.h
compat/include/linux/bitops.h
compat/include/linux/errno.h
compat/include/linux/socket.h
compat/include/linux/poll.h
compat/include/linux/seq_file.h
compat/include/linux/ethtool.h
compat/include/asm/ioctls.h
compat/include/net/sock.h
compat/include/net/bluetooth/*.h -> upstream/include_net_bluetooth/*.h
```

The socket compat layer now includes the first HCI control-channel send/receive
path:
`struct msghdr` carries `msg_iter`, `struct proto_ops` includes Linux-shaped
`bind/sendmsg/recvmsg/ioctl/poll` hooks, `copy_to_iter()` supports socket read
side copies, and `sk_alloc()` uses
`proto->obj_size` so Bluetooth protocol sockets can allocate their private
`bt_sock`-derived state.  `linux/skbuff.h` also has the extra skb helpers needed
by this stage, including `skb_tailroom_reserve()`.

Important de-duplication rule: many generic Linux headers in
`linux_bluetooth/upstream/compat/include/linux` overlap with
`wireless/ieee80211/include/linux`.  They are temporary staging headers from the
early Bluetooth bring-up.  Before adding new declarations to a Bluetooth-local
generic `linux/*.h`, check the IEEE80211/mac80211 compat version first and
prefer reusing or moving the common helper there.  Bluetooth-local headers
should trend toward only:

```text
net/bluetooth/*.h wrappers
net/sock.h Bluetooth socket gaps until shared socket compat exists
driver-local VHCI/miscdevice gaps that are not already shared
```

Safe migrated duplicates currently are `linux/leds.h`, `linux/socket.h`,
`linux/bitops.h`, `linux/unaligned.h`, `linux/ethtool.h`, `linux/list.h` and
`linux/workqueue.h`.
Bluetooth
forwards `leds` and `socket` to the IEEE80211/mac80211 compat headers without
pulling in Wi-Fi-only socket/skbuff state.  `bitops`, `unaligned` and
`ethtool` have been split into the new generic
`wireless/linux_compat/include/linux` layer, and both BT and Wi-Fi wrappers
forward there.  `list` has also been split into that layer; Wi-Fi's monolithic
compat still carries its historical copy but sets shared list guard macros so
the shared header can be included afterward without duplicate definitions.  The
shared ethtool header now owns both Wi-Fi ethtool structs and Bluetooth's AF
socket timestamp-info structs.
`workqueue` is also split into the shared layer.  Wi-Fi still keeps a few
historical queue helpers in `cfg80211_compat.h`, so the shared workqueue header
uses fine-grained guards instead of a single broad "misc helper" guard.

Do not blindly forward Bluetooth's `linux/skbuff.h` yet.  The corresponding
Wi-Fi header can include the monolithic `cfg80211_compat.h`, which currently
defines Wi-Fi-specific `struct sk_buff` and dummy `struct sock`; those collide
with Bluetooth's `net/sock.h` and HCI `sk_buff` staging.  Forwarding that
header should wait until the shared layer is split or guarded.

Current build integration:

```text
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_SOURCES=n by default
Make.defs/CMakeLists.txt add compat/include when the option is enabled
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_LIB selects lib.c
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_EIR selects eir.c
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CODEC selects hci_codec.c
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF selects af_bluetooth.c
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CORE selects hci_core.c, hci_conn.c,
hci_event.c and hci_sync.c
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_SOCK selects hci_sock.c
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_MGMT selects mgmt.c, mgmt_config.c and
mgmt_util.c
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_L2CAP selects l2cap_core.c and
l2cap_sock.c
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_SMP selects smp.c
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_ISO selects iso.c
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI enables the VHCI port
CONFIG_DRIVERS_BLUETOOTH_LINUX_HCI_VHCI builds hci_vhci.c from drivers/bluetooth
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI_AUTOSTART opens hci_vhci
after SIM_BTHWSIM bring-up
```

The four sim hwsim defconfigs (`hwsim_bt1`, `hwsim_bt2`, `hwsim_ble1`,
`hwsim_ble2`) explicitly enable the current upstream-safe subset:

```text
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_SOURCES=y
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_LIB=y
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF=y
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI=y
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI_AUTOSTART=y
CONFIG_DRIVERS_BLUETOOTH_LINUX=y
CONFIG_DRIVERS_BLUETOOTH_LINUX_HCI_VHCI=y
```

Important: the first listed upstream files are not a claim that all required
Linux primitives are complete.  They are the first compile targets for the
compat layer.  Expected next blockers are `struct hci_dev`, HCI device
registration, kernel workqueue/list locking, endian helpers, and socket glue.

Current `lib.c` stage boundary:

- `baswap()`, `bt_to_errno()`, `bt_status()` and Bluetooth logging helpers stay
  in upstream `net_bluetooth/lib.c`.
- compat provides declaration-level socket/skbuff support only so
  `bluetooth.h` can be parsed.
- full `struct sock`, socket wakeups and socket ownership are not complete yet
  and must be completed before enabling upstream socket-heavy files.

Current `af_bluetooth.c` stage:

- `CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF` is enabled by the four hwsim
  defconfigs.
- `sim_bringup()` calls `linux_bt_upstream_af_init()` before opening VHCI, so
  upstream `subsys_initcall(bt_init)` registers `PF_BLUETOOTH` first.
- `linux_bt_upstream_af_status()` reports the compat socket-family registry
  through `btctl upstream`, including whether `PF_BLUETOOTH` is registered and
  the register/unregister counters.
- `linux_bt_upstream_socket_probe()` is the first userspace-to-AF probe for
  the BlueZ path.  `btctl upstream socket hci|l2cap|iso` calls the registered
  upstream `PF_BLUETOOTH` family `create()` function, so the observable failure
  moves from "family is missing" to a per-protocol socket boundary.  HCI,
  L2CAP and ISO now have guarded staging registrations until their upstream
  socket files can take over completely.
- L2CAP staging socket ops now include framework-level `getname`,
  `setsockopt`, `getsockopt` and `accept` hooks.  These hooks use the existing
  `linux_bt_l2cap_bind_state` and expose the Linux/BlueZ-facing
  `SOL_L2CAP` option ABI (`L2CAP_OPTIONS`, `L2CAP_LM`, `L2CAP_CONNINFO`) plus
  the currently represented `SOL_BLUETOOTH` socket options (`BT_SECURITY`,
  `BT_DEFER_SETUP`, `BT_MODE`, `BT_SNDMTU`, `BT_RCVMTU`).
- BlueZ daemon-owned A2DP closeout now calls the L2CAP option probe on real
  opened L2CAP socket handles before connect.  The hwsim validator requires
  `L2CAP_OPTIONS` and `L2CAP_LM` set/get evidence in
  `bluez-a2dp-current-complete-closeout`, so L2CAP option parity is no longer
  only an unvalidated helper path.
- ISO staging socket ops now include framework-level `getname`, `listen`,
  `accept`, `setsockopt` and `getsockopt` hooks for the LE Audio hwsim
  observation path.  `BT_ISO_QOS` and `BT_ISO_BASE` are stateful on the bound
  ISO socket and are verified through the BlueZ-shaped `bind-cis` ->
  `apply-qos` lifecycle; unsupported or unbound ISO option probes are not
  accepted as parity evidence.
- `rfcomm_sock_init()` now registers a native-facing transitional
  `BTPROTO_RFCOMM` socket family.  This gives BlueZ/profile tests a real
  `AF_BLUETOOTH/BTPROTO_RFCOMM` create/bind/connect/listen/sendmsg/recvmsg
  observation point instead of only bounded RFCOMM-over-L2CAP profile markers.
  The bearer is still a minimal RFCOMM UIH-shaped frame over the existing
  hwsim L2CAP path.
- `linux_bt_upstream_rfcomm_socket_*_handle()` now exposes that RFCOMM socket
  family to profile closeout helpers.  `bluezhfp closeout` uses it for HFP/HSP
  HF/AG roles, and the hwsim validator requires `proto=BTPROTO_RFCOMM` evidence
  before accepting the HFP/HSP RFCOMM path.
- Full DLC/session, credit flow, PN/MSC/RPN, tty and accept queue parity must
  still come from importing/adapting Linux `rfcomm/core.c` and
  `rfcomm/sock.c`.
- `sco_init()` now registers a native-facing transitional `BTPROTO_SCO` socket
  family.  This gives HFP/HSP audio closeout a real
  `AF_BLUETOOTH/BTPROTO_SCO` create/bind/connect/sendmsg/recvmsg observation
  point instead of direct HCI SCO packet injection.
- `linux_bt_upstream_sco_socket_*_handle()` exposes that SCO socket family to
  profile closeout helpers.  `bluezhfp closeout` uses it for HFP/HSP HF/AG
  roles, and the hwsim validator requires `proto=BTPROTO_SCO` evidence before
  accepting the HFP/HSP audio bearer path.
- Full SCO/eSCO connection manager, incoming accept/defer setup, codec/offload
  policy and disconnect/error lifecycle parity must still come from
  importing/adapting Linux `sco.c`.
- `hci_sock_init()` now has a guarded staging registration when full upstream
  `hci_sock.c` is not enabled.  It registers `BTPROTO_HCI` through upstream
  `proto_register()` and `bt_sock_register()`, then allocates sockets through
  upstream `bt_sock_alloc()`, so `btctl upstream socket hci` reaches a Linux
  AF_BLUETOOTH/HCI create boundary.  This is not a replacement for upstream
  `hci_sock.c`; it is the temporary hand-off point until
  `CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_SOCK` can be enabled.
- `l2cap_init()` now has the matching guarded staging registration while full
  upstream L2CAP is disabled.  It registers `BTPROTO_L2CAP`, allocates
  `struct l2cap_pinfo` through upstream `bt_sock_alloc()`, initializes the
  `rx_busy` list, and accepts an imported upstream `struct sockaddr_l2` bind probe so the
  socket reaches `BT_BOUND` with recorded local PSM/CID.  Connect/send/recv
  intentionally remain unsupported until upstream `l2cap_sock.c` and
  `l2cap_core.c` replace the temporary boundary.
- `iso_init()` now has the matching guarded staging registration while full
  upstream ISO is disabled.  `linux_bt_upstream_af_init()` calls it after
  upstream `bt_init()` because the imported `af_bluetooth.c` does not call
  `iso_init()` in this tree.  The staging path registers `BTPROTO_ISO`,
  allocates an ISO-private `bt_sock` wrapper through upstream
  `bt_sock_alloc()`, accepts an imported upstream `struct sockaddr_iso` bind
  probe, and moves the socket to `BT_BOUND`.  Connect/send/recv intentionally
  remain unsupported until upstream `iso.c` replaces the temporary boundary.
- The user-space socket probe now chooses a Linux-like default socket type per
  protocol: HCI remains `SOCK_RAW`, while L2CAP and ISO use `SOCK_SEQPACKET`
  when that type is available.
- `btctl upstream iso-send <addr-type> <handle> <hex...>` is the first ISO
  socket data-plane staging probe.  It creates and binds a `BTPROTO_ISO`
  socket, injects the supplied handle into the temporary ISO private object,
  sends the user payload through the socket `sendmsg()` op, wraps it as an HCI
  ISO data packet, and hands it to the upstream VHCI send path.  This advances
  the BlueZ ISO socket -> kernel -> VHCI direction while leaving real
  connection/session semantics to upstream `iso.c`.
- `btctl upstream l2cap-send <psm> <cid> <handle> <hex...>` is the matching
  L2CAP socket data-plane staging probe.  It creates and binds a
  `BTPROTO_L2CAP` socket, injects the supplied ACL handle into the temporary
  bind state, sends the user payload through the socket `sendmsg()` op, wraps
  it as HCI ACL data with an L2CAP basic header, and hands it to the upstream
  VHCI send path.  This advances the BlueZ L2CAP/A2DP socket -> kernel -> VHCI
  direction while leaving real connection/session/CID semantics to upstream
  `l2cap_sock.c` and `l2cap_core.c`.
- Controller-to-host protocol socket receive fanout is now staged too.
  `hci_recv_frame()` calls `linux_bt_upstream_proto_sock_recv()` after HCI raw
  socket fanout.  ACL payloads are parsed as HCI ACL + L2CAP basic header and
  the L2CAP payload is queued into matching bound L2CAP sockets by CID.  ISO
  payloads are parsed as HCI ISO data and the SDU payload is queued into bound
  ISO sockets.  This is only a socket receive queue bridge; upstream L2CAP/ISO
  must replace it with real reassembly, flow control and connection ownership.
- Persistent nsh-facing protocol socket probes are available for the sim model:
  `btctl upstream l2cap-bind <psm> <cid> <handle>` and `btctl upstream
  iso-bind <addr-type> <handle>` keep one staging socket alive in the current
  sim instance, while `l2cap-recv [max]` and `iso-recv [max]` read queued
  payloads.  This enables AP/STA-style two-terminal hwsim experiments before
  full BlueZ-owned sockets are wired through.
- `btctl upstream l2cap-write <hex...>` and `btctl upstream iso-write
  <hex...>` reuse those persistent sockets for repeated sendmsg calls, so the
  staging flow now models a longer-lived BlueZ protocol socket more closely
  than the one-shot `l2cap-send`/`iso-send` helpers.
- `btctl upstream l2cap-connect <psm> <cid>` and `btctl upstream iso-connect
  <addr-type>` route through staging protocol `connect` ops and advance kept
  sockets from `BT_BOUND` to `BT_CONNECTED`.  They do not establish real
  controller links yet; they only make the socket lifecycle observable in the
  same order Linux/BlueZ expects.
- `btctl upstream l2cap-listen [backlog]` routes through the staging L2CAP
  `listen` op and advances the kept socket to `BT_LISTEN`.  ACL/L2CAP fanout
  treats `BT_LISTEN` sockets as valid receive targets, letting a receiver nsh
  terminal model server-side profile/media CID intake before upstream
  accept/channel ownership is enabled.
- `btctl upstream l2cap-close` and `btctl upstream iso-close` explicitly call
  the staging socket release op and clear the persistent probe state.  This
  keeps the hwsim test lifecycle aligned with Linux socket close/release even
  before BlueZ owns the descriptors directly.
- `struct proto` compat now carries Linux's `owner` and `obj_size` fields, and
  `sk_alloc()` honors `proto->obj_size`.  This is required because Linux
  Bluetooth sockets embed `struct sock` at the start of larger objects such as
  `struct bt_sock` and `struct hci_pinfo`.
- `struct sock` compat now has the first lifecycle callback fields
  (`sk_destruct`, `sk_state_change`, `sk_data_ready`, `sk_write_space`) and
  `sk_free()`/`sock_put()` call the destructor before freeing.  The proto
  register/unregister stubs also expose counters in `btctl upstream`.
- `hci_sock.c` include preparation advanced: upstream `hci_mon.h` was imported
  from the Linux source tree, Bluetooth-local `net/bluetooth/hci_mon.h` now
  forwards to that imported header, `linux/compat.h` provides `compat_ptr()`,
  and `linux/utsname.h` was moved into the shared `wireless/linux_compat`
  include layer with both `release` and `machine` fields.
- HCI device runtime compat advanced for `hci_sock.c`: the VHCI-stage
  `struct hci_dev` wrapper now carries Linux-like `flags`, `dev_flags` and
  `promisc`; `hci_dev_get()`, `hci_dev_put()`, `hci_dev_hold()`,
  `hci_dev_{set,clear,test,test_and_set}_flag()`, `hci_dev_lock()` and
  `hci_dev_unlock()` are available on top of the shared VHCI registry.
  `hci_dev_open()` and `hci_dev_do_close()` currently update the `HCI_UP`
  bit only; full power/open/close semantics still belong to upstream
  `hci_core.c`.
- Socket runtime compat advanced for `hci_sock.c`: `struct socket` and
  `struct sock` now expose the first `proto_ops` and queue/lifecycle fields,
  including `sk_write_queue`, `sock_orphan()`, `sock_queue_rcv_skb()`,
  `datagram_poll()` and the standard `sock_no_*` operation helpers.  This is
  generic socket scaffolding for upstream HCI/L2CAP/ISO sockets, not
  Bluetooth protocol reimplementation.
- The guarded HCI staging create path now mirrors more of upstream
  `hci_sock_create()`: it assigns a `proto_ops` table, sets
  `sock->state = SS_UNCONNECTED`, and exposes a release op that drops the
  allocated `bt_sock`.  `btctl upstream socket hci` reports `state`, `ops`,
  `sk-state` and `sk-proto`, making the userspace-to-HCI socket boundary
  observable from nsh.
- The guarded HCI staging path now also has a minimal `bind` op and a
  HCI-private socket object shape.  Its private object starts with
  `struct bt_sock`, carries `hdev`, `channel` and flags fields, and supports
  `btctl upstream socket hci raw|user|monitor|control|logging [dev]`.  RAW and
  USER bind to a concrete HCI index, while CONTROL/MONITOR/LOGGING bind to
  `HCI_DEV_NONE`, matching the channel split used by upstream `hci_sock_bind()`.
  Bind failures are returned by the nsh probe so channel/device gaps remain
  visible.
- The monitor/logging side of that staging HCI socket now has a first Linux
  shape too.  `HCI_CHANNEL_MONITOR` sockets are tracked in a local monitor
  list, control socket open/close, mgmt command and mgmt event records are
  broadcast as `hci_mon_hdr` packets with `HCI_MON_CTRL_OPEN`,
  `HCI_MON_CTRL_CLOSE`, `HCI_MON_CTRL_COMMAND` and `HCI_MON_CTRL_EVENT`, and
  `HCI_CHANNEL_LOGGING` validates user logging frames before broadcasting
  `HCI_MON_USER_LOGGING`.  `btctl upstream` reports monitor register,
  unregister and event counters.  Full monitor replay, filtering and timestamp
  semantics still belong to upstream `hci_sock.c`.
- RAW/USER HCI socket TX now has a first Linux-shaped path.  The staging
  `sendmsg` op accepts H4-shaped data for `HCI_CHANNEL_RAW` and
  `HCI_CHANNEL_USER`: byte 0 is the packet type, remaining bytes are the HCI
  payload.  It requires a bound `hci_dev`, checks `HCI_UP`, validates the packet
  type and calls the bound controller's `hdev->send()` callback.  `btctl
  upstream socket-send raw 0 cmd 03 0c 00` exercises that path with HCI Reset;
  `MGMT_OP_SET_POWERED` through the mgmt socket now opens/closes the staging
  `hci_dev`, matching the Linux expectation that RAW/USER TX requires an up
  controller.
- RAW/USER HCI socket RX fanout now starts at the `hci_recv_frame()` boundary.
  After controller/medium -> host accounting, staging code queues H4-shaped
  packets into matching RAW/USER socket receive queues: RAW receives
  command/event/ACL/SCO/ISO and USER receives event/ACL/SCO/ISO/DRV.  RX frames
  are also mirrored to monitor sockets as `HCI_MON_*_RX_PKT` events.  The
  `linux_bt_upstream_hci_sock_recv()` symbol has a no-op fallback when
  `CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_SOCK=y` or upstream AF is disabled,
  so later enabling real upstream `hci_sock.c` can replace the staging fanout
  without breaking `hci_recv_frame()`.
- RAW HCI socket filter compatibility now has a first staging implementation.
  The HCI socket private object carries upstream-shaped `struct hci_filter` and
  `cmsg_mask` fields; `setsockopt(SOL_HCI, HCI_FILTER)` and
  `getsockopt(SOL_HCI, HCI_FILTER)` work on `HCI_CHANNEL_RAW`, and
  controller-to-host RAW fanout applies type/event/opcode filtering before
  queueing H4 data.  `btctl upstream socket-filter <dev> <type-mask>
  <event-mask0> <event-mask1> [opcode]` exercises the same proto_ops boundary.
- HCI socket `getname` is staged too.  Bound HCI sockets now implement
  `proto_ops.getname(peer=0)` and return a Linux `sockaddr_hci` containing
  `AF_BLUETOOTH`, the bound `hci_dev` id and the HCI channel; peer getname
  remains `-EOPNOTSUPP`, matching upstream `hci_sock_getname()`.
- HCI socket ioctl staging now covers the first BlueZ/tool controller
  enumeration and state-control path.  `HCIGETDEVLIST` returns registered hci
  ids and flags in Linux `hci_dev_list_req` shape, while `HCIGETDEVINFO` fills
  Linux `hci_dev_info` with id, name, type, flags and available TX/RX
  statistics.  `HCIDEVUP`, `HCIDEVDOWN` and `HCIDEVRESET` now drive the staging
  `hci_dev` up/down/reset state.  `HCIDEVRESTAT` resets the staging stats, and
  `HCISETSCAN`, `HCISETAUTH`, `HCISETENCRYPT`, `HCISETPTYPE`,
  `HCISETLINKPOL`, `HCISETLINKMODE`, `HCISETACLMTU` and `HCISETSCOMTU` record
  the requested values in staging `hci_dev` fields so `HCIGETDEVINFO` can
  expose them.  `HCIGETCONNLIST` and `HCIGETCONNINFO` now also use Linux
  `hci_conn_list_req` and `hci_conn_info_req` ABI shapes; the temporary rows
  now come from a small hci-conn-like staging snapshot table that L2CAP/ISO
  bind, connect, listen, handle assignment and release paths update.  This
  keeps HCI ioctl code from depending directly on protocol socket internals
  while BlueZ-facing tools observe ACL/LE/BIS/CIS-style connection queries
  before the real upstream `hci_conn_hash` is active.  Bound HCI control
  sockets are tracked as staging mgmt event listeners; connected snapshot
  transitions enqueue Linux-shaped `MGMT_EV_DEVICE_CONNECTED`, while release
  or non-connected transitions enqueue `MGMT_EV_DEVICE_DISCONNECTED`.  The
  connected event currently carries empty EIR data and is a bridge toward
  upstream `mgmt_device_connected()`.  `btctl upstream mgmt-listen` keeps one
  control socket bound so `btctl upstream mgmt-read [max]` can drain queued
  asynchronous connection and block-list mgmt events before
  `btctl upstream mgmt-close` releases it.
  `HCIGETAUTHINFO` returns staging
  `auth_enable` for a matching ACL connection, while `HCIBLOCKADDR` and
  `HCIUNBLOCKADDR` maintain a small staging reject-address table that mirrors
  the shape of upstream `hci_sock_reject_list_add/del()` and emit
  Linux-shaped `MGMT_EV_DEVICE_BLOCKED` / `MGMT_EV_DEVICE_UNBLOCKED` events
  on successful changes.  `MGMT_OP_BLOCK_DEVICE` and
  `MGMT_OP_UNBLOCK_DEVICE` now use the same staging reject-address table and
  command-complete/event behavior through the HCI control channel; the
  `btctl upstream mgmt-socket 0x0026 0 1` and `0x0027 0 1` probes build a
  synthetic BR/EDR `mgmt_addr_info` payload for this path.
  `MGMT_OP_START_DISCOVERY` and `MGMT_OP_STOP_DISCOVERY` now maintain a small
  staging discovery state and emit Linux-shaped `MGMT_EV_DISCOVERING`; the
  `btctl upstream mgmt-socket 0x0023 0 7` and `0x0024 0 7` probes build the
  matching one-byte discovery type payload.  `MGMT_EV_DEVICE_FOUND` is also
  staged from the hwsim ADV raw medium through `btctl upstream
  mgmt-poll-discovery [max]`, synthesizing a stable LE public address from the
  peer sim role and Complete Local Name EIR from the ADV payload.
  `MGMT_OP_PAIR_DEVICE` and `MGMT_OP_CANCEL_PAIR_DEVICE` are now staged with
  Linux request/response shapes; `0x0019 0 <addr-seed>` creates or updates the
  matching synthetic LE device-list entry, stores the IO capability, and marks
  it paired, while `0x001a 0 <addr-seed>` exercises cancel-pair and normally
  reports invalid params until a real pending pair state exists.
  `MGMT_OP_UNPAIR_DEVICE` is also staged: `0x001b 0 <addr-seed>` clears the
  paired state, optionally disconnects a matching staging connection snapshot,
  returns Linux `mgmt_rp_unpair_device`, and emits
  `MGMT_EV_DEVICE_UNPAIRED` to other control sockets.  `MGMT_OP_GET_CONN_INFO`
  and `MGMT_OP_GET_CLOCK_INFO` now query the hci-conn-like staging snapshot
  and return Linux `mgmt_rp_get_conn_info` / `mgmt_rp_get_clock_info` response
  shapes with staging RSSI/TX-power and clock values; the `btctl upstream
  mgmt-socket 0x0031 0 <addr-seed>` and `0x0032 0 <addr-seed>` probes build
  the matching `mgmt_addr_info` payloads.  `MGMT_OP_ADD_DEVICE` and
  `MGMT_OP_REMOVE_DEVICE` now maintain a small staging device list and emit
  Linux-shaped `MGMT_EV_DEVICE_ADDED` / `MGMT_EV_DEVICE_REMOVED` events; the
  `btctl upstream mgmt-socket 0x0033 0 <addr-seed>` probe uses action `0x01`
  and `0x0034 0 <addr-seed>` removes the same synthetic BR/EDR entry.
  `MGMT_OP_GET_DEVICE_FLAGS` and `MGMT_OP_SET_DEVICE_FLAGS` are now staged on
  top of that device-list entry and emit Linux-shaped
  `MGMT_EV_DEVICE_FLAGS_CHANGED`; `btctl upstream mgmt-socket 0x004f 0
  <addr-seed>` queries the flags and `0x0050 0 <addr-seed>` sets the current
  flag bit.  Real
  `hci_cmd_sync_status()` command execution, native connection hash ownership
  and native `hdev->reject_list` ownership remain upstream `hci_core.c` /
  `hci_conn.c` work.  `btctl upstream socket-ioctl [dev]
  [up|down|reset|restat|scan|auth|encrypt|ptype|linkpol|linkmode|aclmtu|scomtu|connlist|conninfo|authinfo|block|unblock]
  [dev-opt|acl|le|cis|bis]` exercises these through `proto_ops.ioctl`.
- The guarded HCI staging path now also has a minimal `sendmsg` op for
  `HCI_CHANNEL_CONTROL`.  `btctl upstream mgmt-socket <opcode> [index] [param]`
  creates an upstream `PF_BLUETOOTH/BTPROTO_HCI` socket, binds the control
  channel, builds a Linux `struct mgmt_hdr`, and sends it through that staging
  op.  The send path now follows the upstream `hci_sock.c` split more closely:
  fallback `mgmt_init()` registers an `hci_mgmt_chan` for
  `HCI_CHANNEL_CONTROL`, and sendmsg dispatches through that channel's handler
  table.  Control bind now fails if the mgmt channel has not been registered,
  matching upstream's `hci_mgmt_chan_find()` gate.  The fallback dispatcher also
  validates handler data length and the `MGMT_INDEX_NONE` versus controller
  index split before queuing command-complete status.  `READ_VERSION`,
  `READ_COMMANDS`, `READ_INDEX_LIST` and `READ_INFO` return binary
  command-complete payloads; setting commands still call the minimal
  `linux_bt_mgmt_dispatch()` facade until upstream `mgmt.c` replaces the
  fallback channel.  The path increments `hci-mgmt-socket-cmd` and
  `hci-mgmt-chan-register` counters, queues Linux-shaped
  `MGMT_EV_CMD_COMPLETE` skb objects on `sk_receive_queue`, and successful
  setting commands also queue `MGMT_EV_NEW_SETTINGS` with the current settings
  bitmask.  The staging `recvmsg` op dequeues those skb objects through
  `copy_to_iter()` and sets `MSG_TRUNC` when the caller buffer is smaller than
  the skb.  `btctl upstream mgmt-socket` now uses a larger receive buffer so
  the current `READ_INFO` and `READ_COMMANDS` binary responses are observable,
  and prints the first received event header, command status and recv flags.
  This is intentionally only a BlueZ-facing control-channel probe; complete
  binary mgmt semantics still belong to upstream `hci_sock.c` and `mgmt.c`.
- `linux_bt_upstream_hci_status()` now reports `flags`, `dev-flags` and
  `promisc` counters for each registered HCI device, so nsh can observe the
  HCI socket/user-channel/monitor-facing state as it is wired up.
- The remaining protocol init/cleanup stubs are guarded by their upstream
  Kconfig switches so future `hci_sock.c`, `l2cap_core.c`, `mgmt.c` and
  `iso.c` enablement can replace staging symbols instead of colliding with
  them.
- compat now has the first socket-family registration shape:
  `struct net_proto_family`, `struct socket`, `struct sock`, `sk_alloc()`,
  `sock_init_data()`, `sk_add_node()`, `sk_del_node_init()`,
  `sk_for_each()`, lock reclassification stubs and module request/get/put
  stubs.
- `struct sock` now carries receive/error `sk_buff_head` queues, shutdown and
  error state, sndbuf/priority fields, wait queue and a Linux-like
  `refcount_t` so `bt_sock_recvmsg()`, `bt_sock_poll()`, `bt_sock_ioctl()` and
  wait helpers can keep their upstream shape.
- compat provides the first `sock_register()` / `sock_unregister()` registry
  for `PF_BLUETOOTH`.  This is still an internal staging registry, not yet a
  full NuttX VFS/socket-domain integration.
- `init_net` is a single shared compat object defined by
  `linux_bt_upstream_net.c`; upstream files must not get one header-local copy
  per compilation unit.
- `subsys_initcall(bt_init)` is no longer a dead macro for this porting path:
  it emits `linux_bt_compat_subsys_initcall_bt_init()`, and
  `linux_bt_upstream_af_init()` calls that bridge when
  `CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF` is enabled.  `sim_bringup()` invokes
  AF_BLUETOOTH init before VHCI autostart.
- Additional Linux includes used by `af_bluetooth.c` now have compat stubs:
  `linux/stringify.h`, `linux/sched/signal.h`, `linux/proc_fs.h`,
  `linux/sockios.h`, `asm/ioctls.h`, `leds.h` and `selftest.h`.
- seq/proc/ioctl/poll scaffolding now includes `seq_operations`, minimal
  procfs helpers, `SIOCETHTOOL`, `TIOCINQ`, `TIOCOUTQ`,
  `ETHTOOL_GET_TS_INFO`, wait-queue state macros and skb datagram helper
  stubs.
- This is still declaration/registration scaffolding plus narrow HCI/L2CAP
  staging boundaries only.  Full runtime socket semantics must come from
  upstream `af_bluetooth.c`, `hci_sock.c`, `l2cap_sock.c` and `iso.c` as their
  compat dependencies are completed.

Current `hci_vhci.c` preparation:

- The imported Linux virtual HCI driver is staged behind
  `CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI` and built by the NuttX
  `drivers/bluetooth` layer through
  `CONFIG_DRIVERS_BLUETOOTH_LINUX_HCI_VHCI`.
- compat now has initial `miscdevice`, `file_operations`, `write_iter`,
  `poll`, wait queue, delayed work and `sk_buff_head` queue scaffolding.
- compat `DEFINE_DEBUGFS_ATTRIBUTE()` now emits real `file_operations` with
  read/write methods backed by `simple_read_from_buffer()` and
  `kstrtoull_from_user()`, so hci_vhci debugfs attributes such as
  `msft_opcode_fops` are no longer dead integer stubs.
- compat `kstrtobool_from_user()` accepts the usual true/false and on/off style
  inputs used by hci_vhci debugfs toggles.
- compat `module_misc_device(vhci_miscdev)` now generates a callable
  `linux_bt_compat_module_misc_register_vhci_miscdev()` function instead of a
  dead static stub.
- `compat/include/net/bluetooth/hci_core.h` is currently a VHCI-stage wrapper,
  not the full upstream header.  It keeps upstream `hci.h` and `bluetooth.h`
  visible while declaring the `struct hci_dev`, quirk, register/free and
  `hci_recv_frame()` boundary needed by `hci_vhci.c`.
- `linux_bt_upstream_hci.c` owns the shared HCI device registry and the current
  VHCI-stage implementations of HCI allocation, registration, quirk handling,
  receive accounting and devcoredump stubs.  This replaced earlier header-only
  static inline state so future upstream files observe one controller registry.
- `linux_bt_upstream_hci_status()` exports the shared registry for
  `btctl upstream`, including registered `hci_dev` entries, bus, quirks and
  rx/tx counters.
- `linux_bt_upstream_hci_send()` is the current host-to-driver TX boundary.  It
  builds a Linux `sk_buff`, sets `hci_skb_pkt_type()`, updates TX accounting and
  invokes the default `hci_dev->send()` callback.  It is a staging hook for
  upstream `mgmt.c` and `hci_sock.c`, not a replacement protocol path.
- `btctl upstream sendhex` feeds real byte payloads into that same TX boundary
  for early HCI smoke work, for example HCI Reset command payload `03 0c 00`.
  This is still a staging hook; command construction should eventually come
  from upstream mgmt/socket paths.
- `drivers/bluetooth/linux_bt_upstream_vhci.c` is the NuttX-facing bind boundary.  It should
  connect the upstream VHCI HCI packet send/receive edge to `SIM_BTHWSIM`
  public-file records after `struct hci_dev` and HCI registration semantics are
  represented.
- `btctl upstream open` registers the imported upstream VHCI miscdevice and
  calls the upstream driver's own `vhci_fops.open()` path to create the default
  VHCI instance.  This mirrors the Wi-Fi hwsim strategy: keep the Linux driver
  source as the authority and add only the NuttX-facing initialize/open bridge.
- `btctl upstream create [opcode]` provides a deterministic nsh-side VHCI
  controller creation path.  It writes an H4 vendor packet through upstream
  `vhci_fops.write_iter()`, so the actual controller creation still happens in
  the imported `vhci_get_user()` / `vhci_create_device()` logic.  The default
  opcode is `0x00`, matching the normal virtual controller path; `0x40` and
  `0x80` retain upstream VHCI's external-config and raw-device meanings.
- `CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI_AUTOSTART` wires that same
  open bridge into `sim_bringup()` after `SIM_BTHWSIM` has created the
  public-file medium.  The four Bluetooth hwsim defconfigs enable it so each
  nsh instance starts with the upstream VHCI misc device opened.
- The VHCI/hwsim bridge follows Linux VHCI direction semantics.  Bytes read
  from upstream `vhci_fops.read()` are host-to-controller traffic and may be
  drained to the hwsim medium.  Bytes written through upstream
  `vhci_fops.write_iter()` are controller-to-host traffic and must stop at
  `hci_recv_frame()` / the host stack; they are not echoed back to the medium.
- `linux_bt_upstream_vhci_write_default()` wraps the upstream `vhci_fops.write_iter()`
  path.  `btctl upstream poll` now reads raw hwsim records, prepends the H4
  packet type and injects them through upstream `vhci_get_user()` /
  `hci_recv_frame()` rather than bypassing the VHCI driver.
- `linux_bt_upstream_vhci_read_default()` wraps upstream `vhci_fops.read()` so
  `btctl upstream read` can observe host-to-controller H4 bytes queued by
  `vhci_send_frame()`.
- `linux_bt_upstream_vhci_drain_default()` also wraps upstream `vhci_fops.read()`
  but parses the H4 packet type and appends only ACL and ISO payloads to the
  matching hwsim public medium.  HCI command/event/vendor traffic remains local
  to the host/controller control plane, matching Linux VHCI direction
  semantics.  This creates a manual
  `host -> vhci_send_frame() -> vhci_read() -> SIM_BTHWSIM -> peer
  vhci_write_iter()` data pump while the real controller scheduling path is
  still being ported.  Empty `vhci_read()` (`-EAGAIN`) is treated as a completed
  drain, and the status path exposes `m2h`, `h2m` and `read-empty` counters for
  manual pump debugging.
- `btctl upstream pump` is a convenience nsh command that runs `drain` followed
  by `poll`.  It does not bypass the upstream VHCI driver; it only advances the
  public-file medium from a single command in each sim terminal.
- `btaudio upstream-a2dp-source start` now queues a synthetic A2DP media
  sample through the staged `BTPROTO_L2CAP` socket sendmsg path first.  The
  socket path packages the media payload as HCI ACL + L2CAP CID `0x0041`, then
  follows the normal upstream VHCI `send -> readq -> drain -> hwsim ACL ->
  peer poll -> write_iter` path.
- `btaudio upstream-le-broadcast-source start [big] [bis]` now queues a
  synthetic LE Audio sample through the staged `BTPROTO_ISO` socket sendmsg
  path first.  The socket path packages the media payload as HCI ISO and
  follows the same VHCI data pump using the hwsim ISO medium.
- `btaudio upstream-a2dp-sink start|read|stop` now keeps the receive side on
  the staged L2CAP socket too.  `start` binds/listens on PSM `0x0019`, CID
  `0x0041`, handle `0x0040`; `read [max]` polls the hwsim medium and drains
  the L2CAP socket receive queue; `stop` releases the socket.
- `btaudio upstream-le-broadcast-sink sync|start|stop [big] [bis] [max]`
  mirrors the LE Audio receive side on the staged ISO socket.  `sync` binds and
  connects the BIG/BIS-derived handle, `start` polls the hwsim ISO medium and
  receives SDU payload, and `stop` releases the socket.
- These upstream audio commands validate the transport/data-plane shape only.
  Full AVDTP/A2DP, BAP/CAP and ISO socket/session semantics must come from the
  imported Linux `l2cap`, `hci_core`, `hci_event`, `mgmt` and `iso` paths as the
  compat layer is completed.
- `SIM_BTHWSIM` now exposes a raw binary read path in addition to the existing
  text diagnostic read.  Raw reads use independent `.raw.roleX.off` offsets so
  diagnostic polling does not consume upstream HCI injection records.
- `btctl upstream poll` consumes raw ACL/ISO records and injects them through
  the upstream VHCI write path.  Control-plane HCI command/event/vendor traffic
  remains local to the controller/host boundary and is not treated as simulated
  air-medium data.  The current `hci_recv_frame()` staging
  hook accounts RX at the HCI device boundary; upstream `hci_core.c` still
  needs to replace it with Linux's real rx work, event dispatch, socket wakeup
  and protocol fanout.
- This is deliberately not a protocol shim; protocol behavior should remain in
  the imported upstream Bluetooth files.

Current `hci_core.h` / `eir.c` preparation:

- Basic Linux primitive headers now exist for spinlocks, mutexes, atomics,
  refcounts, workqueues, wait queues, IDR, RCU list, SRCU and LEDs.
- These headers are compatibility scaffolding only; they do not replace Linux
  protocol behavior.
- The next real upstream target is to satisfy the `struct hci_dev` fields used
  by `eir.c` while preserving the imported `eir.c` logic.

Staging rule:

```text
stage 1: NET_LINUX_BLUETOOTH_UPSTREAM_SOURCES=y
         NET_LINUX_BLUETOOTH_UPSTREAM_LIB=y
         NET_LINUX_BLUETOOTH_UPSTREAM_EIR=n
         NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CODEC=n
         NET_LINUX_BLUETOOTH_UPSTREAM_AF=n
         NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CORE=n
         NET_LINUX_BLUETOOTH_UPSTREAM_HCI_SOCK=n
         NET_LINUX_BLUETOOTH_UPSTREAM_MGMT=n
         NET_LINUX_BLUETOOTH_UPSTREAM_L2CAP=n
         NET_LINUX_BLUETOOTH_UPSTREAM_SMP=n
         NET_LINUX_BLUETOOTH_UPSTREAM_ISO=n

stage 2: enable EIR after hci_dev/uuid list compat is ready
stage 3: enable HCI_CODEC after sk_buff/HCI sync compat is ready
stage 4: enable HCI_VHCI after hci_dev registration, miscdevice, debugfs,
         file operations and sk_buff queue semantics are ready
stage 5: enable AF_BLUETOOTH after NuttX socket family/domain registration
         and Linux proto/socket ownership semantics are mapped
stage 6: enable HCI_CORE and replace the VHCI-stage hci_dev wrapper with
         Linux's real HCI device, command/event, rx work and connection state
stage 7: enable HCI_SOCK and MGMT so BlueZ-observable control paths come from
         upstream hci_sock.c and mgmt.c
stage 8: enable L2CAP, then SMP, then ISO to retire the temporary L2CAP/SMP
         and BIG/BIS synthetic hwsim state machines
```

Immediate next porting steps:

1. Add `compat/` headers that provide a NuttX-friendly subset of Linux kernel
   primitives needed by the imported Bluetooth files.
2. Make `lib.c` compile first, then add only the compat it proves necessary.
3. Move to `eir.c` by adapting Linux `hci_core.h` dependencies and only after
   enough of `struct hci_dev` is
   represented to keep upstream semantics intact.
4. Move to `hci_codec.c` after `sk_buff`, codec list, and sync command stubs
   have Linux-like semantics.
5. Replace temporary mgmt shim calls with upstream `mgmt.c` command handlers.
6. Replace temporary L2CAP/SMP/ISO shim objects with upstream `l2cap_core.c`,
   `smp.c`, and `iso.c` state machines.
7. Port upstream `drivers/bluetooth/hci_vhci.c` using the same strategy as
   Wi-Fi `mac80211_hwsim_linux.c`: Linux driver source plus a thin
   NuttX-facing initialize/bind layer.
8. Bind the upstream HCI controller boundary to `SIM_BTHWSIM` public-file radio
   transport.

## 2026-06-11 staged mgmt SET_IO_CAPABILITY

已在 staging HCI control socket 中接入 `MGMT_OP_SET_IO_CAPABILITY`，保存 controller 级默认 IO capability，并让 synthetic `PAIR_DEVICE` payload 使用该值。当前仍是 NuttX-facing staging handler；最终目标是切换到 upstream `net/bluetooth/mgmt.c` 的 pending command、SMP 触发和 key distribution 语义。

## 2026-06-11 staged pairing user confirmation

已新增 staged `MGMT_OP_USER_CONFIRM_REPLY` / `MGMT_OP_USER_CONFIRM_NEG_REPLY`。当 synthetic `PAIR_DEVICE` 使用非 `NoInputNoOutput` IO capability 时，本地 device entry 会进入 pending confirmation，并通过 control sockets 广播 `MGMT_EV_USER_CONFIRM_REQUEST`。确认 reply 会标记 paired，否认 reply 会清除 pending 并保持 unpaired。

这仍不是最终 upstream 形态：真正 Linux 路径应由 `net/bluetooth/mgmt.c` 的 pending command、`net/bluetooth/smp.c` 的 pairing method/key distribution，以及 `hci_event.c` 的 user confirmation command complete 共同驱动。

## 2026-06-11 staged mgmt pending command ownership

新增 `linux_bt_hci_mgmt_pending` staging 表，用来保存 pending `MGMT_OP_PAIR_DEVICE` 的 socket ownership、controller index 和 peer address。非 `NoInputNoOutput` pairing 会返回 internal `-EINPROGRESS`，mgmt dispatch 不再立即发送 pair command-complete；后续 user confirmation/cancel/remove 会完成或清理 pending entry。

这一步的目标是为替换到 upstream `mgmt_pending_add()` 铺路。还未完成的 upstream 语义包括：pending command list 的完整生命周期、HCI command status/complete 绑定、SMP method selection、LTK/IRK/CSRK distribution、bond persistence 和 identity resolving。

## 2026-06-11 persistent mgmt send path

新增 `linux_bt_upstream_mgmt_send_probe()`，复用 `linux_bt_upstream_mgmt_listen_probe()` 打开的 HCI control socket 发送 mgmt command。pending pair 的 socket ownership 因此可以被同一个 persistent socket 观察到：pair command 挂起，user confirmation 后原 socket 收到 pair command-complete。

这一步继续收敛到 upstream `hci_sock.c` + `mgmt.c` 的 BlueZ-observable 行为，但 payload 构造和部分 command handler 仍是 staging，后续应由 imported upstream mgmt command table 和 SMP/HCI event 完成路径替换。

## 2026-06-11 staged passkey pairing branch

已新增 staged `MGMT_OP_USER_PASSKEY_REPLY` / `MGMT_OP_USER_PASSKEY_NEG_REPLY`，并在 `PAIR_DEVICE` 使用 `KeyboardOnly(0x02)` IO capability 时广播 `MGMT_EV_USER_PASSKEY_REQUEST`。passkey reply 会完成原 pending pair command；negative reply 会失败并清理 pending entry。

当前 passkey 值由 probe builder 固定为 `123456`，只用于保持 BlueZ-observable mgmt binary shape。最终应由 imported `net/bluetooth/smp.c` 负责 method selection、confirm value、random value、TK/STK/LTK/IRK/CSRK distribution，并由 upstream `mgmt.c` 发出真实事件和 command-complete。

## 2026-06-11 staged LE LTK distribution event

新增 `linux_bt_hci_ltk_state` staging 表和 `MGMT_EV_NEW_LONG_TERM_KEY` 广播。LE pairing 成功后会保存一条 deterministic staged LTK，并向所有 bound control sockets 发送 `struct mgmt_ev_new_long_term_key`。

这一步只补 mgmt 可观察事件和临时 key store。最终应由 upstream `net/bluetooth/smp.c` 在真实 pairing/key distribution 阶段创建 LTK，由 upstream `mgmt.c` 发送 key event，并补齐 IRK/CSRK、BR/EDR Link Key、identity resolving 和持久化加载/保存。

## 2026-06-11 hwsim usecase test matrix

新增 `tools/firmware/sim/test-bt-hwsim-usecases.sh`，用于生成 BT/BLE hwsim per-terminal nsh command files。矩阵覆盖 BR/EDR 基础、BLE 基础、mgmt pairing 三种分支、A2DP-like L2CAP media 和 LE-Audio-like ISO media。

这不是 upstream 替换本身，但它为后续每个 upstream 文件接管 shim 提供回归入口：每接入一块 `hci_sock.c`、`mgmt.c`、`l2cap_core.c`、`smp.c` 或 `iso.c`，都应复跑相应用例并记录日志差异。

## 2026-06-11 hwsim usecase log validator

新增 `tools/firmware/sim/validate-bt-hwsim-usecases.py`，用于校验 BT/BLE hwsim per-terminal logs。它检查每个用例的关键 BlueZ-like 用户可见输出，并把 panic/assert/failed 行视为失败。后续每当 upstream `hci_sock.c`、`mgmt.c`、`l2cap_core.c`、`smp.c`、`iso.c` 接管一块 staging shim，都应复用该 validator 记录回归结果。

## 2026-06-11 hwsim usecase runner

新增 `tools/firmware/sim/run-bt-hwsim-usecases.py`，用于实际启动 NuttX sim role、执行 generated nsh command files、采集 per-role logs 并调用 validator。后续 upstream Bluetooth 文件逐步接管 staging shim 时，应以该 runner 作为可重复回归入口。

## 2026-06-11 hwsim runner manifest

`run-bt-hwsim-usecases.py` now writes `run-results.json` containing the executed case list, role log paths, run errors, validator return code and aggregate PASS/FAIL.  Future upstream replacement steps should preserve this manifest alongside logs as regression evidence.

## 2026-06-11 hwsim preflight result

BT/BLE hwsim test preflight shows the generated runner and validator are syntactically valid and list the expected cases, but `build/nuttx-sim-bt1`, `build/nuttx-sim-bt2`, `build/nuttx-sim-ble1`, and `build/nuttx-sim-ble2` are currently missing.  Added `tools/firmware/sim/preflight-bt-hwsim-usecases.sh` to make this prerequisite check repeatable before running regression cases.

## 2026-06-11 first runnable hwsim build policy

The four hwsim defconfigs now build successfully with
`CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_SOURCES=y`,
`CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_LIB=y`,
`CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI=y` and
`CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_VHCI_AUTOSTART=y`.

`CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_AF` is explicitly disabled in
`hwsim_bt1`, `hwsim_bt2`, `hwsim_ble1` and `hwsim_ble2` for this first runnable
stage.  The imported Linux `af_bluetooth.c` remains the semantic target, but it
still needs the rest of the Linux socket core/wait/ioctl/proc compat before it
can replace the staged NuttX-facing `PF_BLUETOOTH` shim.

The current generated binaries are:

```text
build/nuttx-sim-bt1
build/nuttx-sim-bt2
build/nuttx-sim-ble1
build/nuttx-sim-ble2
```

`tools/firmware/sim/preflight-bt-hwsim-usecases.sh` passes with all four
binaries present.

## BNEP/PAN networking import

Imported for the BR/EDR PAN/IP networking path required by future BT1/BT2
`ping` and `iperf` hwsim tests:

```text
net/bluetooth/bnep/core.c
net/bluetooth/bnep/sock.c
net/bluetooth/bnep/netdev.c
net/bluetooth/bnep/bnep.h
```

Local paths:

```text
wireless/linux_bluetooth/upstream/net_bluetooth/bnep/core.c
wireless/linux_bluetooth/upstream/net_bluetooth/bnep/sock.c
wireless/linux_bluetooth/upstream/net_bluetooth/bnep/netdev.c
wireless/linux_bluetooth/upstream/net_bluetooth/bnep/bnep.h
wireless/linux_bluetooth/upstream/net_bluetooth/bnep/PORTING.md
```

Build switch:

```text
CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP
```

The switch is wired into `wireless/linux_bluetooth/Make.defs` but remains off by
default until Linux netdevice, namespace, socket/ioctl and skb dependencies are
mapped onto NuttX networking semantics.

## 2026-06-20 L2CAP connected-state option boundary

The current L2CAP socket option convergence gate now checks connected-state
behavior without inventing fake owner state.  A2DP daemon-owned L2CAP handles run
`linux_bt_upstream_l2cap_socket_connected_option_probe()` after connect.

Observed Linux-aligned behavior in hwsim:

- `SOL_L2CAP` / `L2CAP_OPTIONS` on a connected socket returns `-EINVAL`.
- `SOL_BLUETOOTH` / `BT_MODE` follows the upstream ECRED feature gate.  In the
  current hwsim configuration ECRED is disabled, so the observed result is
  `-ENOPROTOOPT` and the evidence is tagged `bt-mode-gate=ecred-disabled`.
- `L2CAP_CONNINFO` is deliberately not asserted yet because full real
  `l2cap_conn` ownership is still a remaining convergence item.  The hwsim log
  records `conninfo-skip=needs-real-l2cap-conn` instead of treating a synthetic
  handle as native parity.

Validation artifacts:

- `build/logs/build-bt1-l2cap-connected-options.log`: PASS.
- `build/logs/build-bt2-l2cap-connected-options.log`: PASS.
- `build/logs/build-ble1-l2cap-connected-options.log`: PASS.
- `build/logs/build-ble2-l2cap-connected-options.log`: PASS.
- `build/logs/run-l2cap-connected-options.log`: PASS for A2DP closeout and LE
  Audio BAP/PACS/ASCS.
- `build/logs/validate-l2cap-connected-options.log`: PASS targeted replay.

## 2026-06-20 L2CAP_CONNINFO convergence audit

`L2CAP_CONNINFO` remains a real upstream convergence item.  The current hwsim
validator deliberately keeps the connected-state L2CAP option gate at
`conninfo-skip=needs-real-l2cap-conn`.

Negative audit results:

- Adding per-socket staging `l2cap_chan` objects and attaching them to hwsim
  `l2cap_conn` during connect built successfully, but caused A2DP hwsim to exit
  before the connected-option probe completed.
- Reading `L2CAP_CONNINFO` from the HCI connection hash also built, but failed
  the same A2DP hwsim gate.

Retained passing evidence:

- `build/logs/run-l2cap-connected-options.log`: PASS for A2DP closeout and LE
  Audio BAP/PACS/ASCS.
- `build/logs/validate-l2cap-connected-options.log`: PASS.

Required upstream-shaped completion path:

- Move staging L2CAP sockets closer to upstream `l2cap_sock_alloc()` and
  `l2cap_sock_init()` semantics.
- Establish safe `l2cap_chan` callback ops, `l2cap_conn_add()` /
  `l2cap_chan_add()` lock ordering, hci_conn lifetime, and release/teardown.
- Only then promote `L2CAP_CONNINFO` from a documented skip boundary to required
  hwsim evidence backed by `chan->conn->hcon`.
