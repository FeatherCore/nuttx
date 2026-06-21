## 2026-06-14 BLE NET/IPSP owner/refcount closeout

本轮继续收口 BLE NET：`bluez-daemon-ipsp-closeout-full` 已加严为 upstream 6LoWPAN owner/refcount 生命周期验证，不再只依赖 datapath counters。

新增强制 evidence：

- active 阶段：`upstream-owner-state=netdev:1,coc:1,peer:1`
- active refs：`upstream-owner-refs=netdev:1,chan:1,peer:1`
- active datapath：`upstream-owner-active-tx` 必须非零
- cleanup 阶段：`upstream-owner-state=netdev:0,coc:0,peer:0`
- cleanup refs：`upstream-owner-refs=netdev:0,chan:0,peer:0`

实现点：

- `net/bluetooth/6lowpan.c` 维护 sim IPSP owner active/refcount state。
- `linux_bt_6lowpan_status()` 输出 owner state/refcount/active TX/RX。
- validator 在 BLE1/BLE2 两端同时检查 active 和 cleanup 状态。

验证结果：PASS

- build: `FeatherCore/build/logs/build-ble1-bluez-daemon-ipsp-owner-state.log`
- build: `FeatherCore/build/logs/build-ble2-bluez-daemon-ipsp-owner-state.log`
- run: `FeatherCore/build/logs/run-bluez-daemon-ipsp-owner-state.log`
- manifest: `FeatherCore/build/bt-hwsim-usecases-bluez-daemon-ipsp-owner-state/run-results.json`

当前结论：BLE NET/IPSP 的 daemon 路径现在覆盖 BlueZ daemon profile/D-Bus/mainloop、upstream L2CAP socket ABI、kernel 6LoWPAN owner handoff、ping/TCP/UDP/IPHC/fragment，以及断开后的 owner/refcount 清理闭环。

## 2026-06-14 bluetoothd-style LE IPSP L2CAP socket ownership tightening

本轮继续收口 BLE NET：`bluezdaemon ipsp-connect` 不再只打印 `fd-handoff=le-l2cap-coc` marker，而是在 profile connect 阶段先通过 Linux upstream L2CAP socket ABI 建立 IPSP CoC 证据，再注册 6LoWPAN `bt0`。

新增强制 evidence：

- `upstream-l2cap-bind: psm=0x0023 cid=0x0040 handle=0x0074 create-ret=0 bind-ret=0`
- `upstream-l2cap-connect: psm=0x0023 cid=0x0040 connect-ret=0`
- `upstream-l2cap-close: released`
- validator 要求两轮 daemon IPSP lifecycle 都出现上述 L2CAP socket ABI evidence。

验证结果：PASS

- build: `FeatherCore/build/logs/build-ble1-bluez-daemon-ipsp-l2cap-socket.log`
- build: `FeatherCore/build/logs/build-ble2-bluez-daemon-ipsp-l2cap-socket.log`
- run: `FeatherCore/build/logs/run-bluez-daemon-ipsp-l2cap-socket.log`
- manifest: `FeatherCore/build/bt-hwsim-usecases-bluez-daemon-ipsp-l2cap-socket/run-results.json`

`bluez-daemon-ipsp-closeout-full` 现在同时覆盖：

- daemon Profile1/mainloop/D-Bus/security lifecycle。
- LE IPSP L2CAP socket bind/connect/close ownership through upstream socket ABI。
- `fd-handoff=le-l2cap-coc owner=kernel-6lowpan profile=ipsp`。
- BLE1/BLE2 `bt0` IPv6 ping。
- TCP/UDP iperf。
- IPHC TX/RX 与 fragment counters。
- `upstream-owner=net_bluetooth/6lowpan+sim-ipsp-datapath-owner`。
- 两轮 lifecycle 与 final cleanup。

当前结论：BLE NET 的 daemon closeout 已经从 pure marker 推进到实际 upstream L2CAP socket ABI + 6LoWPAN datapath 的组合验证，离 Linux/BlueZ 原生 IPSP profile 语义更近了一步。

