#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import time
import unittest

from dasbus.loop import EventLoop
from dasbus.typing import Variant

from bluechi.api import Node


class TestNodeIsDisconnected(unittest.TestCase):

    node_state = None
    timestamp = None

    def test_node_is_disconnected(self):
        loop = EventLoop()

        def on_state_change(state: Variant):
            self.timestamp = round(time.time() * 1000000)
            self.node_state = state.get_string()
            loop.quit()

        n = Node("node-foo")
        assert n.status == "online"

        n.on_status_changed(on_state_change)
        timestamp = n.last_seen_timestamp

        loop.run()

        # verify that the node is disconnected after more than
        # NodeHeartbeatThreshold seconds have elapsed
        assert self.timestamp - timestamp > 6000000
        assert self.node_state == "offline"


if __name__ == "__main__":
    unittest.main()
