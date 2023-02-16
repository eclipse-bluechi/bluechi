#!/usr/bin/python3

from datetime import datetime
import sys
from dasbus.connection import SessionMessageBus
from dasbus.loop import EventLoop

bus = SessionMessageBus()

if len(sys.argv) != 3:
    print("No node name and unit supplied")
    sys.exit(1)

node_name = sys.argv[1]
unit_name = sys.argv[2]

manager = bus.get_proxy("org.containers.hirte",  "/org/containers/hirte")
node_path = manager.GetNode(node_name)
node = bus.get_proxy("org.containers.hirte",  node_path)

loop = EventLoop()

def job_removed(id, job_path, node_name, unit, result, job_time):
    if job_path == my_job_path:
        run_time = (datetime.utcnow() - start_time).total_seconds()
        print(f"Started '{unit}' on node '{node_name}' with result '{result}' in {run_time*1000:.1f} msec, " 
                + f"systemd job net time {job_time} msec")
        loop.quit()

start_time = datetime.utcnow()

manager.JobRemoved.connect(job_removed)
my_job_path = node.StartUnit(unit_name, "replace")
loop.run()
