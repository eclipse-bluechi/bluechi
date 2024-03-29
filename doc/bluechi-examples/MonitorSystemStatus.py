# SPDX-License-Identifier: MIT-0

from bluechi.api import Controller
from dasbus.loop import EventLoop
from dasbus.typing import Variant


def on_system_status_changed(status: Variant):
    con_status = status.get_string()
    print(con_status)


loop = EventLoop()

mgr = Controller()
mgr.on_status_changed(on_system_status_changed)

loop.run()
