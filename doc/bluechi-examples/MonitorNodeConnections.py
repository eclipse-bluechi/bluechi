#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

from dasbus.loop import EventLoop
from dasbus.typing import Variant

from bluechi.api import Controller, Node

loop = EventLoop()

nodes = []
for node in Controller().list_nodes():
    n = Node(node[0])

    def changed_wrapper(node_name: str):
        def on_connection_status_changed(status: Variant):
            con_status = status.get_string()
            print(f"Node {node_name}: {con_status}")

        return on_connection_status_changed

    n.on_status_changed(changed_wrapper(n.name))
    nodes.append(n)


loop.run()
