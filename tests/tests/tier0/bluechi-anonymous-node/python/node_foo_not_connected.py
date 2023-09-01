# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from bluechi.api import Manager


class TestNodeFooNotConnected(unittest.TestCase):

    def test_node_foo_not_connected(self):
        mgr = Manager()

        nodes = mgr.list_nodes()
        assert len(nodes) == 1
        assert nodes[0][0] == "everyone-but-foo"
        assert nodes[0][2] == "offline"


if __name__ == "__main__":
    unittest.main()
