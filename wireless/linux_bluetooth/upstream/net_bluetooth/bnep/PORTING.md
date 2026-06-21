## 2026-06-17 Native BNEP/PAN jumbo over imported L2CAP

The PAN data-plane closeout now covers large payloads on the imported Linux
BNEP and L2CAP path:

```text
BlueZ Network1 / bneptest
  -> connected L2CAP fd
  -> BNEPCONNADD
  -> imported BNEP session/netdev
  -> bnep_tx_frame
  -> kernel_sendmsg
  -> l2cap_chan_send
  -> l2cap_do_send
  -> hwsim ACL start/continuation packets
  -> peer L2CAP/BNEP RX
  -> netif_rx
```

Previous failure mode:

- `BNEPCONNADD` succeeded and `btn0` came up, but `ping -s 2000` lost both
  replies.
- Counters showed small frames moving through BNEP, while large frames did not
  complete on the native L2CAP/BNEP data path.
- The active TX path was imported BNEP `kernel_sendmsg`; fixing only the older
  side shim did not affect the failing path.

Fix:

- `l2cap_sim_attach_channel()` now models a configured sim BR/EDR channel by
  keeping `conn->mtu`, `chan->omtu`, and `chan->imtu` large enough for PAN
  jumbo validation instead of forcing the channel back to
  `L2CAP_DEFAULT_MTU`.
- `l2cap_do_send()` now allocates ACL packets dynamically and sends skb head
  plus `frag_list` payloads as ACL start/continuation packets into hwsim.
- The native `l2cap_chan_send()` MTU check and `-EMSGSIZE` behavior remain in
  place; the sim attach point now supplies the negotiated MTU that a real
  controller/configuration exchange would otherwise establish.

Validation:

- `FeatherCore/build/logs/build-bt1-l2cap-sim-mtu-aclfrag-v2.log`: PASS.
- `FeatherCore/build/logs/build-bt2-l2cap-sim-mtu-aclfrag-v2.log`: PASS.
- `FeatherCore/build/logs/run-l2cap-sim-mtu-aclfrag-v2-jumbo.log`: PASS
  `bluez-network-jumbo-ping`.
- `FeatherCore/build/logs/run-current-functional-l2cap-sim-mtu-aclfrag-v2.log`:
  PASS `bluez-current-functional-closeout` for `bt1`, `bt2`, `ble1`, and
  `ble2`.

Jumbo evidence:

- `btn0` MTU 2500.
- Two `2000 bytes from 10.77.0.1` replies.
- `2 packets transmitted, 2 received, 0% packet loss`.
- Native counters included `bnep-native-tx-frame-ok=6`,
  `bnep-native-l2cap-delivered=6`, and `bnep-native-netif-rx=6`.

## 2026-06-17 session-thread TX ownership

The hwsim-only immediate TX drain in imported `bnep/netdev.c` has been
removed.  `bnep_net_xmit()` now queues to `sk_write_queue` and wakes the
session thread, matching the Linux ownership model more closely:

- NuttX lower-half TX calls Linux `ndo_start_xmit`.
- `ndo_start_xmit` queues the skb to the BNEP socket write queue.
- Imported `bnep_session` drains the TX queue and calls `bnep_tx_frame`.
- `bnep_tx_frame` sends via `kernel_sendmsg` to the connected L2CAP socket.

Validation:

- `FeatherCore/build/logs/build-bt1-bnep-session-thread-tx-owner.log`: PASS.
- `FeatherCore/build/logs/build-bt2-bnep-session-thread-tx-owner.log`: PASS.
- `FeatherCore/build/logs/run-bnep-session-thread-tx-owner-verify.log`: PASS.

The hwsim batch covered `bneptest` fd handoff, ping, reconnect stress,
Network ping, Network reconnect stress, and Network iperf matrix.  Validators
now require non-zero `bnep-native-session-tx-dequeue`, so this path cannot
pass if TX is not owned by the imported BNEP session thread.

## 2026-06-17 iperf matrix closeout

The mixed TCP/UDP multi-cycle BlueZ Network matrix is closed again after
aligning the simulator packet buffer size and making UDP iperf sends bounded
and retryable.

Implementation notes:

- BT/BLE hwsim defconfigs now set `CONFIG_NET_ETH_PKTSIZE=1514`, matching the
  BNEP full Ethernet-frame lowerhalf path instead of relying on the smaller
  default packet size.
- `apps/netutils/iperf` UDP client now sets `SO_SNDTIMEO` and retries
  transient send congestion (`ENOMEM`, `EAGAIN`, `EWOULDBLOCK`).  This prevents
  a blocked UDP `sendto()` from stalling the disconnect/reconnect lifecycle
  while preserving real throughput validation when the path is healthy.

Validation:

- `FeatherCore/build/logs/build-bt1-iperf-udp-send-timeout.log`: PASS.
- `FeatherCore/build/logs/build-bt2-iperf-udp-send-timeout.log`: PASS.
- `FeatherCore/build/logs/run-iperf-udp-send-timeout-matrix-verify.log`: PASS.
- `FeatherCore/build/logs/run-bnep-network-iperf-matrix-closeout-verify.log`: PASS.

The combined closeout batch included:

- `bluez-bneptest-fd-handoff`
- `bluez-bneptest-ping`
- `bluez-bneptest-reconnect-stress`
- `bluez-network-ping`
- `bluez-network-reconnect-stress`
- `bluez-network-iperf-udp`
- `bluez-network-iperf-udp-reverse`
- `bluez-network-iperf-matrix`

The previous matrix gap note below is retained as history but is superseded by
this PASS result.

## 2026-06-17 clean-baseline verification and current matrix gap

BT1/BT2 were rebuilt serially after the latest BNEP/Network convergence work:

- `FeatherCore/build/logs/build-bt1-bnep-network-clean-baseline-serial.log`: PASS.
- `FeatherCore/build/logs/build-bt2-bnep-network-clean-baseline-serial.log`: PASS.

The current hwsim BNEP/BlueZ Network clean baseline passes:

- `bluez-bneptest-fd-handoff`
- `bluez-bneptest-ping`
- `bluez-bneptest-reconnect-stress`
- `bluez-network-ping`
- `bluez-network-reconnect-stress`
- `bluez-network-iperf-udp`
- `bluez-network-iperf-udp-reverse`

Validation artifacts:

- `FeatherCore/build/logs/run-bnep-kernel-sendmsg-network-clean-baseline-final-verify.log`
- `FeatherCore/build/bt-hwsim-bnep-kernel-sendmsg-network-clean-baseline-final-verify/run-results.json`

Open BNEP/Network convergence gap:

