#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from bluechi.api import Node


class TestGetProperties(unittest.TestCase):
    def test_get_properties(self):
        node_foo = Node("node-foo")
        props = node_foo.get_unit_properties(
            "bluechi-agent.service",
            "org.freedesktop.systemd1.Service",
        )

        assert len(props) > 0
        # check some properties as example
        assert props["Type"].get_string() == "simple"
        assert not props["RemainAfterExit"].get_boolean()


if __name__ == "__main__":
    unittest.main()
