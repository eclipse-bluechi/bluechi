#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
from typing import Any, Iterator, List, Optional, Set, Tuple, Union

from bluechi_test.client import Client

LOGGER = logging.getLogger(__name__)


class SystemCtl:

    binary_name = "systemctl"

    def __init__(self, client: Client) -> None:
        self.client = client

        self.tracked_services: List[str] = []

    def _do_operation_on_unit(
        self, unit_name: str, operation: str, check_result: bool, expected_result: int
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:

        result, output = self.client.exec_run(
            f"{SystemCtl.binary_name} {operation} {unit_name}"
        )
        if check_result and result != expected_result:
            raise Exception(
                f"Failed to {operation} service {unit_name} with {result} (expected {expected_result}): {output}"
            )
        return result, output

    def _do_operation(
        self, operation: str, check_result: bool, expected_result: int
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:

        result, output = self.client.exec_run(f"{SystemCtl.binary_name} {operation}")
        if check_result and result != expected_result:
            raise Exception(
                f"Failed to execute {operation} with {result} (expected {expected_result}): {output}"
            )
        return result, output

    def start_unit(
        self, unit_name: str, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        # track started units to stop and reset failures on cleanup
        self.tracked_services.append(unit_name)

        return self._do_operation_on_unit(
            unit_name, "start", check_result, expected_result
        )

    def stop_unit(
        self,
        unit_name: str,
        check_result: bool = True,
        expected_result: int = 0,
        operation_results: Set[str] = ("success", "exit-code"),
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        result, output = self._do_operation_on_unit(
            unit_name, "stop", check_result, expected_result
        )

        if check_result:
            # It's not enough to check result for systemctl stop, it will return failure only if stopping unit is
            # prohibited or unit doesn't exist, so we need to check also operation result
            op_res, op_out = self._do_operation_on_unit(
                unit_name, 'show --property="Result"', check_result, 0
            )
            if op_res != 0:
                raise Exception(
                    f"Failed to fetch stop operation result for service {unit_name} with {op_res}: {op_out}"
                )
            op_res_value = op_out.replace("Result=", "")
            if op_res_value not in operation_results:
                raise Exception(
                    f"Operation stop on service {unit_name} finished with result {op_res_value}"
                    f" (expected {operation_results})"
                )

        return result, output

    def restart_unit(
        self, unit_name: str, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        # track started units to stop and reset failures on cleanup
        self.tracked_services.append(unit_name)

        return self._do_operation_on_unit(
            unit_name, "restart", check_result, expected_result
        )

    def set_default_target(
        self, target: str, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        command = f"set-default {target}"
        return self._do_operation(command, check_result, expected_result)

    def get_default_target(
        self, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        command = "get-default"
        return self._do_operation(command, check_result, expected_result)

    def reset_failed_for_unit(
        self, unit_name: str, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._do_operation_on_unit(
            unit_name, "reset-failed", check_result, expected_result
        )

    def daemon_reload(
        self,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self.client.exec_run(f"{SystemCtl.binary_name} daemon-reload")

    def get_unit_state(self, unit_name: str) -> str:
        _, output = self.client.exec_run(
            f"{SystemCtl.binary_name} is-active {unit_name}"
        )
        return output

    def is_unit_in_state(self, unit_name: str, expected_state: str) -> bool:
        latest_state = self.get_unit_state(unit_name)
        LOGGER.info(f"Got state '{latest_state}' for unit {unit_name}")
        return latest_state == expected_state

    def enable_unit(
        self, unit_name: str, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        self.tracked_services.append(unit_name)

        return self._do_operation_on_unit(
            unit_name, "enable", check_result, expected_result
        )

    def disable_unit(
        self, unit_name: str, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        self.tracked_services.append(unit_name)

        return self._do_operation_on_unit(
            unit_name, "disable", check_result, expected_result
        )

    def is_enabled(
        self, unit_name: str, check_result: bool = False, expected_result: int = 0
    ) -> str:
        return self._do_operation_on_unit(
            unit_name, "is-enabled", check_result, expected_result
        )

    def is_unit_enabled(
        self, unit_name: str, check_result: bool = True, expected_result: int = 0
    ) -> bool:
        _, out = self.is_enabled(unit_name=unit_name)
        return "enabled" == out

    def is_unit_disabled(
        self, unit_name: str, check_result: bool = True, expected_result: int = 0
    ) -> bool:
        _, out = self.is_enabled(unit_name=unit_name)
        return "disabled" == out

    def service_is_active(self, unit_name: str) -> bool:
        result, _ = self.client.exec_run(
            f"{SystemCtl.binary_name} is-active {unit_name}"
        )
        return result == 0

    def get_unit_freezer_state(self, unit_name: str) -> str:
        _, output = self.client.exec_run(
            f"{SystemCtl.binary_name} show {unit_name} --property=FreezerState"
        )
        state = str(output).split("=", 1)
        result = ""
        if len(state) > 1:
            result = state[1]
        return result

    def list_units(
        self,
        all_units: bool = False,
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        command = "list-units --legend=false --plain{}".format(
            " --all" if all_units else ""
        )
        return self._do_operation(command, check_result, expected_result)

    def list_unit_files(
        self,
        all_unit_files: bool = False,
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        command = "list-unit-files --legend=false{}".format(
            " -all" if all_unit_files else ""
        )
        return self._do_operation(command, check_result, expected_result)
