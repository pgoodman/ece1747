import sys
from collections import Counter,  defaultdict
import babeltrace
import csv

duration = 2000
q_begin = 300
q_end = 1300

def moving_average(prev, curr):
    if not prev:
        return
    curr.num_req = 0.95 * prev.num_req + 0.05 * curr.num_req
    curr.proc_time = 0.95 * prev.proc_time + 0.05 * curr.proc_time
    curr.num_update = 0.95 * prev.num_update + 0.05 * curr.num_update
    curr.sent_time = 0.95 * prev.sent_time + 0.05 * curr.sent_time

class Iteration(object):
    __slots__ = ('num_req', 'proc_time', 'num_update', 'sent_time')
    def __init__(self):
        self.num_req = 0
        self.proc_time = 0
        self.num_update = 0
        self.sent_time = 0

def process_trace():
    if len(sys.argv) != 2:
        msg = 'Usage: python {} TRACEPATH'.format(sys.argv[0])
        raise ValueError(msg)

    # a trace collection holds one to many traces
    col = babeltrace.TraceCollection()

    # add the trace provided by the user
    # (LTTng traces always have the 'ctf' format)
    if col.add_trace(sys.argv[1], 'ctf') is None:
        raise RuntimeError('Cannot add trace')

    # this holds the last `sched_switch` timestamp
    threads = defaultdict(lambda: [])
    
    tid_to_vtid = {}
    vtid_to_tid = {}

    quest_handlers = []
    max_value = -1

    stage1_begin_time = defaultdict(lambda: [])
    stage2_begin_time = defaultdict(lambda: [])
    stage3_begin_time = defaultdict(lambda: [])

    stage1_end_time = defaultdict(lambda: [])
    stage2_end_time = defaultdict(lambda: [])
    stage3_end_time = defaultdict(lambda: [])
    
    # iterate events
    for event in col.events:

        # Special event that allows us to associate `tid`s with `vtid`s, so that
        # we don't get confused :D
        if "thread_begin" in event.name:
            vtid = int(event['vtid'])
            tid = int(event['tid'])
            tid_to_vtid[tid] = vtid
            vtid_to_tid[vtid] = tid

        elif "begin_first_stage" in event.name:
            vtid = int(event['vtid'])
            stage1_begin_time[vtid].append(event.timestamp)

        # Get the first set of info from stage 1 for a given thread.
        elif "end_first_stage" in event.name:
            it = Iteration()
            it.num_req = event['num_req']
            it.proc_time = event['proc_time']
            vtid = int(event['vtid'])
            threads[vtid].append(it)
            stage1_end_time[vtid].append(event.timestamp)

        elif "begin_second_stage" in event.name:
            vtid = int(event['vtid'])
            stage2_begin_time[vtid].append(event.timestamp)

        elif "end_second_stage" in event.name:
            vtid = int(event['vtid'])
            stage2_end_time[vtid].append(event.timestamp)

        elif "begin_third_stage" in event.name:
            vtid = int(event['vtid'])
            stage3_begin_time[vtid].append(event.timestamp)

        # Extend the information from the first stage with that of the
        # third stage.
        elif "end_third_stage" in event.name:
            vtid = int(event['vtid'])
            it = threads[vtid][-1]
            it.num_update = event['num_update']
            it.sent_time = event['sent_time']

            if 2 <= len(threads[vtid]):
                moving_average(threads[vtid][-2], it)

            stage3_end_time[vtid].append(event.timestamp)

        elif "quest_manager" in event.name:
            tid = int(event['tid'])
            vtid = tid_to_vtid[tid]  # Don't use the event `vtid`.
            time = len(threads[vtid])
            quest_handlers.append((time, vtid))
    
    vtids = list(threads.keys())

    def average_stage_time(begin_ls, end_ls):
        stage_time = zip(begin_ls, end_ls)
        stage_time = tuple(map(lambda x: x[1] - x[0], stage_time))
        return round((sum(stage_time) / float(len(stage_time))) / 1000000000.0, 6)

    def average_lock_time(begin_ls, end_ls):
        return average_stage_time(begin_ls, end_ls)

    stage1_time = {}
    stage12_time = {}
    stage2_time = {}
    stage23_time = {}
    stage3_time = {}
    stage31_time = {}

    # Figure out the average time spent on each stage per thread.
    for vtid in vtids:
        stage1_time[vtid] = average_stage_time(stage1_begin_time[vtid],
                                               stage1_end_time[vtid])
        stage2_time[vtid] = average_stage_time(stage1_begin_time[vtid],
                                               stage1_end_time[vtid])
        stage3_time[vtid] = average_stage_time(stage1_begin_time[vtid],
                                               stage1_end_time[vtid])

        stage12_time[vtid] = average_stage_time(stage1_end_time[vtid],
                                                stage2_begin_time[vtid])
        stage23_time[vtid] = average_stage_time(stage2_end_time[vtid],
                                                stage3_begin_time[vtid])
        stage31_time[vtid] = average_stage_time(stage3_end_time[vtid],
                                                stage1_begin_time[vtid][1:])

    for vtid in vtids:
        times = [
            stage1_time[vtid],
            stage12_time[vtid],
            stage2_time[vtid],
            stage23_time[vtid],
            stage3_time[vtid],
            stage31_time[vtid],
        ]
        total_time = sum(times)
        avg_times = tuple(map(lambda x: x / total_time, times))
        print(vtid, avg_times)

    # Add on a sentinel quest handler event, that makes sure that the below
    # `for` loop that pokes in `0`s into `quest_events` will always have events
    # up until iteration where the quest is ending.
    quest_handlers.append([q_end, -1])

    # Open up CSV writers for each of the various kinds of charts that we
    # want to show.
    quest_writer = csv.writer(open("/tmp/quests.dat", "w"))
    num_req_writer = csv.writer(open("/tmp/num_req.dat", "w"))
    proc_time_writer = csv.writer(open("/tmp/proc_time.dat", "w"))
    num_update_writer = csv.writer(open("/tmp/num_update.dat", "w"))
    sent_time_writer = csv.writer(open("/tmp/sent_time.dat", "w"))
    stage_time = csv.writer(open("/tmp/stage_time.dat", "w"))
    stage_breakdown = csv.writer(open("/tmp/stage_breakdown.dat", "w"))

    # Double check that we have exactly 1000 events.
    num_events = min(len(events) for events in threads.values())

    # Fill in every event as "missing".
    quest_events = {}
    for vtid in vtids:
        quest_events[vtid] = ['?'] * num_events

    # Poke zeroes into the individual points of the events within the ranges
    # where a thread holds the quest region.
    for i, handler in enumerate(quest_handlers):
        if i + 1 == len(quest_handlers):
            break

        # Use this quest handler and the next quest handler thread to form
        # a time range of points where this quest handler thread should have
        # non-`?` data points.
        next_handler = quest_handlers[i + 1]
        vtid = handler[1]
        start_time = max(handler[0], q_begin)
        end_time = min(next_handler[0], q_end)

        for time in range(start_time, end_time):
            quest_events[vtid][time] = 0

    # Dump out all the info. This aggregates across threads.
    for time in range(num_events):
        num_req = [time]
        proc_time = [time]
        num_update = [time]
        sent_time = [time]
        quest = [time]

        # Aggregate info from each thread.
        for vtid in vtids:
            it = threads[vtid][time]
            num_req.append(it.num_req)
            proc_time.append(it.proc_time)
            num_update.append(it.num_update)
            sent_time.append(it.sent_time)
            quest.append(quest_events[vtid][time])

        # Dump aggregated rows of data.
        num_req_writer.writerow(num_req)
        proc_time_writer.writerow(proc_time)
        num_update_writer.writerow(num_update)
        sent_time_writer.writerow(sent_time)
        quest_writer.writerow(quest)

if '__main__' == __name__:
    process_trace()
