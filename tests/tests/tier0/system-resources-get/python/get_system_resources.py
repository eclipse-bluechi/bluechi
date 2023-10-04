# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from bluechi.api import Node


class TestGetSystemResources(unittest.TestCase):

    def test_get_system_resources(self):
        node_foo = Node("node-foo")
        system_resources = node_foo.get_system_resources()

        assert len(system_resources) == 4
        assert system_resources[0] > 0
        assert system_resources[1] > 0
        assert system_resources[2] > 0
        assert system_resources[3] > 0


if __name__ == "__main__":
    unittest.main()
