#!/usr/bin/python3

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

def job_removed(id, job_path, node_name, unit, result):
    if job_path == my_job_path:
        print(f"Start {unit} on node {node_name} completed, result: {result}")
        loop.quit()

manager.JobRemoved.connect(job_removed)
my_job_path = node.StartUnit(unit_name, "replace")
loop.run()
