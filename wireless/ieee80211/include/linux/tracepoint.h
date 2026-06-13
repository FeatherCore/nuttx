#ifndef __WIRELESS_IEEE80211_INCLUDE_LINUX_TRACEPOINT_H
#define __WIRELESS_IEEE80211_INCLUDE_LINUX_TRACEPOINT_H
#include <linux/cfg80211_compat.h>
#define TRACE_EVENT(name, proto, args, struct_entry, assign, print) \
	static inline void trace_##name(proto) { }
#define DECLARE_EVENT_CLASS(name, proto, args, struct_entry, assign, print)
#define DEFINE_EVENT(template, name, proto, args) \
	static inline void trace_##name(proto) { }
#define TRACE_INCLUDE_FILE file
#define TP_PROTO(args...) args
#define TP_ARGS(args...) args
#define TP_STRUCT__entry(args...)
#define TP_fast_assign(args...)
#define TP_printk(fmt, args...) fmt
#define __field(type, item)
#define __array(type, item, len)
#define __dynamic_array(type, item, len)
#define __string(item, src)
#define __assign_str(dst)
#define __get_str(field) ""
#define __entry NULL
#endif
