#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
from enum import Enum
from typing import Any, Dict, Iterator, List, Optional, Tuple, Union

from bluechi_test.client import Client

LOGGER = logging.getLogger(__name__)


class LogLevel(str, Enum):
    DEBUG = "DEBUG"
    INFO = "INFO"
    WARN = "WARN"
    ERROR = "ERROR"


class BluechiCtl:

    binary_name = "bluechictl"

    def __init__(self, client: Client) -> None:
        self.client = client

        self.tracked_services: Dict[str, List[str]] = dict()

    def _run(
        self, log_txt: str, cmd: str, check_result: bool, expected_result: int
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        LOGGER.debug(log_txt)
        result, output = self.client.exec_run(f"{BluechiCtl.binary_name} {cmd}")
        if check_result and result != expected_result:
            raise Exception(
                f"{log_txt} failed with {result} (expected {expected_result}): {output}"
            )
        return result, output

    def list_units(
        self, node_name: str = None, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        if node_name:
            return self._run(
                f"Listing units on node {node_name}",
                f"list-units {node_name}",
                check_result,
                expected_result,
            )

        return self._run(
            "Listing units on all nodes", "list-units", check_result, expected_result
        )

    def list_unit_files(
        self, node_name: str = None, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        if node_name:
            return self._run(
                f"Listing unit files on node {node_name}",
                f"list-unit-files {node_name}",
                check_result,
                expected_result,
            )

        return self._run(
            "Listing unit files on all nodes",
            "list-unit-files",
            check_result,
            expected_result,
        )

    def is_enabled(
        self,
        node_name: str,
        unit_name: str,
        check_result: bool = False,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        # track started units to stop and reset failures on cleanup
        if node_name not in self.tracked_services:
            self.tracked_services[node_name] = []
        self.tracked_services[node_name].append(unit_name)

        return self._run(
            f"Fetching enablement status of unit '{unit_name}' on node '{node_name}'",
            f"is-enabled {node_name} {unit_name}",
            check_result,
            expected_result,
        )

    def get_unit_status(
        self,
        node_name: str,
        unit_name: str,
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Getting status of unit '{unit_name}' on node '{node_name}'",
            f"status {node_name} {unit_name}",
            check_result,
            expected_result,
        )

    def get_node_status(
        self, node_name: str = None, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        if node_name:
            return self._run(
                f"Getting status of node '{node_name}'",
                f"status {node_name}",
                check_result,
                expected_result,
            )

        return self._run(
            "Getting status of all nodes", "status", check_result, expected_result
        )

    def start_unit(
        self,
        node_name: str,
        unit_name: str,
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        # track started units to stop and reset failures on cleanup
        if node_name not in self.tracked_services:
            self.tracked_services[node_name] = []
        self.tracked_services[node_name].append(unit_name)

        return self._run(
            f"Starting unit '{unit_name}' on node '{node_name}'",
            f"start {node_name} {unit_name}",
            check_result,
            expected_result,
        )

    def reload_unit(
        self,
        node_name: str,
        unit_name: str,
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Reloading unit {unit_name} on node {node_name}",
            f"reload {node_name} {unit_name}",
            check_result,
            expected_result,
        )

    def reset_failed(
        self,
        node_name: str = "",
        units: List[str] = [],
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        cmd = f"reset-failed {node_name} {' '.join(units)}".strip()
        return self._run(
            f"ResetFailed on node {node_name} for units {units}",
            cmd,
            check_result,
            expected_result,
        )

    def get_default_target(
        self,
        node_name: str = "",
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        cmd = f"get-default {node_name}"
        return self._run(
            f"GetDefaultTarget of node {node_name}",
            cmd,
            check_result,
            expected_result,
        )

    def set_default_target(
        self,
        node_name: str = "",
        target: str = "",
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        cmd = f"set-default {node_name} {target}"
        return self._run(
            f"SetDefaultTarget of node {node_name} ",
            cmd,
            check_result,
            expected_result,
        )

    def kill_unit(
        self,
        node_name: str,
        unit_name: str,
        whom: str = "",
        signal: str = "",
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        cmd = f"kill {node_name} {unit_name}"
        if whom != "":
            cmd = cmd + f" --kill-whom={whom}"
        if signal != "":
            cmd = cmd + f" --signal={signal}"

        return self._run(
            f"Killing unit {unit_name} on node {node_name} with whom='{whom}' and signal='{signal}'",
            cmd,
            check_result,
            expected_result,
        )

    def stop_unit(
        self,
        node_name: str,
        unit_name: str,
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Stopping unit '{unit_name}' on node '{node_name}'",
            f"stop {node_name} {unit_name}",
            check_result,
            expected_result,
        )

    def daemon_reload_node(
        self, node_name: str, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Daemon Reload on node '{node_name}'",
            f"daemon-reload {node_name}",
            check_result,
            expected_result,
        )

    def enable_unit(
        self,
        node_name: str,
        unit_name: str,
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Enabling unit '{unit_name}' on node '{node_name}'",
            f"enable {node_name} {unit_name}",
            check_result,
            expected_result,
        )

    def disable_unit(
        self,
        node_name: str,
        unit_name: str,
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Disabling unit '{unit_name}' on node '{node_name}'",
            f"disable {node_name} {unit_name}",
            check_result,
            expected_result,
        )

    def freeze_unit(
        self,
        node_name: str,
        unit_name: str,
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Freezing unit {unit_name} on node {node_name}",
            f"freeze {node_name} {unit_name}",
            check_result,
            expected_result,
        )

    def thaw_unit(
        self,
        node_name: str,
        unit_name: str,
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Thawing unit {unit_name} on node {node_name}",
            f"thaw {node_name} {unit_name}",
            check_result,
            expected_result,
        )

    def set_log_level_on_node(
        self,
        node_name: str,
        log_level: str,
        check_result: bool = True,
        expected_result: int = 0,
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Setting log level on node {node_name}",
            f"set-loglevel {node_name} {log_level}",
            check_result,
            expected_result,
        )

    def set_log_level_on_controller(
        self, log_level: str, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            f"Setting log level '{log_level}' on controller",
            f"set-loglevel {log_level}",
            check_result,
            expected_result,
        )

    def enable_metrics(
        self, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            "Enabling metrics", "metrics enable", check_result, expected_result
        )

    def disable_metrics(
        self, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run(
            "Disabling metrics", "metrics disable", check_result, expected_result
        )

    def version(
        self, check_result: bool = True, expected_result: int = 0
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        return self._run("Getting version", "version", check_result, expected_result)
