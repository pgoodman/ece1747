import sys
from collections import Counter,  defaultdict
import babeltrace
import csv

class Iteration(object):
    __slots__ = ('time', 'num_req', 'proc_time', 'num_update', 'sent_time')
    def __init__():
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
    # iterate events
    for event in col.events:
        # keep only `sched_switch` events
        if "join" in event.name:
            num_player += 1
        elif  "quit" in event.name:
            num_player -= 1
        elif "first_stage" in event.name:
            it = Iteration()
            it.time = event.timestamp
            it.num_req = event['num_req']
            it.proc_time = event['proc_time']
            threads[event['vtid']].append(it)
        elif "third_stage" in event.name:
            it = threads[event['vtid']][-1]
            it.num_update = event['num_update']
            it.send_time = event['sent_time']

    writer = csv.writer(open("data.csv", "wb"))

    num_events = min(len(events) for events in threads.values())
    tids = list(threads.keys())

    writer.writerow(["time"] + tids + ["","time"] + tids + ["","time"] + tids + ["","time"] + tids)

    for i in range(num_events):
        num_req = []
        proc_time = []
        num_update = []
        sent_time = []
        time = 0
        for tid in tids:
            it = threads[tid][i]
            time = time or it.time
            num_req.append(it.num_req)
            proc_time.append(it.proc_time)
            num_update.append(it.num_update)
            sent_time.append(it.sent_time)

        row = [time]
        row.extend(num_req)
        row.append(["",time])
        row.extend(proc_time)
        row.append(["",time])
        row.extend(num_update)
        row.append(["",time])
        row.extend(sent_time)
        writer.writerow(row)

if __name__ == '__main__':
    top5proc()