边界：这仍不是 unmodified upstream `bluetoothd` 的真实 IPSP plugin/ObjectManager/mainloop，也不是完全由 imported `net/bluetooth/6lowpan.c` 独占 peer/netdev callback owner。下一步 BLE NET 若继续深入，应让 `bluezdaemon` 的 profile/session/refcount/mainloop 更接近真实 BlueZ daemon 结构，并继续把 wrapper/counter evidence 替换为实际 upstream callback ownership。

## 2026-06-14 bluetoothd-style LE IPSP daemon closeout

本轮继续收口 BLE NET，从 `bluezipsp` adapter 再推进到 `bluezdaemon` 的 bluetoothd-style IPSP profile/mainloop/D-Bus lifecycle。

新增内容：

- `bluezdaemon ipsp-connect [ifname]`
- `bluezdaemon ipsp-status`
- `bluezdaemon ipsp-disconnect`
- 新增 hwsim case：`bluez-daemon-ipsp-closeout-full`

新增强制 evidence：

- daemon profile registration：`third/bluez/src/main.c + src/profile.c + src/device.c + profiles/network/connection.c + profiles/network/ipsp.c`。
- `dbus=org.bluez.Profile1`，`owner=bluetoothd`，`security=medium`，`authorize=ok`。
- daemon mainloop ownership：`watch-add=mgmt,dbus,l2cap-coc,6lowpan`，`timer-add=connect-timeout`。
- profile connect：`fd-handoff=le-l2cap-coc owner=kernel-6lowpan profile=ipsp connected=1`。
- D-Bus `InterfacesAdded` / `InterfacesRemoved` for `/org/bluez/hci0/dev_peer/ipsp0`。
- owner-lost cleanup 与 mainloop cleanup：`watches=0 timers=0 owner=bluetoothd`。

验证结果：PASS

- build: `FeatherCore/build/logs/build-ble1-bluez-daemon-ipsp-closeout.log`
- build: `FeatherCore/build/logs/build-ble2-bluez-daemon-ipsp-closeout.log`
- run: `FeatherCore/build/logs/run-bluez-daemon-ipsp-closeout.log`
- manifest: `FeatherCore/build/bt-hwsim-usecases-bluez-daemon-ipsp-closeout/run-results.json`

`bluez-daemon-ipsp-closeout-full` 同时覆盖：

- BLE1/BLE2 daemon-style IPSP connect/status/disconnect。
- Profile1 registration/mainloop/D-Bus ownership/security lifecycle。
- BLE1/BLE2 `bt0` IPv6 ping。
- TCP/UDP iperf。
- IPHC TX/RX 与 fragment counters。
- `upstream-owner=net_bluetooth/6lowpan+sim-ipsp-datapath-owner`。
- 两轮 lifecycle 与 final cleanup。

当前结论：BLE NET 已从 `btctl` probe、`bluezipsp` adapter，推进到 `bluezdaemon` bluetoothd-style IPSP closeout gate，并在完整 BLE 6LoWPAN/IP datapath 下通过。

边界：这仍然不是 unmodified upstream `bluetoothd` 的真实 IPSP plugin/ObjectManager/mainloop 实例。下一步 BLE NET 若继续深入，应把 `bluezdaemon ipsp-*` 的 markers 替换为真实 BlueZ daemon object/session/refcount/mainloop，真实 LE L2CAP CoC socket ownership，以及 imported `net/bluetooth/6lowpan.c` peer/netdev callback owner 的实际执行路径。

## 2026-06-14 BlueZ-facing LE IPSP D-Bus/profile lifecycle tightening

本轮继续收口 BLE NET 的 BlueZ 用户态语义：`bluezipsp` 不再只输出 connect/status/disconnect 的 profile-shaped marker，而是补充并强制验证 BlueZ profile/D-Bus/security lifecycle evidence。

新增强制 evidence：

