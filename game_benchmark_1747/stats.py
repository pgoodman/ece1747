import sys
from collections import Counter,  defaultdict
import babeltrace
import csv

class Iteration(object):
    __slots__ = ('time', 'num_req', 'proc_time', 'num_update', 'sent_time')
    def __init__(self):
        self.time = 0
        self.num_req = 0
        self.proc_time = 0
        self.num_update = 0
        self.sent_time = 0

def top5proc():
    if len(sys.argv) != 2:
        msg = 'Usage: python {} TRACEPATH'.format(sys.argv[0])
        raise ValueError(msg)

    # a trace collection holds one to many traces
    col = babeltrace.TraceCollection()

    # add the trace provided by the user
    # (LTTng traces always have the 'ctf' format)
    if col.add_trace(sys.argv[1], 'ctf') is None:
        raise RuntimeError('Cannot add trace')

    # this counter dict will hold execution times:
    #
    #   task command name -> total execution time (ns)
    exec_times = Counter()

    # this holds the last `sched_switch` timestamp
    last_ts = None
    num_player = 0
    threads = defaultdict(lambda: [])
    
    stage_1 = 0
    stage_2 = 1
    first_time = 0

    tid_to_vtid = []
    quest_handlers = []
    max_value = -1

    # iterate events
    for event in col.events:
        # keep only `sched_switch` events
        if "join" in event.name:
            num_player += 1
        elif  "quit" in event.name:
            num_player -= 1
        elif "first_stage" in event.name:
            it = Iteration()
            first_time = first_time or event.timestamp
            it.time = max(event.timestamp - first_time, 0)
            it.num_req = event['num_req']
            it.proc_time = event['proc_time']
            vtid = event['vtid']
            threads[vtid].append(it)
            if vtid not in tid_to_vtid:
                tid_to_vtid.append(vtid)
        elif "third_stage" in event.name:
            it = threads[event['vtid']][-1]
            it.num_update = event['num_update']
            it.sent_time = event['sent_time']
        elif "quest_manager" in event.name:
            tid = event['tid']
            vtid = tid_to_vtid[int(tid)]
            time = len(threads[vtid])
            quest_handlers.append((time, vtid, tid))
    
    # Force the end of the quest.
    quest_handlers.append([700, -1, -1])

    for h in quest_handlers:
        print(h)

    quest_writer = csv.writer(open("quests.dat", "w"))
    num_req_writer = csv.writer(open("num_req.dat", "w"))
    proc_time_writer = csv.writer(open("proc_time.dat", "w"))
    num_update_writer = csv.writer(open("num_update.dat", "w"))
    sent_time_writer = csv.writer(open("sent_time.dat", "w"))

    num_events = min(len(events) for events in threads.values())
    

    quest_events = []
    for i in range(len(threads)):
        quest_events.append(['?'] * num_events)

    vtids = list(threads.keys())
    vtid_to_tid = dict((v,k) for k,v in enumerate(tid_to_vtid))
    
    for i, handler in enumerate(quest_handlers):
        if i + 1 == len(quest_handlers):
            break
        next_handler = quest_handlers[i + 1]
        tid = handler[2]

        for t in range(handler[0], next_handler[0]):
            quest_events[tid][t] = 0

    for i in range(num_events):
        num_req = [i]
        proc_time = [i]
        num_update = [i]
        sent_time = [i]
        quest = [i]
        time = 0
        for vtid in vtids:
            it = threads[vtid][i]
            time = time or it.time
            num_req.append(it.num_req)
            proc_time.append(it.proc_time)
            num_update.append(it.num_update)
            sent_time.append(it.sent_time)
            quest.append(quest_events[vtid_to_tid[vtid]][i])

        num_req_writer.writerow(num_req)
        proc_time_writer.writerow(proc_time)
        num_update_writer.writerow(num_update)
        sent_time_writer.writerow(sent_time)
        quest_writer.writerow(quest)

if __name__ == '__main__':
    top5proc()