- `bluez-network-iperf-matrix` still fails in the latest run.
- The failure is not a missing `BNEPCONNADD` first-cycle path: BNEP sessions
  and ping connectivity are established across reconnects.
- The unresolved case is mixed TCP/UDP multi-cycle reconnect where UDP iperf
  reports zero payload after the previous TCP/UDP cycles and then blocks the
  remaining teardown/reconnect coverage.
- Keep fixing this at the native BNEP/IP/socket lifecycle boundary.  Do not
  restore staged hwsim bridge fallback to make the matrix pass.

Latest failing artifact:

- `FeatherCore/build/logs/run-bnep-kernel-sendmsg-iperf-matrix-per-round-pump-verify.log`

## 2026-06-17 no-fallback native closeout verification

After tightening native BNEP datapath closeout to require zero kept-probe and
zero rebind-probe `sockfd_lookup` fallback counters, BT1/BT2 builds and the
focused BNEP/BlueZ Network hwsim gates were rerun.

Build results:

- `FeatherCore/build/logs/build-bt1-bnep-nofallback-convergence.log`: PASS.
- `FeatherCore/build/logs/build-bt2-bnep-nofallback-convergence.log`: PASS.

hwsim validation results:

- `bluez-bneptest-fd-handoff`: PASS.
- `bluez-bneptest-ping`: PASS for `bt1` and `bt2`.
- `bluez-bneptest-reconnect-stress`: PASS for `bt1` and `bt2`.
- `bluez-network-ping`: PASS for `bt1` and `bt2`.
- `bluez-network-reconnect-stress`: PASS for `bt1` and `bt2`.
- `bluez-network-iperf-matrix`: PASS for `bt1` and `bt2`.

Artifacts:

- `FeatherCore/build/logs/run-bnep-nofallback-convergence-verify.log`
- `FeatherCore/build/bt-hwsim-bnep-nofallback-convergence-verify/run-results.json`

This confirms the current BlueZ `bneptest` and BlueZ Network native closeout
paths use the real `socket-fd` handoff path and do not depend on kept-probe or
rebind-probe fallback lookup.

## 2026-06-17 fd lookup fallback visibility verification

After adding explicit kept-probe/rebind-probe lookup counters, BT1/BT2 builds
and the focused BNEP/BlueZ Network hwsim gates were rerun.

Build results:

- `FeatherCore/build/logs/build-bt1-bnep-fdlookup-convergence.log`: PASS.
- `FeatherCore/build/logs/build-bt2-bnep-fdlookup-convergence.log`: PASS.

hwsim validation results:

- `bluez-bneptest-fd-handoff`: PASS.
- `bluez-bneptest-ping`: PASS for `bt1` and `bt2`.
- `bluez-bneptest-reconnect-stress`: PASS for `bt1` and `bt2`.
- `bluez-network-ping`: PASS for `bt1` and `bt2`.
- `bluez-network-reconnect-stress`: PASS for `bt1` and `bt2`.
- `bluez-network-iperf-matrix`: PASS for `bt1` and `bt2`.

Artifacts:

- `FeatherCore/build/logs/run-bnep-fdlookup-convergence-verify.log`
- `FeatherCore/build/bt-hwsim-bnep-fdlookup-convergence-verify/run-results.json`

This keeps the stage rule intact: source-level convergence changes are closed
only after build and hwsim gates pass.

## 2026-06-17 build and hwsim verification

After the BNEP upstream-convergence tightening, BT1/BT2 builds and the core
BNEP/BlueZ Network hwsim gates were rerun.

Build results:

- `FeatherCore/build/logs/build-bt1-bnep-upstream-convergence.log`: PASS.
- `FeatherCore/build/logs/build-bt2-bnep-upstream-convergence.log`: PASS.

hwsim validation results:

- `bluez-bneptest-fd-handoff`: PASS.
- `bluez-bneptest-ping`: PASS for `bt1` and `bt2`.
- `bluez-bneptest-reconnect-stress`: PASS for `bt1` and `bt2`.
- `bluez-network-ping`: PASS for `bt1` and `bt2`.
- `bluez-network-reconnect-stress`: PASS for `bt1` and `bt2`.
- `bluez-network-iperf-matrix`: PASS for `bt1` and `bt2`.

Artifacts:

- `FeatherCore/build/logs/run-bnep-upstream-convergence-verify.log`
- `FeatherCore/build/bt-hwsim-bnep-upstream-convergence-verify/run-results.json`

This verifies the current fd-source, no-overlap, teardown-cleanup, BlueZ
`bneptest`, BlueZ Network, reconnect-stress, and iperf matrix gates after the
latest convergence changes.

## 2026-06-17 upstream convergence phase start

The next phase is now explicitly "upstream convergence / glue reduction".
For BNEP this means native evidence must be tied to completed Linux-shaped
operations, not to optimistic hwsim probe points.

First convergence tightening:

- `BNEPCONNADD` now records `bnep-native-fd-handoff` only after
  `bnep_add_connection()` succeeds and the ioctl result has been copied back to
  userspace successfully.
- If the user copy-out fails, the ioctl returns the failure and the hwsim
  evidence no longer claims a completed native fd handoff.
- Generic native BNEP ioctl counters now ignore failed ioctl returns.  A failed
  `BNEPCONNADD`, `BNEPCONNDEL`, `BNEPGETCONNLIST`, `BNEPGETCONNINFO`, or
  `BNEPGETSUPPFEAT` no longer contributes to the successful native ioctl
  coverage counters.
- The native fd handoff status now records the fd ownership source as either
  `socket-fd` or `kept-probe-l2cap`.  This keeps true BlueZ/`bneptest` socket
  fd handoff distinct from the compatibility path that reuses the kept probe
  L2CAP socket.
- Native BNEP datapath and BlueZ `bneptest` fd handoff validators now require
  `bnep-native-fd-source=socket-fd`.  The compatibility
  `kept-probe-l2cap` path is still visible for diagnostics, but it no longer
  satisfies native closeout evidence.
- Successful `BNEPCONNDEL` now clears the active fd ownership snapshot:
  fd/source return to `-1`/`none`, and PSM/CID/handle/role are reset to zero
  once the cleanup counter is recorded.
- Native BNEP teardown validators now require the cleared fd snapshot:
  `bnep-native-fd-last=-1`, zero PSM/CID/handle/role, and
  `bnep-native-fd-source=none`.
- A native fd handoff marker is ignored while another native fd ownership
  snapshot is already active.  This prevents a second observability event from
  silently replacing the active fd/source/PSM/CID/role state.
- Overlapping native fd handoff attempts are now counted in
  `bnep-native-fd-handoff-reject`.  The active snapshot is still preserved, but
  the rejected event is no longer silent.
