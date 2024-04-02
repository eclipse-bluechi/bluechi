#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from bluechi.api import Node, Variant


class TestSetProperty(unittest.TestCase):
    def test_set_property(self):
        node_foo = Node("node-foo")

        unit_description = node_foo.get_unit_property(
            "simple.service", "org.freedesktop.systemd1.Unit", "Description"
        )
        assert unit_description.get_string() == "Just being true once"

        v = Variant("s", "I changed")
        node_foo.set_unit_properties("simple.service", False, [("Description", v)])
        new_unit_description = node_foo.get_unit_property(
            "simple.service", "org.freedesktop.systemd1.Unit", "Description"
        )

        assert new_unit_description.get_string() == "I changed"


if __name__ == "__main__":
    unittest.main()
