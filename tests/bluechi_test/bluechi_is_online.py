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

    def agent_is_online(self, wait: int = None) -> bool:
        cmd = ["agent"]

        if wait:
            cmd.extend(["--wait", str(wait)])

        result, output = self.run(
            "Checking if agent is active.",
            " ".join(cmd),
            False,
            0,
        )
        return result == 0

    def node_is_online(self, node_name: str, wait: int = None) -> bool:
        cmd = ["node", node_name]

        if wait:
            cmd.extend(["--wait", str(wait)])

        result, output = self.run(
            "Checking controller status.",
            " ".join(cmd),
            False,
            0,
        )
        return result == 0

    def system_is_online(self, wait: int = None) -> bool:
        cmd = ["system"]

        if wait:
            cmd.extend(["--wait", str(wait)])

        result, output = self.run(
            "Checking system status.",
            " ".join(cmd),
            False,
            0,
        )
        return result == 0

    def monitor_node(self, node_name: str) -> Tuple[bool, str]:
        result, output = self.run(
            f"Monitoring status of node {node_name}.",
            f"node {node_name} --monitor",
            False,
            0,
        )
        return result == 0, output

    def monitor_agent(self) -> Tuple[bool, str]:
        result, output = self.run(
            "Monitoring status of the agent.",
            "agent --monitor",
            False,
            0,
        )
        return result == 0, output

    def monitor_system(self) -> Tuple[bool, str]:
        result, output = self.run(
            "Monitoring status of the system.",
            "system --monitor",
            False,
            0,
        )
        return result == 0, output