- `profile register service=ipsp`，锚定 `third/bluez/src/profile.c + third/bluez/src/device.c`。
- IPSP UUID `00001820-0000-1000-8000-00805f9b34fb` 与 `org.bluez.Network1` object。
- `owner=:client.ipsp`、`security=medium`、`authorize=ok`、`connect-profile=ok`。
- D-Bus object add：`/org/bluez/hci0/dev_peer/ipsp0`，interfaces `org.bluez.Network1,org.bluez.Device1`。
- status 查询要求 `dbus-owner=:client.ipsp` 与 `connected-query=ok`。
- disconnect 要求 owner-lost cleanup、`InterfacesRemoved` 语义和 object-remove 后 `objects=0 owners=0 refs=0`。

验证结果：PASS

- build: `FeatherCore/build/logs/build-ble1-bluez-ipsp-dbus-lifecycle.log`
- build: `FeatherCore/build/logs/build-ble2-bluez-ipsp-dbus-lifecycle.log`
- run: `FeatherCore/build/logs/run-bluez-ipsp-dbus-lifecycle.log`
- manifest: `FeatherCore/build/bt-hwsim-usecases-bluez-ipsp-dbus-lifecycle/run-results.json`

`bluez-ipsp-closeout-full` 现在同时覆盖：

- BlueZ-facing IPSP profile connect/status/disconnect。
- D-Bus object ownership、owner-lost cleanup、object removal。
- Security/authorization policy marker。
- BLE1/BLE2 `bt0` IPv6 ping。
- TCP/UDP iperf、IPHC TX/RX、fragment counters。
- `upstream-owner=net_bluetooth/6lowpan+sim-ipsp-datapath-owner`。
- 两轮 lifecycle 和 final cleanup。

当前结论：BLE NET 的 BlueZ-facing closeout 已经从低层 `btctl` probe 推进到 apps/bluez 工具入口，并额外具备 profile registration、D-Bus object ownership/security lifecycle 的强验证证据。

边界：这仍是 `bluezipsp` adapter 的 BlueZ-shaped lifecycle，不是 unmodified `bluetoothd` 中真实 IPSP plugin/ObjectManager 实例。下一步若继续 BLE NET，应把这些 lifecycle markers 进一步替换为真实 bluetoothd mainloop/D-Bus object ownership、Profile1 registration、LE L2CAP CoC socket ownership 和 imported upstream 6LoWPAN callback owner 的实际代码路径。

## 2026-06-14 BlueZ-facing LE IPSP profile closeout

本轮继续收口 BLE NET 用户态入口：新增 apps 侧 BlueZ-facing LE IPSP profile adapter `bluezipsp`，避免 BLE 6LoWPAN/IPSP closeout 只依赖低层 `btctl upstream 6lowpan-*` probe。

新增内容：

- `FeatherCore/apps/wireless/linux_bluetooth/bluez/tools/ipsp_main.c`
- Kconfig/Makefile/CMake/Make.defs 注册 `CONFIG_LINUX_BLUEZ_IPSP`，默认构建 `bluezipsp`。
- `bluezipsp connect [ifname]`：以 `third/bluez/profiles/network/connection.c + profiles/network/ipsp.c` 语义标记发起 LE IPSP profile connect，并注册 `bt0`。
- `bluezipsp status`：输出 BlueZ IPSP profile marker 和完整 Linux 6LoWPAN status。
- `bluezipsp disconnect`：以 BlueZ profile disconnect marker 触发 6LoWPAN/IPSP teardown。
- 新增 hwsim case：`bluez-ipsp-closeout-full`。

验证结果：PASS

- build: `FeatherCore/build/logs/build-ble1-bluez-ipsp-closeout-v2.log`
- build: `FeatherCore/build/logs/build-ble2-bluez-ipsp-closeout-v2.log`
- run: `FeatherCore/build/logs/run-bluez-ipsp-closeout-v2.log`
- manifest: `FeatherCore/build/bt-hwsim-usecases-bluez-ipsp-closeout-v2/run-results.json`

`bluez-ipsp-closeout-full` 覆盖：

