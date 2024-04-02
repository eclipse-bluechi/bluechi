#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from bluechi.api import Node


class TestNodeIsConnected(unittest.TestCase):

    def test_node_is_connected(self):
        n = Node("node-foo")
        assert n.status == "online"


if __name__ == "__main__":
    unittest.main()
