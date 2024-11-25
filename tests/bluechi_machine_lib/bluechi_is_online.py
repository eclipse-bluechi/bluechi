#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest
import subprocess
from typing import Tuple


class BluechiIsOnline:
    """
    A Python module for the 'bluechi-is-online' command to check the status of agents, nodes, and the system.
    """

    def __init__(self, executable_path: str = "/usr/bin/bluechi-is-online"):
        self.executable_path = executable_path

    def _run_command(self, args: list) -> Tuple[int, str, str]:
        command = [self.executable_path] + args
        try:
            process = subprocess.Popen(
                command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
            )
            print(f"Executing command: {process.args}")
            stdout, stderr = process.communicate()

            print(
                f"Command execution completed: '{process.args}', "
                f"return code: {process.returncode}, stdout: '{stdout.strip()}', stderr: '{stderr.strip()}'"
            )

            return process.returncode, stdout.strip(), stderr.strip()

        except Exception as e:
            print(f"Error while executing command '{command}': {e}")
            raise


class TestBluechiIsOnline(unittest.TestCase):
    def setUp(self):
        self.bluechi = BluechiIsOnline()

    def test_agent_is_online(self):
        output = self.bluechi._run_command(["agent"])
        assert output == "online", "Agent is offline"

    def test_node_is_online(self):
        output = self.bluechi._run_command(["node", "node-foo"])
        assert output == "online", "Node 'node-foo' is offline"

    def test_system_is_online(self):
        output = self.bluechi._run_command(["system"])
        assert output == "online", "System is offline"


if __name__ == "__main__":
    unittest.main()
