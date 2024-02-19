# SPDX-License-Identifier: LGPL-2.1-or-later

import logging

from enum import Enum
from typing import Tuple, Iterator, Any, Optional, Union

from bluechi_test.client import Client

LOGGER = logging.getLogger(__name__)


class LogLevel(str, Enum):
    DEBUG = "DEBUG"
    INFO = "INFO"
    WARN = "WARN"
    ERROR = "ERROR"


class BluechiCtl():

    binary_name = "bluechictl"

    def __init__(self, client: Client) -> None:
        self.client = client

    def _run(self, log_txt: str, cmd: str, check_result: bool, expected_result: int) \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        LOGGER.debug(log_txt)
        result, output = self.client.exec_run(f"{BluechiCtl.binary_name} {cmd}")
        LOGGER.debug(f"{log_txt} finished with result '{result}' and output:\n{output}")
        if check_result and result != expected_result:
            raise Exception(f"{log_txt} failed with {result} (expected {expected_result}): {output}")
        return result, output

    def list_units(self, node_name: str, check_result: bool = True, expected_result: int = 0) \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Listing units on node {node_name}",
            f"list-units {node_name}",
            check_result,
            expected_result
        )

    def get_status(self, node_name: str, unit_name: str, check_result: bool = True, expected_result: int = 0) \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Getting status of unit '{unit_name}' on node '{node_name}'",
            f"status {node_name} {unit_name}",
            check_result,
            expected_result
        )

    def start_unit(self, node_name: str, unit_name: str, check_result: bool = True, expected_result: int = 0) \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Starting unit '{unit_name}' on node '{node_name}'",
            f"start {node_name} {unit_name}",
            check_result,
            expected_result
        )

    def stop_unit(self, node_name: str, unit_name: str, check_result: bool = True, expected_result: int = 0) \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Stopping unit '{unit_name}' on node '{node_name}'",
            f"stop {node_name} {unit_name}",
            check_result,
            expected_result
        )

    def enable_unit(self, node_name: str, unit_name: str, check_result: bool = True, expected_result: int = 0) \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Enabling unit '{unit_name}' on node '{node_name}'",
            f"enable {node_name} {unit_name}",
            check_result,
            expected_result
        )

    def freeze_unit(self, node_name: str, unit_name: str, check_result: bool = True, expected_result: int = 0) \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Freezing unit {unit_name} on node {node_name}",
            f"freeze {node_name} {unit_name}",
            check_result,
            expected_result
        )

    def thaw_unit(self, node_name: str, unit_name: str, check_result: bool = True, expected_result: int = 0) \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Thawing unit {unit_name} on node {node_name}",
            f"thaw {node_name} {unit_name}",
            check_result,
            expected_result
        )

    def set_log_level_on_node(self,
                              node_name: str,
                              log_level: str,
                              check_result: bool = True,
                              expected_result: int = 0) \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Setting log level on node {node_name}",
            f"set-loglevel {node_name} {log_level}",
            check_result,
            expected_result
        )

    def set_log_level_on_controller(self,
                                    log_level: str,
                                    check_result: bool = True,
                                    expected_result: int = 0) \
            -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Setting log level '{log_level}' on controller",
            f"set-loglevel {log_level}",
            check_result,
            expected_result
        )
