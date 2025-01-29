#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.loop import EventLoop
from dasbus.typing import Variant

from bluechi.api import Controller, Monitor, Node


class TestStartTransientUnit(unittest.TestCase):

    def __init__(self, methodName="runTest"):
        super().__init__(methodName)

        self.transient_unit = "transient.service"
        self.received_new_signal = False
        self.received_removed_signal = False

    def test_start_transient_unit(self):
        loop = EventLoop()

        controller = Controller()
        monitor_path = controller.create_monitor()
        monitor = Monitor(monitor_path)
        monitor.subscribe("node-foo", self.transient_unit)

        def on_new_unit(node, unit, reason):
            if unit == self.transient_unit:
                self.received_new_signal = True

        def on_removed_unit(node, unit, reason):
            if unit == self.transient_unit:
                self.received_removed_signal = True
                loop.quit()

        monitor.on_unit_new(on_new_unit)
        monitor.on_unit_removed(on_removed_unit)

        node = Node("node-foo")
        node.start_transient_unit(
            self.transient_unit,
            "replace",
            [
                ("Description", Variant("s", "My Test Transient Service")),
                (
                    "ExecStart",
                    Variant(
                        "a(sasb)", [("/usr/bin/sleep", ["/usr/bin/sleep", "1"], False)]
                    ),
                ),
            ],
            [],
        )

        loop.run()

        assert self.received_new_signal, "Did not receive new signal"
        assert self.received_removed_signal, "Did not receive remove signal"


if __name__ == "__main__":
    unittest.main()
