# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.test import BlueChiTest
from bluechi_test.machine import BlueChiControllerMachine, BlueChiAgentMachine
from bluechi_test.config import BlueChiControllerConfig, BlueChiAgentConfig

node_foo_name = "node-foo"


def exec(ctrl: BlueChiControllerMachine, nodes: Dict[str, BlueChiAgentMachine]):
    # bluechi-agent is running, check if it is connected in Agent
    result, output = ctrl.run_python(os.path.join("python", "is_agent_connected.py"))
    if result != 0:
        raise Exception(output)

    # stop bluechi service and verify that the agent emits a status changed signal
    result, output = ctrl.run_python(os.path.join("python", "monitor_ctrl_down.py"))
    if result != 0:
        raise Exception(output)


def test_monitor_agent_loses_connection(
        bluechi_test: BlueChiTest,
        bluechi_ctrl_default_config: BlueChiControllerConfig):

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name]
    node_foo_config = BlueChiAgentConfig(
        file_name="agent.conf",
        node_name=node_foo_name,
        controller_host="localhost",
        controller_port=bluechi_ctrl_default_config.port,
    )

    bluechi_test.set_bluechi_ctrl_machine_config(bluechi_ctrl_default_config, node_foo_config)
    # don't add node_foo_config to bluechi_test to prevent it being started
    # as separate container
    bluechi_test.run(exec)
