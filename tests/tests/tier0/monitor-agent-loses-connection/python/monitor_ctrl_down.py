#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.error import DBusError
from dasbus.loop import EventLoop
from dasbus.typing import Variant

from bluechi.api import Agent, Node


class TestMonitorCtrlDown(unittest.TestCase):
    def setUp(self) -> None:
        super().setUp()

        self.agent_state = None
        self.agent_disconnected_timestamp = 0
        self.agent_on_node = Agent()

    def test_monitor_ctrl_down(self):
        loop = EventLoop()

        def on_state_change(state: Variant):
            self.agent_state = state.get_string()
            self.agent_disconnected_timestamp = self.agent_on_node.disconnect_timestamp
            loop.quit()

        self.agent_on_node.on_status_changed(on_state_change)

        # stop the bluechi controller service to trigger a disconnected message in the agent
        # hacky solution to cause a disconnect:
        #   stop the controller service
        # problem:
        #   dasbus will raise a DBusError for connection timeout on v1.4, v1.7 doesn't
        #   the command will be executed anyway, resulting in the desired shutdown of the node
        #   therefore, the DBusError is silenced for now, but all other errors are still raised
        try:
            node = Node("node-foo")
            node.stop_unit("bluechi-controller.service", "replace")
        except DBusError:
            pass

        loop.run()

        assert self.agent_state == "offline"
        assert self.agent_disconnected_timestamp != 0


if __name__ == "__main__":
    unittest.main()
