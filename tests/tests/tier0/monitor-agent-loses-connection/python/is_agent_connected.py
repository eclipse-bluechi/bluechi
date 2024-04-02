#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from bluechi.api import Agent

# Test to verify that the agent on the node is connected to the controller
# and the properties for status and disconnected timestamp are set


class TestAgentIsConnected(unittest.TestCase):
    def test_agent_is_connected(self):
        agent = Agent()
        assert agent.status == "online"
        assert agent.disconnect_timestamp == 0


if __name__ == "__main__":
    unittest.main()
