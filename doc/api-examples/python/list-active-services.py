#!/usr/bin/python3
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

from collections import namedtuple

import dasbus.connection

bus = dasbus.connection.SystemMessageBus()

NodeUnitInfo = namedtuple(
    "NodeUnitInfo",
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

controller = bus.get_proxy("org.eclipse.bluechi", "/org/eclipse/bluechi")
units = controller.ListUnits()
for node, units in units.items():
    for u in units:
        info = NodeUnitInfo(*u)
        if info.active_state == "active" and info.name.endswith(".service"):
            print(f"Node: {node}, Unit: {info.name}")
