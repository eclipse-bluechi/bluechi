#!/usr/bin/python3

import sys
import dasbus.connection
bus = dasbus.connection.SystemMessageBus()

if len(sys.argv) != 3:
    print("No node name, unit supplied")
    sys.exit(1)

node_name = sys.argv[1]
unit_name = sys.argv[2]

manager = bus.get_proxy("org.eclipse.bluechi", "/org/eclipse/bluechi")
node_path = manager.GetNode(node_name)
node = bus.get_proxy("org.eclipse.bluechi", node_path)

value = node.GetUnitProperty(unit_name, "org.freedesktop.systemd1.Service", "CPUWeight")
print(value)