- Current single-session native datapath closeout validators require
  `bnep-native-fd-handoff-reject=0`.  Any overlapping fd handoff attempt now
  fails the normal native closeout path instead of being tolerated as noise.
- Reconnect and reconnect-stress validators also require
  `bnep-native-fd-handoff-reject=0`, so repeated connect/disconnect cycles
  must keep fd ownership serialized across the whole lifecycle.
- BlueZ `bneptest` native-boundary and native-closeout lines now also print
  `fd-source=socket-fd`, and the validator requires that source marker for
  `bneptest` native fd handoff coverage.
- `sockfd_lookup` compatibility fallback use is now visible in status:
  `bnep-native-fd-lookup-kept-probe` counts direct kept-probe socket reuse, and
  `bnep-native-fd-lookup-rebind-probe` counts automatic probe socket rebuild
  from fd state.
- Current native BNEP datapath closeout validators now require both fallback
  lookup counters to remain zero.  Kept-probe and rebind-probe lookup paths are
  still visible for diagnostics, but they no longer satisfy native closeout.

Why this matters:

- The fd handoff marker is NuttX hwsim observability glue, not upstream Linux
  behavior.
- Moving it to the completed-success path makes the marker describe the same
  externally visible result that BlueZ or `bneptest` observes.
- This reduces a false-positive seam where a validator could otherwise see
  native handoff evidence from an ioctl that ultimately failed.
- Counting only successful native ioctls keeps the status output aligned with
  user-visible Linux ioctl results instead of treating failed attempts as
  successful upstream coverage.
- Separating `socket-fd` from `kept-probe-l2cap` makes the remaining
  compatibility seam visible instead of letting both paths satisfy the same
  evidence shape.
- Requiring `socket-fd` in native closeout turns that visible seam into an
  enforced convergence boundary: native coverage must come from a real fd
  handoff path.
- Clearing the fd snapshot on teardown keeps post-disconnect status aligned
  with Linux object lifetime: an inactive BNEP session no longer carries stale
  connected-fd ownership metadata.
- Requiring the cleared snapshot in teardown gates prevents stale fd metadata
  from being accepted as a completed native disconnect lifecycle.
- Rejecting overlapping handoff snapshots keeps the hwsim status model closer
  to the Linux object lifetime: one active BNEP session owns one connected
  L2CAP fd until teardown.
- Counting rejected overlapping handoffs keeps future multi-session or stale
  marker bugs visible instead of hiding them behind a stable active snapshot.
- Requiring zero rejects in the current closeout keeps this phase honest:
  one native session owns one fd, and extra handoff attempts must be fixed or
  moved into a future explicit multi-session validator.
- Applying the same zero-reject rule to reconnect gates prevents overlap bugs
  from hiding in lifecycle/stress coverage after the single-session datapath
  gate has already been tightened.
- Printing the fd source at the tool boundary makes source ownership auditable
  before the lower-level native status dump is inspected.
- Exposing fallback lookup counters makes remaining compatibility ownership
  measurable before those paths are removed or excluded from future native
  validators.
- Requiring zero fallback lookup counters shifts native closeout from "fallback
  is visible" to "fallback is forbidden for this coverage class".

Boundary:

- This is a convergence tightening, not a new feature.
- Future BNEP convergence should continue moving observability and ownership
  toward direct imported Linux `bnep/sock.c`, `core.c`, and `netdev.c` behavior,
  while preserving the existing fd handoff, datapath, iperf, MTU, reconnect,
  teardown, and staging-rejection gates.

## 2026-06-15 current BNEP/NET progress snapshot

BNEP/NET has reached the current hwsim functional closeout bar.

Current validated coverage:

- Native BNEP contract markers are emitted from `linux_bt_upstream_af_status()`.
- BlueZ Network and BlueZ `bneptest` paths require connected L2CAP fd handoff
  into the BNEP native ioctl/session path.
- Native datapath gates require Linux-shaped ownership:
  `ndo_start_xmit`, BNEP TX/RX, L2CAP/hwsim delivery, `netif_rx`, NuttX IP RX,
  teardown, reconnect, and cleanup.
- TCP/UDP iperf, reverse-direction iperf, MTU soak, reconnect stress, and
  BLE IPSP/6LoWPAN iperf have PASS artifacts in the current hwsim baseline.
- Validator checks now reject fallback staging on native datapath closeout via
  `bnep-staging-active=0`, `bnep-staging-add=0`, and `bnep-staging-del=0`.

Current boundary:

- The current BNEP/NET state is closed for hwsim functional validation.
- The remaining work is source-level convergence: continue shrinking staging
  and compatibility glue until more of the path is owned directly by imported
  Linux `bnep/core.c`, `sock.c`, `netdev.c`, and unmodified BlueZ Network
  profile behavior.
- Future changes must keep the current native fd handoff, netdev/datapath,
  iperf, MTU, reconnect, teardown, and staging-rejection gates green.

## 2026-06-14 BlueZ bneptest native fd handoff closeout

`bluez-bneptest-fd-handoff` 已加严为 connected L2CAP fd 进入 imported Linux BNEP native ioctl/session 的验证。

- `bluezbneptest fd-handoff` 创建 connected L2CAP fd。
- imported `bnep/sock.c` 在 `BNEPCONNADD` 成功后调用 `linux_bt_upstream_bnep_note_native_fd_handoff()`。
- `linux_bt_upstream_af_status()` 暴露 fd handoff state：`bnep-native-fd-handoff`、`bnep-native-fd-active`、`bnep-native-fd-psm`、`bnep-native-fd-cid`、`bnep-native-fd-handle`、`bnep-native-fd-role`。
- `BNEPCONNDEL` 成功后清理 `bnep-native-fd-active` 并递增 `bnep-native-fd-cleanup`。
- 最新验证：`FeatherCore/build/bt-hwsim-usecases-bluez-bneptest-fd-native-handoff/run-results.json`，结果 PASS。

## 2026-06-15 native BNEP contract status

`linux_bt_upstream_af_status()` 现在固定输出 native BNEP contract，供
`bluezbneptest`、`blueznetwork` 和 hwsim validator 共同验证。

新增 contract 字段：

```text
bnep-native-contract-version=1
bnep-native-source-map=sock.c,core.c,netdev.c,linux_bt_bnep_netdev.c
bnep-native-session-ownership=sockfd_lookup,BNEPCONNADD,bnep_add_connection,alloc_netdev,register_netdev,kthread_run,bnep_session
bnep-native-datapath-ownership=NuttX-IP,ndo_start_xmit,bnep_tx_frame,L2CAP,hwsim,bnep_rx_frame,netif_rx,NuttX-IP-RX
bnep-native-fd-ownership=connected-l2cap-fd,bnep-control-fd,sockfd_lookup,sockfd_put-error,session-sock-owner
bnep-native-cleanup-ownership=BNEPCONNDEL,bnep_del_connection,unregister_netdev,session_stop,fd_cleanup
```

