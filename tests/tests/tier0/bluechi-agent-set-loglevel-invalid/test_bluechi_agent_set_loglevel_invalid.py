# SPDX-License-Identifier: LGPL-2.1-or-later
from typing import Dict
from bluechi_test.test import BluechiTest
from bluechi_test.container import BluechiControllerContainer, BluechiNodeContainer
from bluechi_test.config import BluechiControllerConfig, BluechiNodeConfig


NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerContainer, _: Dict[str, BluechiNodeContainer]):

    _, output = ctrl.exec_run(f"bluechictl set-loglevel {NODE_FOO} INF")
    print(output)
    assert b'Disconnect' not in output


def test_agent_config_c_option(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiNodeConfig):

    bluechi_node_default_config.node_name = NODE_FOO
    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_node_config(bluechi_node_default_config)

    bluechi_test.run(exec)
