/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright (C) 2014 Intel Corporation */

#include <nuttx/config.h>

#include <linux/list.h>

#ifdef CONFIG_NET_LINUX_BLUETOOTH_UPSTREAM_HCI_CODEC
void hci_read_supported_codecs(struct hci_dev *hdev);
void hci_read_supported_codecs_v2(struct hci_dev *hdev);
void hci_codec_list_clear(struct list_head *codec_list);
#else
static inline void hci_read_supported_codecs(struct hci_dev *hdev)
{
	(void)hdev;
}

static inline void hci_read_supported_codecs_v2(struct hci_dev *hdev)
{
	(void)hdev;
}

static inline void hci_codec_list_clear(struct list_head *codec_list)
{
	(void)codec_list;
}
#endif
