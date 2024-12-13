#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import time
import unittest

from bluechi_machine_lib.util import Timeout

from bluechi.api import Node

NODE_NAME = "node-foo"


class TestNodeIsConnected(unittest.TestCase):
    def test_node_is_connected(self):

        with Timeout(5, f"Timeout while waiting for agent '{NODE_NAME}' to be online"):
            status = "offline"
            while status != "online":
                try:
                    status = Node(NODE_NAME).status
                    time.sleep(0.5)
                except Exception:
                    pass


if __name__ == "__main__":
    unittest.main()
