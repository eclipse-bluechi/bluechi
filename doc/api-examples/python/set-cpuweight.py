#!/usr/bin/python3
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

import sys

import dasbus.connection
from dasbus.typing import Variant

bus = dasbus.connection.SystemMessageBus()

if len(sys.argv) != 4:
    print(f"Usage: python {sys.argv[0]} node_name unit_name cpu_weight")
    sys.exit(1)

node_name = sys.argv[1]
unit_name = sys.argv[2]
value = int(sys.argv[3])

# Don't persist change
runtime = True

controller = bus.get_proxy("org.eclipse.bluechi", "/org/eclipse/bluechi")
node_path = controller.GetNode(node_name)
node = bus.get_proxy("org.eclipse.bluechi", node_path)

node.SetUnitProperties(unit_name, runtime, [("CPUWeight", Variant("t", value))])
