# Linux Bluetooth compat layer for NuttX

This directory is for Linux kernel primitive compatibility used by the imported
`net/bluetooth` sources.

Rule: reuse existing Linux compatibility from the Wi-Fi/mac80211 port before
adding new Bluetooth-local compatibility here.  In particular,
`wireless/linux_compat/include` is the shared generic Linux compat include path,
and `wireless/ieee80211/include` is added after it as a legacy Wi-Fi/mac80211
fallback include path for this port.
Bluetooth-local headers should be limited to Bluetooth-specific wrappers, such
as `<net/bluetooth/*.h>`, VHCI-stage `hci_core.h`, or gaps that are not already
covered by the shared IEEE80211/mac80211 Linux compat layer.

Do not change upstream Bluetooth protocol logic to compensate for missing
NuttX APIs.  Prefer this order:

```text
1. Reuse wireless/linux_compat/include when the helper is already split out.
2. Reuse wireless/ieee80211/include Linux compat if it is safe and does not
   pull in Wi-Fi-only socket/skbuff state.
3. Move generally useful Linux compat to wireless/linux_compat/include when
   practical.
4. Add Bluetooth-local compat only for Bluetooth-specific gaps.
5. Edit imported upstream protocol code only as a last resort and document why.
```

Bluetooth-local headers currently exist for the earliest staged files:

```text
include/linux/types.h
include/linux/kernel.h
include/linux/list.h
include/linux/skbuff.h
include/linux/mutex.h
include/linux/workqueue.h
include/linux/refcount.h
include/linux/atomic.h
include/linux/bitops.h
include/net/sock.h
```

Known overlap with the Wi-Fi/mac80211 Linux compat layer:

```text
linux/types.h
linux/kernel.h
linux/list.h
linux/slab.h
linux/spinlock.h
linux/mutex.h
linux/workqueue.h
linux/wait.h
linux/skbuff.h
linux/debugfs.h
linux/ethtool.h
linux/bitops.h
linux/rculist.h
linux/srcu.h
linux/idr.h
linux/err.h
linux/errno.h
linux/socket.h
linux/poll.h
linux/leds.h
linux/module.h
linux/init.h
linux/sched.h
linux/unaligned.h
```

Do not expand these Bluetooth-local versions unless the existing
`wireless/ieee80211/include/linux` version is missing a Bluetooth-required
semantic.  When a duplicate must remain temporarily, keep it small and document
the Bluetooth-specific reason.  The intended direction is to retire generic
duplicates into a shared Linux compat surface rather than keeping parallel Wi-Fi
and Bluetooth implementations.

Safe direct de-duplication already done:

```text
linux/leds.h
linux/socket.h
linux/bitops.h -> wireless/linux_compat/include/linux/bitops.h
linux/unaligned.h -> wireless/linux_compat/include/linux/unaligned.h
linux/ethtool.h -> wireless/linux_compat/include/linux/ethtool.h
linux/list.h -> wireless/linux_compat/include/linux/list.h
linux/workqueue.h -> wireless/linux_compat/include/linux/workqueue.h
```

Bluetooth's local `linux/leds.h` forwards to the Wi-Fi/mac80211 compat header.
Bluetooth-required `struct led_classdev` was added to the shared header.
Bluetooth's local `linux/socket.h` forwards to the shared Linux UAPI socket
header and then includes `<sys/socket.h>` for the NuttX/POSIX-facing glue that
still needs host socket constants.

Important limitation: several Wi-Fi/mac80211 Linux headers include the
monolithic `cfg80211_compat.h`, which currently also defines Wi-Fi-specific
`struct sk_buff` and dummy `struct sock`.  Bluetooth cannot blindly forward
headers such as `linux/workqueue.h` or `linux/skbuff.h` until the shared compat
layer is split or guarded so those Wi-Fi socket/skbuff definitions do not
collide with Bluetooth's `net/sock.h` and Bluetooth HCI `sk_buff` staging.

