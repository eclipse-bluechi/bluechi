# SPDX-License-Identifier: LGPL-2.1-or-later

import sys
from bluechi.api import Manager, Monitor
from dasbus.loop import EventLoop

if len(sys.argv) != 3:
    print("Usage: python create_monitor.py <node-to-watch> <service-to-watch>")
    exit(1)

node = sys.argv[1]
service = sys.argv[2]

loop = EventLoop()
mgr = Manager()
monitor_path = mgr.create_monitor()
monitor = Monitor(monitor_path)
monitor.subscribe(node, service)

print(monitor_path, flush=True)

loop.run()
