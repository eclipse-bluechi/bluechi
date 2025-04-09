#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import unittest

from bluechi.api import Node

NODE_CTRL_NAME = os.getenv("NODE_CTRL_NAME", "node-ctrl")


class TestNodeIsConnected(unittest.TestCase):
    def test_node_is_connected(self):
        n = Node(NODE_CTRL_NAME)
        assert n.status == "online"
        assert n.name == NODE_CTRL_NAME


if __name__ == "__main__":
    unittest.main()