- BLE1/BLE2 通过 `bluezipsp connect` 建立 `bt0`。
- BlueZ-facing IPSP UUID `00001820-0000-1000-8000-00805f9b34fb` 与 PSM `0x0023` marker。
- `fd-handoff=le-l2cap-coc owner=kernel-6lowpan profile=ipsp` marker。
- BLE1/BLE2 IPv6 ping。
- TCP iperf 与 UDP iperf。
- IPHC TX/RX、fragment counters、`tx-fallback=0`。
- `upstream-owner=net_bluetooth/6lowpan+sim-ipsp-datapath-owner`。
- 两轮 `bluezipsp connect -> data -> bluezipsp disconnect` lifecycle。
- final cleanup：`registered=0`、`ipsp-state=closed`。

当前结论：BLE NET 现在已有 BlueZ-facing IPSP profile-shaped 入口的完整 hwsim closeout 证据，不再只依赖 `btctl upstream 6lowpan-up/status/down` probe。

边界：`bluezipsp` 仍是 NuttX apps 侧 BlueZ-shaped adapter，不是 unmodified upstream `bluetoothd` IPSP plugin/object manager。下一步 BLE NET 若继续深入，应把 `bluezipsp` 的 connect/disconnect surface 继续收敛到真实 BlueZ daemon profile registration、D-Bus object ownership、security policy、LE L2CAP CoC socket ownership，以及 imported `net/bluetooth/6lowpan.c` 的真实 peer/netdev callback implementation。

## 2026-06-14 BLE 6LoWPAN/IPSP datapath owner tightening

本轮继续收口 BLE NET upstream ownership：在上一轮 `sim-ipsp-owner` 的 attach/detach wrapper 基础上，把 TX/RX datapath counter 也收束到 imported Linux `net/bluetooth/6lowpan.c` 侧 wrapper。

新增/调整：

- `bt_6lowpan_sim_transmit_packet()`：集中记录 owner TX、xmit、`bt_xmit()` 证据。
- `bt_6lowpan_sim_receive_packet()`：集中记录 owner RX、deliver、recv callback 证据。
- `bt_6lowpan_sim_owner()` 现在报告 `net_bluetooth/6lowpan+sim-ipsp-datapath-owner`。
- `ble-ip-closeout-full` validator 同步要求新的 owner 名称。

验证结果：PASS

- build: `FeatherCore/build/logs/build-ble1-ble-ip-sim-ipsp-datapath-owner.log`
- build: `FeatherCore/build/logs/build-ble2-ble-ip-sim-ipsp-datapath-owner.log`
- run: `FeatherCore/build/logs/run-ble-ip-sim-ipsp-datapath-owner.log`
- manifest: `FeatherCore/build/bt-hwsim-usecases-ble-ip-sim-ipsp-datapath-owner/run-results.json`

`ble-ip-closeout-full` 覆盖：

- BLE1/BLE2 `bt0` IPv6 ping。
- TCP iperf 与 reverse TCP iperf。
- UDP iperf 与 reverse UDP iperf。
- IPHC TX/RX 与 fragment counters。
- 两轮 6LoWPAN/IPSP up/down lifecycle。
- final cleanup：`registered=0`、`ipsp-state=closed`。

当前结论：BLE NET 当前 hwsim closeout 不再只是 bridge/IPSP 可用性证明；IPSP attach/detach 和 TX/RX datapath evidence 都已经由 imported Linux `net/bluetooth/6lowpan.c` 内的 sim owner wrapper 聚合，并在完整 closeout gate 下通过。

边界：这仍然是 NuttX glue + imported upstream wrapper 的推进阶段，不是 unmodified upstream `net/bluetooth/6lowpan.c` 完全独占。下一步 BLE NET 若继续深入，应把 wrapper 证据替换为真实 upstream `setup_netdev()`、peer list lookup、`bt_xmit()`、recv callback、LE L2CAP CoC callback 与 BlueZ IPSP/Profile initiation 的实际 owner implementation。

## 2026-06-14 BLE 6LoWPAN/IPSP owner handoff tightening

