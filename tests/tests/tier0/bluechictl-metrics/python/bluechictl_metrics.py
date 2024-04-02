#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import re
import unittest

from bluechi_machine_lib.bluechictl import (
    BackgroundRunner,
    BluechiCtl,
    SimpleEventEvaluator,
)

node_foo_name = "node-foo"
service_simple = "simple.service"


class TestBluechictlMetrics(unittest.TestCase):
    def setUp(self) -> None:
        signal_agent_metrics_start_pat = re.compile(r"^.*Agent systemd StartUnit job.*")
        signal_agent_metrics_stop_pat = re.compile(r"^.*Agent systemd StopUnit job.*")
        signal_controller_metrics_pat = re.compile(
            r"^.*BlueChi job gross measured time.*"
        )
        expected_patterns = {
            signal_agent_metrics_start_pat,
            signal_agent_metrics_stop_pat,
            signal_controller_metrics_pat,
        }
        args = ["metrics", "listen"]
        self.evaluator = SimpleEventEvaluator(expected_patterns)
        self.bgrunner = BackgroundRunner(args, self.evaluator)
        self.bluechictl = BluechiCtl()

    def test_bluechictl_metrics(self):
        self.bluechictl.metrics_enable()
        self.bgrunner.start()
        self.bluechictl.unit_start(node_foo_name, service_simple)
        self.bluechictl.unit_stop(node_foo_name, service_simple)
        self.bgrunner.stop()
        assert self.evaluator.processing_finished()


if __name__ == "__main__":
    unittest.main()
