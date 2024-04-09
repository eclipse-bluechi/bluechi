#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes["node-foo"]

    result, output = node_foo.run_python(os.path.join("python", "switch_controller.py"))
    if result != 0:
        raise Exception(output)


def test_bluechi_agent_switch_controller(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):

    bluechi_node_default_config.node_name = "node-foo"
    bluechi_ctrl_default_config.allowed_node_names = [
        bluechi_node_default_config.node_name
    ]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(bluechi_node_default_config)

    bluechi_test.run(exec)
