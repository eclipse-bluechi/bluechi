#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from bluechi_machine_lib.bluechictl import BackgroundRunner, BluechiCtl, EventEvaluator

node_name_foo = "node-foo"
service_simple = "simple.service"


class MonitorSpecificNodeAndUnitEventEvaluator(EventEvaluator):
    def __init__(self):
        self.created = False
        self.removed = False

    def process_line(self, line: str) -> None:
        if not self.created and "Unit created (reason:" in line:
            print(
                f"Received UnitCreated signal for service '{service_simple}' "
                f"on node '{node_name_foo}'"
            )
            self.created = True

        elif self.created and not self.removed and "Unit removed (reason:" in line:
            print(
                f"Received UnitRemoved signal for service '{service_simple}' "
                f"on node '{node_name_foo}'"
            )
            self.removed = True

    def processing_finished(self) -> bool:
        return self.removed and self.created


class TestMonitorSpecificNodeAndUnit(unittest.TestCase):
    def setUp(self) -> None:
        self.evaluator = MonitorSpecificNodeAndUnitEventEvaluator()
        self.bgrunner = BackgroundRunner(
            ["monitor", f"{node_name_foo}", f"{service_simple}"], self.evaluator
        )
        self.bluechictl = BluechiCtl()

    def test_monitor_specific_node_and_unit(self):
        self.bgrunner.start()
        # Running bluechictl status on inactive unit should raise UnitCreated and UnitRemoved signals
        res, out, _ = self.bluechictl.status_unit(node_name_foo, service_simple)
        assert res == 0
        assert "inactive" in out
        self.bgrunner.stop()
        assert self.evaluator.processing_finished()


if __name__ == "__main__":
    unittest.main()
