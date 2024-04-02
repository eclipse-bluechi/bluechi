#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from bluechi.api import Node


class TestNodeHasPeerIP(unittest.TestCase):
    def test_node_has_peer_ip(self):
        n = Node("node-foo")
        assert n.peer_ip != ""


if __name__ == "__main__":
    unittest.main()
