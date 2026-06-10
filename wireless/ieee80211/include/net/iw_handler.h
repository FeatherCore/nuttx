#ifndef __WIRELESS_IEEE80211_INCLUDE_NET_IW_HANDLER_H
#define __WIRELESS_IEEE80211_INCLUDE_NET_IW_HANDLER_H

struct iw_request_info
{
  unsigned int cmd;
  unsigned int flags;
};

struct iw_handler_def
{
  int dummy;
};
#endif
