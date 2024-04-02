#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import re
import unittest

from bluechi_machine_lib.bluechictl import BackgroundRunner, SimpleEventEvaluator
from dasbus.error import DBusError

from bluechi.api import Node

node_foo_name = "node-foo"
service_simple = "simple.service"


class TestBluechictlStatusWatch(unittest.TestCase):

    def setUp(self) -> None:
        node_status_offline = re.compile(r"^" + re.escape(node_foo_name) + ".*offline")
        node_status_online = re.compile(r"^" + re.escape(node_foo_name) + ".*online")
        expected_patterns = {node_status_online, node_status_offline}
        args = ["status", "-w"]
        self.evaluator = SimpleEventEvaluator(expected_patterns)
        self.bgrunner = BackgroundRunner(args, self.evaluator)

    def test_bluechictl_status_watch(self):
        self.bgrunner.start()
        # stop the bluechi controller service to trigger a disconnected message in the agent
        # hacky solution to cause a disconnect:
        #   stop the controller service
        # problem:
        #   dasbus will raise a DBusError for connection timeout on v1.4, v1.7 doesn't
        #   the command will be executed anyway, resulting in the desired shutdown of the node
        #   therefore, the DBusError is silenced for now, but all other errors are still raised
        try:
            node = Node(node_foo_name)
            node.stop_unit("bluechi-agent.service", "replace")
        except DBusError:
            pass
        self.bgrunner.stop()
        assert self.evaluator.processing_finished()


if __name__ == "__main__":
    unittest.main()
