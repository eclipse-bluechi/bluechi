# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.error import DBusError
from dasbus.loop import EventLoop

from bluechi.api import Manager, Monitor, Node


node_name_foo = "node-foo"
service_simple = "simple.service"


class TestUnsubscribe(unittest.TestCase):

    def setUp(self) -> None:
        self.loop = EventLoop()
        self.mgr = Manager()
        monitor_path = self.mgr.create_monitor()
        self.monitor = Monitor(monitor_path=monitor_path)

        self.times_removed_called = 0

        def on_unit_removed(node: str, unit: str, reason: str) -> None:
            self.times_removed_called += 1
            self.loop.quit()

        self.monitor.on_unit_removed(on_unit_removed)

    def test_unsubscribe(self):

        node_foo = Node(node_name_foo)

        # create subscription, start unit and run event loop till removed signal is received
        sub_id = self.monitor.subscribe(node_name_foo, service_simple)
        assert node_foo.start_unit(service_simple, "replace") != ""
        self.loop.run()

        # subscription is already automatically removed when the event loop quits (no consumer)
        # therefore, expect a DBussError to be raised
        with self.assertRaises(DBusError):
            self.monitor.unsubscribe(sub_id)

        assert self.times_removed_called == 1


if __name__ == "__main__":
    unittest.main()
