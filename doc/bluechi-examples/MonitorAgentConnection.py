# SPDX-License-Identifier: MIT-0

from bluechi.api import Agent
from dasbus.loop import EventLoop
from dasbus.typing import Variant


def on_connection_status_changed(status: Variant):
    con_status = status.get_string()
    print(con_status)


loop = EventLoop()

agent = Agent()
agent.on_status_changed(on_connection_status_changed)

loop.run()
