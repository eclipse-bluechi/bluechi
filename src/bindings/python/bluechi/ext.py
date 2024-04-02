#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from dasbus.connection import MessageBus
from typing import Callable, List, NamedTuple, Tuple
from collections import namedtuple
from dasbus.loop import EventLoop
from dasbus.typing import UInt32, ObjPath

from bluechi.api import Node, Controller


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

def unit_changes_from_tuples(tuples: List[Tuple[str, str, str]]) -> List[UnitChange]:
    changes: List[UnitChange] = []
    for change in tuples:
        changes.append(UnitChange(change[0], change[1], change[2]))
    return changes

EnableUnitsResponse = NamedTuple("EnableUnits", carries_install_info=bool, changes=List[UnitChange])


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

        Controller(bus=self.node.bus).on_job_removed(on_job_removed)

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
        resp = self.node.enable_unit_files(files, False, False)
        return EnableUnitsResponse(resp[0], unit_changes_from_tuples(resp[1]))

    def disable_unit_files(self, files: List[str]) -> List[UnitChange]:
        return unit_changes_from_tuples(self.node.disable_unit_files(files, False))
