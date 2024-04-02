#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.error import DBusError
from dasbus.loop import EventLoop

from bluechi.api import Metrics, Node, UInt64


class TestMetricsIsEnabled(unittest.TestCase):
    def setUp(self) -> None:
        self.loop = EventLoop()
        self.metrics = Metrics()
        self.signal_received = False
        self.loop_quitter = None

    def test_metrics_is_enabled(self):
        def on_start_unit_metrics_received(
            node_name: str,
            job_id: str,
            unit: str,
            job_time_micros: UInt64,
            start_time: UInt64,
        ) -> None:
            if node_name == "node-foo" and unit == "simple.service":
                self.signal_received = True
                self.loop.quit()

        try:
            # This will fail if metrics are disabled
            self.metrics.on_start_unit_job_metrics(on_start_unit_metrics_received)
        except DBusError as ex:
            raise Exception(f"Error subscribing to metrics: {ex}")

        n = Node("node-foo")
        assert n.start_unit("simple.service", "replace") != ""

        self.loop.run()
        assert n.stop_unit("simple.service", "replace") != ""

        assert self.signal_received


if __name__ == "__main__":
    unittest.main()