意义：

- BNEP gate 不再只检查若干计数是否非零，而是要求 native socket/core/netdev/bridge 的 Linux-shaped ownership contract 同时出现。
- `BNEPCONNADD` 必须对应 connected L2CAP fd handoff、`bnep_add_connection()`、`register_netdev()`、`kthread_run()` 和 `bnep_session()`。
- datapath 必须对应 `NuttX IP -> Linux ndo_start_xmit -> BNEP -> L2CAP/hwsim -> BNEP RX -> netif_rx -> NuttX IP RX`。
- cleanup 必须对应 `BNEPCONNDEL`、session teardown、`unregister_netdev()` 和 fd cleanup。

边界：

- 这是 native Linux BNEP semantic contract 在 NuttX hwsim adapter 下的收紧。
- 仍不等于未修改 upstream `bluetoothd` Network plugin、真实 system D-Bus、SDP policy agent 和完全 controller-driven L2CAP/BNEP ownership 全部完成。

# Linux Bluetooth BNEP porting notes

Imported from:

```text
/home/uan/Feather-develop-BT/third/linux-hwe-6.17-6.17.0/net/bluetooth/bnep
```

Files:

```text
core.c
sock.c
netdev.c
bnep.h
```

Purpose:

- BR/EDR PAN/IP networking path for future `ping` and `iperf` hwsim tests.
- Keep Linux BNEP semantics authoritative; add NuttX compatibility shims rather than reimplementing BNEP behavior.

Current state:

- Source is imported and wired to Kconfig/Make.defs behind `CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP`.
- The config is intentionally off by default because Linux netdevice, module, socket, namespace, and skb/netdev operations still need compatibility work.
- Next implementation step is to map Linux BNEP netdev lifecycle in `netdev.c` onto a NuttX network device so PAN can expose an IP interface for BT1/BT2 ping and iperf.

## 2026-06-11 netdevice compat staging

BNEP `netdev.c` depends on Linux Ethernet netdevice semantics:

```text
struct net_device
struct net_device_stats
struct net_device_ops
alloc_netdev()
register_netdev()
unregister_netdev()
netdev_priv()
netif_start_queue()/netif_stop_queue()/netif_wake_queue()
netif_trans_update()
netif_rx()
ether_setup()
eth_validate_addr()
eth_broadcast_addr()
```

Reuse policy:

- Bluetooth now forwards `linux/netdevice.h`, `linux/etherdevice.h` and
  `linux/if_ether.h` through Bluetooth-local wrappers to the existing
  Wi-Fi/mac80211 compatibility layer via `include_next`.
- This avoids creating a second private Linux netdevice model under Bluetooth.
- Shared additions made for BNEP so far: `net_device.broadcast`,
  `net_device.stats`, `net_device.watchdog_timeo`, `netif_trans_update()` and
  a placeholder `netif_rx()`.

Still missing for a real PAN IP interface:

```text
Linux skb <-> NuttX packet handoff in netif_rx()
Linux net_device registration mapped to a NuttX network device
BNEP TX path from NuttX netdev xmit into L2CAP socket write queue
BNEP RX path from L2CAP socket into NuttX network stack
BNEP socket/ioctl path for BNEPCONNADD/BNEPCONNDEL and BlueZ network profile
```

Do not enable `CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP` in role defconfigs
until those pieces are intentionally staged.

## 2026-06-11 NuttX role IP tooling

The four NuttX sim Bluetooth hwsim role defconfigs now include the command and
socket prerequisites for later PAN IP tests:

```text
hwsim_bt1
hwsim_bt2
hwsim_ble1
hwsim_ble2
```

Enabled prerequisites:

```text
CONFIG_NET_IPv4=y
CONFIG_NET_TCP=y
CONFIG_NET_UDP=y
CONFIG_NET_ICMP_SOCKET=y
CONFIG_NET_SOCKOPTS=y
CONFIG_SYSTEM_PING=y
CONFIG_NETUTILS_IPERF=y
CONFIG_NET_TUN=y
CONFIG_SIM_NETDEV=y
CONFIG_NETUTILS_IPERFTEST_DEVNAME="btn0"
```

Validation:

```text
build/logs/build-bt1-ip-ping-iperf.log
build/logs/build-bt2-ip-ping-iperf.log
build/logs/build-ble1-ip-ping-iperf.log
build/logs/build-ble2-ip-ping-iperf.log
build/logs/preflight-bt-networking-ip-ping-iperf.log
```

This satisfies the defconfig/application side of the future `ping`/`iperf`
test.  The BNEP port still needs real session creation, connected L2CAP socket
ownership, and packet handoff between Linux BNEP `net_device` semantics and the
NuttX network stack before `btn0` can carry traffic between `bt1` and `bt2`.

## 2026-06-11 BNEP ioctl session lifecycle staging

The fallback BNEP socket path now keeps a small Linux-shaped session table
while the full imported BNEP source remains behind
`CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP`.

Implemented ioctl behavior:

```text
BNEPGETSUPPFEAT  -> BIT(BNEP_SETUP_RESPONSE)
BNEPCONNADD      -> create staging session, return device name such as btn-l2
BNEPGETCONNLIST  -> return current staging sessions
BNEPGETCONNINFO  -> return role/state/dst/device for a session
BNEPCONNDEL      -> delete staging session
```

Validation:

```text
build/logs/build-bt1-bnep-l2cap-session.log
build/logs/build-bt1-bnep-l2cap-session-final.log
build/logs/run-bt-hwsim-bnep-l2cap-session.log
build/bt-hwsim-usecases-bnep-l2cap-session/bnep-session.bt1.log
```

Observed pass evidence:

```text
upstream-l2cap-bind: psm=0x0019 cid=0x0041 handle=0x0040 create-ret=0 bind-ret=0
upstream-l2cap-connect: psm=0x0019 cid=0x0041 connect-ret=0 state=1
name=BNEPCONNADD ioctl-ret=0 connadd-device=btn-l2 bnep-staging-active=1
name=BNEPGETCONNLIST ioctl-ret=0 connlist-cnum=1 connlist0-state=0x0001 connlist0-device=btn-l2
name=BNEPGETCONNINFO ioctl-ret=0 conninfo-role=0x1115 conninfo-state=0x0001 conninfo-device=btn-l2
name=BNEPCONNDEL ioctl-ret=0 bnep-staging-active=0
upstream-l2cap-close: released
```

