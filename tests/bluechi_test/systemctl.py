# SPDX-License-Identifier: LGPL-2.1-or-later

import logging

from typing import Any, Iterator, Optional, Tuple, Union

from bluechi_test.client import Client

LOGGER = logging.getLogger(__name__)


class SystemCtl():

    binary_name = "systemctl"

    def __init__(self, client: Client) -> None:
        self.client = client

    def _do_operation_on_unit(self, unit_name: str, operation: str, check_result: bool, expected_result: int) \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:

        LOGGER.debug(f"{operation} unit '{unit_name}'")
        result, output = self.client.exec_run(f"{SystemCtl.binary_name} {operation} {unit_name}")
        LOGGER.debug(f"{operation} unit '{unit_name}' finished with result '{result}' and output:\n{output}")
        if check_result and result != expected_result:
            raise Exception(
                f"Failed to {operation} service {unit_name} with {result} (expected {expected_result}): {output}")
        return result, output

    def start_unit(self, unit_name: str, check_result: bool = True, expected_result: int = 0)  \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._do_operation_on_unit(unit_name, "start", check_result, expected_result)

    def stop_unit(self, unit_name: str, check_result: bool = True, expected_result: int = 0)  \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._do_operation_on_unit(unit_name, "stop", check_result, expected_result)

    def restart_unit(self, unit_name: str, check_result: bool = True, expected_result: int = 0)  \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._do_operation_on_unit(unit_name, "restart", check_result, expected_result)

    def reset_failed_for_unit(self, unit_name: str, check_result: bool = True, expected_result: int = 0)  \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._do_operation_on_unit(unit_name, "reset-failed", check_result, expected_result)

    def daemon_reload(self)  \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self.client.exec_run(f"{SystemCtl.binary_name} daemon-reload")

    def get_unit_state(self, unit_name: str) -> str:
        _, output = self.client.exec_run(f"{SystemCtl.binary_name} is-active {unit_name}")
        return output

    def is_unit_in_state(self, unit_name: str, expected_state: str) -> bool:
        latest_state = self.get_unit_state(unit_name)
        LOGGER.info(f"Got state '{latest_state}' for unit {unit_name}")
        return latest_state == expected_state

    def is_unit_enabled(self, unit_name: str, check_result: bool = True, expected_result: int = 0) -> bool:
        _, output = self._do_operation_on_unit(unit_name, "is-enabled", check_result, expected_result)
        return output == "enabled"

    def is_unit_disabled(self, unit_name: str, check_result: bool = True, expected_result: int = 0) -> bool:
        _, output = self._do_operation_on_unit(unit_name, "is-enabled", check_result, expected_result)
        return output == "disabled"

    def service_is_active(self, unit_name: str) -> bool:
        result, _ = self.client.exec_run(f"{SystemCtl.binary_name} is-active {unit_name}")
        return result == 0

    def get_unit_freezer_state(self, unit_name: str) -> str:
        _, output = self.client.exec_run(f"{SystemCtl.binary_name} show {unit_name} --property=FreezerState")
        state = str(output).split('=', 1)
        result = ""
        if len(state) > 1:
            result = state[1]
        return result
