#!/usr/bin/python3
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

import sys
from collections import namedtuple

import dasbus.connection

bus = dasbus.connection.SystemMessageBus()

UnitInfo = namedtuple(
    "UnitInfo",
    [
        "name",
        "description",
        "load_state",
        "active_state",
        "sub_state",
        "follower",
        "object_path",
        "job_id",
        "job_type",
        "job_object_path",
    ],
)

if len(sys.argv) != 2:
    print("No node name supplied")
    sys.exit(1)

node_name = sys.argv[1]

controller = bus.get_proxy("org.eclipse.bluechi", "/org/eclipse/bluechi")
node_path = controller.GetNode(node_name)
node = bus.get_proxy("org.eclipse.bluechi", node_path)

units = node.ListUnits()
for u in units:
    info = UnitInfo(*u)
    print(f"{info.name} - {info.description}")
