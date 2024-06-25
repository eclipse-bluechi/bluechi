#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import time
import unittest

from dasbus.typing import Variant

from bluechi.api import Node


class TestNodeIsConnected(unittest.TestCase):
    def test_node_is_connected(self):

        def on_state_change(state: Variant):
            assert False

        n = Node("node-foo")
        assert n.status == "online"

        n.on_status_changed(on_state_change)
        timestamp = n.last_seen_timestamp

        # verify that the node remains connected and LastSeenTimespamp is not
        # updated after more than NodeHeartbeatThreshold seconds have elapsed
        time.sleep(10)

        assert n.status == "online"
        assert n.last_seen_timestamp == timestamp


if __name__ == "__main__":
    unittest.main()
