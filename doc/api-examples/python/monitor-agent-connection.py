#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: MIT-0

from typing import Dict

import dasbus.connection
from dasbus.loop import EventLoop
from dasbus.typing import Variant

loop = EventLoop()
bus = dasbus.connection.SystemMessageBus()

agent_props_proxy = bus.get_proxy(
    "org.eclipse.bluechi.Agent",
    "/org/eclipse/bluechi",
    "org.freedesktop.DBus.Properties",
)


def on_connection_status_changed(
    interface: str,
    changed_props: Dict[str, Variant],
    invalidated_props: Dict[str, Variant],
) -> None:
    con_status = changed_props["Status"].get_string()
    print(f"Agent status: {con_status}")


agent_props_proxy.PropertiesChanged.connect(on_connection_status_changed)

loop.run()
