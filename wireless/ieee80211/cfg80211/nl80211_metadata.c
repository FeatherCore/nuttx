// SPDX-License-Identifier: GPL-2.0-only
/*
 * Minimal nl80211 generic-netlink metadata registration for NuttX.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <linux/if_ether.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/nl80211.h>

#include <nuttx/kmalloc.h>
#include <nuttx/net/netlink_kernel.h>
#include <nuttx/queue.h>

#include <net/genetlink.h>

#define NL80211_METADATA_WIPHY        0
#define NL80211_METADATA_IFINDEX      1
#define NL80211_METADATA_IFNAME       "wlan0"
#define NL80211_METADATA_WIPHY_NAME   "phy0"
#define NL80211_METADATA_MAC          { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 }
#define NL80211_METADATA_PAYLOAD_MAX  2048

enum nl80211_metadata_multicast_groups {
	NL80211_METADATA_MCGRP_CONFIG,
	NL80211_METADATA_MCGRP_SCAN,
	NL80211_METADATA_MCGRP_REGULATORY,
	NL80211_METADATA_MCGRP_MLME,
	NL80211_METADATA_MCGRP_VENDOR,
	NL80211_METADATA_MCGRP_NAN,
};

static const struct genl_multicast_group nl80211_metadata_mcgrps[] = {
	[NL80211_METADATA_MCGRP_CONFIG] = {
		.name = NL80211_MULTICAST_GROUP_CONFIG,
	},
	[NL80211_METADATA_MCGRP_SCAN] = {
		.name = NL80211_MULTICAST_GROUP_SCAN,
	},
	[NL80211_METADATA_MCGRP_REGULATORY] = {
		.name = NL80211_MULTICAST_GROUP_REG,
	},
	[NL80211_METADATA_MCGRP_MLME] = {
		.name = NL80211_MULTICAST_GROUP_MLME,
	},
	[NL80211_METADATA_MCGRP_VENDOR] = {
		.name = NL80211_MULTICAST_GROUP_VENDOR,
	},
	[NL80211_METADATA_MCGRP_NAN] = {
		.name = NL80211_MULTICAST_GROUP_NAN,
	},
};

static struct genl_family nl80211_metadata_fam = {
	.name = NL80211_GENL_NAME,
	.hdrsize = 0,
	.version = 1,
	.maxattr = NL80211_ATTR_MAX,
	.mcgrps = nl80211_metadata_mcgrps,
	.n_mcgrps = ARRAY_SIZE(nl80211_metadata_mcgrps),
	.module = THIS_MODULE,
};

static enum nl80211_iftype nl80211_metadata_iftype(void)
{
#ifdef CONFIG_EXAMPLES_WIFI_HWSIM_AP
	return NL80211_IFTYPE_AP;
#else
	return NL80211_IFTYPE_STATION;
#endif
}

static bool metadata_align_cursor(char **cursor, char *end, size_t len)
{
	size_t aligned = NLA_ALIGN(len);

	if (*cursor + aligned > end)
		return false;

	if (aligned > len)
		memset(*cursor + len, 0, aligned - len);

	*cursor += aligned;
	return true;
}

static bool metadata_put_attr(char **cursor, char *end, uint16_t type,
			      const void *data, size_t len)
{
	struct nlattr *attr;
	size_t attrlen = NLA_HDRLEN + len;

	if (*cursor + NLA_ALIGN(attrlen) > end)
		return false;

	attr = (struct nlattr *)*cursor;
	attr->nla_len = attrlen;
	attr->nla_type = type;

	if (len > 0)
		memcpy((char *)attr + NLA_HDRLEN, data, len);

	return metadata_align_cursor(cursor, end, attrlen);
}

static bool metadata_put_u8(char **cursor, char *end, uint16_t type,
			    uint8_t value)
{
	return metadata_put_attr(cursor, end, type, &value, sizeof(value));
}

static bool metadata_put_u32(char **cursor, char *end, uint16_t type,
			     uint32_t value)
{
	return metadata_put_attr(cursor, end, type, &value, sizeof(value));
}

static bool metadata_put_string(char **cursor, char *end, uint16_t type,
				const char *value)
{
	return metadata_put_attr(cursor, end, type, value, strlen(value) + 1);
}

static struct nlattr *metadata_start_nested(char **cursor, char *end,
					    uint16_t type)
{
	struct nlattr *attr;

	if (*cursor + NLA_HDRLEN > end)
		return NULL;

	attr = (struct nlattr *)*cursor;
	attr->nla_len = NLA_HDRLEN;
	attr->nla_type = type | NLA_F_NESTED;
	*cursor += NLA_HDRLEN;
	return attr;
}

static bool metadata_end_nested(char **cursor, char *end, struct nlattr *attr)
{
	size_t attrlen = *cursor - (char *)attr;

	attr->nla_len = attrlen;
	*cursor = (char *)attr;
	return metadata_align_cursor(cursor, end, attrlen);
}

static bool metadata_put_flag_nested(char **cursor, char *end,
				     uint16_t parent, uint16_t flag)
{
	struct nlattr *nest;

	nest = metadata_start_nested(cursor, end, parent);
	if (!nest)
		return false;

	if (!metadata_put_attr(cursor, end, flag, NULL, 0))
		return false;

	return metadata_end_nested(cursor, end, nest);
}

static bool metadata_put_supported_iftypes(char **cursor, char *end)
{
	struct nlattr *nest;

	nest = metadata_start_nested(cursor, end, NL80211_ATTR_SUPPORTED_IFTYPES);
	if (!nest)
		return false;

	if (!metadata_put_attr(cursor, end, NL80211_IFTYPE_STATION, NULL, 0) ||
	    !metadata_put_attr(cursor, end, NL80211_IFTYPE_AP, NULL, 0))
		return false;

	return metadata_end_nested(cursor, end, nest);
}

static bool metadata_put_supported_commands(char **cursor, char *end)
{
	static const uint32_t commands[] = {
		NL80211_CMD_GET_WIPHY,
		NL80211_CMD_GET_INTERFACE,
		NL80211_CMD_SET_INTERFACE,
		NL80211_CMD_NEW_INTERFACE,
		NL80211_CMD_DEL_INTERFACE,
		NL80211_CMD_START_AP,
		NL80211_CMD_STOP_AP,
		NL80211_CMD_CONNECT,
		NL80211_CMD_DISCONNECT,
		NL80211_CMD_TRIGGER_SCAN,
		NL80211_CMD_GET_SCAN,
		NL80211_CMD_REGISTER_FRAME,
		NL80211_CMD_FRAME,
		NL80211_CMD_GET_PROTOCOL_FEATURES,
	};
	struct nlattr *nest;
	size_t i;

	nest = metadata_start_nested(cursor, end, NL80211_ATTR_SUPPORTED_COMMANDS);
	if (!nest)
		return false;

	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (!metadata_put_u32(cursor, end, i + 1, commands[i]))
			return false;
	}

	return metadata_end_nested(cursor, end, nest);
}

static struct netlink_response_s *
metadata_alloc_response(const struct nlmsghdr *req, uint8_t cmd,
			bool multipart)
{
	struct netlink_response_s *resp;
	struct genlmsghdr *genlhdr;

	resp = kmm_zalloc(SIZEOF_NETLINK_RESPONSE_S(NL80211_METADATA_PAYLOAD_MAX));
	if (!resp)
		return NULL;

	genlhdr = (struct genlmsghdr *)NLMSG_DATA(&resp->msg);
	genlhdr->cmd = cmd;
	genlhdr->version = nl80211_metadata_fam.version;

	resp->msg.nlmsg_type = req->nlmsg_type;
	resp->msg.nlmsg_flags = multipart ? NLM_F_MULTI : 0;
	resp->msg.nlmsg_seq = req->nlmsg_seq;
	resp->msg.nlmsg_pid = req->nlmsg_pid;

	return resp;
}

static void metadata_finish_response(struct netlink_response_s *resp,
				     char *cursor)
{
	struct genlmsghdr *genlhdr = (struct genlmsghdr *)NLMSG_DATA(&resp->msg);
	size_t payload = cursor - (char *)genlhdr;

	resp->msg.nlmsg_len = NLMSG_LENGTH(payload);
}

static int metadata_add_ack(NETLINK_HANDLE handle,
			    const struct nlmsghdr *req, int error)
{
	struct netlink_response_s *resp;
	struct nlmsgerr *err;

	if ((req->nlmsg_flags & NLM_F_ACK) == 0 && error == 0)
		return 0;

	resp = kmm_zalloc(SIZEOF_NETLINK_RESPONSE_S(sizeof(*err)));
	if (!resp)
		return -ENOMEM;

	err = (struct nlmsgerr *)NLMSG_DATA(&resp->msg);
	err->error = error;
	memcpy(&err->msg, req, sizeof(*req));

	resp->msg.nlmsg_len = NLMSG_LENGTH(sizeof(*err));
	resp->msg.nlmsg_type = NLMSG_ERROR;
	resp->msg.nlmsg_seq = req->nlmsg_seq;
	resp->msg.nlmsg_pid = req->nlmsg_pid;

	netlink_add_response(handle, resp);
	return 0;
}

static int metadata_add_done(NETLINK_HANDLE handle, const struct nlmsghdr *req)
{
	struct netlink_response_s *resp;

	resp = kmm_zalloc(SIZEOF_NETLINK_RESPONSE_S(0));
	if (!resp)
		return -ENOMEM;

	resp->msg.nlmsg_len = NLMSG_LENGTH(0);
	resp->msg.nlmsg_type = NLMSG_DONE;
	resp->msg.nlmsg_seq = req->nlmsg_seq;
	resp->msg.nlmsg_pid = req->nlmsg_pid;

	netlink_add_response(handle, resp);
	return 0;
}

static int metadata_get_protocol_features(NETLINK_HANDLE handle,
					  const struct nlmsghdr *req)
{
	struct netlink_response_s *resp;
	char *cursor;
	char *end;

	resp = metadata_alloc_response(req, NL80211_CMD_GET_PROTOCOL_FEATURES,
				       false);
	if (!resp)
		return -ENOMEM;

	cursor = (char *)NLMSG_DATA(&resp->msg) + GENL_HDRLEN;
	end = cursor + NL80211_METADATA_PAYLOAD_MAX - GENL_HDRLEN;

	if (!metadata_put_u32(&cursor, end, NL80211_ATTR_PROTOCOL_FEATURES,
			      NL80211_PROTOCOL_FEATURE_SPLIT_WIPHY_DUMP)) {
		kmm_free(resp);
		return -EMSGSIZE;
	}

	metadata_finish_response(resp, cursor);
	netlink_add_response(handle, resp);
	return metadata_add_ack(handle, req, 0);
}

static int metadata_get_interface(NETLINK_HANDLE handle,
				  const struct nlmsghdr *req)
{
	static const uint8_t mac[ETH_ALEN] = NL80211_METADATA_MAC;
	struct netlink_response_s *resp;
	char *cursor;
	char *end;

	resp = metadata_alloc_response(req, NL80211_CMD_NEW_INTERFACE, false);
	if (!resp)
		return -ENOMEM;

	cursor = (char *)NLMSG_DATA(&resp->msg) + GENL_HDRLEN;
	end = cursor + NL80211_METADATA_PAYLOAD_MAX - GENL_HDRLEN;

	if (!metadata_put_u32(&cursor, end, NL80211_ATTR_WIPHY,
			      NL80211_METADATA_WIPHY) ||
	    !metadata_put_u32(&cursor, end, NL80211_ATTR_IFINDEX,
			      NL80211_METADATA_IFINDEX) ||
	    !metadata_put_string(&cursor, end, NL80211_ATTR_IFNAME,
				 NL80211_METADATA_IFNAME) ||
	    !metadata_put_u32(&cursor, end, NL80211_ATTR_IFTYPE,
			      nl80211_metadata_iftype()) ||
	    !metadata_put_attr(&cursor, end, NL80211_ATTR_MAC, mac,
			       sizeof(mac))) {
		kmm_free(resp);
		return -EMSGSIZE;
	}

	metadata_finish_response(resp, cursor);
	netlink_add_response(handle, resp);
	return metadata_add_ack(handle, req, 0);
}

static int metadata_get_wiphy(NETLINK_HANDLE handle,
			      const struct nlmsghdr *req)
{
	struct netlink_response_s *resp;
	char *cursor;
	char *end;
	bool multipart = (req->nlmsg_flags & NLM_F_DUMP) != 0;

	resp = metadata_alloc_response(req, NL80211_CMD_NEW_WIPHY, multipart);
	if (!resp)
		return -ENOMEM;

	cursor = (char *)NLMSG_DATA(&resp->msg) + GENL_HDRLEN;
	end = cursor + NL80211_METADATA_PAYLOAD_MAX - GENL_HDRLEN;

	if (!metadata_put_u32(&cursor, end, NL80211_ATTR_WIPHY,
			      NL80211_METADATA_WIPHY) ||
	    !metadata_put_string(&cursor, end, NL80211_ATTR_WIPHY_NAME,
				 NL80211_METADATA_WIPHY_NAME) ||
	    !metadata_put_u8(&cursor, end, NL80211_ATTR_MAX_NUM_SCAN_SSIDS, 1) ||
	    !metadata_put_supported_iftypes(&cursor, end) ||
	    !metadata_put_supported_commands(&cursor, end) ||
	    !metadata_put_flag_nested(&cursor, end,
				      NL80211_ATTR_SOFTWARE_IFTYPES,
				      NL80211_IFTYPE_STATION) ||
	    !metadata_put_flag_nested(&cursor, end,
				      NL80211_ATTR_SOFTWARE_IFTYPES,
				      NL80211_IFTYPE_AP)) {
		kmm_free(resp);
		return -EMSGSIZE;
	}

	metadata_finish_response(resp, cursor);
	netlink_add_response(handle, resp);

	if (multipart)
		return metadata_add_done(handle, req);

	return metadata_add_ack(handle, req, 0);
}

int nl80211_init(void)
{
	int ret;

	ret = genl_register_family(&nl80211_metadata_fam);
	return ret == -EEXIST ? 0 : ret;
}

void nl80211_exit(void)
{
	genl_unregister_family(&nl80211_metadata_fam);
}

ssize_t nl80211_metadata_sendto(NETLINK_HANDLE handle,
				const struct nlmsghdr *nlmsg,
				size_t len, int flags,
				const struct sockaddr_nl *to,
				socklen_t tolen)
{
	const struct genlmsghdr *genlhdr;
	int ret;

	if (len < sizeof(*nlmsg) ||
	    nlmsg->nlmsg_len < NLMSG_LENGTH(GENL_HDRLEN) ||
	    nlmsg->nlmsg_len > len)
		return -EINVAL;

	genlhdr = (const struct genlmsghdr *)NLMSG_DATA(nlmsg);

	switch (genlhdr->cmd) {
	case NL80211_CMD_GET_PROTOCOL_FEATURES:
		ret = metadata_get_protocol_features(handle, nlmsg);
		break;
	case NL80211_CMD_GET_INTERFACE:
		ret = metadata_get_interface(handle, nlmsg);
		break;
	case NL80211_CMD_GET_WIPHY:
		ret = metadata_get_wiphy(handle, nlmsg);
		break;
	default:
		ret = metadata_add_ack(handle, nlmsg, -EOPNOTSUPP);
		break;
	}

	return ret < 0 ? ret : (ssize_t)len;
}
