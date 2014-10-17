#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER trace_LB

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./trace.h"

#if !defined(TRACE_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define TRACE_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(
	trace_LB,
	tp_name,
	TP_ARGS(),
	TP_FIELDS(
	)
)

TRACEPOINT_LOGLEVEL(
	trace_LB,
	tp_name,
	TRACE_WARNING)

TRACEPOINT_EVENT(
	trace_LB,
	tp_player_join,
	TP_ARGS(),
	TP_FIELDS(
	)
)
//Player join and quit tracepoint
TRACEPOINT_LOGLEVEL(
	trace_LB,
	tp_player_join,
	TRACE_WARNING)

TRACEPOINT_EVENT(
	trace_LB,
	tp_player_quit,
	TP_ARGS(),
	TP_FIELDS(
	)
)

TRACEPOINT_LOGLEVEL(
	trace_LB,
	tp_player_quit,
	TRACE_WARNING)

TRACEPOINT_EVENT(
	trace_LB,
	tp_proc_entry,
	TP_ARGS(),
	TP_FIELDS(
	)
)


//Processing entry and exit
TRACEPOINT_LOGLEVEL(
	trace_LB,
	tp_proc_entry,
	TRACE_WARNING
)
TRACEPOINT_EVENT(
	trace_LB,
	tp_proc_exit,
	TP_ARGS(),
	TP_FIELDS(
	)
)

TRACEPOINT_LOGLEVEL(
	trace_LB,
	tp_proc_exit,
	TRACE_WARNING
)

TRACEPOINT_EVENT(
	trace_LB,
	tp_first_stage,
	TP_ARGS(int, num_req, int, proc_time),
	TP_FIELDS(
		ctf_integer(int, num_req , num_req)
		ctf_integer(int, proc_time , proc_time)
	)
)

TRACEPOINT_LOGLEVEL(
	trace_LB,
  tp_first_stage,
	TRACE_WARNING)

TRACEPOINT_EVENT(
	trace_LB,
	tp_overloaded,
	TP_ARGS(int, tid),
	TP_FIELDS(
		ctf_integer(int,tid , tid)
	)
)

TRACEPOINT_LOGLEVEL(
	trace_LB,
  tp_overloaded,
	TRACE_WARNING)
#endif
#include <lttng/tracepoint-event.h>
