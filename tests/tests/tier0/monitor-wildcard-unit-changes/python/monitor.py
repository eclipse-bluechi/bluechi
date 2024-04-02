#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.loop import EventLoop

from bluechi.api import Controller, Monitor, Node, Structure

# In this test, first the wildcard subscription is created and then the systemd units on both nodes
# are started. These systemd only units start and exit, therefore triggering all lifecycle events.
# The test loop continues till the unit removed signals from both services (from the respective nodes)
# are received. Subsequently, assert that at all types of signals are received at least once.


node_name_foo = "node-foo"
node_name_bar = "node-bar"

service_simple = "simple.service"
service_also_simple = "also-simple.service"


class CallCounter:
    def __init__(self) -> None:
        self.times_new_has_been_called = 0
        self.times_removed_has_been_called = 0
        self.times_state_change_has_been_called = 0
        self.times_property_change_has_been_called = 0


class TestMonitorWildcardUnitChanges(unittest.TestCase):
    def setUp(self) -> None:
        self.call_counter = {
            node_name_foo: {service_simple: CallCounter()},
            node_name_bar: {service_also_simple: CallCounter()},
        }

        self.loop = EventLoop()
        self.mgr = Controller()
        self.monitor = Monitor(self.mgr.create_monitor())

        def on_unit_new(node: str, unit: str, reason: str) -> None:
            if node in self.call_counter and unit in self.call_counter[node]:
                self.call_counter[node][unit].times_new_has_been_called += 1

        def on_unit_removed(node: str, unit: str, reason: str) -> None:
            if node in self.call_counter and unit in self.call_counter[node]:
                self.call_counter[node][unit].times_removed_has_been_called += 1

                foo_simple = self.call_counter[node_name_foo][service_simple]
                bar_also_simple = self.call_counter[node_name_bar][service_also_simple]
                if (
                    foo_simple.times_removed_has_been_called > 0
                    and bar_also_simple.times_removed_has_been_called > 0
                ):
                    self.loop.quit()

        def on_unit_state_changed(
            node: str, unit: str, active_state: str, sub_state: str, reason: str
        ) -> None:
            if node in self.call_counter and unit in self.call_counter[node]:
                self.call_counter[node][unit].times_state_change_has_been_called += 1

        def on_unit_property_changed(
            node: str, unit: str, interface: str, props: Structure
        ) -> None:
            if node in self.call_counter and unit in self.call_counter[node]:
                self.call_counter[node][unit].times_property_change_has_been_called += 1

        self.monitor.on_unit_new(on_unit_new)
        self.monitor.on_unit_removed(on_unit_removed)
        self.monitor.on_unit_state_changed(on_unit_state_changed)
        self.monitor.on_unit_properties_changed(on_unit_property_changed)

    def test_monitor_wildcard_unit_changes(self):
        node_foo = Node(node_name_foo)
        node_bar = Node(node_name_bar)

        # start subscription on all nodes and units
        self.monitor.subscribe("*", "*")

        assert node_foo.start_unit(service_simple, "replace") != ""
        assert node_bar.start_unit(service_also_simple, "replace") != ""

        # will stop when the unit removed signal has been received for
        # both services on the respective nodes
        self.loop.run()

        # verify that the signals for those events were received
        # (at least once since the number of calls is a systemd internal)

        foo_simple = self.call_counter[node_name_foo][service_simple]
        bar_also_simple = self.call_counter[node_name_bar][service_also_simple]

        assert foo_simple.times_new_has_been_called >= 1
        assert foo_simple.times_removed_has_been_called >= 1
        assert foo_simple.times_state_change_has_been_called >= 1
        assert foo_simple.times_property_change_has_been_called >= 1

        assert bar_also_simple.times_new_has_been_called >= 1
        assert bar_also_simple.times_removed_has_been_called >= 1
        assert bar_also_simple.times_state_change_has_been_called >= 1
        assert bar_also_simple.times_property_change_has_been_called >= 1


if __name__ == "__main__":
    unittest.main()
