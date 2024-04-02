#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.error import DBusError
from dasbus.loop import EventLoop
from dasbus.typing import Variant

from bluechi.api import Controller, Node


class TestNodeDisconnect(unittest.TestCase):

    node_state = None

    def test_node_disconnect(self):
        loop = EventLoop()

        def on_state_change(state: Variant):
            self.node_state = state.get_string()
            loop.quit()

        mgr = Controller()
        nodes = mgr.list_nodes()
        if len(nodes) < 1:
            raise Exception("No connected node found")

        # get name of first node
        node_name = nodes[0][0]
        node = Node(node_name)

        node.on_status_changed(on_state_change)

        # stop the bluechi-agent service to trigger a disconnected message
        # hacky solution to cause a disconnect:
        #   stop the agent service
        # problem:
        #   dasbus will raise a DBusError for connection timeout on v1.4, v1.7 doesn't
        #   the command will be executed anyway, resulting in the desired shutdown of the node
        #   therefore, the DBusError is silenced for now, but all other errors are still raised
        try:
            node.stop_unit("bluechi-agent.service", "replace")
        except DBusError:
            pass

        loop.run()

        assert self.node_state == "offline"


if __name__ == "__main__":
    unittest.main()