This is intentionally still a staging boundary.  Native Linux BNEP obtains the
session peer and transport from a connected L2CAP socket passed through
`BNEPCONNADD`.  The current `connadd l2cap` path uses the kept L2CAP socket
owned by the NSH probe, so the next porting step is BlueZ-style fd lookup and
then binding the resulting BNEP `net_device` to the shared NuttX
network-device adapter.

## 2026-06-11 BNEP netdev lifecycle staging

The fallback BNEP session table now mirrors the next Linux BNEP lifecycle
boundary by attaching a staging network-device state to each active session.

Current observable fields:

```text
bnep-netdev-registered=1
bnep-netdev-name=btn-l2
bnep-netdev-mtu=1500
```

`BNEPCONNDEL` clears that state and increments the unregister counter exposed
through `btctl upstream status`.

This is deliberately a lifecycle marker, not yet packet handoff.  The real data
path still needs to replace this marker with Linux `register_netdev()` through
the shared `ieee80211/netdevice_compat.c` adapter, then wire:

```text
NuttX IP TX -> Linux net_device ndo_start_xmit -> BNEP -> L2CAP
L2CAP RX -> BNEP -> netif_rx() -> NuttX IP RX
```

Local non-network checks:

```text
build/logs/show-bt-hwsim-bnep-netdev-staging-case.log
build/logs/list-bt-hwsim-usecases-after-bnep-netdev.log
build/logs/pycompile-bt-hwsim-usecases-after-bnep-netdev.log
```

Offline build attempt:

```text
build/logs/build-bt1-bnep-netdev-staging.log
```

It currently stops on the missing local `argtable3` dependency archive because
external network access is intentionally disabled for this phase.

## 2026-06-11 BNEP netdev bridge boundary

The Linux-BT tree now has an explicit BNEP/PAN netdev bridge boundary:

```text
wireless/linux_bluetooth/linux_bt_bnep_netdev.c
```

This file intentionally does not reimplement the generic Linux netdevice
adapter.  It wraps the shared Linux `register_netdev()`,
`unregister_netdev()` and `netif_rx()` compatibility path so imported BNEP can
continue to own PAN protocol behavior while NuttX owns the eventual IP packet
handoff.

Additional compatibility staged in this step:

```text
net_device.mtu
net_device.hard_header_len
net_device.tx_queue_len
sk_buff.dev
sk_buff.protocol
sk_buff.mac_header
skb_reset_mac_header()
skb_mac_header()
__skb_put_data()
eth_type_trans()
```

`NET_LINUX_BLUETOOTH_UPSTREAM_BNEP` also pulls in
`ieee80211/netdevice_compat.c` when the Wi-Fi Linux netdevice adapter is not
already built, keeping one shared Linux `struct net_device` -> NuttX
`struct net_driver_s` bridge.

The remaining hard part is still the real data transfer:

```text
NuttX IP TX -> shared netdevice adapter -> BNEP ndo_start_xmit()
BNEP session thread -> L2CAP socket write queue -> hwsim ACL medium
hwsim ACL medium -> L2CAP socket receive queue -> BNEP RX
BNEP RX -> netif_rx() -> NuttX IP RX
```

## 2026-06-11 BNEP socket/ioctl staging

Fallback `bnep_sock_init()` now registers `BTPROTO_BNEP` through the imported
upstream `AF_BLUETOOTH` family when the full upstream BNEP source is still
disabled:

```text
PF_BLUETOOTH + BTPROTO_BNEP + SOCK_RAW
```

The nsh probes are:

```text
btctl upstream socket bnep
btctl upstream bnep-ioctl suppfeat
btctl upstream bnep-ioctl connlist
btctl upstream bnep-ioctl conninfo [addr-seed]
btctl upstream bnep-ioctl connadd [sock-fd]
btctl upstream bnep-ioctl conndel [addr-seed]
```

Earlier socket-only behavior, superseded by the later session/netdev staging
steps above:

- `BNEPGETSUPPFEAT` reports `BNEP_SETUP_RESPONSE`.
- `BNEPGETCONNLIST` initially reported an empty session list.
- `BNEPGETCONNINFO` initially reported no matching session.
- `BNEPCONNADD` initially reported not connected before the kept-L2CAP
  session path was added.
- `BNEPCONNDEL` initially reported no matching session.

The staging code keeps the Linux/BlueZ-facing ioctl shape visible while
avoiding a fake PAN data plane.  The local probe uses internal command IDs
until the NuttX ioctl layer has a Linux-compatible `_IOW/_IOR` bridge for the
actual userspace ABI.  Once imported upstream BNEP `core.c/sock.c/netdev.c`
can compile with the shared compat layer, this fallback should give way to the
real upstream BNEP implementation.

## 2026-06-11 BNEP hwsim `btn0` ping milestone

BT1 <-> BT2 now has a first passing IP networking test over the Bluetooth hwsim public-file medium.

Implementation summary:

```text
BNEPCONNADD l2cap
  -> staged BNEP session
  -> linux_bt_bnep_hwsim_netdev_register("btn%d")
  -> NuttX Ethernet-like lower-half netdev named btn0
  -> LINUX_BT_HWSIM_TYPE_BNEP / bt-hwsim-bnep.bin
  -> peer btn0
```

This is intentionally a simulator data-plane bridge. It proves that the BNEP/PAN session lifecycle can expose an IP-facing interface and move Ethernet frames through the same public-file hwsim model used elsewhere in the BT sim. It does not replace the imported upstream Linux BNEP implementation.

Validation logs:

```text
build/logs/build-bt1-bnep-ping-btn0.log
build/logs/build-bt2-bnep-ping-btn0.log
build/bt-hwsim-usecases-bnep-ping-btn0/bnep-ping.bt1.log
build/bt-hwsim-usecases-bnep-ping-btn0/bnep-ping.bt2.log
```

Result:

```text
PASS bnep-ping
```

The `bnep-ping` case configures:

```text
BT1 btn0 10.77.0.1/24 -> ping 10.77.0.2
BT2 btn0 10.77.0.2/24 -> ping 10.77.0.1
```

Both directions report `0% packet loss`.

## 2026-06-12 BlueZ-derived fd handoff native data-path milestone

The BR/EDR PAN path has moved past the earlier staged target.  The current
validated path is now:

