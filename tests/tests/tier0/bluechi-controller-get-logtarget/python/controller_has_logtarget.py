#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from bluechi.api import Controller

expected_logtarget = "stderr-full"


class TestHasLogTarget(unittest.TestCase):
    def test_controller_has_logtarget(self):
        controller = Controller()
        assert controller.log_target == expected_logtarget


if __name__ == "__main__":
    unittest.main()