本轮继续推进 BLE NET upstream ownership，不再让当前 `bt0`/IPSP 生命周期证据由 `linux_bt_6lowpan_netdev.c` 零散回填 `note_*` counter。新增 imported Linux `net/bluetooth/6lowpan.c` 侧 wrapper：

- `bt_6lowpan_sim_attach_ipsp()`：集中记录 netdev register、`setup_netdev()`、LE CoC open、`chan_ready_cb()`、peer add。
- `bt_6lowpan_sim_detach_ipsp()`：集中记录 netdev unregister、peer del、LE CoC close、`chan_close_cb()`、delete netdev。
- `bt_6lowpan_sim_owner()` 现在报告 `net_bluetooth/6lowpan+sim-ipsp-owner`。

`ble-ip-ping` validator 同步加严：基础 BLE1/BLE2 IPv6 ping 现在直接要求新的 owner 字符串，以及 peer/CoC/setup/delete/chan-ready/chan-close/bt-xmit/recv-cb lifecycle counters 非零。

验证结果：PASS

- build: `FeatherCore/build/logs/build-ble1-ble-ip-sim-ipsp-owner.log`
- build: `FeatherCore/build/logs/build-ble2-ble-ip-sim-ipsp-owner.log`
- run: `FeatherCore/build/logs/run-ble-ip-sim-ipsp-owner.log`
- manifest: `FeatherCore/build/bt-hwsim-usecases-ble-ip-sim-ipsp-owner/run-results.json`

当前结论：BLE1/BLE2 `bt0` IPv6 ping 保持通过，同时 IPSP attach/detach lifecycle 的 ownership 已从 bridge 侧零散 counter 推进到 imported Linux `net/bluetooth/6lowpan.c` 中的 sim owner wrapper。

边界：这仍不是 unmodified upstream `net/bluetooth/6lowpan.c` 完整独占 owner。`bt0` 注册、NuttX lower-half、hwsim medium handoff 仍需要 NuttX glue；下一步 BLE NET 完整化应继续把真实 upstream `setup_netdev()`、peer list lookup、`bt_xmit()`、L2CAP CoC callback 和 BlueZ IPSP/Profile initiation 从 wrapper/counter 证据推进到实际 owner implementation。

## 2026-06-14 BLE 6LoWPAN/IPSP ping: upstream owner lifecycle gate

本轮继续收口 BLE NET，不再只把 `ble-ip-ping` 视为 `bt0` bridge/IPSP 可用性检查，而是把 imported Linux `net/bluetooth/6lowpan.c` owner 生命周期计数纳入强校验。

新增 validator 要求：

- `upstream-owner-peer-add` / `upstream-owner-peer-del` 非零。
- `upstream-owner-coc-open` / `upstream-owner-coc-close` 非零。
- `upstream-owner-setup-netdev` 非零。
- `upstream-owner-chan-ready` 非零。
- `upstream-owner-bt-xmit` 非零。
- 同时保留 `bt0` 注册、IPSP PSM/CID、imported `net/6lowpan/iphc.c` TX/RX、IPv6 ping 与 teardown 校验。

验证结果：PASS

- build: `FeatherCore/build/logs/build-ble1-ble-ip-upstream-owner-lifecycle.log`
- build: `FeatherCore/build/logs/build-ble2-ble-ip-upstream-owner-lifecycle.log`
- run: `FeatherCore/build/logs/run-ble-ip-upstream-owner-lifecycle.log`
- manifest: `FeatherCore/build/bt-hwsim-usecases-ble-ip-upstream-owner-lifecycle/run-results.json`

当前结论：BLE1/BLE2 IPv6 ping 已经不是单纯证明 NuttX-facing bridge 能收发，而是要求 imported Linux Bluetooth 6LoWPAN owner 侧的 peer lifecycle、LE CoC open/close、`setup_netdev()`、`chan_ready_cb()` 与 `bt_xmit()` 语义证据同时存在。

