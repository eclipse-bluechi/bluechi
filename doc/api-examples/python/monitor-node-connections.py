#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

from typing import Dict

import dasbus.connection
from dasbus.loop import EventLoop
from dasbus.typing import Variant

loop = EventLoop()
bus = dasbus.connection.SystemMessageBus()

controller = bus.get_proxy("org.eclipse.bluechi", "/org/eclipse/bluechi")
nodes = controller.ListNodes()
cached_nodes = []
for n in nodes:
    # node: [name, path, status]
    node = bus.get_proxy("org.eclipse.bluechi", n[1], "org.freedesktop.DBus.Properties")

    def changed_wrapper(node_name: str):
        def on_connection_status_changed(
            interface: str,
            changed_props: Dict[str, Variant],
            invalidated_props: Dict[str, Variant],
        ) -> None:
            con_status = changed_props["Status"].get_string()
            print(f"Node {node_name}: {con_status}")

        return on_connection_status_changed

    node.PropertiesChanged.connect(changed_wrapper(n[0]))
    cached_nodes.append(node)

loop.run()
