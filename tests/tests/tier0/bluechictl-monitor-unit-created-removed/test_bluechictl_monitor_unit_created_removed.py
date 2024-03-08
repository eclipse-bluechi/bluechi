# SPDX-License-Identifier: LGPL-2.1-or-later

import os

from typing import Dict

from bluechi_test.test import BluechiTest
from bluechi_test.machine import BluechiControllerMachine, BluechiAgentMachine
from bluechi_test.config import BluechiControllerConfig, BluechiAgentConfig

node_name_foo = "node-foo"
service_simple = "simple.service"


def exec(ctrl: BluechiControllerMachine, nodes: Dict[str, BluechiAgentMachine]):
    node_foo = nodes[node_name_foo]
    node_foo.copy_systemd_service(service_simple)
    assert node_foo.wait_for_unit_state_to_be(service_simple, "inactive")

    result, output = ctrl.run_python(os.path.join("python", "monitor.py"))


def test_bluechictl_monitor_unit_created_removed(
        bluechi_test: BluechiTest,
        bluechi_ctrl_default_config: BluechiControllerConfig,
        bluechi_node_default_config: BluechiAgentConfig):

    bluechi_node_default_config.node_name = node_name_foo
    bluechi_ctrl_default_config.allowed_node_names = [bluechi_node_default_config.node_name]

    bluechi_test.set_bluechi_controller_config(bluechi_ctrl_default_config)
    bluechi_test.add_bluechi_agent_config(bluechi_node_default_config)

    bluechi_test.run(exec)