```text
bluezbneptest pan-up
  -> socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP)
  -> bind/connect public L2CAP fd
  -> socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_BNEP)
  -> ioctl(BNEPCONNADD, connected_l2cap_fd)
  -> imported Linux bnep/sock.c
  -> imported Linux bnep/core.c bnep_add_connection()
  -> imported Linux BNEP net_device_ops / bnep_session
  -> NuttX btn0 lower-half adapter
  -> hwsim ACL/BNEP medium
  -> peer imported Linux BNEP RX / netif_rx()
```

Validated gates:

```text
PASS bluez-bneptest-fd-handoff
PASS bluez-bneptest-ping
PASS bluez-bneptest-reconnect
```

Current evidence for the latest reconnect cleanup run:

```text
build/logs/build-bt1-bluez-bneptest-l2cap-close-cleanup-fixed.log
build/logs/build-bt2-bluez-bneptest-l2cap-close-cleanup-fixed.log
build/logs/run-bluez-bneptest-native-bnep-reconnect-cleanup.log
build/bt-hwsim-usecases-bluez-bneptest-native-bnep-reconnect-cleanup/run-results.json
```

The `bluez-bneptest-reconnect` gate now verifies two complete
`pan-up -> ping -> pan-down` rounds on both BT1 and BT2.  The validator
requires `bnep-ioctl-connadd/conndel=2`,
`bnep-native-session-create/start/stop/terminate=2`,
`bnep-native-netdev-register/unregister=2`, final `bnep-native-active=0`,
and zero packet loss for the IPv4 ping traffic.

The same cleanup has also been verified by the three-round stress gate:

```text
PASS bluez-bneptest-reconnect-stress
build/logs/run-bluez-bneptest-native-bnep-reconnect-stress-cleanup.log
build/bt-hwsim-usecases-bluez-bneptest-native-bnep-reconnect-stress-cleanup/run-results.json
```

That gate requires `bnep-ioctl-connadd/conndel=3`,
`bnep-native-session-create/start/stop/terminate=3`,
`bnep-native-netdev-register/unregister=3`, final `bnep-native-active=0`,
and zero packet loss for all three ping rounds on both BT1 and BT2.

The close-cleanup build has also re-validated the BlueZ-derived native BNEP
throughput matrix:

```text
PASS bluez-bneptest-iperf-tcp
PASS bluez-bneptest-iperf-tcp-reverse
PASS bluez-bneptest-iperf-udp
PASS bluez-bneptest-iperf-udp-reverse

build/logs/run-bluez-bneptest-native-bnep-iperf-tcp-cleanup.log
build/logs/run-bluez-bneptest-native-bnep-iperf-tcp-reverse-cleanup.log
build/logs/run-bluez-bneptest-native-bnep-iperf-udp-cleanup.log
build/logs/run-bluez-bneptest-native-bnep-iperf-udp-reverse-cleanup.log
```

Those gates require `Mbits/sec`, `iperf exit`, `BNEPCONNADD` with a real
connected L2CAP fd, `bnep-staging-add=0`, and non-zero native data-path
counters such as `bnep-native-netdev-xmit`, `bnep-native-tx-frame-ok`,
`bnep-native-l2cap-delivered`, `bnep-native-rx-frame-ok`, and
`bnep-native-netif-rx`.

Implementation note: per-fd upstream L2CAP close now clears the NuttX-side
L2CAP bound-state table by `sk` and by `psm/cid/handle` before releasing the
upstream socket.  Without this cleanup the second BlueZ `pan-up` failed at
L2CAP `bind()` with `EADDRINUSE` after the first native BNEP session had
already stopped.

Remaining BNEP/PAN porting targets:

- Replace the current synthetic L2CAP connect shim with fully adapted
  upstream `l2cap_sock_connect()` semantics.
- Move from BlueZ-derived `bluezbneptest` to the full `bluetoothd` network
  plugin path, including service/profile policy.
- Cover multi-peer/multi-session, abnormal process exit cleanup, and longer
  soak/throughput runs.

## 2026-06-12 BlueZ-derived bneptest fd handoff ping recheck

The current tree revalidates the BlueZ-derived `bneptest.c` path with the `bluez-bneptest-ping` hwsim case.  This is no longer only the `btctl upstream bnep-ioctl connadd l2cap` staging probe: `bluezbneptest pan-up` opens L2CAP and BNEP sockets and passes the connected L2CAP fd through `BNEPCONNADD`, matching the Linux userspace ABI shape used by BlueZ networking tools.

Latest gate:

```text
PASS bluez-bneptest-ping
build/logs/build-bt1-bluez-bneptest-ping-recheck.log
build/logs/build-bt2-bluez-bneptest-ping-recheck.log
build/logs/run-bluez-bneptest-ping-recheck.log
build/bt-hwsim-usecases-bluez-bneptest-ping-recheck/run-results.json
```

The validator checks fd handoff, `btn0`, IPv4 ping, imported/native BNEP TX/RX counters, and teardown.  Representative counters from the passing run:

```text
BT1 bnep-native-netdev-xmit=10 bnep-native-tx-frame-ok=10 bnep-native-l2cap-delivered=11 bnep-native-rx-frame-ok=22 bnep-native-netif-rx=22
BT2 bnep-native-netdev-xmit=11 bnep-native-tx-frame-ok=11 bnep-native-l2cap-delivered=10 bnep-native-rx-frame-ok=20 bnep-native-netif-rx=20
```

Remaining BNEP/PAN work is now narrower: BlueZ Network profile daemon ownership, iperf TCP/UDP throughput, large frame and fragmentation stress, reconnect/long-duration stability under load, and replacing any remaining NuttX lower-half shortcut semantics with fuller Linux BNEP session-thread behavior where observable timing or error handling differs.

## 2026-06-12 BlueZ-derived BNEP iperf recheck with non-zero throughput gate

The four BlueZ-derived BNEP iperf hwsim gates were re-run and passed on the current worktree:

```text
PASS bluez-bneptest-iperf-tcp
PASS bluez-bneptest-iperf-tcp-reverse
PASS bluez-bneptest-iperf-udp
PASS bluez-bneptest-iperf-udp-reverse
```

Artifacts:

```text
build/logs/run-bluez-bneptest-iperf-tcp-recheck.log
build/logs/run-bluez-bneptest-iperf-tcp-reverse-recheck.log
build/logs/run-bluez-bneptest-iperf-udp-recheck.log
build/logs/run-bluez-bneptest-iperf-udp-reverse-recheck.log
build/bt-hwsim-usecases-bluez-bneptest-iperf-tcp-recheck/run-results.json
build/bt-hwsim-usecases-bluez-bneptest-iperf-tcp-reverse-recheck/run-results.json
build/bt-hwsim-usecases-bluez-bneptest-iperf-udp-recheck/run-results.json
build/bt-hwsim-usecases-bluez-bneptest-iperf-udp-reverse-recheck/run-results.json
```

