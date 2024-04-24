#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import time
import unittest

from bluechi.api import Node


class TestNodeHeartbeat(unittest.TestCase):
    def test_node_heartbeat(self):
        n = Node("node-foo")
        assert n.status == "online"

        # verify that heartbeat occurs at least once in 2 seconds
        # since last_seen_timestamp is epoch, i.e. in seconds value
        # we check every 2 seconds twice to be sure
        timestamp1 = n.last_seen_timestamp
        time.sleep(2)
        timestamp2 = n.last_seen_timestamp
        time.sleep(2)
        timestamp3 = n.last_seen_timestamp
        print(f"timestamp1={timestamp1}")
        print(f"timestamp2={timestamp2}")
        print(f"timestamp3={timestamp3}")
        assert timestamp2 > timestamp1
        assert timestamp3 > timestamp2


if __name__ == "__main__":
    unittest.main()
