#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER trace_load_balancing

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./trace.h"

#if !defined(TRACE_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define TRACE_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(
	trace_load_balancing,
	tp_name,
	TP_ARGS(),
	TP_FIELDS(
	)
)

TRACEPOINT_LOGLEVEL(
	trace_load_balancing,
	tp_name,
	TRACE_WARNING)

TRACEPOINT_EVENT(
	trace_load_balancing,
	tp_name_1_int,
	TP_ARGS(int, nb),
	TP_FIELDS(
		ctf_integer(int,int , nb)
	)
)

TRACEPOINT_LOGLEVEL(
	trace_load_balancing,
	tp_name_1_int,
	TRACE_WARNING)

#endif
#include <lttng/tracepoint-event.h>
