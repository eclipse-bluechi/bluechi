#!/usr/bin/python3
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

from collections import namedtuple

import dasbus.connection

bus = dasbus.connection.SystemMessageBus()

NodeInfo = namedtuple("NodeInfo", ["name", "object_path", "status", "peer_ip"])

controller = bus.get_proxy("org.eclipse.bluechi", "/org/eclipse/bluechi")
nodes = controller.ListNodes()
for n in nodes:
    info = NodeInfo(*n)
    print(f"Node: {info.name}, State: {info.status}")
