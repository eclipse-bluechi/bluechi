#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

from dasbus.loop import EventLoop
from dasbus.typing import Variant

from bluechi.api import Agent


class TestSwitchController(unittest.TestCase):
    def setUp(self) -> None:
        super().setUp()

        self.controller_address = None

    def test_switch_controller(self):
        loop = EventLoop()

        def on_controller_address_change(controller_address: Variant):
            self.controller_address = controller_address.get_string()
            loop.quit()

        agent = Agent()
        agent.on_controller_address_changed(on_controller_address_change)
        agent.switch_controller("tcp:host=127.0.0.1,port=8420")

        loop.run()

        assert agent.controller_address == "tcp:host=127.0.0.1,port=8420"
        assert self.controller_address == "tcp:host=127.0.0.1,port=8420"


if __name__ == "__main__":
    unittest.main()
