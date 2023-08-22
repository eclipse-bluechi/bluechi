# SPDX-License-Identifier: GPL-2.0-or-later

import pytest
from typing import Dict

from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig

NODE_FOO = "node-foo"
NODE_BAR = "node-bar"


def start_with_invalid_port(ctrl: BluechiControllerContainer, nodes: Dict[str, BluechiNodeContainer]):
    _, output = nodes[NODE_FOO].exec_run('systemctl status bluechi-agent')
    output_systemd = str(output)
    if "fail" in output_systemd.lower():
        raise Exception(f"{NODE_FOO} bluechi-agent should NOT failed during the tart of the service")

    _, output = nodes[NODE_BAR].exec_run('systemctl status bluechi-agent')
    output_systemd = str(output)
    if "fail" not in output_systemd.lower():
        raise Exception(f"{NODE_BAR} bluechi-agent should FAILED during the start but DO NOT FAILED")


@pytest.mark.timeout(15)
def test_agent_invalid_port_configuration(
        bluechi_test: BluechiTest,
        bluechi_node_default_config: BluechiNodeConfig, bluechi_ctrl_default_config: BluechiControllerConfig):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO
    node_foo_cfg.manager_port = "8420"

    node_bar_cfg = bluechi_node_default_config.deep_copy()
    node_bar_cfg.node_name = NODE_BAR
    node_bar_cfg.manager_port = "842O"

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO, NODE_BAR]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.add_bluechi_node_config(node_foo_cfg)
    bluechi_test.add_bluechi_node_config(node_bar_cfg)

    bluechi_test.run(start_with_invalid_port)