边界：`upstream-owner=net_bluetooth/6lowpan+bridge-ipsp` 仍然说明当前 `bt0` 设备注册和 hwsim medium handoff 还没有完全由 unmodified upstream `net/bluetooth/6lowpan.c` 独占。下一步若继续 BLE NET，应把当前 bridge-owned netdev/handoff 进一步替换成 imported upstream peer/netdev/L2CAP callback owner，同时保持本轮更严格的 `ble-ip-ping` gate 继续 PASS。

# Linux Bluetooth 6LoWPAN porting boundary

This directory contains the imported Linux `net/6lowpan` helper sources used
by `net/bluetooth/6lowpan.c`.

The target is not a NuttX-only BLE tunnel.  The target is the Linux-observable
Bluetooth LE IP path:

```text
BlueZ / AF_BLUETOOTH / mgmt
        |
        v
Linux LE L2CAP CoC
        |
        v
Linux net/bluetooth/6lowpan.c
        |
        v
Linux net/6lowpan IPHC helpers
        |
        v
NuttX netdev + IPv6/ICMPv6
        |
        v
SIM_BTHWSIM ACL public-file medium
```

Required compatibility before enabling
`CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_6LOWPAN`:

1. Linux IPv6 route and neighbour lookup surface used by peer selection.
2. `lowpan_dev` allocation, registration and private-data access semantics.
3. Linux `net_device` type/flags/headroom/MTU behavior for 6LoWPAN devices.
4. LE L2CAP CoC channel callbacks for open, close, receive, suspend and resume.
5. Mapping of Linux 6LoWPAN RX `netif_rx()` into NuttX IPv6 packet delivery.
6. Mapping of NuttX IPv6 TX into Linux `ndo_start_xmit()` and LE CoC send.
7. hwsim validation for `BLE1 <-> BLE2` IPv6 ping before UDP/TCP throughput.

Do not mark BLE IP complete from an L2CAP payload probe alone.  The completion
gate is an IP packet that traverses the imported Linux Bluetooth 6LoWPAN path
and the imported generic Linux 6LoWPAN compression/decompression helpers.

## 2026-06-12 BLE IP hwsim gate

The current FeatherCore sim path has moved past a payload-only probe.  The
`ble-ip-ping` gate now validates a BLE1/BLE2 IPv6 packet path over `bt0`:

```text
BLE1 bt0 fc00::1/64 <-> BLE2 bt0 fc00::2/64
btctl upstream 6lowpan-up bt%d
ifup bt0
ping6
btctl upstream 6lowpan-down
```

Latest evidence:

```text
PASS ble-ip-ping
build/logs/build-ble1-ble-ip-current.log
build/logs/build-ble2-ble-ip-current.log
build/logs/run-ble-ip-ping-current.log
build/bt-hwsim-usecases-ble-ip-ping-current/run-results.json
```

Both directions report zero packet loss.  Runtime status also shows IPHC
traffic rather than uncompressed fallback:

```text
BLE1: tx=2 rx=2 tx-iphc=2 rx-iphc=2 tx-fallback=0
BLE2: tx=4 rx=4 tx-iphc=4 rx-iphc=4 tx-fallback=0
```

Teardown is also checked: after `6lowpan-down`, both sides report
`registered=0`, `ifname=-`, and `ipsp-state=closed`.

This is still not the final Linux Bluetooth 6LoWPAN completion point.  The
current path uses the FeatherCore NuttX-facing BLE 6LoWPAN/IPSP bridge plus
imported generic Linux `net/6lowpan` IPHC helpers.  Remaining work is to move
the Bluetooth-specific ownership to the imported Linux
`net/bluetooth/6lowpan.c` semantics and upstream LE L2CAP CoC channel
lifecycle, including BlueZ userspace IPSP/profile initiation, security policy,
multi-peer behavior, fragmentation stress, reconnect cleanup, and error paths.

## 2026-06-12 upstream Bluetooth 6LoWPAN object link/init gate

The BLE IP hwsim gate was tightened so `ble-ip-ping` no longer proves only the NuttX-facing bridge and generic IPHC helpers.  The final image now links the imported upstream Bluetooth `net/bluetooth/6lowpan.c` object and calls its module-init wrapper, alongside the generic upstream `net/6lowpan` core module-init wrapper, before registering the current `bt0` bridge.