Shared split already started:

```text
wireless/linux_compat/include/linux/bitops.h
wireless/linux_compat/include/linux/unaligned.h
wireless/linux_compat/include/linux/ethtool.h
wireless/linux_compat/include/linux/list.h
wireless/linux_compat/include/linux/utsname.h
```

Shared improvements still staged in the Wi-Fi/mac80211 compat layer:

```text
wireless/ieee80211/include/linux/cfg80211_compat.h: external sock guard
```

`linux_compat/include/linux/ethtool.h` contains the Wi-Fi ethtool structures
and the Bluetooth AF socket timestamp-info structures, including
`struct kernel_ethtool_ts_info`.

`linux_compat/include/linux/list.h` contains the shared list and hlist helpers.
The Wi-Fi monolithic `cfg80211_compat.h` still carries its historical copy for
now, but marks `__WIRELESS_LINUX_COMPAT_LIST_TYPES_DEFINED` and
`__WIRELESS_LINUX_COMPAT_LIST_HELPERS_DEFINED` so the shared header can be
included afterward without duplicate definitions.  Bluetooth uses the shared
header through its local wrapper.

`linux_compat/include/linux/workqueue.h` contains the shared NuttX work queue
bridge used by both Wi-Fi/mac80211 and Linux Bluetooth.  The old Wi-Fi
monolithic compat header still owns a few historical helpers, so the shared
header uses fine-grained guards for workqueue types, globals, core queue
helpers, init macros, schedule helpers and cancel/flush helpers.  Do not add
new Bluetooth-local workqueue declarations; extend the shared header instead
unless the semantic is Bluetooth-specific.

`linux_compat/include/linux/utsname.h` contains the shared `init_utsname()`
helper.  It now exposes both `release` and `machine`, which are needed by
Linux Bluetooth monitor socket code.  The Wi-Fi/mac80211 wrapper forwards to
the shared header with `include_next`.

Bluetooth-local `linux/compat.h` currently only provides `compat_ptr()` for
`hci_sock.c` compat ioctl handling.  Bluetooth-local `net/bluetooth/hci_mon.h`
is a wrapper around the imported upstream monitor header and should stay close
to the Linux source.

The current temporary `linux_bt_core.c` shim exists only to keep the NuttX sim
hwsim path usable while these upstream files are made buildable one translation
unit at a time.

## Current HCI socket send/receive path

The compat layer now carries enough socket surface for the first HCI
control-channel send/receive probe:

- `struct proto` includes `owner` and `obj_size`, and `sk_alloc()` honors
  `obj_size` so upstream Bluetooth sockets can allocate private state larger
  than `struct sock`.
- `struct socket`, `struct proto_ops` and `struct msghdr` cover the bind,
  sendmsg and recvmsg shape used by the current HCI staging socket.
- `copy_to_iter()` supports the receive side used to return Linux-shaped mgmt
  event skb payloads to the caller.
- `MSG_TRUNC` is surfaced when a recvmsg caller provides a buffer smaller than
  the queued skb, matching the Linux socket observation point needed by larger
  mgmt responses.
- `struct sk_buff` helpers include the minimal queue/copy/tailroom pieces
  needed to build mgmt command skb objects.
- The imported `hci_mon.h` header is now used by staging HCI sockets for
  monitor/logging records.  Monitor sockets receive `hci_mon_hdr` framed
  control open/close/command/event records, and logging sockets can submit
  `HCI_MON_USER_LOGGING` shaped records.
- RAW/USER HCI socket sendmsg now follows Linux's H4 shape for the staging
  path: first byte packet type, remaining bytes payload, sent to the bound
  `hci_dev->send()` after `HCI_UP` and packet type checks.
- RAW/USER HCI socket recvmsg now has staging fanout from `hci_recv_frame()`:
  controller-to-host payloads are copied into socket queues with the H4 packet
  type byte restored, and monitor sockets receive matching RX records.
