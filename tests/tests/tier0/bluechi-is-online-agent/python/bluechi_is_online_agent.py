#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest
from bluechi_machine_lib.bluechi_is_online import BluechiIsOnline


class TestAgentIsConnected(unittest.TestCase):
    def test_agent_is_connected(self):
        agent = BluechiIsOnline.TestBluechiIsOnline.test_agent_is_online()
        assert agent.status == "online"


if __name__ == "__main__":
    unittest.main()