New status fields required by the validator:

```text
upstream-core-init=1 upstream-core-ret=0
upstream-bt6lowpan-init=1 upstream-bt6lowpan-ret=0
upstream-owner=bridge-ipsp
```

Latest gate:

```text
PASS ble-ip-ping
build/logs/build-ble1-ble-ip-upstream-init-compat.log
build/logs/build-ble2-ble-ip-upstream-init-compat.log
build/logs/run-ble-ip-ping-upstream-init.log
build/bt-hwsim-usecases-ble-ip-ping-upstream-init/run-results.json
```

This deliberately names the remaining boundary.  `upstream-owner=bridge-ipsp` means the current `bt0` device lifecycle and LE IPSP medium handoff are still owned by `linux_bt_6lowpan_netdev.c`; the upstream Bluetooth file is linked and initialized, but not yet the owner of `setup_netdev()`, `chan_ready_cb()`, `bt_xmit()`, peer lookup, or L2CAP CoC callbacks.  The next real completion step is to replace that bridge ownership with the imported `net/bluetooth/6lowpan.c` peer/netdev lifecycle while preserving the passing BLE1/BLE2 IPv6 ping gate.

## 2026-06-15 upstream helper contract tightening

The BLE 6LoWPAN bridge now reports a stricter upstream helper contract.

Current helper boundary:

```text
linux_bt_6lowpan_netdev.c
  -> bt_6lowpan_sim_attach_ipsp()
  -> bt_6lowpan_sim_detach_ipsp()
  -> bt_6lowpan_sim_transmit_packet()
  -> bt_6lowpan_sim_receive_packet()
  -> imported net/bluetooth/6lowpan.c owner counters/state
```

`linux_bt_6lowpan_status()` now includes:

```text
upstream-helper-contract=attach_ipsp,detach_ipsp,transmit_packet,receive_packet
upstream-helper-owner=net_bluetooth/6lowpan
upstream-helper-boundary=bridge-calls-upstream-helper
```

This removes the bridge-side dependency on individual `bt_6lowpan_sim_note_*()` calls and keeps the observable ownership lifecycle centralized in the imported Bluetooth 6LoWPAN file.  It is still not the final upstream object graph: the real next step is to move from helper calls to actual imported `setup_netdev()`, `chan_ready_cb()`, `chan_close_cb()`, `bt_xmit()`, receive callback, peer lookup, and L2CAP channel ownership.
## 2026-06-17 hwsim closeout after packet-size alignment

The BLE IP/6LoWPAN hwsim path was rebuilt and validated after setting the
BT/BLE simulator defconfigs to `CONFIG_NET_ETH_PKTSIZE=1514`.

Build:

- `FeatherCore/build/logs/build-ble1-eth-pktsize-1514.log`: PASS.
- `FeatherCore/build/logs/build-ble2-eth-pktsize-1514.log`: PASS.

hwsim:

- `ble-ip-ping`: PASS.
- `ble-ip-reconnect-stress`: PASS.
- `ble-ip-iperf-tcp`: PASS.
- `ble-ip-iperf-tcp-reverse`: PASS.
- `ble-ip-iperf-udp`: PASS.
- `ble-ip-iperf-udp-reverse`: PASS.

Artifacts:

- `FeatherCore/build/logs/run-ble-ip-ping-after-pktsize-1514-verify.log`
- `FeatherCore/build/bt-hwsim-ble-ip-ping-after-pktsize-1514-verify/run-results.json`
- `FeatherCore/build/logs/run-ble-ip-closeout-after-pktsize-1514-verify.log`
- `FeatherCore/build/bt-hwsim-ble-ip-closeout-after-pktsize-1514-verify/run-results.json`

Current boundary:

- BLE IP is closed for the current hwsim functional target.
- Future work should continue reducing compatibility glue around imported
  Linux 6LoWPAN/IPSP ownership, while keeping ping, reconnect, TCP/UDP iperf,
  reverse direction, and teardown gates green.
