#!/usr/bin/python3
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

import sys

import dasbus.connection
from dasbus.typing import get_native

bus = dasbus.connection.SystemMessageBus()

if len(sys.argv) != 3:
    print("No node name and unit supplied")
    sys.exit(1)

node_name = sys.argv[1]
unit_name = sys.argv[2]

controller = bus.get_proxy("org.eclipse.bluechi", "/org/eclipse/bluechi")
node_path = controller.GetNode(node_name)
node = bus.get_proxy("org.eclipse.bluechi", node_path)

properties = node.GetUnitProperties(unit_name, "org.freedesktop.systemd1.Unit")
print("Unit properties:")
print(get_native(properties))

print("Service properties:")
properties = node.GetUnitProperties(unit_name, "org.freedesktop.systemd1.Service")
print(get_native(properties))
