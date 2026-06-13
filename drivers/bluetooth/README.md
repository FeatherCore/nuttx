# Linux Bluetooth controller driver port

This directory is the NuttX driver-layer home for Linux Bluetooth controller
drivers, mirroring Linux `drivers/bluetooth`.

The Linux host stack port remains under:

```text
wireless/linux_bluetooth
```

The first controller driver staged here is:

```text
Linux source: drivers/bluetooth/hci_vhci.c
Staged source: wireless/linux_bluetooth/upstream/drivers_bluetooth/hci_vhci.c
Driver bridge: drivers/bluetooth/linux_bt_upstream_vhci.c
Build owner:   drivers/bluetooth
```

This split is intentional:

```text
Linux net/bluetooth      -> NuttX wireless/linux_bluetooth
Linux drivers/bluetooth  -> NuttX drivers/bluetooth
```

Keep upstream driver sources as close to Linux as possible.  The local
`linux_bt_upstream_vhci.c` file is the NuttX-facing open/read/write/pump
bridge for the imported VHCI driver; it should not grow host-stack protocol
logic.  Compatibility APIs should stay in the Linux Bluetooth compat layer
unless they are genuinely driver-local.
