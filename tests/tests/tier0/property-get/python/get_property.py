#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from bluechi.api import Node


class TestGetProperty(unittest.TestCase):
    def test_get_property(self):
        node_foo = Node("node-foo")
        cpu_weight = node_foo.get_unit_property(
            "bluechi-agent.service", "org.freedesktop.systemd1.Service", "CPUWeight"
        )

        assert cpu_weight.get_uint64() > 0


if __name__ == "__main__":
    unittest.main()
