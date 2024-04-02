#!/usr/bin/python3
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

import sys

import dasbus.connection
from dasbus.typing import get_native

bus = dasbus.connection.SystemMessageBus()

if len(sys.argv) != 5:
    print("No node name, unit, interface and property supplied")
    sys.exit(1)

node_name = sys.argv[1]
unit_name = sys.argv[2]
iface_name = sys.argv[3]
property_name = sys.argv[4]

controller = bus.get_proxy("org.eclipse.bluechi", "/org/eclipse/bluechi")
node_path = controller.GetNode(node_name)
node = bus.get_proxy("org.eclipse.bluechi", node_path)

val = node.GetUnitProperty(unit_name, iface_name, property_name)
print(f"{property_name}: {get_native(val)}")
