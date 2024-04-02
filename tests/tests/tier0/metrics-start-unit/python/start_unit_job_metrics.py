#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.loop import EventLoop

from bluechi.api import Controller, Metrics, Node, UInt64


class TestStartUnitJobMetrics(unittest.TestCase):
    def setUp(self) -> None:
        self.loop = EventLoop()

        self.mgr = Controller()
        self.mgr.enable_metrics()

        self.metrics = Metrics()

        self.received_node_name = ""
        self.received_job_id = ""
        self.received_unit_name = ""
        self.received_job_time = -1
        self.received_start_time = -1

    def test_start_unit_job_metrics(self):
        def on_start_metrics(
            node_name: str,
            job_id: str,
            unit: str,
            job_time_micros: UInt64,
            start_time: UInt64,
        ) -> None:
            self.received_node_name = node_name
            self.received_job_id = job_id
            self.received_unit_name = unit
            self.received_job_time = job_time_micros
            self.received_start_time = start_time

            self.loop.quit()

        self.metrics.on_start_unit_job_metrics(on_start_metrics)

        n = Node("node-foo")
        assert n.start_unit("simple.service", "replace") != ""

        self.loop.run()

        assert self.received_node_name == "node-foo"
        assert self.received_job_id != ""
        assert self.received_unit_name == "simple.service"
        assert self.received_job_time >= 0
        assert self.received_start_time >= 0


if __name__ == "__main__":
    unittest.main()
