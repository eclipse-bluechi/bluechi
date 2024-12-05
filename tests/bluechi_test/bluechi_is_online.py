#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Any, Dict, Iterator, List, Optional, Tuple, Union

from bluechi_test.client import Client


class BluechiIsOnline:

    binary_name = "bluechi-is-online"

    def __init__(self, client: Client) -> None:
        self.client = client
        self.tracked_services: Dict[str, List[str]] = dict()

    def run(
        self, log_txt: str, cmd: str, check_result: bool, expected_result: int
    ) -> Tuple[Optional[int], Union[Iterator[bytes], Any, Tuple[bytes, bytes]]]:
        result, output = self.client.exec_run(f"{BluechiIsOnline.binary_name} {cmd}")
        if check_result and result != expected_result:
            raise Exception(
                f"{log_txt} failed with {result} (expected {expected_result}): {output}"
            )
        return result, output

    def agent_is_online(self) -> bool:
        result, output = self.run(
            "Checking if agent is active.",
            "agent",
            False,
            0,
        )
        return result == 0

    def node_is_online(self, node_name: str) -> bool:
        result, output = self.run(
            "Checking controller status.",
            f"node {node_name}",
            False,
            0,
        )
        return result == 0
