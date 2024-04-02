#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import sys

from dasbus.loop import EventLoop

from bluechi.api import Monitor

if len(sys.argv) != 2:
    sys.exit(1)

monitor_path = sys.argv[1]

loop = EventLoop()

monitor = Monitor(monitor_path)


def on_new(node, unit, reason):
    print(f"Unit New: {node} -- {unit}", flush=True)


def on_removed(node, unit, reason):
    print(f"Unit Removed: {node} -- {unit}", flush=True)


def on_state(node, unit, active, sub, reason):
    print(f"Unit state: {node} -- {unit}", flush=True)


def on_props(node, unit, interface, props):
    print(f"Unit props: {node} -- {unit}", flush=True)


monitor.on_unit_new(on_new)
monitor.on_unit_removed(on_removed)
monitor.on_unit_state_changed(on_state)
monitor.on_unit_properties_changed(on_props)


def on_removed(reason):
    print(f"Removed, reason: {reason}", flush=True)
    loop.quit()


monitor.get_proxy().PeerRemoved.connect(on_removed)

print(monitor.bus.connection.get_unique_name(), flush=True)

loop.run()
