# SPDX-License-Identifier: LGPL-3.0-or-later

from dasbus.connection import MessageBus
from typing import Callable, List, NamedTuple
from collections import namedtuple
from dasbus.loop import EventLoop
from dasbus.typing import UInt32, ObjPath

from pyhirte.api import Node, Manager


UnitInfo = namedtuple(
    "UnitInfo",
    [
        "name",
        "description",
        "load_state",
        "active_state",
        "sub_state",
        "follower",
        "object_path",
        "job_id",
        "job_type",
        "job_object_path",
    ],
)


UnitChange = namedtuple(
    "UnitChange",
    [
        "change_type",
        "symlink_file",
        "symlink_destination",
    ],
)


class EnableUnitsResponse(NamedTuple):
    carries_install_info: bool
    changes: List[UnitChange]


class Unit:
    def __init__(
        self, node_name: str, bus: MessageBus = None, use_systembus=True
    ) -> None:
        self.node = Node(node_name, bus, use_systembus)
        self.job_result = ""

    def _wait_for_complete(
        self, operation: Callable[[str, str], ObjPath], unit: str
    ) -> str:
        event_loop = EventLoop()

        def on_job_removed(
            _: UInt32, job_path: ObjPath, node_name: str, unit_name: str, result: str
        ) -> None:
            if job_path == wait_for_job_path:
                self.job_result = result
                event_loop.quit()

        Manager(bus=self.node.bus).on_job_removed(on_job_removed)

        wait_for_job_path = operation(unit, "replace")
        event_loop.run()

        job_result = self.job_result
        self.job_result = ""
        return job_result

    def start_unit(self, unit: str) -> str:
        return self._wait_for_complete(self.node.start_unit, unit)

    def stop_unit(self, unit: str) -> str:
        return self._wait_for_complete(self.node.stop_unit, unit)

    def restart_unit(self, unit: str) -> str:
        return self._wait_for_complete(self.node.restart_unit, unit)

    def reload_unit(self, unit: str) -> str:
        return self._wait_for_complete(self.node.reload_unit, unit)

    def enable_unit_files(self, files: List[str]) -> EnableUnitsResponse:
        return self.node.enable_unit_files(files, False, False)

    def disable_unit_files(self, files: List[str]) -> List[UnitChange]:
        return self.node.disable_unit_files(files, False)
