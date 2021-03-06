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
	TP_FIELDS()
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

//Timing for the first stage that is receiving
//requests from the clients
TRACEPOINT_EVENT(
  trace_LB,
  tp_thread_begin,
  TP_ARGS(int, tid),
  TP_FIELDS(
    ctf_integer(int, tid , tid)
  )
)

TRACEPOINT_LOGLEVEL(
  trace_LB,
  tp_thread_begin,
  TRACE_WARNING)


TRACEPOINT_EVENT(
  trace_LB,
  tp_begin_first_stage,
  TP_ARGS(),
  TP_FIELDS()
)

TRACEPOINT_LOGLEVEL(
  trace_LB,
  tp_begin_first_stage,
  TRACE_WARNING)

//Timing for the first stage that is receiving
//requests from the clients
TRACEPOINT_EVENT(
	trace_LB,
	tp_end_first_stage,
	TP_ARGS(int, num_req, int, proc_time),
	TP_FIELDS(
		ctf_integer(int, num_req , num_req)
		ctf_integer(int, proc_time , proc_time)
	)
)

TRACEPOINT_LOGLEVEL(
	trace_LB,
  tp_end_first_stage,
	TRACE_WARNING)

TRACEPOINT_EVENT(
  trace_LB,
  tp_begin_second_stage,
  TP_ARGS(),
  TP_FIELDS()
)

TRACEPOINT_LOGLEVEL(
  trace_LB,
  tp_begin_second_stage,
  TRACE_WARNING)

TRACEPOINT_EVENT(
  trace_LB,
  tp_end_second_stage,
  TP_ARGS(),
  TP_FIELDS()
)

TRACEPOINT_LOGLEVEL(
  trace_LB,
  tp_end_second_stage,
  TRACE_WARNING)

TRACEPOINT_EVENT(
  trace_LB,
  tp_begin_third_stage,
  TP_ARGS(),
  TP_FIELDS()
)

TRACEPOINT_LOGLEVEL(
  trace_LB,
  tp_begin_third_stage,
  TRACE_WARNING)

// number of request sent and overall timing in the
// third phase
TRACEPOINT_EVENT(
	trace_LB,
	tp_end_third_stage,
	TP_ARGS(int, num_update, int, send_time),
	TP_FIELDS(
		ctf_integer(int, num_update , num_update)
		ctf_integer(int, sent_time , send_time)
	)
)

TRACEPOINT_LOGLEVEL(
	trace_LB,
	tp_end_third_stage,
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

TRACEPOINT_EVENT(
  trace_LB,
  tp_contention,
  TP_ARGS(int, region, int, num_locks, int, num_contended_locks),
  TP_FIELDS(
    ctf_integer(int,region , region)
    ctf_integer(int,num_locks , num_locks)
    ctf_integer(int,num_contended_locks , num_contended_locks)
  )
)

TRACEPOINT_LOGLEVEL(
  trace_LB,
  tp_contention,
  TRACE_WARNING)

//thread managing region where the quest is located
TRACEPOINT_EVENT(
	trace_LB,
	tp_quest_manager,
	TP_ARGS(int, tid),
	TP_FIELDS(
		ctf_integer(int,tid , tid)
	)
)

TRACEPOINT_LOGLEVEL(
	trace_LB,
  tp_quest_manager,
	TRACE_WARNING)

//Quest duration
TRACEPOINT_EVENT(
	trace_LB,
	tp_quest_begin,
	TP_ARGS(),
	TP_FIELDS(
	)
)

TRACEPOINT_LOGLEVEL(
	trace_LB,
  tp_quest_begin,
	TRACE_WARNING)

TRACEPOINT_EVENT(
	trace_LB,
	tp_quest_end,
	TP_ARGS(),
	TP_FIELDS(
	)
)

TRACEPOINT_LOGLEVEL(
	trace_LB,
  tp_quest_end,
	TRACE_WARNING)

TRACEPOINT_EVENT(
	trace_LB,
	tp_region_summary,
	TP_ARGS(int, num_players, int, num_interactions, int , pos_x, int, pos_y),
	TP_FIELDS(
		ctf_integer(int,num_players , num_players)
		ctf_integer(int,num_interactions , num_interactions)
		ctf_integer(int,pos_x , pos_x)
		ctf_integer(int,pos_y , pos_y)
	)
)

TRACEPOINT_LOGLEVEL(
	trace_LB,
  tp_player_merge,
	TRACE_WARNING)

#endif
#include <lttng/tracepoint-event.h>