The validator now includes a stricter `BNEP_IPERF_THROUGHPUT_REQUIRED` regex so an iperf gate requires at least one non-zero Bytes and non-zero `Mbits/sec` line in addition to the existing `BNEPCONNADD`, `btn0`, client `iperf exit`, and native BNEP TX/RX counter checks.

This moves BT BNEP iperf from unverified to validated for the basic TCP/UDP bidirectional hwsim cases.  Remaining BNEP stress work is larger frames, longer runs, concurrent sessions, packet-loss/jitter criteria for UDP, and daemon-level BlueZ Network profile ownership.

## 2026-06-14 BlueZ bneptest native BNEP datapath closeout

`bluez-bneptest-ping` 和四个 `bluez-bneptest-iperf-*` hwsim gate 已重新验证通过，并且验证门槛从 fd handoff diagnostic 扩展到 native BNEP datapath counter。

新增证据采集：

- `bluezbneptest status` 打印 `linux_bt_upstream_af_status()`。
- iperf server/client 两侧在 traffic 完成后、`BNEPCONNDEL` 前采集 native status。
- validator 要求 connected L2CAP fd handoff 后 `bnep-native-fd-active=1`，并要求 native BNEP TX/RX counter 非零。

通过的 gates：

```text
PASS bluez-bneptest-ping
PASS bluez-bneptest-iperf-tcp
PASS bluez-bneptest-iperf-tcp-reverse
PASS bluez-bneptest-iperf-udp
PASS bluez-bneptest-iperf-udp-reverse
```

验证产物：

```text
build/logs/build-bt1-bluez-bneptest-datapath-fd-native-closeout.log
build/logs/build-bt2-bluez-bneptest-datapath-fd-native-closeout.log
build/logs/run-bluez-bneptest-datapath-fd-native-closeout.log
build/bt-hwsim-usecases-bluez-bneptest-datapath-fd-native-closeout/run-results.json
```

当前 BNEP/PAN 基础数据面已经证明：BlueZ-derived userspace ABI -> `BNEPCONNADD` connected L2CAP fd -> imported Linux BNEP session/netdev -> hwsim medium -> peer native RX -> NuttX IP stack。剩余 BNEP/PAN 工作继续聚焦完整 `bluetoothd` Network plugin ownership、NAP/GN/PANU lifecycle、策略错误路径和更深压力矩阵。

## 2026-06-15 BlueZ Network native BNEP ownership closeout

The BT Network closeout has been tightened around the imported Linux BNEP path instead of the earlier hwsim-only bridge boundary.

Required ownership evidence now includes:

- BlueZ `bneptest` connected L2CAP fd handoff into `BNEPCONNADD`.
- BlueZ Network Profile `Network1.Connect` handoff into the same native BNEP ioctl path.
- Imported Linux BNEP `sock.c` ownership for `BNEPGETSUPPFEAT`, `BNEPCONNADD`, `BNEPGETCONNLIST`, `BNEPGETCONNINFO`, and `BNEPCONNDEL`.
- Imported Linux BNEP `core.c` ownership for `bnep_add_connection`, session link/unlink, `kthread_run`, session thread, TX/RX queues, `bnep_tx_frame`, and `bnep_rx_frame`.
- Imported Linux BNEP `netdev.c` ownership for `register_netdev`, `unregister_netdev`, `ndo_start_xmit`, and `netif_rx`.
- NuttX lower-half bridge ownership for the Linux netdev TX/RX handoff into the NuttX IP stack.
- Final cleanup ledger with no active fd, session, thread, netdev, queue, skb, L2CAP ref, or BNEP ref.

This moves BT PAN from staged bridge validation toward the requested Linux semantic path:

```text
NuttX IP -> Linux net_device ndo_start_xmit -> BNEP -> L2CAP -> hwsim
hwsim -> L2CAP -> BNEP -> netif_rx -> NuttX IP
```

The next sequential network target is BLE IP: complete BLE1 <-> BLE2 ping over Linux-style IPSP/6LoWPAN with the same ownership-ledger standard.

## 2026-06-15 BT BNEP iperf native ownership gate tightening

BT BNEP iperf gates now distinguish generic throughput evidence from BlueZ Network Profile ownership evidence.

- `BNEP_IPERF_THROUGHPUT_REQUIRED` remains a generic non-zero throughput check and is safe for `btbneptest`, `bluez-bneptest`, and `blueznetwork` cases.
- `BNEP_NETWORK_IPERF_OWNERSHIP_REQUIRED` is specific to `blueznetwork` cases and requires the BlueZ Network Profile handoff path:

```text
Network1.Connect -> connected L2CAP fd -> BNEPCONNADD -> btn0
```

The Network iperf gate now also requires native BNEP fd ownership, session ownership, datapath ownership, status after connect/status/disconnect, and final `bnep-native-active=0` cleanup evidence.  This means TCP, TCP reverse, UDP, UDP reverse, soak, and matrix iperf runs must prove both non-zero throughput and imported Linux BNEP ownership.

## 2026-06-15 upstream helper/object contract tightening

The BNEP native status path now exposes an explicit helper/object contract:

```text
bnep-native-helper-contract=sock_ioctl_connadd,bnep_add_connection,netdev_setup,session_thread,ndo_start_xmit,bnep_rx_frame,conndel_cleanup
bnep-native-helper-owner=net_bluetooth/bnep
bnep-native-helper-boundary=bluez-fd-handoff-to-imported-bnep
```

This is the current semantic boundary for the NuttX hwsim port:

```text
BlueZ Network/bneptest connected L2CAP fd
        |
        v
BNEPCONNADD / imported BNEP socket ioctl ownership
        |
        v
imported bnep_add_connection + Linux netdev setup
        |
        v
BNEP session thread + ndo_start_xmit + bnep_rx_frame
        |
        v
BNEPCONNDEL cleanup
```

The validator now requires this contract for native datapath, reconnect, and reconnect-stress gates.  This keeps future BNEP validation aligned to Linux object ownership instead of accepting bridge-side counters alone.

## 2026-06-15 native datapath gate rejects staging fallback

The common `BNEP_NATIVE_DATAPATH_REQUIRED` validator contract now also
requires the legacy fallback session counters to stay at zero:

```text
bnep-staging-active=0
bnep-staging-add=0
bnep-staging-del=0
```

This makes the hwsim BNEP/PAN gates fail if a future change silently falls
back to the old `BNEP-staging` socket path.  The fallback remains in the tree
only for diagnostic or non-upstream-BNEP configurations; it no longer counts
as acceptable evidence for native datapath closeout.

Revalidated gates under the stricter contract:

