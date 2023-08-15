#!/usr/bin/python3

import sys
from dasbus.typing import get_native
import dasbus.connection
bus = dasbus.connection.SystemMessageBus()

if len(sys.argv) != 3:
    print("No node name and unit supplied")
    sys.exit(1)

node_name = sys.argv[1]
unit_name = sys.argv[2]

manager = bus.get_proxy("io.github.eclipse-bluechi.bluechi", "/io/github/eclipse-bluechi/bluechi")
node_path = manager.GetNode(node_name)
node = bus.get_proxy("io.github.eclipse-bluechi.bluechi", node_path)

properties = node.GetUnitProperties(unit_name, "org.freedesktop.systemd1.Unit")
print("Unit properties:")
print(get_native(properties))

print("Service properties:")
properties = node.GetUnitProperties(unit_name, "org.freedesktop.systemd1.Service")
print(get_native(properties))
