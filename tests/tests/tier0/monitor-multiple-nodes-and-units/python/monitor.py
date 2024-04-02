#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.loop import EventLoop

from bluechi.api import Controller, Monitor, Node, Structure

node_name_foo = "node-foo"
node_name_bar = "node-bar"

service_simple = "simple.service"
service_also_simple = "also-simple.service"


class TestMonitorMultipleNodesAndUnits(unittest.TestCase):
    def setUp(self) -> None:
        self.loop = EventLoop()
        self.mgr = Controller()

        self.times_new_called = 0
        self.times_state_changed_called = 0
        self.times_prop_changed_called = 0
        self.times_removed_called = 0
        self.received_signal_from_node_bar = False

    def test_monitor_multiple_nodes_and_units(self):

        monitor_path = self.mgr.create_monitor()
        monitor = Monitor(monitor_path=monitor_path)

        def on_unit_new(node: str, unit: str, reason: str) -> None:
            self.times_new_called += 1
            self.received_signal_from_node_bar = node == node_name_bar

        def on_unit_state_changed(
            node: str, unit: str, active_state: str, sub_state: str, reason: str
        ) -> None:
            self.times_state_changed_called += 1
            self.received_signal_from_node_bar = node == node_name_bar

        def on_unit_property_changed(
            node: str, unit: str, interface: str, props: Structure
        ) -> None:
            self.times_prop_changed_called += 1
            self.received_signal_from_node_bar = node == node_name_bar

        def on_unit_removed(node: str, unit: str, reason: str) -> None:
            self.times_removed_called += 1
            self.received_signal_from_node_bar = node == node_name_bar

            # received two unit_removed signals (simple.service and also-simple.service)
            if self.times_removed_called == 2:
                self.loop.quit()

        monitor.on_unit_new(on_unit_new)
        monitor.on_unit_state_changed(on_unit_state_changed)
        monitor.on_unit_properties_changed(on_unit_property_changed)
        monitor.on_unit_removed(on_unit_removed)

        monitor.subscribe_list(node_name_foo, [service_simple, service_also_simple])

        node_bar = Node(node_name_bar)
        assert node_bar.start_unit(service_simple, "replace") != ""

        node_foo = Node(node_name_foo)
        assert node_foo.start_unit(service_simple, "replace") != ""
        assert node_foo.start_unit(service_also_simple, "replace") != ""

        self.loop.run()

        assert not self.received_signal_from_node_bar
        assert self.times_new_called >= 2
        assert self.times_state_changed_called >= 2
        assert self.times_prop_changed_called >= 2
        assert self.times_removed_called >= 2


if __name__ == "__main__":
    unittest.main()
