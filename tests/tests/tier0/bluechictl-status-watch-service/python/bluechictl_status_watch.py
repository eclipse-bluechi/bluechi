#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import re
import unittest

from bluechi_machine_lib.bluechictl import BackgroundRunner, SimpleEventEvaluator

from bluechi.api import Node

node_foo_name = "node-foo"
simple_service = "simple.service"


class TestBluechictlStatusWatch(unittest.TestCase):

    def setUp(self) -> None:
        service_status_active = re.compile(r"^" + simple_service + ".* active")
        service_status_inactive = re.compile(r"^" + simple_service + ".* inactive")
        expected_patterns = {service_status_active, service_status_inactive}
        args = ["status", node_foo_name, simple_service, "-w"]
        self.evaluator = SimpleEventEvaluator(expected_patterns)
        self.bgrunner = BackgroundRunner(args, self.evaluator)

    def test_bluechictl_status_watch(self):
        self.bgrunner.start()
        node = Node(node_foo_name)
        node.stop_unit(simple_service, "replace")
        node.start_unit(simple_service, "replace")
        node.stop_unit(simple_service, "replace")

        self.bgrunner.stop()
        assert self.evaluator.processing_finished()


if __name__ == "__main__":
    unittest.main()
