# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.test import BlueChiTest
from bluechi_test.machine import BlueChiControllerMachine, BlueChiAgentMachine
from bluechi_test.config import BlueChiControllerConfig, BlueChiAgentConfig

node_foo_name = "node-foo"


def exec(ctrl: BlueChiControllerMachine, nodes: Dict[str, BlueChiAgentMachine]):

    assert ctrl.wait_for_unit_state_to_be("bluechi-controller", "active")
    assert ctrl.wait_for_unit_state_to_be("bluechi-agent", "active")

    # bluechi-agent is running, check if it is connected
    result, output = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    if result != 0:
        raise Exception(output)


def test_bluechi_and_agent_on_same_machine(
        bluechi_test: BlueChiTest,
        bluechi_ctrl_default_config: BlueChiControllerConfig):

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name]
    node_foo_config = BlueChiAgentConfig(
        file_name="agent.conf",
        node_name=node_foo_name,
        controller_host="localhost",
        controller_port=bluechi_ctrl_default_config.port
    )

    bluechi_test.set_bluechi_ctrl_machine_config(bluechi_ctrl_default_config, node_foo_config)
    # don't add node_foo_config to bluechi_test to prevent it being started
    # as separate container
    bluechi_test.run(exec)
