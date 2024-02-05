# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.test import BlueChiTest
from bluechi_test.machine import BlueChiControllerMachine, BlueChiAgentMachine
from bluechi_test.config import BlueChiControllerConfig, BlueChiAgentConfig

node_foo_name = "node-foo"


def exec(ctrl: BlueChiControllerMachine, nodes: Dict[str, BlueChiAgentMachine]):

    node_foo = nodes[node_foo_name]

    assert ctrl.wait_for_unit_state_to_be("bluechi-controller", "active")
    assert node_foo.wait_for_unit_state_to_be("bluechi-agent", "active")

    # bluechi and bluechi-agent are running, check that agent is not connected
    result, output = ctrl.run_python(os.path.join("python", "node_foo_not_connected.py"))
    if result != 0:
        raise Exception(output)


def test_anonymous_node(
        bluechi_test: BlueChiTest,
        bluechi_ctrl_default_config: BlueChiControllerConfig,
        bluechi_node_default_config: BlueChiAgentConfig):

    node_foo_config = bluechi_node_default_config.deep_copy()
    node_foo_config.node_name = node_foo_name

    bluechi_ctrl_default_config.allowed_node_names = ["everyone-but-foo"]

    bluechi_test.set_bluechi_ctrl_machine_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_machine_configs(node_foo_config)

    bluechi_test.run(exec)
