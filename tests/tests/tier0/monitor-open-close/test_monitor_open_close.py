# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.test import BlueChiTest
from bluechi_test.machine import BlueChiControllerMachine, BlueChiAgentMachine
from bluechi_test.config import BlueChiControllerConfig, BlueChiAgentConfig


def exec(ctrl: BlueChiControllerMachine, nodes: Dict[str, BlueChiAgentMachine]):
    result, output = ctrl.run_python(os.path.join("python", "monitor.py"))

    if result != 0:
        raise Exception(output)


def test_monitor_open_close(
        bluechi_test: BlueChiTest,
        bluechi_ctrl_default_config: BlueChiControllerConfig,
        bluechi_node_default_config: BlueChiAgentConfig):

    bluechi_node_default_config.node_name = "node-foo"
    bluechi_ctrl_default_config.allowed_node_names = [bluechi_node_default_config.node_name]

    bluechi_test.set_bluechi_ctrl_machine_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_machine_configs(bluechi_node_default_config)

    bluechi_test.run(exec)
