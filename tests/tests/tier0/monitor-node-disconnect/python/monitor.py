# SPDX-License-Identifier: GPL-2.0-or-later

import unittest

from dasbus.error import DBusError
from dasbus.loop import EventLoop

from bluechi.api import Manager, Node


class TestNodeDisconnect(unittest.TestCase):

    node_state = None

    def test_node_disconnect(self):
        loop = EventLoop()

        def on_state_change(_: str, state: str):
            self.node_state = state
            loop.quit()

        mgr = Manager()
        mgr.on_node_connection_state_changed(on_state_change)

        nodes = mgr.list_nodes()
        if len(nodes) < 1:
            raise Exception("No connected node found")

        # get name of first node
        node_name = nodes[0][0]
        node = Node(node_name)

        # stop the bluechi-agent service to trigger a disconnected message
        # hacky solution to cause a disconnect:
        #   stop the agent service
        # problem:
        #   dasbus will raise a DBusError for connection timeout on v1.4, v1.7 doesn't
        #   the command will be executed anyway, resulting in the desired shutdown of the node
        #   therefore, the DBusError is silenced for now, but all other errors are still raised
        try:
            node.stop_unit('bluechi-agent.service', 'replace')
        except DBusError:
            pass

        loop.run()

        assert self.node_state == "offline"


if __name__ == "__main__":
    unittest.main()
