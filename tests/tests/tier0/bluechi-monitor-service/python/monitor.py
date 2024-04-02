#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import re
import unittest

from bluechi_machine_lib.bluechictl import BackgroundRunner, BluechiCtl, EventEvaluator

node_name_foo = "node-foo"
service_simple = "simple.service"


class MonitorSpecificNodeAndUnitStateEventEvaluator(EventEvaluator):
    def __init__(self):
        self.state_pat = re.compile(r"\s*ActiveState: (?P<active_state>[\S]+)\s*")
        self.inactive = False
        self.active = True

    def process_line(self, line: str) -> None:
        match = self.state_pat.match(line)
        if not match:
            print(f"Ignoring line '{line}'")
            return
        active_state = match.group("active_state")
        if active_state == "inactive" and self.active and not self.inactive:
            # Active -> Inactive
            print(
                f"Service '{service_simple}' on node '{node_name_foo}' was stopped successfully"
            )
            self.active = False
            self.inactive = True
        elif active_state == "active" and self.inactive:
            # Inactive -> Active
            print(
                f"Service '{service_simple}' on node '{node_name_foo}' was started successfully"
            )
            self.active = True

    def processing_finished(self) -> bool:
        return self.inactive and self.active


class TestMonitorSpecificNodeAndUnit(unittest.TestCase):
    def setUp(self) -> None:
        self.evaluator = MonitorSpecificNodeAndUnitStateEventEvaluator()
        self.bgrunner = BackgroundRunner(
            ["monitor", f"{node_name_foo}", f"{service_simple}"], self.evaluator
        )
        self.bluechictl = BluechiCtl()

    def test_monitor_specific_node_and_unit(self):
        self.bgrunner.start()
        res, out, _ = self.bluechictl.unit_stop(node_name_foo, service_simple)
        assert res == 0
        res, out, _ = self.bluechictl.unit_start(node_name_foo, service_simple)
        assert res == 0

        self.bgrunner.stop()
        assert self.evaluator.processing_finished()


if __name__ == "__main__":
    unittest.main()
