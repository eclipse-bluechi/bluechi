# SPDX-License-Identifier: LGPL-2.1-or-later

import logging

from typing import Any, Iterator, Optional, Tuple, Union

from bluechi_test.client import Client

LOGGER = logging.getLogger(__name__)


class SystemCtl():

    binary_name = "systemctl"

    def __init__(self, client: Client) -> None:
        self.client = client

    def _do_operation_on_unit(self, unit_name: str, operation: str) \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:

        LOGGER.debug(f"{operation} unit '{unit_name}'")
        result, output = self.client.exec_run(f"{SystemCtl.binary_name} {operation} {unit_name}")
        LOGGER.debug(f"{operation} unit '{unit_name}' finished with result '{result}' \
            and output:\n{output}")
        if result != 0:
            raise Exception(f"Failed to {operation} service {unit_name}: {output}")
        return result, output

    def start_unit(self, unit_name: str) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._do_operation_on_unit(unit_name, "start")

    def stop_unit(self, unit_name: str) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._do_operation_on_unit(unit_name, "stop")

    def restart_unit(self, unit_name: str) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._do_operation_on_unit(unit_name, "restart")

    def daemon_reload(self) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self.client.exec_run(f"{SystemCtl.binary_name} daemon-reload")

    def get_unit_state(self, unit_name: str) -> str:
        _, output = self.client.exec_run(f"{SystemCtl.binary_name} is-active {unit_name}")
        return output

    def is_unit_in_state(self, unit_name: str, expected_state: str) -> bool:
        latest_state = self.get_unit_state(unit_name)
        LOGGER.info(f"Got state '{latest_state}' for unit {unit_name}")
        return latest_state == expected_state

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
