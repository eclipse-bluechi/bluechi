#!/usr/bin/python3
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

import sys

import dasbus.connection
from dasbus.loop import EventLoop
from dasbus.typing import get_native


def print_dict_changes(old, new):
    for key in sorted(set(old.keys()) | set(new.keys())):
        if key not in old:
            print(f" {key}: {new[key]}")
        elif key in new:
            o = old[key]
            n = new[key]
            if o != n:
                print(f" {key}: {o} -> {n}")


if len(sys.argv) < 2:
    print("No unit name supplied")
    sys.exit(1)

unit_name = sys.argv[1]
node_name = "*"  # Match all

if len(sys.argv) > 2:
    node_name = sys.argv[2]

bus = dasbus.connection.SystemMessageBus()

controller = bus.get_proxy("org.eclipse.bluechi", "/org/eclipse/bluechi")

monitor_path = controller.CreateMonitor()
monitor = bus.get_proxy("org.eclipse.bluechi", monitor_path)

old_values = {}


def unit_property_changed(node, unit, interface, props):
    old_value = old_values.get(node, {})
    new_value = get_native(props)

    print(f"Unit {unit} on node {node} changed for iface {interface}:")
    print_dict_changes(old_value, new_value)

    old_values[node] = {**old_value, **new_value}


def unit_new(node, unit, reason):
    print(f"New Unit {unit} on node {node}, reason: {reason}")


def unit_state_changed(node, unit, active_state, substate, reason):
    print(
        f"Unit {unit} on node {node}, changed to state: {active_state} ({substate}), reason: {reason}"
    )


def unit_removed(node, unit, reason):
    print(f"Removed Unit {unit} on node {node}, reason: {reason}")
    if node in old_values:
        del old_values[node]


monitor.UnitPropertiesChanged.connect(unit_property_changed)
monitor.UnitNew.connect(unit_new)
monitor.UnitStateChanged.connect(unit_state_changed)
monitor.UnitRemoved.connect(unit_removed)

monitor.Subscribe(node_name, unit_name)

if node_name == "":
    print(f"Waiting for changes to unit `{unit_name}` on any node")
else:
    print(f"Waiting for changes to unit `{unit_name}` on node '{node_name}'")

loop = EventLoop()
loop.run()
