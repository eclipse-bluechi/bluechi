#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import time
import unittest

from bluechi.api import Node


class TestNodeIsConnected(unittest.TestCase):
    def test_node_is_connected(self):
        n = Node("node-foo")
        assert n.status == "online"

        # verify that the last seen timestamp is updated with each heartbeat
        timestamp = n.last_seen_timestamp
        time.sleep(2.5)
        assert n.last_seen_timestamp > timestamp


if __name__ == "__main__":
    unittest.main()
