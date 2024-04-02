#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.loop import EventLoop

from bluechi.api import Controller, Monitor, Node, Structure

node_name_foo = "node-foo"

service_simple = "simple.service"


class TestMonitorSpecificNodeAndUnit(unittest.TestCase):
    def setUp(self) -> None:
        self.loop = EventLoop()
        self.mgr = Controller()

        self.times_new_called = 0
        self.times_state_changed_called = 0
        self.times_prop_changed_called = 0
        self.times_removed_called = 0

    def test_monitor_specific_node_and_unit(self):

        monitor_path = self.mgr.create_monitor()
        monitor = Monitor(monitor_path=monitor_path)

        def on_unit_new(node: str, unit: str, reason: str) -> None:
            self.times_new_called += 1

        def on_unit_state_changed(
            node: str, unit: str, active_state: str, sub_state: str, reason: str
        ) -> None:
            self.times_state_changed_called += 1

        def on_unit_property_changed(
            node: str, unit: str, interface: str, props: Structure
        ) -> None:
            self.times_prop_changed_called += 1

        def on_unit_removed(node: str, unit: str, reason: str) -> None:
            self.times_removed_called += 1
            self.loop.quit()

        monitor.on_unit_new(on_unit_new)
        monitor.on_unit_state_changed(on_unit_state_changed)
        monitor.on_unit_properties_changed(on_unit_property_changed)
        monitor.on_unit_removed(on_unit_removed)

        monitor.subscribe(node_name_foo, service_simple)

        node_foo = Node(node_name_foo)
        assert node_foo.start_unit(service_simple, "replace") != ""
        self.loop.run()

        assert self.times_new_called >= 1
        assert self.times_state_changed_called >= 1
        assert self.times_prop_changed_called >= 1
        assert self.times_removed_called >= 1


if __name__ == "__main__":
    unittest.main()
