#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

node_foo_name = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    result, output = ctrl.run_python(os.path.join("python", "is_node_disconnected.py"))
    if result != 0:
        raise Exception(output)


def test_bluechi_heartbeat_node_disconnected(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name]
    bluechi_ctrl_default_config.heartbeat_interval = "2000"

    bluechi_node_default_config.node_name = node_foo_name
    bluechi_node_default_config.heartbeat_interval = "0"

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(bluechi_node_default_config)

    bluechi_test.run(exec)
