#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import time
import unittest

from dasbus.error import DBusError

from bluechi.api import Job, Node

node_name_foo = "node-foo"
service_simple = "simple.service"


class TestStartAndCancel(unittest.TestCase):
    def _get_unit_state(self, node: Node, service: str) -> str:
        prop = node.get_unit_property(
            service, "org.freedesktop.systemd1.Unit", "ActiveState"
        )
        return prop.get_string()

    def test_start_and_cancel(self):
        node_foo = Node(node_name_foo)
        job_path = node_foo.start_unit(service_simple, "replace")
        assert job_path != ""

        # delay job assertions a bit
        time.sleep(0.5)

        job = Job(job_path)
        assert job.node == node_foo.name
        assert job.state in ["waiting", "running"]
        assert self._get_unit_state(node_foo, service_simple) == "activating"

        job.cancel()

        # after cancelling:
        # - the job should be removed - so we expect another call to fail
        # - the started unit should still be activating (canceling job won't stop it)
        time.sleep(0.5)
        with self.assertRaises(DBusError):
            job.state
        assert self._get_unit_state(node_foo, service_simple) == "activating"


if __name__ == "__main__":
    unittest.main()
