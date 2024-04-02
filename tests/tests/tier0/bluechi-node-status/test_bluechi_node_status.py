#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest

node_foo_name = "node-foo"


def verify_status_output(output: str, node: str, status: str):
    split_output = output.splitlines()
    valid = False
    for line in split_output:
        if node in line and status in line:
            valid = True
    if not valid:
        raise Exception(
            f"Node '{node}' is not reported as '{status}', output: '{output}'"
        )


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[node_foo_name]

    # Initially node foo should be online
    result, output = ctrl.bluechictl.get_node_status(node_foo_name)
    assert result == 0
    verify_status_output(output, node_foo_name, "online")

    # Stop node foo and retest status
    node_foo.systemctl.stop_unit("bluechi-agent")

    result, output = ctrl.bluechictl.get_node_status(node_foo_name)
    assert result == 0
    verify_status_output(output, node_foo_name, "offline")


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
