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
    node_foo = nodes[node_foo_name]

    result, output = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    if result != 0:
        raise Exception(output)

    # If node is connected, expect it to have an IP
    result, output = ctrl.run_python(os.path.join("python", "has_node_ip.py"))
    if result != 0:
        raise Exception(output)

    node_foo.systemctl.stop_unit("bluechi-agent.service")

    result, output = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    if result == 0:
        raise Exception(
            f"Expected bluechi-agent {node_foo_name} to be offline, but was online"
        )

    # If node is disconnected, expect it to have no IP
    result, output = ctrl.run_python(os.path.join("python", "has_node_ip.py"))
    if result == 0:
        raise Exception(
            f"Expected bluechi-agent {node_foo_name} to no IP when offline, but still has"
        )


def test_bluechi_node_status(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
    bluechi_node_default_config: BluechiAgentConfig,
):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = node_foo_name

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_test.run(exec)
