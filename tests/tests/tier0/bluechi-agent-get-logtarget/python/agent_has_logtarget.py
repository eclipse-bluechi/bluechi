#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from bluechi.api import Agent

expected_logtarget = "stderr-full"


class TestHasLogTarget(unittest.TestCase):
    def test_agent_has_logtarget(self):
        agent = Agent()
        assert agent.log_target == expected_logtarget


if __name__ == "__main__":
    unittest.main()
