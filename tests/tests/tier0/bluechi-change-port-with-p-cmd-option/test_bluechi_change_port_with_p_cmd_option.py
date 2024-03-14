# SPDX-License-Identifier: LGPL-2.1-or-later

from typing import Dict
import os
import time

from bluechi_test.test import BluechiTest
from bluechi_test.machine import BluechiControllerMachine, BluechiAgentMachine
from bluechi_test.config import BluechiControllerConfig, BluechiAgentConfig

NODE_FOO = "node-foo"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[NODE_FOO]
    bluechi_agent_str = "bluechi-agent"
    bluechi_controller_str = "bluechi-controller"

    # Copy the script to get the process listening on a port
    ctrl.copy_container_script("cmd-on-port.sh")
    node_foo.copy_container_script("cmd-on-port.sh")

    # change controller port to 8421
    ctrl.restart_with_modified_service_file(
        bluechi_controller_str,
        [
            "sed -i '/^ExecStart=/ s/$/ -p 8421/'"
        ],
    )
    assert ctrl.wait_for_unit_state_to_be(bluechi_controller_str, "active")

    # Check if port 8421 is listenning
    time.sleep(1)  # Give some time to the controller to start listening
    res, output = ctrl.exec_run("bash /var/cmd-on-port.sh 8421")
    assert res == 0
    assert "bluechi-controller" in str(output)

    # Check if node disconnected
    result, _ = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    assert result

    # change node port to 8421
    node_foo.restart_with_modified_service_file(
        bluechi_agent_str,
        [
            "sed -i '/^ExecStart=/ s/$/ -p 8421/'"
        ],
    )
    assert node_foo.wait_for_unit_state_to_be(bluechi_agent_str, "active")

    # Check if node connected
    time.sleep(1)
    res, output = node_foo.exec_run("bash /var/cmd-on-port.sh 8421")
    assert res == 0
    assert "bluechi-agent" in str(output)

    result, _ = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    assert not result


def test_agent_invalid_port_configuration(
    bluechi_test: BluechiTest,
    bluechi_node_default_config: BluechiAgentConfig,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):

    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_FOO

    bluechi_ctrl_default_config.allowed_node_names = [NODE_FOO]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_test.additional_ports = {"8421": "8421"}

    bluechi_test.run(exec)
