# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict
from bluechi_test.test import BluechiTest
from bluechi_test.machine import BluechiControllerMachine, BluechiAgentMachine
from bluechi_test.config import BluechiControllerConfig, BluechiAgentConfig

NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[NODE_FOO]

    service = "org.eclipse.bluechi.Agent"
    object = "/org/eclipse/bluechi"
    interface = "org.eclipse.bluechi.Agent"

    ctrl.exec_run(f"bluechictl set-loglevel {NODE_FOO} INFO")
    _, output = node_foo.exec_run(f"busctl  get-property {service} {object} {interface} LogLevel")
    assert "INFO" in output

    ctrl.exec_run(f"bluechictl set-loglevel {NODE_FOO} DEBUG")
    _, output = node_foo.exec_run(f"busctl  get-property {service} {object} {interface} LogLevel")
    assert "DEBUG" in output


def test_controller_setloglevel(
        bluechi_test: BluechiTest,
        bluechi_node_default_config: BluechiAgentConfig, bluechi_ctrl_default_config: BluechiControllerConfig):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_test.run(exec)
