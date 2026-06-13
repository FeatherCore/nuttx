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