- L2CAP has a deliberately narrow create/release/bind staging path: the
  fallback `l2cap_init()` registers `BTPROTO_L2CAP`, uses `bt_sock_alloc()`
  with `proto->obj_size = sizeof(struct l2cap_pinfo)`, initializes the L2CAP
  private `rx_busy` list, and accepts an imported upstream `struct sockaddr_l2` bind probe
  that records local PSM/CID and moves the socket to `BT_BOUND`.  It does not
  implement connect/send/recv yet; those semantics must come from upstream
  `l2cap_sock.c`/`l2cap_core.c`.
- ISO has the matching narrow create/release/bind staging path for LE Audio
  socket bring-up.  The fallback `iso_init()` registers `BTPROTO_ISO`, uses
  `bt_sock_alloc()` with an ISO-private `bt_sock` wrapper, and accepts imported
  upstream `struct sockaddr_iso` bind probes.  It does not implement
  connect/send/recv yet; those semantics must come from upstream `iso.c`.
- The staging probe keeps protocol socket type selection close to Linux usage:
  HCI uses `SOCK_RAW`, while L2CAP and ISO use `SOCK_SEQPACKET` when available.
- ISO `sendmsg()` has a narrow staging data-plane hook: after a probe-created
  socket is bound, the user payload is wrapped as an HCI ISO data packet and
  sent through the upstream VHCI path.  The temporary probe supplies the HCI
  handle explicitly; upstream `iso.c` must replace that with real ISO
  connection/session ownership.
- L2CAP `sendmsg()` has the matching staging data-plane hook: after a
  probe-created socket is bound, the user payload is wrapped as HCI ACL data
  with an L2CAP basic header and sent through the upstream VHCI path.  The
  temporary probe supplies the ACL handle and CID explicitly; upstream
  `l2cap_sock.c`/`l2cap_core.c` must replace that with real connection and
  channel ownership.
- L2CAP/ISO `recvmsg()` now drains protocol socket receive queues.  A staging
  fanout from `hci_recv_frame()` parses HCI ACL/L2CAP and HCI ISO headers,
  queues upper payloads into bound protocol sockets, and increments receive
  counters.  This keeps the receive observation point Linux-shaped without
  claiming full upstream reassembly or flow-control behavior.
- The `btctl upstream l2cap-bind`/`iso-bind` commands keep one staging protocol
  socket alive per sim instance, and `l2cap-recv`/`iso-recv` read its queued
  payloads.  This is temporary hwsim scaffolding for two-terminal testing, not
  a replacement for BlueZ-owned upstream sockets.
- `l2cap-write` and `iso-write` reuse those persistent sockets for repeated
  sendmsg calls, while `l2cap-send`/`iso-send` remain one-shot create/bind/send
  helpers.
- `l2cap-connect` and `iso-connect` call the staging protocol connect ops and
  move kept sockets from `BT_BOUND` to `BT_CONNECTED`, without claiming real
  link/session establishment.
- `l2cap-close` and `iso-close` release the persistent staging sockets through
  the protocol ops table, exercising the same release hook used by one-shot
  probes.

This is not the final mgmt implementation.  The current
`btctl upstream mgmt-socket` command only proves that a BlueZ-shaped HCI
control socket can enter the Linux-like sendmsg boundary, dispatch through a
fallback `hci_mgmt_chan` handler table, queue
`MGMT_EV_CMD_COMPLETE`/`MGMT_EV_NEW_SETTINGS` shaped skb objects, and read one
event back through recvmsg.  The fallback channel includes binary responses for
`READ_VERSION`, `READ_COMMANDS`, `READ_INDEX_LIST` and `READ_INFO`, and now
validates the same broad index split as Linux mgmt: global commands use
`MGMT_INDEX_NONE`, controller commands use a concrete HCI index.  Setting
commands still delegate to the temporary mgmt facade.  Full binary mgmt
responses, events, monitor replay and wakeup semantics should move to imported
upstream `hci_sock.c` and `mgmt.c`.
