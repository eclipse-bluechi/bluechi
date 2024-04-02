#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.error import DBusError
from dasbus.loop import EventLoop

from bluechi.api import Controller, Monitor, Node


class TestMonitorWildcardNodeReconnect(unittest.TestCase):
    def setUp(self) -> None:
        self.expected_nodes_unit_new = ["node-foo", "node-bar", "node-baz"]
        self.expected_nodes_unit_removed = ["node-foo"]

        self.loop = EventLoop()
        self.mgr = Controller()
        self.monitor = Monitor(self.mgr.create_monitor())

        def on_unit_new(node: str, unit: str, reason: str) -> None:
            if unit == "*" and reason == "virtual":
                self.expected_nodes_unit_new.remove(node)

            if not self.expected_nodes_unit_new:
                self.loop.quit()

        def on_unit_removed(node: str, unit: str, reason: str) -> None:
            if unit == "*" and reason == "virtual":
                self.expected_nodes_unit_removed.remove(node)

        self.monitor.on_unit_new(on_unit_new)
        self.monitor.on_unit_removed(on_unit_removed)

    def test_monitor_wildcard_node_reconnect(self):

        # start subscription on all nodes and units
        self.monitor.subscribe("*", "*")
        # will stop when all virtual unit_new signals for all nodes are received
        self.loop.run()

        assert len(self.expected_nodes_unit_new) == 0
        assert len(self.expected_nodes_unit_removed) == 1

        # node-foo is about to be restarted, so the unit_new and unit_removed
        # callbacks should be called - push node-foo back into the list
        node_name_foo = self.expected_nodes_unit_removed[0]
        self.expected_nodes_unit_new.append(node_name_foo)
        node = Node(node_name_foo)
        # for an explanation for the try-except please see monitor-node-disconnect/monitor.py
        try:
            node.restart_unit("bluechi-agent.service", "replace")
        except DBusError:
            pass

        # will stop when the virtual unit_new signal for node-foo is received
        self.loop.run()

        assert len(self.expected_nodes_unit_new) == 0
        assert len(self.expected_nodes_unit_removed) == 0


if __name__ == "__main__":
    unittest.main()
