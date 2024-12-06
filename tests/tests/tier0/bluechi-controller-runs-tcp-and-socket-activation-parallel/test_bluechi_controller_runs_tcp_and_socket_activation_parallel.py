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
NODE_REMOTE = "node-remote"


def exec(ctrl: BluechiControllerMachine, _: Dict[str, BluechiAgentMachine]):
    ctrl.systemctl.stop_unit("bluechi-controller.service", check_result=False)
    assert ctrl.wait_for_unit_state_to_be("bluechi-controller.service", "inactive")

    ctrl.systemctl.start_unit("bluechi-controller.socket")
    assert ctrl.wait_for_unit_state_to_be("bluechi-controller.socket", "active")

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
    ctrl.wait_for_unit_state_to_be("bluechi-controller.service", "active")

    with Timeout(3, f"Node {NODE_LOCAL} didn't get online in time"):
        is_online = False
        while not is_online:
            _, output = ctrl.bluechictl.get_node_status(NODE_LOCAL, check_result=False)
            is_online = "online" in output

    with Timeout(3, f"Node {NODE_REMOTE} didn't get online in time"):
        is_online = False
        while not is_online:
            _, output = ctrl.bluechictl.get_node_status(NODE_REMOTE, check_result=False)
            is_online = "online" in output


def test_bluechi_controller_runs_tcp_and_socket_activation_parallel(
    bluechi_test: BluechiTest,
    bluechi_node_default_config: BluechiAgentConfig,
    bluechi_ctrl_default_config: BluechiControllerConfig,
):
    node_foo_cfg = bluechi_node_default_config.deep_copy()
    node_foo_cfg.node_name = NODE_REMOTE
    node_foo_cfg.heartbeat_interval = 500
    bluechi_test.add_bluechi_agent_config(node_foo_cfg)

    bluechi_ctrl_default_config.allowed_node_names = [NODE_LOCAL, NODE_REMOTE]
    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)

    bluechi_test.run(exec)
