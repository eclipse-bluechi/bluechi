#!/usr/bin/python3

import sys
from dasbus.typing import Variant
import dasbus.connection
bus = dasbus.connection.SystemMessageBus()

if len(sys.argv) != 4:
    print("No node name, unit and value supplied")
    sys.exit(1)

node_name = sys.argv[1]
unit_name = sys.argv[2]
value = int(sys.argv[3])

# Don't persist change
runtime = True

manager = bus.get_proxy("io.github.eclipse-bluechi.bluechi", "/io/github/eclipse-bluechi/bluechi")
node_path = manager.GetNode(node_name)
node = bus.get_proxy("io.github.eclipse-bluechi.bluechi", node_path)

node.SetUnitProperties(unit_name, runtime, [("CPUWeight", Variant("t", value))])
