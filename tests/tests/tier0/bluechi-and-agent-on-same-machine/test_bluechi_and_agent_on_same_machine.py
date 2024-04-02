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

    assert ctrl.wait_for_unit_state_to_be("bluechi-controller", "active")

    node_foo_config = BluechiAgentConfig(
        file_name="agent.conf",
        node_name=node_foo_name,
        controller_host="localhost",
        controller_port=ctrl.config.port,
    )
    ctrl.create_file(
        node_foo_config.get_confd_dir(),
        node_foo_config.file_name,
        node_foo_config.serialize(),
    )
    result, _, wait_result = ctrl.systemctl_start_and_wait("bluechi-agent", 1)
    assert result == 0
    assert wait_result

    # bluechi-agent is running, check if it is connected
    result, output = ctrl.run_python(os.path.join("python", "is_node_connected.py"))
    if result != 0:
        raise Exception(output)


def test_bluechi_and_agent_on_same_machine(
    bluechi_test: BluechiTest, bluechi_ctrl_default_config: BluechiControllerConfig
):

    bluechi_ctrl_default_config.allowed_node_names = [node_foo_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    # don't add node_foo_config to bluechi_test to prevent it being started
    # as separate container
    bluechi_test.run(exec)