```text
PASS bluez-network-iperf-matrix
PASS bluez-bneptest-iperf-tcp
```

Current native datapath acceptance rule:

```text
connected L2CAP fd -> BNEPCONNADD -> imported BNEP sock/core/netdev
  -> Linux net_device ndo_start_xmit -> BNEP/L2CAP/hwsim
  -> peer BNEP rx_frame -> netif_rx -> NuttX IP
```

Any path that increments `bnep-staging-add` or leaves
`bnep-staging-active` non-zero is now treated as a regression for the native
BNEP/PAN closeout gates.

## 2026-06-17 kernel_sendmsg-only BNEP TX convergence

The imported BNEP core no longer owns a direct hwsim ACL fallback from
`bnep_tx_frame()`.  BNEP TX must now leave the BNEP layer through the upstream
shape:

```text
BNEP netdev -> bnep_tx_frame() -> kernel_sendmsg(sock) -> L2CAP -> HCI/hwsim
```

Supporting changes:

- The exported status contract includes
  `bnep-native-core-tx-path=kernel_sendmsg-only`.
- BlueZ-derived L2CAP socket fd handoff now uses hwsim BR handle `0x0052`, which
  is inside the sim BR connection range.
- The L2CAP send-side attach path now uses `l2cap_sim_attach_channel()` so the
  socket seen by `bnep_add_connection()` has an upstream-shaped `l2cap_chan` /
  `l2cap_conn` relationship.

Validated after the change:

```text
PASS bluez-bneptest-fd-handoff
PASS bluez-bneptest-ping
PASS bluez-bneptest-reconnect-stress
PASS bluez-network-ping
PASS bluez-network-reconnect-stress
```

Validation artifacts:

```text
FeatherCore/build/logs/build-bt1-bnep-kernel-sendmsg-sockif-handle.log
FeatherCore/build/logs/build-bt2-bnep-kernel-sendmsg-sockif-handle.log
FeatherCore/build/logs/run-bnep-kernel-sendmsg-final-verify.log
FeatherCore/build/bt-hwsim-bnep-kernel-sendmsg-final-verify/
```

Known residual:

- `bluez-network-iperf-matrix` still misses the fourth full lifecycle on both
  roles.  The data path itself is active in that run, but the matrix does not
  satisfy the strict `connadd/conndel/session-stop/session-terminate=4` gate.
  Keep this as a Network iperf multi-cycle residual rather than weakening the
  kernel-sendmsg-only BNEP contract.

## 2026-06-18 BNEP-owned L2CAP MTU handoff closeout

The kernel-sendmsg-only BNEP path exposed one final MTU ownership gap: the
connected L2CAP fd handed to `BNEPCONNADD` could be marked BNEP-owned without
being attached to the sim L2CAP channel.  Small ping traffic could pass, but
1400-byte PAN payloads failed in `bnep_tx_frame()` with `kernel_sendmsg()`
returning `-EMSGSIZE`.

Fix summary:

- `linux_bt_upstream_l2cap_socket_mark_bnep_owner()` now ensures the BR/EDR
  sim connection exists and calls the L2CAP sim attach path for the handed-off
  fd.
- The existing `l2cap_sim_attach_channel()` path raises `conn->mtu`,
  `chan->omtu`, and `chan->imtu` to the sim hwsim MTU, so imported BNEP can send
  MTU-sized PAN frames through the upstream-shaped L2CAP socket.
- VHCI now returns hwsim ACL/ISO send errors to the caller, making future medium
  failures visible instead of silently queuing the skb.
- `bnep_tx_frame()` logs `BNEP TX failed len ... ret ...` on failure; the final
  closeout run has no such entries.

Validation:

```text
PASS bluez-current-functional-closeout
PASS bluez-a2dp-upstream-convergence-closeout
PASS bluez-le-audio-umbrella
PASS bluez-mgmt-reconnect-stress
PASS bluez-hciioctl-basic
PASS hci-le-lifecycle
```

Artifacts:

```text
FeatherCore/build/logs/run-bnep-attach-owner-closeout.log
FeatherCore/build/bt-hwsim-bnep-attach-owner-closeout/
FeatherCore/build/logs/run-core-gates-after-bnep-attach.log
FeatherCore/build/bt-hwsim-core-gates-after-bnep-attach/
```

The accepted BNEP/PAN datapath remains:

```text
BlueZ Network1.Connect
  -> connected L2CAP fd
  -> BNEPCONNADD
  -> imported BNEP session/netdev
  -> bnep_tx_frame()
  -> kernel_sendmsg()
  -> imported L2CAP socket/channel
  -> VHCI ACL hwsim medium
  -> peer BNEP rx_frame()
  -> netif_rx()
  -> NuttX IP
```

## 2026-06-18 BNEP staging fallback made diagnostic-only

The legacy `BNEP-staging` socket fallback has been moved behind the explicit
`CONFIG_NET_LINUX_BLUETOOTH_BNEP_STAGING_DIAGNOSTIC` option.  The default
closeout path must use imported Linux BNEP through
`CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_BNEP` and must report that the staging
fallback is not compiled.

Implementation notes:

- `NET_LINUX_BLUETOOTH_BNEP_STAGING_DIAGNOSTIC` is disabled by default,
  depends on `NET_LINUX_BLUETOOTH_UPSTREAM_AF`, and is mutually exclusive with
  `NET_LINUX_BLUETOOTH_UPSTREAM_BNEP`.
- `linux_bt_upstream_af_status()` now emits
  `bnep-staging-fallback-compiled=0/1`.
- Native BNEP validators require `bnep-staging-fallback-compiled=0` together
  with zero staging active/add/delete counters.
- Non-upstream BNEP probe helper behavior is preserved without restoring the
  old staging socket registration path.

Validation:

- Four role builds passed with logs under
  `FeatherCore/build/logs/build-{bt1,bt2,ble1,ble2}-bnep-no-staging-fallback-v3.log`.
- `FeatherCore/build/logs/run-bnep-no-staging-fallback-closeout.log` passed
  `bluez-current-functional-closeout`.
- `FeatherCore/build/bt-hwsim-bnep-no-staging-fallback-closeout/run-results.json`
  reports `passed=true` and `validate_rc=0`.
- Negative scan found no BNEP TX, ping, datapath, or staging-fallback regression
  markers.

Current convergence meaning:

- BNEP/PAN no longer has an implicit staging fallback in the active hwsim
  closeout contract.
- The remaining BNEP work is not to reintroduce that shortcut, but to keep
  reducing surrounding socket/lower-half/hwsim helper glue while preserving the
  imported Linux BNEP object graph as the owner of PAN behavior.
