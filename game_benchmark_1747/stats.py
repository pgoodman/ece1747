import sys
from collections import Counter,  defaultdict
import babeltrace
import csv

duration = 2000
q_begin = 300
q_end = 1700
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

    # iterate events
    for event in col.events:
        # Special event that allows us to associate `tid`s with `vtid`s, so that
        # we don't get confused :D
        if "thread_begin" in event.name:
            vtid = int(event['vtid'])
            tid = int(event['tid'])
            tid_to_vtid[tid] = vtid
            vtid_to_tid[vtid] = tid

        # Get the first set of info from stage 1 for a given thread.
        elif "first_stage" in event.name:
            it = Iteration()
            it.num_req = event['num_req']
            it.proc_time = event['proc_time']
            vtid = int(event['vtid'])
            threads[vtid].append(it)

        # Extend the information from the first stage with that of the
        # third stage.
        elif "third_stage" in event.name:
            vtid = int(event['vtid'])
            it = threads[vtid][-1]
            it.num_update = event['num_update']
            it.sent_time = event['sent_time']

        elif "quest_manager" in event.name:
            tid = int(event['tid'])
            vtid = tid_to_vtid[tid]  # Don't use the event `vtid`.
            time = len(threads[vtid])
            quest_handlers.append((time, vtid))
    
    # Add on a sentinel quest handler event, that makes sure that the below
    # `for` loop that pokes in `0`s into `quest_events` will always have events
    # up until iteration where the quest is ending.
    quest_handlers.append([q_end, -1])

    # Open up CSV writers for each of the various kinds of charts that we
    # want to show.
    quest_writer = csv.writer(open("quests.dat", "w"))
    num_req_writer = csv.writer(open("num_req.dat", "w"))
    proc_time_writer = csv.writer(open("proc_time.dat", "w"))
    num_update_writer = csv.writer(open("num_update.dat", "w"))
    sent_time_writer = csv.writer(open("sent_time.dat", "w"))

    # Double check that we have exactly 1000 events.
    num_events = min(len(events) for events in threads.values())    
    vtids = list(threads.keys())

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
