#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.loop import EventLoop

from bluechi.api import Controller, Monitor, Node

node_name_foo = "node-foo"
service_simple = "simple.service"


class TestUnsubscribe(unittest.TestCase):
    def setUp(self) -> None:
        self.loop = EventLoop()
        self.mgr = Controller()
        monitor_path = self.mgr.create_monitor()
        self.monitor = Monitor(monitor_path=monitor_path)

        self.sub_id = -1
        self.unsubscribe_ex = None

        def on_unit_removed(node: str, unit: str, reason: str) -> None:
            # catch exception if raised, otherwise it'll prevent the loop quit
            # and result in a timeout. assert on the exception later.
            try:
                self.monitor.unsubscribe(self.sub_id)
            except Exception as ex:
                self.unsubscribe_ex = Exception(
                    f"Failed to unsubscribe '{self.sub_id}', got exception: {ex}"
                )
            self.loop.quit()

        self.monitor.on_unit_removed(on_unit_removed)

    def test_unsubscribe(self):

        node_foo = Node(node_name_foo)

        # create subscription, start unit and run event loop till removed signal is received
        self.sub_id = self.monitor.subscribe(node_name_foo, service_simple)
        assert node_foo.start_unit(service_simple, "replace") != ""
        self.loop.run()

        assert self.unsubscribe_ex is None


if __name__ == "__main__":
    unittest.main()
