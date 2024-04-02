#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

from dasbus.loop import EventLoop
from dasbus.typing import Variant

from bluechi.api import Agent


def on_connection_status_changed(status: Variant):
    con_status = status.get_string()
    print(con_status)


loop = EventLoop()

agent = Agent()
agent.on_status_changed(on_connection_status_changed)

loop.run()
