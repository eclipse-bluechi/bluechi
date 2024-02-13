# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict
import os

from bluechi_test.test import BluechiTest
from bluechi_test.machine import BluechiControllerMachine, BluechiAgentMachine
from bluechi_test.config import BluechiControllerConfig, BluechiAgentConfig
from bluechi_test.util import read_file

NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[NODE_FOO]
    config_file_location = "/var/tmp"
    bluechi_agent_str = "bluechi-agent"
    bluechi_controller_str = "bluechi-controller"
    file_location_ctrl = os.path.join("config-files", "ctrl_port_8421.conf")
    file_location_agent = os.path.join("config-files", "agent_port_8421.conf")

    # Copying relevant config files into the nodes container
    content = read_file(file_location_agent)
    node_foo.create_file(config_file_location, file_location_agent, content)
    content = read_file(file_location_ctrl)
    ctrl.create_file(config_file_location, file_location_ctrl, content)

    ctrl.restart_with_config_file(
        os.path.join(config_file_location, "ctrl_port_8421.conf"), bluechi_controller_str)
    assert ctrl.wait_for_unit_state_to_be(bluechi_controller_str, "active")

    # Check if port 8421 is listenning and 8420 is disconnected
    _, output = ctrl.exec_run("lsof -i:8421")
    assert "bluechi-c" in str(output)
    _, output = ctrl.exec_run("lsof -i:8420")
    assert b'' == output

    # Check if node disconnected
    result, _ = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    assert result

    node_foo.restart_with_config_file(
        os.path.join(config_file_location, "agent_port_8421.conf"), bluechi_agent_str)
    assert node_foo.wait_for_unit_state_to_be(bluechi_agent_str, "active")

    # Check if node connected
    _, output = node_foo.exec_run("lsof -i:8421")
    assert "bluechi-a" in str(output)

    result, _ = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    assert not result


def test_agent_invalid_port_configuration(
        bluechi_test: BluechiTest,
        bluechi_node_default_config: BluechiAgentConfig, bluechi_ctrl_default_config: BluechiControllerConfig):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_test.additional_ports = {"8421": "8421"}

    bluechi_test.run(exec)
