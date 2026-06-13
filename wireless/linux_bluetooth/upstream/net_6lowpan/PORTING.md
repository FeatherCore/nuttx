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
