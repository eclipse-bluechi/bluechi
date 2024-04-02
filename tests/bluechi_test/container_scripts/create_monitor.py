#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import sys

from dasbus.loop import EventLoop

from bluechi.api import Controller, Monitor

if len(sys.argv) != 3:
    print("Usage: python create_monitor.py <node-to-watch> <service-to-watch>")
    exit(1)

node = sys.argv[1]
service = sys.argv[2]

loop = EventLoop()
mgr = Controller()
monitor_path = mgr.create_monitor()
monitor = Monitor(monitor_path)
monitor.subscribe(node, service)

print(monitor_path, flush=True)

loop.run()
