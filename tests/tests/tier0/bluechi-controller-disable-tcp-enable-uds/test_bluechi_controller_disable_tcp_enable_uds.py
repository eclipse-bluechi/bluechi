#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import logging
from typing import Dict

from bluechi_test.config import BluechiAgentConfig, BluechiControllerConfig
from bluechi_test.machine import BluechiAgentMachine, BluechiControllerMachine
from bluechi_test.test import BluechiTest
from bluechi_test.util import Timeout

LOGGER = logging.getLogger(__name__)
NODE_LOCAL = "node-local"


def exec(ctrl: BluechiControllerMachine, _: Dict[str, BluechiAgentMachine]):
    assert ctrl.wait_for_unit_state_to_be("bluechi-controller.service", "active")

    # Copy the port check script and run it
    ctrl.copy_container_script("cmd-on-port.sh")
    res, output = ctrl.exec_run(f"bash /var/cmd-on-port.sh {ctrl.config.port}")
    assert (
        res == 2
    ), f"Expected bluechi-controller to not listen on port {ctrl.config.port}"

    # Create configuration for local bluechi-agent
    agent_config = BluechiAgentConfig(
        file_name="agent.conf",
        controller_address="unix:path=/run/bluechi/bluechi.sock",
        node_name=NODE_LOCAL,
    )
    ctrl.create_file(
        agent_config.get_confd_dir(),
        agent_config.file_name,
        agent_config.serialize(),
    )
    ctrl.systemctl.start_unit("bluechi-agent.service")
    ctrl.wait_for_unit_state_to_be("bluechi-agent.service", "active")

    with Timeout(3, f"Node {NODE_LOCAL} didn't get online in time"):
        is_online = False
        while not is_online:
            _, output = ctrl.bluechictl.get_node_status(NODE_LOCAL, check_result=False)
            is_online = "online" in output


def test_bluechi_controller_disable_tcp_enable_uds(
    bluechi_test: BluechiTest,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):
    bluechi_ctrl_default_config.allowed_node_names = [NODE_LOCAL]
    bluechi_ctrl_default_config.use_tcp = False
    bluechi_ctrl_default_config.use_uds = True
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
