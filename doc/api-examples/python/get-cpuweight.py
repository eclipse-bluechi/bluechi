#!/usr/bin/python3
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

import sys

import dasbus.connection

bus = dasbus.connection.SystemMessageBus()

if len(sys.argv) != 3:
    print(f"Usage: python {sys.argv[0]} node_name unit_name")
    sys.exit(1)

node_name = sys.argv[1]
unit_name = sys.argv[2]

controller = bus.get_proxy("org.eclipse.bluechi", "/org/eclipse/bluechi")
node_path = controller.GetNode(node_name)
node = bus.get_proxy("org.eclipse.bluechi", node_path)

value = node.GetUnitProperty(unit_name, "org.freedesktop.systemd1.Service", "CPUWeight")
print(value.get_uint64())
