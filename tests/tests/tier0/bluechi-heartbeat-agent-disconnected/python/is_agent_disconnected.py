#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import time
import unittest

from dasbus.loop import EventLoop
from dasbus.typing import Variant

from bluechi.api import Agent


class TestAgentIsDisconnected(unittest.TestCase):

    state = None
    timestamp = None

    def test_agent_is_disconnected(self):
        loop = EventLoop()

        def on_state_change(state: Variant):
            self.timestamp = round(time.time() * 1000000)
            self.state = state.get_string()
            loop.quit()

        agent = Agent()
        assert agent.status == "online"

        agent.on_status_changed(on_state_change)
        timestamp = agent.last_seen_timestamp

        loop.run()

        # verify that the agent is disconnected after more than
        # ControllerHeartbeatThreshold seconds have elapsed
        assert self.timestamp - timestamp > 6000000
        assert self.state == "offline"


if __name__ == "__main__":
    unittest.main()
